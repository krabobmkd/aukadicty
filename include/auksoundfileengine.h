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
#include <exec/tasks.h>
#include <dos/dosextens.h>

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


struct AukSoundFile;
struct AukSoundFileEngine;

/* internal, Message structure for inter-thread communication */
typedef struct AukSFEMessage {
    struct Message msg;         /* Amiga message header (must be first) */
    AukSoundFileEngine *engine;
    AukSFEMessageType type;     /* Message type */
    struct AukSoundFile* file;  /* Sound file (retained) */
    int errorCode;              /* Error code if type == ERROR */
} AukSFEMessage;

/* internal, for compiled list of jobs to do in a row */
typedef struct AukSFEJob {
    struct AukSoundFile *soundfile; /* weak pointer, alredy retained in Node */
    /* could either init or load a part, ...or anything? !=0 means error. */
    int job( struct AukSoundFile *soundfile );
} AukSFEJob;


/* Engine state */
typedef struct AukSoundFileEngine {
    /* Worker process */
    struct Process* workerProcess;
    int workerRunning;

    /* Message ports */
    struct MsgPort* mainReplyPort;  /* For receiving replies */

    /* main process put new files here:  */
    struct AukSFEFileNode* files_new;

    /* read process
       - remove files in  files_new set them in files_managed.
       From then, files pass multiple states:
       - 0 unknown
       - 1 file type/length/frequency/nbchans known. ->message it back to main process.
       - 2 stream all file part by part to just keep min/max and stats.
          If many files, we read parts of each in turns.
          For each files/Part message the main process.
       - 3 state is "file known", no more immediate task for it.

       For state 2 and 3,It can be asked to read again the sound signal to make
       the buffer available to mixer, player or exporter.

       If asked to remove file at any moment with AukSoundFileEngine_ReleaseFile,
       file is being released on the last use.
       */
       /*
        note as
       */
    struct AukSFEFileNode* files_managed;

    /* thread internal vars */
    /* jobs to be done in a row between messagings. If not enough will just be done later. */
#define SFEMaxJobs 32
    AukSFEJob jobs[SFEMaxJobs];
    int     jobsCount;

    /* Shutdown flag */
    int shutdownRequested;

} AukSoundFileEngine;

/*
 * Initialize the sound file engine.
 * Creates the worker thread.
 *
 * @myProcess process just to get standard output
 * @return  Pointer to engine, or NULL on failure
 */
AukSoundFileEngine* AukSoundFileEngine_Init(struct Process *mainProcess);

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
