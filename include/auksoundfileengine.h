#ifndef AUKSOUNDFILEENGINE_H
#define AUKSOUNDFILEENGINE_H

/*
 * AukSoundFileEngine - Background Sound File Loading Engine
 *
 * Manages a worker process/thread that loads and caches sound files
 * in the background. Uses AukObject retain system for safe sharing
 * between main thread and worker.
 *
 * Architecture:
 * - Main process requests files, receives status updates via messages
 * - Worker process reads files through format plugins (WAV, 8SVX, etc.)
 * - Three loading phases per file:
 *   Phase 1: Detect format, read metadata (sampleRate, channels, frameCount)
 *   Phase 2: Stream through file computing min/max statistics for waveform display
 *   Phase 3: On-demand buffer loading for playback/mixing consumers
 *
 * Usage:
 * 1. Call AukSoundFileEngine_Init() at app startup
 * 2. Request files with AukSoundFileEngine_RequestFile()
 * 3. Check status or wait for signals when file is ready
 * 4. Release files with AukSoundFileEngine_ReleaseFile()
 * 5. Call AukSoundFileEngine_Shutdown() at app exit
 */

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/ports.h>
#include <exec/tasks.h>
#ifdef AMIGA
#include <dos/dosextens.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
struct AukSoundFile;
struct AukSoundFileEngine;
struct AukSFEBufferPool;

/* File status (also used in AukSoundFile) */
typedef enum {
    AUKSFE_STATUS_NONE = 0,
    AUKSFE_STATUS_PENDING,         /* Waiting to be processed */
    AUKSFE_STATUS_STATED_PHASE1,   /* Metadata ready (format, rate, channels, length) */
    AUKSFE_STATUS_STATED_PHASE2,   /* Min/max computation in progress */
    AUKSFE_STATUS_STATED_PHASE3,   /* All min/max ready, on-demand loading only */
    AUKSFE_STATUS_ERROR            /* Error occurred */
} AukSFEFileStatus;

/* Message types for communication */
typedef enum {
    AUKSFE_MSG_NONE = 0,
    AUKSFE_MSG_ADD_FILE,        /* Request to load a file */
    AUKSFE_MSG_REMOVE_FILE,     /* Request to release a file */
    AUKSFE_MSG_FILE_STATED,     /* File stats are ready (format, length, etc.) */
    AUKSFE_MSG_FILE_MINMAX_PROGRESS, /* Min/max partially computed */
    AUKSFE_MSG_FILE_READY,      /* File fully analyzed */
    AUKSFE_MSG_FILE_ERROR,      /* Error loading file */
    AUKSFE_MSG_BUFFER_READY,    /* A requested buffer part is now available */
    AUKSFE_MSG_SHUTDOWN,        /* Shutdown worker thread */
    AUKSFE_MSG_WAKEUP           /* Wake worker to process pending work */
} AukSFEMessageType;

struct AukSoundFile;
struct AukSoundFileEngine;

/* Internal: Message structure for inter-thread communication */
typedef struct AukSFEMessage {
    struct Message msg;             /* Amiga message header (must be first) */
    struct AukSoundFileEngine *engine;
    AukSFEMessageType type;         /* Message type */
    struct AukSoundFile* file;      /* Sound file reference */
    int errorCode;                  /* Error code if type == ERROR */
    int iChannel;                   /* Channel index for BUFFER_READY */
    int iPart;                      /* Part index for BUFFER_READY */
} AukSFEMessage;

/* Internal: File node in engine's managed list */
typedef struct AukSFEFileNode {
    struct AukSFEFileNode* next;
    struct AukSoundFile* file;      /* The sound file object */
    AukSFEFileStatus status;        /* Current loading status */
    int refCount;                   /* Number of external references */
    /* Phase 2 progress tracking */
    unsigned long minmaxProgress;   /* How many 256-sample chunks processed */
} AukSFEFileNode;

/* Internal: Job types for worker thread */
typedef enum {
    AUKSFE_JOB_NONE = 0,
    AUKSFE_JOB_INIT_PHASE1,         /* Open file, detect format, get metadata */
    AUKSFE_JOB_INIT_PHASE2,         /* Compute min/max chunk */
    AUKSFE_JOB_LOAD_BUFFER          /* Load a buffer part for consumer */
} AukSFEJobType;

/* Internal: Job structure for worker thread */
typedef struct AukSFEJob {
    AukSFEJobType type;
    struct AukSoundFile *soundfile;  /* Weak pointer (already retained in node) */
    int iChannel;                    /* For LOAD_BUFFER */
    int iPart;                       /* For LOAD_BUFFER */
} AukSFEJob;

/* Buffer pool chunk - fixed size pre-allocated buffer */
#define AUKSFE_POOL_CHUNK_SIZE 32768  /* 32KB = 16384 16-bit stereo samples */

typedef struct AukSFEPoolChunk {
    struct AukSFEPoolChunk* next;   /* For free list */
    unsigned char data[AUKSFE_POOL_CHUNK_SIZE];
} AukSFEPoolChunk;

/* Buffer pool - pre-allocated memory for sound buffers */
typedef struct AukSFEBufferPool {
    void* memory;                   /* Base allocation */
    unsigned long totalBytes;       /* Total pool size */
    unsigned long totalChunks;      /* Number of chunks */
    unsigned long usedChunks;       /* Chunks in use */
    AukSFEPoolChunk* freeList;      /* Available chunks */
} AukSFEBufferPool;

/* Engine state */
typedef struct AukSoundFileEngine {
    /* Worker process */
    struct Process* workerProcess;
    int workerRunning;

    /* Message ports */
    struct MsgPort* mainReplyPort;      /* For receiving replies in main process */
    struct MsgPort* workerPort;         /* Worker's message port (on Amiga: pr_MsgPort) */

    /* Base path for resolving relative filenames */
    char* basePath;

    /* Main process adds new files here (accessed with Forbid/Permit) */
    struct AukSFEFileNode* files_new;

    /* Worker manages files here after picking them up from files_new */
    struct AukSFEFileNode* files_managed;

    /* Buffer pool for sound data */
    AukSFEBufferPool* bufferPool;

    /* Worker thread internal state */
#define SFE_MAX_JOBS 32
    AukSFEJob jobs[SFE_MAX_JOBS];
    int jobsCount;

    /* Shutdown flag */
    int shutdownRequested;

} AukSoundFileEngine;

/* Sound file loading engine - singleton */
extern AukSoundFileEngine *soundFileEngine;

/* ============================================================
 * Public API
 * ============================================================ */

/*
 * Initialize the sound file engine.
 * Creates the worker thread and buffer pool.
 *
 * @param mainProcess    Main process (for stdout on Amiga)
 * @param basePath       Base path for resolving relative filenames
 * @param poolSizeBytes  Size of buffer pool in bytes (min 256KB, 0 = default 2MB)
 * @return               Pointer to engine, or NULL on failure
 */
AukSoundFileEngine* AukSoundFileEngine_Init(
    struct Process *mainProcess,
    const char* basePath,
    unsigned long poolSizeBytes
);

/*
 * Shutdown the sound file engine.
 * Waits for worker thread to finish, releases all files and pool.
 *
 * @param engine    Engine to shutdown (freed after this call)
 */
void AukSoundFileEngine_Shutdown(AukSoundFileEngine* engine);

/*
 * Request a sound file to be loaded.
 * The file will be loaded in the background.
 *
 * @param engine    The engine
 * @param filename  Path to the sound file (relative to basePath)
 * @return          Pointer to AukSoundFile (caller should release when done)
 *                  or NULL on error
 */
struct AukSoundFile* AukSoundFileEngine_RequestFile(
    AukSoundFileEngine* engine,
    const char* filename
);

/*
 * Release a sound file.
 * Decrements reference count; file may be unloaded if no more references.
 *
 * @param engine    The engine
 * @param file      File to release
 */
void AukSoundFileEngine_ReleaseFile(
    AukSoundFileEngine* engine,
    struct AukSoundFile* file
);

/*
 * Get the status of a sound file.
 *
 * @param engine    The engine
 * @param file      File to check
 * @return          File status
 */
AukSFEFileStatus AukSoundFileEngine_GetFileStatus(
    AukSoundFileEngine* engine,
    struct AukSoundFile* file
);

/*
 * Check for and process any pending notifications from worker.
 * Call this periodically from main loop.
 *
 * @param engine    The engine
 * @return          Number of notifications processed
 */
int AukSoundFileEngine_ProcessNotifications(AukSoundFileEngine* engine);

/*
 * Get signal mask for waiting on engine notifications.
 * Use this in Wait() to be notified when files are ready.
 *
 * @param engine    The engine
 * @return          Signal mask (0 if engine not initialized)
 */
ULONG AukSoundFileEngine_GetSignalMask(AukSoundFileEngine* engine);

/*
 * Wake the worker thread to process pending work.
 * Call this after requesting buffer loads.
 */
void AukSoundFileEngine_WakeWorker(AukSoundFileEngine* engine);

/* ============================================================
 * Buffer Pool API (internal use, but exposed for testing)
 * ============================================================ */

AukSFEBufferPool* AukSFEBufferPool_Create(unsigned long totalBytes);
void AukSFEBufferPool_Destroy(AukSFEBufferPool* pool);
void* AukSFEBufferPool_AllocChunk(AukSFEBufferPool* pool);
void AukSFEBufferPool_FreeChunk(AukSFEBufferPool* pool, void* chunk);
void AukSFEBufferPool_GetStats(AukSFEBufferPool* pool,
                                unsigned long* outTotal,
                                unsigned long* outUsed,
                                unsigned long* outFree);

#ifdef __cplusplus
}
#endif

#endif /* AUKSOUNDFILEENGINE_H */
