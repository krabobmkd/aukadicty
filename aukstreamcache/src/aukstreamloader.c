/*
 * AukStreamLoader - Stream Loading Implementation
 */

#include "aukstreamloader.h"
#include "aukstreamwave.h"
#include "aukstream8svx.h"
#include "aukstreamconvert.h"
#include "aukstring.h"
#include <proto/dos.h>
#include <proto/exec.h>
#include <string.h>
#include "aukstreamprivate.h"

/*
 * Detect file format from filename extension
 */
AukStreamFileFormat aukloader_DetectFormat(const char* filename) {
    const char* ext;
    unsigned long len;

    if (!filename) {
        return AUK_STREAM_FORMAT_UNKNOWN;
    }

    len = strlen(filename);
    if (len < 4) {
        return AUK_STREAM_FORMAT_UNKNOWN;
    }

    /* Find extension */
    ext = filename + len - 4;

    /* Check for .wav or .wave */
    if (strcmp(ext, ".wav") == 0 || strcmp(ext, ".WAV") == 0) {
        return AUK_STREAM_FORMAT_WAVE;
    }

    if (len >= 5) {
        ext = filename + len - 5;
        if (strcmp(ext, ".wave") == 0 || strcmp(ext, ".WAVE") == 0) {
            return AUK_STREAM_FORMAT_WAVE;
        }
    }

    /* Check for .8svx or .svx */
    ext = filename + len - 5;
    if (strcmp(ext, ".8svx") == 0 || strcmp(ext, ".8SVX") == 0) {
        return AUK_STREAM_FORMAT_8SVX;
    }

    ext = filename + len - 4;
    if (strcmp(ext, ".svx") == 0 || strcmp(ext, ".SVX") == 0) {
        return AUK_STREAM_FORMAT_8SVX;
    }

    return AUK_STREAM_FORMAT_UNKNOWN;
}

/*
 * Determine target data type based on conversion mode
 */
static AukStreamDataType GetTargetDataType(AukStreamConversionMode mode,
                                            AukStreamDataType sourceType) {
    switch (mode) {
        case AUK_STREAM_CONVERT_NONE:
            return sourceType;

        case AUK_STREAM_CONVERT_16BIT:
            return AUK_STREAM_16BIT_SIGNED;

        case AUK_STREAM_CONVERT_32BIT:
            return AUK_STREAM_32BIT_SIGNED;

        default:
            return sourceType;
    }
}

/*
 * Load a stream from file into cache
 */
AukStreamCacheResult aukloader_LoadStream(AukStreamEngine* engine,
                                           const AukStreamRequest* request,
                                           AukCachedStream** outStream) {
    char* fullPath = NULL;
    BPTR file = 0;
    AukStreamFileFormat format;
    AukStreamCacheResult result = AUK_STREAM_OK;
    AukCachedStream* stream = NULL;
    AukStreamDataType sourceType, targetType;
    unsigned long sourceChannels, sampleRate, totalFrames;
    unsigned long framesToLoad, framesLoaded;
    unsigned long chunkIndex;
    void* tempBuffer = NULL;
    unsigned long tempBufferSize;

    if (!engine || !request || !outStream) {
        return AUK_STREAM_ERROR_INVALID;
    }

    *outStream = NULL;

    /* Construct full path */
    fullPath = AukString_MakeAbsolutePath(engine->config.soundDirectory, request->filename);
    if (!fullPath) {
        return AUK_STREAM_ERROR_MEMORY;
    }

    /* Detect format */
    format = aukloader_DetectFormat(fullPath);
    if (format == AUK_STREAM_FORMAT_UNKNOWN) {
        AukString_Free(fullPath);
        return AUK_STREAM_ERROR_FORMAT;
    }

    /* Open file */
    file = Open(fullPath, MODE_OLDFILE);
    if (!file) {
        AukString_Free(fullPath);
        return AUK_STREAM_ERROR_FILE;
    }

    /* Parse file based on format */
    if (format == AUK_STREAM_FORMAT_WAVE) {
        AukWaveInfo waveInfo;
        result = aukwave_ParseHeader(file, &waveInfo);
        if (result != AUK_STREAM_OK) {
            Close(file);
            AukString_Free(fullPath);
            return result;
        }

        sourceType = waveInfo.dataType;
        sourceChannels = waveInfo.channels;
        sampleRate = waveInfo.sampleRate;
        totalFrames = waveInfo.frameCount;

    } else if (format == AUK_STREAM_FORMAT_8SVX) {
        Auk8SVXInfo svxInfo;
        result = auk8svx_ParseHeader(file, &svxInfo);
        if (result != AUK_STREAM_OK) {
            Close(file);
            AukString_Free(fullPath);
            return result;
        }

        sourceType = svxInfo.dataType;
        sourceChannels = svxInfo.channels;
        sampleRate = svxInfo.sampleRate;
        totalFrames = svxInfo.frameCount;

    } else {
        Close(file);
        AukString_Free(fullPath);
        return AUK_STREAM_ERROR_FORMAT;
    }

    /* Determine target type based on conversion mode */
    targetType = GetTargetDataType(engine->config.conversionMode, sourceType);

    /* Determine which frames to load based on request */
    unsigned long startFrame = request->fileStartFrame;
    unsigned long endFrame = request->fileEndFrame;

    /* If time-based request, convert to frames */
    if (request->startTime != 0 || request->endTime != 0) {
        /* Convert fixed-point time to frames */
        if (request->startTime != 0) {
            /* startFrame = (startTime * sampleRate) >> 32 */
            long long temp = (long long)request->startTime * (long long)sampleRate;
            startFrame = (unsigned long)(temp >> 32);
        }

        if (request->endTime != 0) {
            /* endFrame = (endTime * sampleRate) >> 32 */
            long long temp = (long long)request->endTime * (long long)sampleRate;
            endFrame = (unsigned long)(temp >> 32);
        }
    }

    /* Validate and limit frame range */
    if (startFrame >= totalFrames) {
        startFrame = 0;
    }

    if (endFrame == 0 || endFrame > totalFrames) {
        endFrame = totalFrames;
    }

    if (endFrame <= startFrame) {
        Close(file);
        AukString_Free(fullPath);
        return AUK_STREAM_ERROR_INVALID;
    }

    framesToLoad = endFrame - startFrame;

    /* Allocate stream structure */
    stream = (AukCachedStream*)AllocVec(sizeof(AukCachedStream), MEMF_CLEAR);
    if (!stream) {
        Close(file);
        AukString_Free(fullPath);
        return AUK_STREAM_ERROR_MEMORY;
    }

    /* Initialize stream */
    stream->filename = AukString_Duplicate(request->filename);
    stream->info.dataType = targetType;
    stream->info.channels = sourceChannels;
    stream->info.sampleRate = sampleRate;
    stream->info.frameCount = framesToLoad;
    stream->info.bytesPerFrame = aukconvert_GetBytesPerFrame(targetType, sourceChannels);
    stream->refCount = 1;
    stream->ready = 0;
    stream->next = NULL;

    DateStamp(&stream->lastUsedTime);

    /* Calculate number of chunks needed */
    unsigned long totalBytes = framesToLoad * stream->info.bytesPerFrame;
    stream->chunkCount = (totalBytes + AUK_STREAM_CHUNK_SIZE - 1) / AUK_STREAM_CHUNK_SIZE;

    /* Check if we have enough free chunks, evict if needed */
    unsigned long totalChunks, usedChunks, freeChunks;
    aukstreampool_GetStats(engine->pool, &totalChunks, &usedChunks, &freeChunks);

    if (freeChunks < stream->chunkCount) {
        /* Try to evict LRU streams to make room */
        result = aukstream_EvictLRU(engine, totalBytes);
        if (result != AUK_STREAM_OK) {
            /* Could not free enough memory */
            AukString_Free(stream->filename);
            FreeVec(stream);
            Close(file);
            AukString_Free(fullPath);
            return AUK_STREAM_ERROR_MEMORY;
        }
    }

    /* Allocate chunk pointer array */
    stream->chunks = (AukStreamChunk**)AllocVec(stream->chunkCount * sizeof(AukStreamChunk*),
                                                 MEMF_CLEAR);
    if (!stream->chunks) {
        AukString_Free(stream->filename);
        FreeVec(stream);
        Close(file);
        AukString_Free(fullPath);
        return AUK_STREAM_ERROR_MEMORY;
    }

    /* Allocate chunks */
    for (chunkIndex = 0; chunkIndex < stream->chunkCount; chunkIndex++) {
        stream->chunks[chunkIndex] = aukstreampool_AllocChunk(engine->pool);
        if (!stream->chunks[chunkIndex]) {
            /* Out of memory - free what we allocated */
            unsigned long i;
            for (i = 0; i < chunkIndex; i++) {
                aukstreampool_FreeChunk(engine->pool, stream->chunks[i]);
            }
            FreeVec(stream->chunks);
            AukString_Free(stream->filename);
            FreeVec(stream);
            Close(file);
            AukString_Free(fullPath);
            return AUK_STREAM_ERROR_MEMORY;
        }
    }

    /* Allocate temporary buffer for reading (one chunk size) */
    tempBufferSize = AUK_STREAM_CHUNK_SIZE;
    tempBuffer = AllocVec(tempBufferSize, MEMF_CLEAR);
    if (!tempBuffer) {
        unsigned long i;
        for (i = 0; i < stream->chunkCount; i++) {
            aukstreampool_FreeChunk(engine->pool, stream->chunks[i]);
        }
        FreeVec(stream->chunks);
        AukString_Free(stream->filename);
        FreeVec(stream);
        Close(file);
        AukString_Free(fullPath);
        return AUK_STREAM_ERROR_MEMORY;
    }

    /* Load audio data chunk by chunk */
    framesLoaded = 0;
    chunkIndex = 0;

    while (framesLoaded < framesToLoad && chunkIndex < stream->chunkCount) {
        unsigned long framesToRead = (framesToLoad - framesLoaded);
        unsigned long maxFramesPerChunk = AUK_STREAM_CHUNK_SIZE / stream->info.bytesPerFrame;

        if (framesToRead > maxFramesPerChunk) {
            framesToRead = maxFramesPerChunk;
        }

        unsigned long framesRead = 0;

        /* Read from file based on format */
        if (format == AUK_STREAM_FORMAT_WAVE) {
            AukWaveInfo waveInfo;
            waveInfo.dataType = sourceType;
            waveInfo.channels = sourceChannels;
            waveInfo.frameCount = totalFrames;

            result = aukwave_ReadFrames(file, &waveInfo,
                                         startFrame + framesLoaded,
                                         framesToRead, tempBuffer, tempBufferSize,
                                         &framesRead);
        } else {
            Auk8SVXInfo svxInfo;
            svxInfo.dataType = sourceType;
            svxInfo.channels = sourceChannels;
            svxInfo.frameCount = totalFrames;

            result = auk8svx_ReadFrames(file, &svxInfo,
                                         startFrame + framesLoaded,
                                         framesToRead, tempBuffer, tempBufferSize,
                                         &framesRead);
        }

        if (result != AUK_STREAM_OK || framesRead == 0) {
            break;
        }

        /* Convert if needed */
        void* chunkData = aukstreampool_GetChunkData(stream->chunks[chunkIndex]);
        unsigned long convertedFrames;

        result = aukconvert_Convert(tempBuffer, sourceType, framesRead, sourceChannels,
                                     chunkData, targetType, AUK_STREAM_CHUNK_SIZE,
                                     &convertedFrames);

        if (result != AUK_STREAM_OK) {
            break;
        }

        framesLoaded += convertedFrames;
        chunkIndex++;
    }

    /* Cleanup */
    FreeVec(tempBuffer);
    Close(file);
    AukString_Free(fullPath);

    if (result != AUK_STREAM_OK || framesLoaded == 0) {
        /* Failed - cleanup stream */
        unsigned long i;
        for (i = 0; i < stream->chunkCount; i++) {
            aukstreampool_FreeChunk(engine->pool, stream->chunks[i]);
        }
        FreeVec(stream->chunks);
        AukString_Free(stream->filename);
        FreeVec(stream);
        return result;
    }

    /* Update actual frame count */
    stream->info.frameCount = framesLoaded;
    stream->ready = 1;

    /* Add to engine's stream list */
    stream->next = engine->streamList;
    engine->streamList = stream;
    engine->streamCount++;

    *outStream = stream;
    return AUK_STREAM_OK;
}

/*
 * Unload a stream from cache
 */
void aukloader_UnloadStream(AukStreamEngine* engine, AukCachedStream* stream) {
    AukCachedStream** current;
    unsigned long i;

    if (!engine || !stream) return;

    /* Remove from stream list */
    current = &engine->streamList;
    while (*current) {
        if (*current == stream) {
            *current = stream->next;
            break;
        }
        current = &(*current)->next;
    }

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

    /* Free stream */
    FreeVec(stream);

    if (engine->streamCount > 0) {
        engine->streamCount--;
    }
}
