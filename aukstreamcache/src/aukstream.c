/*
 * AukStreamCache - Main Engine Implementation
 * Initialization, shutdown, and public API
 */

#include "aukstream.h"
#include "aukstreaminternal.h"
#include "aukstreamloader.h"
#include "aukstring.h"
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <proto/exec.h>
#include <proto/dos.h>

#include "aukstreamprivate.h"

/* Forward declarations for internal functions */
static void aukstream_CacheProcessEntry(void);

/*
 * Initialize the stream cache engine
 */
AukStreamEngine* aukstream_Init(const AukStreamConfig* config) {
    AukStreamEngine* engine;
    struct TagItem processTags[5];

    if (!config) {
        return NULL;
    }

    /* Validate configuration */
    if (config->cacheSize < 262144) {  /* Minimum 256KB */
        return NULL;
    }

    if (!config->soundDirectory) {
        return NULL;
    }

    /* Allocate engine structure */
    engine = (AukStreamEngine*)AllocVec(sizeof(AukStreamEngine), MEMF_CLEAR);
    if (!engine) {
        return NULL;
    }

    /* Copy configuration */
    engine->config.cacheSize = config->cacheSize;
    engine->config.conversionMode = config->conversionMode;
    engine->config.soundDirectory = AukString_Duplicate(config->soundDirectory);
    if (!engine->config.soundDirectory) {
        FreeVec(engine);
        return NULL;
    }

    /* Create message port for main process */
    engine->mainPort = CreateMsgPort();
    if (!engine->mainPort) {
        AukString_Free(engine->config.soundDirectory);
        FreeVec(engine);
        return NULL;
    }

    /* Create memory pool */
    engine->pool = aukstreampool_Create(config->cacheSize);
    if (!engine->pool) {
        DeleteMsgPort(engine->mainPort);
        AukString_Free(engine->config.soundDirectory);
        FreeVec(engine);
        return NULL;
    }

    /* Initialize stream list */
    engine->streamList = NULL;
    engine->streamCount = 0;
    engine->shutdownFlag = 0;

    /* Set up process creation tags */
    processTags[0].ti_Tag = NP_Entry;
    processTags[0].ti_Data = (ULONG)aukstream_CacheProcessEntry;
    processTags[1].ti_Tag = NP_Name;
    processTags[1].ti_Data = (ULONG)"AukStreamCache";
    processTags[2].ti_Tag = NP_Priority;
    processTags[2].ti_Data = 0;
    processTags[3].ti_Tag = NP_StackSize;
    processTags[3].ti_Data = 8192;
    processTags[4].ti_Tag = TAG_DONE;
    processTags[4].ti_Data = 0;

    /* Create cache process */
    engine->cacheProcess = CreateNewProc(processTags);
    if (!engine->cacheProcess) {
        aukstreampool_Destroy(engine->pool);
        DeleteMsgPort(engine->mainPort);
        AukString_Free(engine->config.soundDirectory);
        FreeVec(engine);
        return NULL;
    }

    /* Wait for cache process to create its message port */
    /* In a real implementation, the cache process would signal back */
    /* For now, we'll create the cache port in the main process */
    engine->cachePort = CreateMsgPort();
    if (!engine->cachePort) {
        /* TODO: Send shutdown message to cache process */
        aukstreampool_Destroy(engine->pool);
        DeleteMsgPort(engine->mainPort);
        AukString_Free(engine->config.soundDirectory);
        FreeVec(engine);
        return NULL;
    }

    return engine;
}

/*
 * Shutdown the stream cache engine
 */
void aukstream_Shutdown(AukStreamEngine* engine) {
    AukCachedStream* stream;
    AukCachedStream* next;
    AukStreamMessage* msg;
    unsigned long i;

    if (!engine) return;

    /* Set shutdown flag */
    engine->shutdownFlag = 1;

    /* Send shutdown message to cache process */
    if (engine->cachePort) {
        msg = (AukStreamMessage*)AllocVec(sizeof(AukStreamMessage), MEMF_CLEAR);
        if (msg) {
            msg->execMsg.mn_ReplyPort = engine->mainPort;
            msg->execMsg.mn_Length = sizeof(AukStreamMessage);
            msg->type = AUK_MSG_SHUTDOWN;
            PutMsg(engine->cachePort, (struct Message*)msg);

            /* Wait for reply */
            WaitPort(engine->mainPort);
            GetMsg(engine->mainPort);
            FreeVec(msg);
        }

        DeleteMsgPort(engine->cachePort);
    }

    /* TODO: Wait for cache process to exit properly */

    /* Free all cached streams */
    stream = engine->streamList;
    while (stream) {
        next = stream->next;

        /* Free chunks */
        if (stream->chunks) {
            for (i = 0; i < stream->chunkCount; i++) {
                if (stream->chunks[i]) {
                    aukstreampool_FreeChunk(engine->pool, stream->chunks[i]);
                }
            }
            FreeVec(stream->chunks);
        }

        /* Free filename */
        if (stream->filename) {
            AukString_Free(stream->filename);
        }

        FreeVec(stream);
        stream = next;
    }

    /* Destroy memory pool */
    if (engine->pool) {
        aukstreampool_Destroy(engine->pool);
    }

    /* Delete message port */
    if (engine->mainPort) {
        DeleteMsgPort(engine->mainPort);
    }

    /* Free configuration */
    if (engine->config.soundDirectory) {
        AukString_Free(engine->config.soundDirectory);
    }

    /* Free engine */
    FreeVec(engine);
}

/*
 * Cache process entry point (called by AmigaOS)
 */
static void aukstream_CacheProcessEntry(void) {
    /* This will be implemented in Phase 2 */
    /* For now, just wait for shutdown message */

    struct MsgPort* port;
    struct Message* msg;
    AukStreamMessage* streamMsg;
    int running = 1;

    port = CreateMsgPort();
    if (!port) {
        return;
    }

    while (running) {
        WaitPort(port);
        while ((msg = GetMsg(port))) {
            streamMsg = (AukStreamMessage*)msg;

            if (streamMsg->type == AUK_MSG_SHUTDOWN) {
                running = 0;
            }

            streamMsg->result = AUK_STREAM_OK;
            ReplyMsg(msg);
        }
    }

    DeleteMsgPort(port);
}

/*
 * Request a stream to be loaded and cached
 */
AukStreamCacheResult aukstream_RequestStream(AukStreamEngine* engine,
                                              const AukStreamRequest* request,
                                              AukCachedStream** outStream) {
    AukCachedStream* stream;
    AukStreamCacheResult result;

    if (!engine || !request || !outStream) {
        return AUK_STREAM_ERROR_INVALID;
    }

    *outStream = NULL;

    /* Check if stream already cached */
    stream = aukstream_FindStream(engine, request->filename);
    if (stream) {
        /* Already in cache - increment reference count */
        stream->refCount++;
        aukstream_Touch(stream);
        *outStream = stream;
        return AUK_STREAM_OK;
    }

    /* Not in cache - load it */
    result = aukloader_LoadStream(engine, request, &stream);
    if (result != AUK_STREAM_OK) {
        return result;
    }

    *outStream = stream;
    return AUK_STREAM_OK;
}

/*
 * Check if a stream is ready for playback
 */
int aukstream_IsReady(AukCachedStream* stream) {
    if (!stream) return 0;
    return stream->ready;
}

/*
 * Get information about a cached stream
 */
AukStreamCacheResult aukstream_GetInfo(AukCachedStream* stream,
                                        AukStreamInfo* outInfo) {
    if (!stream || !outInfo) {
        return AUK_STREAM_ERROR_INVALID;
    }

    *outInfo = stream->info;
    return AUK_STREAM_OK;
}

/*
 * Get zero-copy access to cached audio data
 */
AukStreamCacheResult aukstream_GetAudioData(AukCachedStream* stream,
                                             unsigned long frameOffset,
                                             unsigned long frameCount,
                                             const void** outData,
                                             unsigned long* outAvailable) {
    unsigned long chunkIndex;
    unsigned long framesPerChunk;
    unsigned long frameOffsetInChunk;
    unsigned long availableInChunk;
    void* chunkData;
    unsigned long byteOffset;
    unsigned long remainingFrames;

    if (!stream || !outData || !outAvailable) {
        return AUK_STREAM_ERROR_INVALID;
    }

    *outData = NULL;
    *outAvailable = 0;

    /* Check if stream is ready */
    if (!stream->ready) {
        return AUK_STREAM_ERROR_NOTFOUND;
    }

    /* Validate frame offset */
    if (frameOffset >= stream->info.frameCount) {
        return AUK_STREAM_OK;  /* Beyond end of stream */
    }

    /* Calculate which chunk contains the requested frame */
    framesPerChunk = AUK_STREAM_CHUNK_SIZE / stream->info.bytesPerFrame;
    chunkIndex = frameOffset / framesPerChunk;
    frameOffsetInChunk = frameOffset % framesPerChunk;

    if (chunkIndex >= stream->chunkCount) {
        return AUK_STREAM_OK;  /* Beyond cached data */
    }

    /* Get pointer to chunk data */
    if (!stream->chunks[chunkIndex]) {
        return AUK_STREAM_ERROR_NOTFOUND;
    }

    chunkData = aukstreampool_GetChunkData(stream->chunks[chunkIndex]);
    if (!chunkData) {
        return AUK_STREAM_ERROR_NOTFOUND;
    }

    /* Calculate byte offset within chunk */
    byteOffset = frameOffsetInChunk * stream->info.bytesPerFrame;
    *outData = (const void*)((unsigned char*)chunkData + byteOffset);

    /* Calculate available frames in this chunk */
    availableInChunk = framesPerChunk - frameOffsetInChunk;

    /* Limit to remaining frames in stream */
    remainingFrames = stream->info.frameCount - frameOffset;
    if (availableInChunk > remainingFrames) {
        availableInChunk = remainingFrames;
    }

    /* Limit to requested count */
    if (availableInChunk > frameCount) {
        availableInChunk = frameCount;
    }

    *outAvailable = availableInChunk;
    return AUK_STREAM_OK;
}

/*
 * Release a stream reference
 */
void aukstream_ReleaseStream(AukCachedStream* stream) {
    if (!stream) return;

    if (stream->refCount > 0) {
        stream->refCount--;
    }
}

/*
 * Touch a stream to update its last-used time
 */
void aukstream_Touch(AukCachedStream* stream) {
//    struct DateStamp ds;

    if (!stream) return;

//    DateStamp(&ds);
    DateStamp(&stream->lastUsedTime);
//    stream->lastUsedTime = ds.ds_Tick + (ds.ds_Minute * 3000);

}

/*
 * Get cache statistics
 */
AukStreamCacheResult aukstream_GetCacheStats(AukStreamEngine* engine,
                                              unsigned long* outUsedBytes,
                                              unsigned long* outTotalBytes,
                                              unsigned long* outStreamCount) {
    unsigned long totalChunks, usedChunks, freeChunks;

    if (!engine) {
        return AUK_STREAM_ERROR_INVALID;
    }

    if (outTotalBytes) {
        *outTotalBytes = engine->config.cacheSize;
    }

    if (outStreamCount) {
        *outStreamCount = engine->streamCount;
    }

    if (outUsedBytes) {
        aukstreampool_GetStats(engine->pool, &totalChunks, &usedChunks, &freeChunks);
        *outUsedBytes = usedChunks * sizeof(struct sAukStreamChunk);
    }

    return AUK_STREAM_OK;
}

/*
 * Find a stream by filename
 */
AukCachedStream* aukstream_FindStream(AukStreamEngine* engine, const char* filename) {
    AukCachedStream* stream;

    if (!engine || !filename) {
        return NULL;
    }

    stream = engine->streamList;
    while (stream) {
        if (AukString_Compare(stream->filename, filename) == 0) {
            return stream;
        }
        stream = stream->next;
    }

    return NULL;
}

/*
 * Compare DateStamps for LRU ordering
 * Returns: <0 if a is older, >0 if b is older, 0 if equal
 */
static long CompareDateStamps(const struct DateStamp* a, const struct DateStamp* b) {
    /* Compare days first */
    if (a->ds_Days != b->ds_Days) {
        return a->ds_Days - b->ds_Days;
    }

    /* Compare minutes */
    if (a->ds_Minute != b->ds_Minute) {
        return a->ds_Minute - b->ds_Minute;
    }

    /* Compare ticks */
    return a->ds_Tick - b->ds_Tick;
}

/*
 * Evict least-recently-used streams to free memory
 */
AukStreamCacheResult aukstream_EvictLRU(AukStreamEngine* engine, unsigned long bytesNeeded) {
    AukCachedStream* stream;
    AukCachedStream* oldestStream = NULL;
    unsigned long chunksNeeded;
    unsigned long chunksFreed = 0;
    unsigned long totalChunks, usedChunks, freeChunks;

    if (!engine) {
        return AUK_STREAM_ERROR_INVALID;
    }

    /* Calculate chunks needed */
    chunksNeeded = (bytesNeeded + AUK_STREAM_CHUNK_SIZE - 1) / AUK_STREAM_CHUNK_SIZE;

    /* Keep evicting until we have enough free chunks */
    while (chunksFreed < chunksNeeded) {
        /* Find the oldest unreferenced stream */
        oldestStream = NULL;
        stream = engine->streamList;

        while (stream) {
            /* Only consider streams with refCount == 0 */
            if (stream->refCount == 0) {
                if (!oldestStream ||
                    CompareDateStamps(&stream->lastUsedTime, &oldestStream->lastUsedTime) < 0) {
                    oldestStream = stream;
                }
            }
            stream = stream->next;
        }

        /* If no evictable stream found, we can't free enough memory */
        if (!oldestStream) {
            return AUK_STREAM_ERROR_MEMORY;
        }

        /* Evict the oldest stream */
        chunksFreed += oldestStream->chunkCount;
        aukloader_UnloadStream(engine, oldestStream);

        /* Check if we have enough free chunks now */
        aukstreampool_GetStats(engine->pool, &totalChunks, &usedChunks, &freeChunks);
        if (freeChunks >= chunksNeeded) {
            break;
        }
    }

    return AUK_STREAM_OK;
}
