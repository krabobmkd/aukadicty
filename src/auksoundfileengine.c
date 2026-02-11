/*
 * AukSoundFileEngine - Background Sound File Loading Engine
 *
 * Implements a worker thread pattern for loading sound files
 * in the background using Amiga-style message passing.
 *
 * On Amiga: uses native CreateNewProcTags/MsgPort
 * On PC: uses AmigaStack compatibility layer (Windows + Linux)
 *
 * Phases:
 * 1. Open file, detect format, read metadata
 * 2. Stream through file computing min/max statistics
 * 3. On-demand buffer loading for consumers
 */

#include "auksoundfileengine.h"
#include "auksoundfile.h"
#include "aukstring.h"
#include <string.h>
#include <stdio.h>

#ifdef AMIGA
#include <proto/dos.h>
#include <proto/exec.h>
#include <dos/dostags.h>
#else
/* PC: Use AmigaStack compatibility layer (works on Windows + Linux) */
#include <proto/exec.h>
#include <proto/dos.h>
#include <dos/dostags.h>
#include <stdlib.h>
#endif

/* ============================================================
 * Buffer Pool Implementation
 * ============================================================ */

AukSFEBufferPool* AukSFEBufferPool_Create(unsigned long totalBytes) {
    AukSFEBufferPool* pool;
    unsigned long i, nChunks;
    AukSFEPoolChunk* chunk;

    /* Minimum 256KB */
    if (totalBytes < 262144) {
        totalBytes = 262144;
    }

    pool = (AukSFEBufferPool*)AllocVec(sizeof(AukSFEBufferPool), MEMF_CLEAR | MEMF_PUBLIC);
    if (!pool) return NULL;

    /* Allocate main memory block */
    pool->memory = AllocVec(totalBytes, MEMF_CLEAR | MEMF_PUBLIC);
    if (!pool->memory) {
        FreeVec(pool);
        return NULL;
    }

    pool->totalBytes = totalBytes;
    nChunks = totalBytes / sizeof(AukSFEPoolChunk);
    pool->totalChunks = nChunks;
    pool->usedChunks = 0;

    /* Build free list */
    pool->freeList = NULL;
    for (i = 0; i < nChunks; i++) {
        chunk = (AukSFEPoolChunk*)((unsigned char*)pool->memory + i * sizeof(AukSFEPoolChunk));
        chunk->next = pool->freeList;
        pool->freeList = chunk;
    }

    return pool;
}

void AukSFEBufferPool_Destroy(AukSFEBufferPool* pool) {
    if (!pool) return;
    if (pool->memory) FreeVec(pool->memory);
    FreeVec(pool);
}

void* AukSFEBufferPool_AllocChunk(AukSFEBufferPool* pool) {
    AukSFEPoolChunk* chunk;

    if (!pool || !pool->freeList) return NULL;

    chunk = pool->freeList;
    pool->freeList = chunk->next;
    chunk->next = NULL;
    pool->usedChunks++;

    return chunk->data;
}

void AukSFEBufferPool_FreeChunk(AukSFEBufferPool* pool, void* data) {
    AukSFEPoolChunk* chunk;

    if (!pool || !data) return;

    /* Calculate chunk address from data pointer */
    chunk = (AukSFEPoolChunk*)((unsigned char*)data - /*offsetof(AukSFEPoolChunk, data)*/sizeof(struct AukSFEPoolChunk* ));

    chunk->next = pool->freeList;
    pool->freeList = chunk;
    if (pool->usedChunks > 0) pool->usedChunks--;
}

void AukSFEBufferPool_GetStats(AukSFEBufferPool* pool,
                                unsigned long* outTotal,
                                unsigned long* outUsed,
                                unsigned long* outFree) {
    if (!pool) return;
    if (outTotal) *outTotal = pool->totalChunks;
    if (outUsed) *outUsed = pool->usedChunks;
    if (outFree) *outFree = pool->totalChunks - pool->usedChunks;
}

/* ============================================================
 * Worker Thread Jobs
 * ============================================================ */

/* Phase 1: Open file, detect format, read metadata */
static int job_initPhase1(AukSoundFileEngine* engine, AukSoundFile* soundFile) {
    AukSFFileFormat format;
    SoundReaderPlugin* plugin;
    void* pluginData;
    char absPath[512];

    if (!soundFile || !soundFile->filename) return -1;

    /* Build absolute path */
    if (engine->basePath && soundFile->filename[0] != '/') {
        snprintf(absPath, sizeof(absPath), "%s/%s", engine->basePath, soundFile->filename);
    } else {
        strncpy(absPath, soundFile->filename, sizeof(absPath) - 1);
        absPath[sizeof(absPath) - 1] = '\0';
    }

    /* Detect format */
    format = AukSoundFile_DetectFormat(soundFile->filename);
    if (format == AUKSF_FORMAT_UNKNOWN) {
        soundFile->status = AUKSF_STATUS_ERROR;
        return -1;
    }

    /* Get plugin */
    plugin = AukSoundFile_GetPlugin(format);
    if (!plugin) {
        soundFile->status = AUKSF_STATUS_ERROR;
        return -1;
    }

    /* Open file with plugin */
    pluginData = plugin->open(soundFile, absPath);
    if (!pluginData) {
        soundFile->status = AUKSF_STATUS_ERROR;
        return -1;
    }

    soundFile->fileformatreader = plugin;
    soundFile->soundReaderPluginData = pluginData;

    /* Allocate buffer parts and min/max arrays */
    AukSoundFile_AllocBufferParts(soundFile);
    AukSoundFile_AllocMinMax(soundFile);

    soundFile->status = AUKSF_STATUS_STATED_PHASE1;

    printf("[Worker] Phase 1 complete: %s - %lu Hz, %lu ch, %lu frames\n",
           soundFile->fileformat,
           soundFile->sampleRate,
           soundFile->channels,
           soundFile->frameCount);

    return 0;
}

/* Phase 2: Compute min/max statistics (one chunk at a time) */
static int job_initPhase2Chunk(AukSoundFileEngine* engine, AukSoundFile* soundFile, unsigned long chunkIndex) {
    unsigned long iChan;
    unsigned long sampleStart, sampleEnd, numSamples;
    signed short tempBuffer[256];
    unsigned long i;

    (void)engine;

    if (!soundFile || !soundFile->fileformatreader || !soundFile->minmaxdiv256) {
        return -1;
    }

    /* Calculate sample range for this 256-sample chunk */
    sampleStart = chunkIndex << 8;
    if (sampleStart >= soundFile->frameCount) {
        /* All chunks done */
        soundFile->status = AUKSF_STATUS_STATED_PHASE3;
        return 1;  /* Return 1 to indicate completion */
    }

    sampleEnd = sampleStart + 256;
    if (sampleEnd > soundFile->frameCount) {
        sampleEnd = soundFile->frameCount;
    }
    numSamples = sampleEnd - sampleStart;

    /* Process each channel */
    for (iChan = 0; iChan < soundFile->channels; iChan++) {
        signed short minVal = 32767;
        signed short maxVal = -32768;

        /* We need a temporary SoundBufferPart to read the data */
        SoundBufferPart tempPart;
        tempPart._sampleoffset = sampleStart;
        tempPart._nbSamples = numSamples;
        tempPart._buffer = tempBuffer;
        tempPart._sampleRate = soundFile->sampleRate;

        /* Read samples */
        if (soundFile->fileformatreader->read(soundFile, &tempPart, iChan) != 0) {
            /* Read error - fill with zeros */
            for (i = 0; i < numSamples; i++) {
                tempBuffer[i] = 0;
            }
        }

        /* Find min/max */
        for (i = 0; i < numSamples; i++) {
            if (tempBuffer[i] < minVal) minVal = tempBuffer[i];
            if (tempBuffer[i] > maxVal) maxVal = tempBuffer[i];
        }

        /* Convert to 0-255 range (128 = zero crossing) */
        /* 16-bit range: -32768 to 32767 -> 0 to 255 */
        {
            AukSFMinMax* mm = &soundFile->minmaxdiv256[iChan * soundFile->minmaxStride + chunkIndex];
            mm->min = (unsigned char)((minVal + 32768) >> 8);
            mm->max = (unsigned char)((maxVal + 32768) >> 8);
        }
    }

    soundFile->minmaxdiv256_length = chunkIndex + 1;

    /* Check if all chunks done */
    if (chunkIndex + 1 >= (unsigned long)soundFile->minmaxStride) {
        soundFile->status = AUKSF_STATUS_STATED_PHASE3;
        return 1;
    }

    return 0;
}

/* Load a buffer part for a consumer */
static int job_loadBufferPart(AukSoundFileEngine* engine, AukSoundFile* soundFile, int iChannel, int iPart) {
    SoundBufferPart* part;
    void* buffer;

    if (!soundFile || !soundFile->buffers || !soundFile->fileformatreader) {
        return -1;
    }

    if (iChannel < 0 || iChannel >= (int)soundFile->channels) return -1;
    if (iPart < 0 || iPart >= soundFile->nbBufferParts) return -1;

    part = &soundFile->buffers[iChannel][iPart];

    /* Already loaded? */
    if (part->_state == 1 && part->_buffer != NULL) {
        return 0;
    }

    /* Allocate buffer from pool */
    buffer = AukSFEBufferPool_AllocChunk(engine->bufferPool);
    if (!buffer) {
        /* Pool full - try to evict an unused buffer */
        /* TODO: Implement LRU eviction */
        printf("[Worker] Buffer pool full, cannot load part %d/%d\n", iChannel, iPart);
        return -1;
    }

    part->_buffer = (signed short*)buffer;

    /* Read from file */
    if (soundFile->fileformatreader->read(soundFile, part, iChannel) != 0) {
        /* Read failed */
        AukSFEBufferPool_FreeChunk(engine->bufferPool, buffer);
        part->_buffer = NULL;
        return -1;
    }

    part->_state = 1;
    /* TODO: Set _lastAccessTime for LRU */

    return 0;
}

/* ============================================================
 * Worker Thread
 * ============================================================ */

/* Shared data between main and worker */
typedef struct {
    AukSoundFileEngine* engine;
    int workerReady;
} AukSFESharedData;

static AukSFESharedData* g_sharedData = NULL;

/*
 * Worker thread entry point.
 * Uses Amiga-style message passing on all platforms:
 * - On Amiga: native CreateNewProcTags + MsgPort
 * - On PC: AmigaStack's CreateNewProcSimple + emulated MsgPort
 */
static void SoundFileWorkerThread(void)
{
    AukSoundFileEngine* engine;
    AukSFEFileNode* node;
    int running = 1;
    int workDone;
    struct Process* thisProcess = (struct Process*)FindTask(NULL);
    struct MsgPort* workerPort = &thisProcess->pr_MsgPort;
    AukSFEMessage* msg;

    /* Wait for handshake message from main thread */
    WaitPort(workerPort);
    msg = (AukSFEMessage*)GetMsg(workerPort);
    if (!msg || !g_sharedData) {
        if (msg) ReplyMsg(&msg->msg);
        return;
    }
    engine = g_sharedData->engine;
    g_sharedData->workerReady = 1;
    ReplyMsg(&msg->msg);

    printf("[Worker] Started\n");

    /* Main worker loop */
    while (running) {
        /* Drain all pending messages (non-blocking) */
        while ((msg = (AukSFEMessage*)GetMsg(workerPort)) != NULL) {
            switch (msg->type) {
                case AUKSFE_MSG_SHUTDOWN:
                    running = 0;
                    ReplyMsg(&msg->msg);
                    break;
                case AUKSFE_MSG_ADD_FILE:
                case AUKSFE_MSG_REMOVE_FILE:
                case AUKSFE_MSG_WAKEUP:
                    /* Just wake up to process work */
                    ReplyMsg(&msg->msg);
                    break;
                default:
                    ReplyMsg(&msg->msg);
                    break;
            }
        }

        if (!running) break;

        /* Move files from files_new to files_managed */
        Forbid();
        while (engine->files_new) {
            node = engine->files_new;
            engine->files_new = node->next;
            node->next = engine->files_managed;
            engine->files_managed = node;
        }
        Permit();

        /* Process files */
        workDone = 0;
        for (node = engine->files_managed; node && workDone < 10; node = node->next) {
            AukSoundFile* sf = node->file;
            if (!sf) continue;

            /* Phase 1: Get metadata */
            if (sf->status == AUKSF_STATUS_PENDING) {
                if (job_initPhase1(engine, sf) == 0) {
                    node->status = AUKSFE_STATUS_STATED_PHASE1;
                    /* TODO: Send notification to main process */
                } else {
                    node->status = AUKSFE_STATUS_ERROR;
                }
                workDone++;
            }

            /* Phase 2: Compute min/max statistics */
            else if (sf->status == AUKSF_STATUS_STATED_PHASE1 ||
                     sf->status == AUKSF_STATUS_STATED_PHASE2) {
                /* Do a few chunks at a time */
                int i;
                for (i = 0; i < 16 && sf->status != AUKSF_STATUS_STATED_PHASE3; i++) {
                    job_initPhase2Chunk(engine, sf, node->minmaxProgress);
                    node->minmaxProgress++;
                }
                if (sf->status == AUKSF_STATUS_STATED_PHASE2) {
                    node->status = AUKSFE_STATUS_STATED_PHASE2;
                } else if (sf->status == AUKSF_STATUS_STATED_PHASE3) {
                    node->status = AUKSFE_STATUS_STATED_PHASE3;
                    printf("[Worker] Phase 2 complete: min/max ready\n");
                }
                workDone++;
            }

            /* Phase 3: Load buffer parts on demand */
            else if (sf->status == AUKSF_STATUS_STATED_PHASE3 && sf->buffers) {
                unsigned int iChan;
                int iPart;

                /* Scan for parts that need loading (nblocks > 0 && state == 0) */
                for (iChan = 0; iChan < sf->channels && workDone < 10; iChan++) {
                    for (iPart = 0; iPart < sf->nbBufferParts && workDone < 10; iPart++) {
                        SoundBufferPart* part = &sf->buffers[iChan][iPart];
                        if (part->_nblocks > 0 && part->_state == 0) {
                            if (job_loadBufferPart(engine, sf, iChan, iPart) == 0) {
                                printf("[Worker] Loaded buffer part ch=%d p=%d\n", iChan, iPart);
                            }
                            workDone++;
                        }
                    }
                }
            }
        }

        /* If no work was done, wait for a message (blocks efficiently) */
        if (workDone == 0) {
            WaitPort(workerPort);
            /* Message is still in port - next iteration's GetMsg will retrieve it */
        }
        /* If work was done, loop immediately to continue processing */
    }

    printf("[Worker] Exiting\n");
}

/* Sound file loading engine - singleton */
AukSoundFileEngine *soundFileEngine = NULL;

/* ============================================================
 * Engine API Implementation
 * ============================================================ */

AukSoundFileEngine* AukSoundFileEngine_Init(struct Process* mainProcess,
                                             const char* basePath,
                                             unsigned long poolSizeBytes) {
    AukSoundFileEngine* engine;
    AukSFEMessage initMsg;

    if(soundFileEngine) return soundFileEngine;

    /* Default pool size: 2MB */
    if (poolSizeBytes == 0) {
        poolSizeBytes = 2 * 1024 * 1024;
    }

    engine = (AukSoundFileEngine*)AllocVec(sizeof(AukSoundFileEngine), MEMF_CLEAR | MEMF_PUBLIC);
    if (!engine) return NULL;

    /* Duplicate base path */
    if (basePath) {
        engine->basePath = AukString_Duplicate(basePath);
    }

    /* Create buffer pool */
    engine->bufferPool = AukSFEBufferPool_Create(poolSizeBytes);
    if (!engine->bufferPool) {
        if (engine->basePath) AukString_Free(engine->basePath);
        FreeVec(engine);
        return NULL;
    }

    /* Allocate shared data */
    g_sharedData = (AukSFESharedData*)AllocVec(sizeof(AukSFESharedData), MEMF_CLEAR | MEMF_PUBLIC);
    if (!g_sharedData) {
        AukSFEBufferPool_Destroy(engine->bufferPool);
        if (engine->basePath) AukString_Free(engine->basePath);
        FreeVec(engine);
        return NULL;
    }
    g_sharedData->engine = engine;
    g_sharedData->workerReady = 0;

    /* Create reply port */
    engine->mainReplyPort = CreateMsgPort();
    if (!engine->mainReplyPort) {
        FreeVec(g_sharedData);
        g_sharedData = NULL;
        AukSFEBufferPool_Destroy(engine->bufferPool);
        if (engine->basePath) AukString_Free(engine->basePath);
        FreeVec(engine);
        return NULL;
    }

    /* Create worker process/thread */
    {
#ifdef AMIGA
        engine->workerProcess = CreateNewProcTags(
            NP_Entry, (ULONG)SoundFileWorkerThread,
            NP_Name, (ULONG)"AukSoundFileWorker",
            NP_Output, mainProcess->pr_COS,
            NP_CloseOutput, FALSE,
            NP_FreeSeglist, FALSE,
            TAG_END
        );
#else
        struct TagItem tags[4];
        (void)mainProcess;
        tags[0].ti_Tag = NP_Entry;
        tags[0].ti_Data = (uintptr_t)SoundFileWorkerThread;
        tags[1].ti_Tag = NP_Name;
        tags[1].ti_Data = (uintptr_t)"AukSoundFileWorker";
        tags[2].ti_Tag = NP_Priority;
        tags[2].ti_Data = 0;
        tags[3].ti_Tag = TAG_DONE;
        engine->workerProcess = CreateNewProc(tags);
#endif
    }
    if (!engine->workerProcess) {
        DeleteMsgPort(engine->mainReplyPort);
        FreeVec(g_sharedData);
        g_sharedData = NULL;
        AukSFEBufferPool_Destroy(engine->bufferPool);
        if (engine->basePath) AukString_Free(engine->basePath);
        FreeVec(engine);
        return NULL;
    }

    /* Handshake: send init message and wait for reply */
    memset(&initMsg, 0, sizeof(initMsg));
    initMsg.msg.mn_ReplyPort = engine->mainReplyPort;
    initMsg.msg.mn_Length = sizeof(AukSFEMessage);
    initMsg.type = AUKSFE_MSG_NONE;

    PutMsg(&engine->workerProcess->pr_MsgPort, &initMsg.msg);
    WaitPort(engine->mainReplyPort);
    (void)GetMsg(engine->mainReplyPort);

    if (!g_sharedData->workerReady) {
        printf("[Engine] Worker failed to start\n");
        DeleteMsgPort(engine->mainReplyPort);
        FreeVec(g_sharedData);
        g_sharedData = NULL;
        AukSFEBufferPool_Destroy(engine->bufferPool);
        if (engine->basePath) AukString_Free(engine->basePath);
        FreeVec(engine);
        return NULL;
    }

    engine->workerPort = &engine->workerProcess->pr_MsgPort;
    engine->workerRunning = 1;
    engine->files_new = NULL;
    engine->files_managed = NULL;
    engine->shutdownRequested = 0;
    engine->jobsCount = 0;

    printf("[Engine] Initialized with %lu KB buffer pool\n", poolSizeBytes / 1024);

    soundFileEngine = engine;
    return engine;
}

void AukSoundFileEngine_Shutdown(AukSoundFileEngine* engine) {
    AukSFEFileNode* node;
    AukSFEFileNode* next;

    if (!engine) return;

    /* Send shutdown message to worker and wait for reply */
    if (engine->workerProcess && engine->workerRunning) {
        AukSFEMessage shutdownMsg;
        memset(&shutdownMsg, 0, sizeof(shutdownMsg));
        shutdownMsg.msg.mn_ReplyPort = engine->mainReplyPort;
        shutdownMsg.msg.mn_Length = sizeof(AukSFEMessage);
        shutdownMsg.type = AUKSFE_MSG_SHUTDOWN;

        PutMsg(&engine->workerProcess->pr_MsgPort, &shutdownMsg.msg);
        WaitPort(engine->mainReplyPort);
        (void)GetMsg(engine->mainReplyPort);
    }

    engine->workerRunning = 0;

    /* Free all file nodes */
    node = engine->files_new;
    while (node) {
        next = node->next;
        if (node->file) {
            AukSoundFile_Delete((AukObject*)node->file);
        }
        FreeVec(node);
        node = next;
    }

    node = engine->files_managed;
    while (node) {
        next = node->next;
        if (node->file) {
            AukSoundFile_Delete((AukObject*)node->file);
        }
        FreeVec(node);
        node = next;
    }

    engine->files_new = NULL;
    engine->files_managed = NULL;

    /* Cleanup */
    if (engine->mainReplyPort) DeleteMsgPort(engine->mainReplyPort);
    if (engine->bufferPool) AukSFEBufferPool_Destroy(engine->bufferPool);
    if (engine->basePath) AukString_Free(engine->basePath);

    if (g_sharedData) {
        FreeVec(g_sharedData);
        g_sharedData = NULL;
    }

    FreeVec(engine);
    soundFileEngine = NULL;
    printf("[Engine] Shutdown complete\n");
}

struct AukSoundFile* AukSoundFileEngine_RequestFile(AukSoundFileEngine* engine, const char* filename) {
    AukSFEFileNode* node;
    AukSFEFileNode* newNode;
    AukSoundFile* file;

 return NULL;

    if (!engine || !filename) return NULL;

    /* Check if file already exists */
    Forbid();
    for (node = engine->files_new; node; node = node->next) {
        if (node->file) {
            const char* existingPath = node->file->GetFilename(node->file);
            if (existingPath && strcmp(existingPath, filename) == 0) {
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
                node->refCount++;
                Permit();
                return node->file;
            }
        }
    }
    Permit();

    /* Create new sound file */
    {
        AukObjectPtr newFilePtr = NULL;
        AukSoundFile_New(&newFilePtr);
        file = (AukSoundFile*)newFilePtr;
    }
    if (!file) return NULL;

    file->SetFilename(file, filename);

    /* Create file node */
    newNode = (AukSFEFileNode*)AllocVec(sizeof(AukSFEFileNode), MEMF_CLEAR | MEMF_PUBLIC);
    if (!newNode) {
        AukSoundFile_Delete((AukObject*)file);
        return NULL;
    }

    newNode->file = file;
    newNode->status = AUKSFE_STATUS_PENDING;
    newNode->refCount = 1;
    newNode->minmaxProgress = 0;

    Forbid();
    newNode->next = engine->files_new;
    engine->files_new = newNode;
    Permit();

    /* Wake worker */
    AukSoundFileEngine_WakeWorker(engine);

    return file;
}

void AukSoundFileEngine_ReleaseFile(AukSoundFileEngine* engine, struct AukSoundFile* file) {
    AukSFEFileNode* node;
    AukSFEFileNode* prev;

    if (!engine || !file) return;

    Forbid();

    /* Search in files_new */
    prev = NULL;
    for (node = engine->files_new; node; prev = node, node = node->next) {
        if (node->file == file) {
            node->refCount--;
            if (node->refCount <= 0) {
                if (prev) prev->next = node->next;
                else engine->files_new = node->next;
                Permit();
                AukSoundFile_Delete((AukObject*)node->file);
                FreeVec(node);
                return;
            }
            Permit();
            return;
        }
    }

    /* Search in files_managed */
    prev = NULL;
    for (node = engine->files_managed; node; prev = node, node = node->next) {
        if (node->file == file) {
            node->refCount--;
            if (node->refCount <= 0) {
                if (prev) prev->next = node->next;
                else engine->files_managed = node->next;
                Permit();
                AukSoundFile_Delete((AukObject*)node->file);
                FreeVec(node);
                return;
            }
            Permit();
            return;
        }
    }

    Permit();
}

AukSFEFileStatus AukSoundFileEngine_GetFileStatus(AukSoundFileEngine* engine, struct AukSoundFile* file) {
    AukSFEFileNode* node;

    if (!engine || !file) return AUKSFE_STATUS_NONE;

    for (node = engine->files_new; node; node = node->next) {
        if (node->file == file) return node->status;
    }
    for (node = engine->files_managed; node; node = node->next) {
        if (node->file == file) return node->status;
    }

    return AUKSFE_STATUS_NONE;
}

int AukSoundFileEngine_ProcessNotifications(AukSoundFileEngine* engine) {
    /* TODO: Process reply messages from worker */
    (void)engine;
    return 0;
}

ULONG AukSoundFileEngine_GetSignalMask(AukSoundFileEngine* engine) {
    if (!engine || !engine->mainReplyPort) return 0;
#ifdef AMIGA
    return 1L << engine->mainReplyPort->mp_SigBit;
#else
    return 0;
#endif
}

void AukSoundFileEngine_WakeWorker(AukSoundFileEngine* engine) {
    if (engine && engine->workerPort && engine->mainReplyPort) {
        AukSFEMessage wakeMsg;
        memset(&wakeMsg, 0, sizeof(wakeMsg));
        wakeMsg.msg.mn_ReplyPort = engine->mainReplyPort;
        wakeMsg.msg.mn_Length = sizeof(AukSFEMessage);
        wakeMsg.type = AUKSFE_MSG_WAKEUP;
        PutMsg(engine->workerPort, &wakeMsg.msg);
        /* Don't wait for reply */
    }
}
