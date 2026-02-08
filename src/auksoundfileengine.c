/*
 * AukSoundFileEngine - Background Sound File Loading Engine
 *
 * Implements a worker thread pattern for loading sound files
 * in the background using Amiga-style message passing.
 */

#include "auksoundfileengine.h"
#include "auksoundfile.h"
#include "aukstring.h"

#include <proto/dos.h>
#include <proto/exec.h>
#include <dos/dostags.h>

#include <stdio.h>
#include <string.h>

/* File node in the engine's list */
typedef struct AukSFEFileNode {
    struct AukSFEFileNode* next;
    AukSoundFilePtr file;           /* Retained reference */
    AukSFEFileStatus status;
    int refCount;                   /* How many times requested */
} AukSFEFileNode;

/* Shared data between main and worker threads (MEMF_PUBLIC equivalent) */
typedef struct AukSFESharedData {
    AukSoundFileEngine* engine;
    int workerShouldExit;
    int workerReady;
} AukSFESharedData;

/* Global shared data pointer */
static AukSFESharedData* g_sharedData = NULL;

/* ============================================================
 * Worker Thread
 * ============================================================ */

/* open file, choose plugin, get stats */
static int job_initFile1( struct AukSoundFile *soundfile )
{


}

/* each consumer should lock and unlock by part  */
static int job_LockAndReadSoundPart( struct AukSoundFile *soundfile, int iPart )
{


}
static int job_UnlockSoundPart( struct AukSoundFile *soundfile, int iPart )
{


}
/* init read for min/max stats, we're a consumer like another ones... */
static int job_initFile2( struct AukSoundFile *soundfile )
{


}

/*
 * Worker thread entry point.
 * Waits for messages, processes file requests.
 */
static void SoundFileWorkerThread(void)
{
    struct Process* thisProcess;
    struct MsgPort* workerPort;
    AukSFEMessage* msg;
    int running;

    /* Get our process structure */
    thisProcess = (struct Process*)FindTask(NULL);
    if (!thisProcess) {
        return;
    }


    workerPort = &thisProcess->pr_MsgPort;

    /* Wait for initial handshake message */
    WaitPort(workerPort);
    msg = (AukSFEMessage*)GetMsg(workerPort);
    if (!msg || !g_sharedData) {
        if (msg) ReplyMsg(&msg->msg);
        return;
    }

    /* Signal that we're ready */
    g_sharedData->workerReady = 1;
    ReplyMsg(&msg->msg);

    /* Main worker loop */
    running = 1;
    while (running) {
        WaitPort(workerPort);

        /* Process all pending messages */
        while ((msg = (AukSFEMessage*)GetMsg(workerPort)) != NULL) {
            switch (msg->type) {
                case AUKSFE_MSG_SHUTDOWN:
                    running = 0;
                    ReplyMsg(&msg->msg);
                    break;

                case AUKSFE_MSG_ADD_FILE:
                    /* TODO: Actually load the file here */
                    /* For now, just mark as stated and reply */
                    printf("[Worker] Received ADD_FILE request\n");
                   msg->type = AUKSFE_MSG_FILE_STATED;
                    ReplyMsg(&msg->msg);
                    break;

                case AUKSFE_MSG_REMOVE_FILE:
                {
                    printf("[Worker] Received REMOVE_FILE request\n");
                    ReplyMsg(&msg->msg);
                }
                    break;
                default:
                    ReplyMsg(&msg->msg);
                    break;
            }
        }
        /* process awaken: get list of the jobs to do,
        watch out list is shared with other processes:
        should be a mutex implemented with exec semaphores.
        Well with just Forbid()/Permit() it's okay for the moment.
        Getting job information must be quick !
         */
        Forbid();
  // TODO: fill a jobs list from the files_managed
// and attribute correct job against AukSoundFile state
// this must be quick, so just made up that list.
        Permit();

 // and here after the Permit, we'll execute the actuall Jobs for that turn.
// then send a message to consumer on their process... well do that later. 



    } // end infinite loop with waiting

    g_sharedData->workerShouldExit = 0;
    printf("[Worker] Exiting\n");
}

/* ============================================================
 * Engine API
 * ============================================================ */

AukSoundFileEngine* AukSoundFileEngine_Init(struct Process *mainProcess)
{
    AukSoundFileEngine* engine;
    AukSFEMessage initMsg;
   // struct TagItem procTags[4];

    /* Allocate engine structure */
    engine = (AukSoundFileEngine*)AllocVec(sizeof(AukSoundFileEngine), MEMF_CLEAR | MEMF_PUBLIC);
    if (!engine) {
        return NULL;
    }

    /* Allocate shared data */
    g_sharedData = (AukSFESharedData*)AllocVec(sizeof(AukSFESharedData), MEMF_CLEAR | MEMF_PUBLIC);
    if (!g_sharedData) {
        FreeVec(engine);
        return NULL;
    }
    g_sharedData->engine = engine;
    g_sharedData->workerShouldExit = 0;
    g_sharedData->workerReady = 0;

    /* Create main reply port */
    engine->mainReplyPort = CreateMsgPort();
    if (!engine->mainReplyPort) {
        FreeVec(g_sharedData);
        g_sharedData = NULL;
        FreeVec(engine);
        return NULL;
    }

    /* Create worker process using simple API (no casting needed on PC) */
#ifndef AMIGA
    (void)procTags; /* Unused on PC */
    engine->workerProcess = CreateNewProcSimple(SoundFileWorkerThread, "AukSoundFileWorker", 0);
#else
    engine->workerProcess = CreateNewProcTags(
                NP_Entry,(ULONG)SoundFileWorkerThread,
                NP_Name,(ULONG)"AukSoundFileWorker",
                NP_Output,mainProcess->pr_COS,
                NP_CloseOutput,FALSE,
                NP_FreeSeglist, FALSE,
                TAG_END
                );
#endif
    if (!engine->workerProcess) {
        DeleteMsgPort(engine->mainReplyPort);
        FreeVec(g_sharedData);
        g_sharedData = NULL;
        FreeVec(engine);
        return NULL;
    }

    /* Send handshake message to worker */
    memset(&initMsg, 0, sizeof(initMsg));
    initMsg.msg.mn_ReplyPort = engine->mainReplyPort;
    initMsg.msg.mn_Length = sizeof(AukSFEMessage);
    initMsg.type = AUKSFE_MSG_NONE;

    PutMsg(&engine->workerProcess->pr_MsgPort, &initMsg.msg);
    WaitPort(engine->mainReplyPort);
    (void)GetMsg(engine->mainReplyPort);

    /* Verify worker started */
    if (!g_sharedData->workerReady) {
        printf("[Engine] Worker failed to start\n");
        DeleteMsgPort(engine->mainReplyPort);
        FreeVec(g_sharedData);
        g_sharedData = NULL;
        FreeVec(engine);
        return NULL;
    }

    engine->workerRunning = 1;
    engine->fileList = NULL;
    engine->fileCount = 0;
    engine->shutdownRequested = 0;

    printf("[Engine] Initialized successfully\n");
    return engine;
}

void AukSoundFileEngine_Shutdown(AukSoundFileEngine* engine)
{
    AukSFEMessage shutdownMsg;
    AukSFEFileNode* node;
    AukSFEFileNode* next;

    if (!engine) {
        return;
    }

    /* Send shutdown message to worker */
    if (engine->workerProcess && engine->workerRunning) {
        memset(&shutdownMsg, 0, sizeof(shutdownMsg));
        shutdownMsg.msg.mn_ReplyPort = engine->mainReplyPort;
        shutdownMsg.msg.mn_Length = sizeof(AukSFEMessage);
        shutdownMsg.type = AUKSFE_MSG_SHUTDOWN;

        g_sharedData->workerShouldExit = 1;

        PutMsg(&engine->workerProcess->pr_MsgPort, &shutdownMsg.msg);
        WaitPort(engine->mainReplyPort);
        (void)GetMsg(engine->mainReplyPort);

        engine->workerRunning = 0;
        printf("[Engine] Worker shutdown complete\n");
    }

    /* Release all files in list */
    node = engine->fileList;
    while (node) {
        next = node->next;
        if (node->file) {
            AukObjectPtr_Release((AukObjectPtr*)&node->file);
        }
        FreeVec(node);
        node = next;
    }
    engine->fileList = NULL;
    engine->fileCount = 0;

    /* Cleanup */
    if (engine->mainReplyPort) {
        DeleteMsgPort(engine->mainReplyPort);
    }

    if (g_sharedData) {
        FreeVec(g_sharedData);
        g_sharedData = NULL;
    }

    FreeVec(engine);
    printf("[Engine] Shutdown complete\n");
}

struct AukSoundFile* AukSoundFileEngine_RequestFile(
    AukSoundFileEngine* engine,
    const char* filename)
{
    AukSFEFileNode* node;
    AukSFEFileNode* newNode;
    AukSFEMessage reqMsg;
    AukSoundFile* file;

    if (!engine || !filename) {
        return NULL;
    }

    /* Check if file already in list */
Forbid();
    for (node = engine->files_new; node; node = node->next) {
        if (node->file) {
            const char* existingPath = node->file->GetFilename(node->file);
            if (existingPath && strcmp(existingPath, filename) == 0) {
                /* Already have this file, increment ref */
                node->refCount++;
                Permit();
                return node->file;
            }
        }
    }
    for (node = engine->files_managed; node; node = node->next) {
        if (node->file) {
            const char* existingPath = node->file->GetFilename(node->file);
            if (existingPath && strcmp(existingPath, filename) == 0) {
                /* Already have this file, increment ref */
                node->refCount++;
                Permit();
                return node->file;
            }
        }
    }
Permit();

    /* Create new sound file object */
    {
        AukObjectPtr newFilePtr = NULL;
        AukSoundFile_New(&newFilePtr);
        file = (AukSoundFile*)newFilePtr;
    }
    if (!file) {
        return NULL;
    }

    /* Set filename */
    file->SetFilename(file, filename);

    /* Create file node */
    newNode = (AukSFEFileNode*)AllocVec(sizeof(AukSFEFileNode), MEMF_CLEAR | MEMF_PUBLIC);
    if (!newNode) {
        file->base.Delete((AukObject*)file);
        return NULL;
    }

    newNode->file = file;
    newNode->status = AUKSFE_STATUS_PENDING;
    newNode->refCount = 1;
Forbid();
    newNode->next = engine->files_new;
    engine->files_new = newNode;
Permit(); /* file considered added from that moment */

    /* Send request to worker, to wake it up */
    memset(&reqMsg, 0, sizeof(reqMsg));
    reqMsg.msg.mn_ReplyPort = engine->mainReplyPort;
    reqMsg.msg.mn_Length = sizeof(AukSFEMessage);
    reqMsg.type = AUKSFE_MSG_ADD_FILE;
    reqMsg.file = file;

    PutMsg(&engine->workerProcess->pr_MsgPort, &reqMsg.msg);
    //WaitPort(engine->mainReplyPort);

    // {
    //     AukSFEMessage* reply = (AukSFEMessage*)GetMsg(engine->mainReplyPort);
    //     if (reply) {
    //         /* Update status based on reply */
    //         if (reply->type == AUKSFE_MSG_FILE_STATED) {
    //             newNode->status = AUKSFE_STATUS_STATED;
    //         } else if (reply->type == AUKSFE_MSG_FILE_ERROR) {
    //             newNode->status = AUKSFE_STATUS_ERROR;
    //         }
    //     }
    // }

    /* Return file to caller - node holds reference */
    return file;
}

void AukSoundFileEngine_ReleaseFile(
    AukSoundFileEngine* engine,
    struct AukSoundFile* file)
{
    AukSFEFileNode* node;
    AukSFEFileNode* prev = NULL;
    AukSFEMessage reqMsg;

    if (!engine || !file) {
        return;
    }

    /* Find file in list */
    for (node = engine->fileList; node; prev = node, node = node->next) {
        if (node->file == file) {
            node->refCount--;

            if (node->refCount <= 0) {
                /* Remove from list */
                if (prev) {
                    prev->next = node->next;
                } else {
                    engine->fileList = node->next;
                }


                /* Tell worker to release */
                memset(&reqMsg, 0, sizeof(reqMsg));
                reqMsg.msg.mn_ReplyPort = engine->mainReplyPort;
                reqMsg.msg.mn_Length = sizeof(AukSFEMessage);
                reqMsg.type = AUKSFE_MSG_REMOVE_FILE;
                reqMsg.file = file;

                PutMsg(&engine->workerProcess->pr_MsgPort, &reqMsg.msg);
                WaitPort(engine->mainReplyPort);
                (void)GetMsg(engine->mainReplyPort);

                /* Release our reference */
                AukObjectPtr_Release((AukObjectPtr*)&node->file);
                FreeVec(node);
            }

            break;
        }
    }
}

AukSFEFileStatus AukSoundFileEngine_GetFileStatus(
    AukSoundFileEngine* engine,
    struct AukSoundFile* file)
{
    AukSFEFileNode* node;

    if (!engine || !file) {
        return AUKSFE_STATUS_NONE;
    }

    for (node = engine->fileList; node; node = node->next) {
        if (node->file == file) {
            return node->status;
        }
    }

    return AUKSFE_STATUS_NONE;
}

int AukSoundFileEngine_ProcessNotifications(AukSoundFileEngine* engine)
{
    /* For now, notifications are handled synchronously in RequestFile */
    /* This function would be used for async notifications */
    (void)engine;
    return 0;
}

ULONG AukSoundFileEngine_GetSignalMask(AukSoundFileEngine* engine)
{
    if (!engine || !engine->mainReplyPort) {
        return 0;
    }
    /* On Amiga, this would return 1L << mainReplyPort->mp_SigBit */
    /* On PC, signal masks aren't used the same way */
    return 0;
}
