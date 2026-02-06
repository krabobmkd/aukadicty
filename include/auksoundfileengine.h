#ifndef AUKSOUNDFILEENGINE_H
#define AUKSOUNDFILEENGINE_H

/*
 * AukSoundFileEngine - Background Sound File Loading Engine
 *
 * Manages a worker process/thread that loads and caches sound files
 * in the background. Uses AukObject retain system for safe sharing
 * between main thread and worker.
 *
 * Usage:
 * 1. Call AukSoundFileEngine_Init() at app startup
 * 2. Request files with AukSoundFileEngine_RequestFile()
 * 3. Check status or wait for signals when file is ready
 * 4. Release files with AukSoundFileEngine_ReleaseFile()
 * 5. Call AukSoundFileEngine_Shutdown() at app exit
 *
 * Threading model:
 * - Main thread sends requests via message passing
 * - Worker thread processes requests and sends completion signals
 * - All AukSoundFile objects are reference-counted
 */

#include <exec/types.h>
#include <exec/ports.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
struct AukSoundFile;
struct AukSoundFileEngine;

/* Message types for communication */
typedef enum {
    AUKSFE_MSG_NONE = 0,
    AUKSFE_MSG_ADD_FILE,        /* Request to load a file */
    AUKSFE_MSG_REMOVE_FILE,     /* Request to release a file */
    AUKSFE_MSG_FILE_STATED,     /* File stats are ready (format, length, etc.) */
    AUKSFE_MSG_FILE_LOADED,     /* File data fully loaded */
    AUKSFE_MSG_FILE_ERROR,      /* Error loading file */
    AUKSFE_MSG_SHUTDOWN         /* Shutdown worker thread */
} AukSFEMessageType;

/* File status */
typedef enum {
    AUKSFE_STATUS_NONE = 0,
    AUKSFE_STATUS_PENDING,      /* Waiting to be processed */
    AUKSFE_STATUS_STATING,      /* Reading file header/stats */
    AUKSFE_STATUS_STATED,       /* Stats ready, not fully loaded */
    AUKSFE_STATUS_LOADING,      /* Loading file data */
    AUKSFE_STATUS_LOADED,       /* Fully loaded */
    AUKSFE_STATUS_ERROR         /* Error occurred */
} AukSFEFileStatus;

/* Message structure for inter-thread communication */
typedef struct AukSFEMessage {
    struct Message msg;         /* Amiga message header (must be first) */
    AukSFEMessageType type;     /* Message type */
    struct AukSoundFile* file;  /* Sound file (retained) */
    int errorCode;              /* Error code if type == ERROR */
} AukSFEMessage;

/* Engine state */
typedef struct AukSoundFileEngine {
    /* Worker process */
    struct Process* workerProcess;
    int workerRunning;

    /* Message ports */
    struct MsgPort* mainReplyPort;  /* For receiving replies */

    /* Pending file requests (simple linked list) */
    struct AukSFEFileNode* fileList;
    int fileCount;

    /* Shutdown flag */
    int shutdownRequested;

} AukSoundFileEngine;

/*
 * Initialize the sound file engine.
 * Creates the worker thread.
 *
 * @return  Pointer to engine, or NULL on failure
 */
AukSoundFileEngine* AukSoundFileEngine_Init(void);

/*
 * Shutdown the sound file engine.
 * Waits for worker thread to finish, releases all files.
 *
 * @param engine    Engine to shutdown (freed after this call)
 */
void AukSoundFileEngine_Shutdown(AukSoundFileEngine* engine);

/*
 * Request a sound file to be loaded.
 * The file will be loaded in the background.
 *
 * @param engine    The engine
 * @param filename  Path to the sound file
 * @return          Pointer to AukSoundFile (retained, caller should release when done)
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

#ifdef __cplusplus
}
#endif

#endif /* AUKSOUNDFILEENGINE_H */
