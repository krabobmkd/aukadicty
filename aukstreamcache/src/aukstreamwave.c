/*
 * AukStreamWave - RIFF WAVE File Parser Implementation
 */

#include "aukstreamwave.h"
#include <proto/dos.h>
#include <string.h>

/* RIFF chunk identifiers */
#define RIFF_ID 0x52494646  /* 'RIFF' */
#define WAVE_ID 0x57415645  /* 'WAVE' */
#define FMT_ID  0x666d7420  /* 'fmt ' */
#define DATA_ID 0x64617461  /* 'data' */

/* WAVE format codes */
#define WAVE_FORMAT_PCM 1

/* Helper: Read 32-bit little-endian value */
static unsigned long ReadLE32(BPTR file) {
    unsigned char buf[4];
    if (Read(file, buf, 4) != 4) return 0;
    return (unsigned long)buf[0] |
           ((unsigned long)buf[1] << 8) |
           ((unsigned long)buf[2] << 16) |
           ((unsigned long)buf[3] << 24);
}

/* Helper: Read 16-bit little-endian value */
static unsigned short ReadLE16(BPTR file) {
    unsigned char buf[2];
    if (Read(file, buf, 2) != 2) return 0;
    return (unsigned short)buf[0] | ((unsigned short)buf[1] << 8);
}

/*
 * Parse WAVE file header
 */
AukStreamCacheResult aukwave_ParseHeader(BPTR file, AukWaveInfo* outInfo) {
    unsigned long riffId, fileSize, waveId;
    unsigned long chunkId, chunkSize;
    unsigned short audioFormat, numChannels, bitsPerSample;
    unsigned long sampleRate, byteRate;
    unsigned short blockAlign;
    int foundFmt = 0, foundData = 0;

    if (!file || !outInfo) {
        return AUK_STREAM_ERROR_INVALID;
    }

    /* Seek to beginning */
    Seek(file, 0, OFFSET_BEGINNING);

    /* Read RIFF header */
    riffId = ReadLE32(file);
    fileSize = ReadLE32(file);
    waveId = ReadLE32(file);

    if (riffId != RIFF_ID || waveId != WAVE_ID) {
        return AUK_STREAM_ERROR_FORMAT;
    }

    /* Parse chunks */
    while (!foundFmt || !foundData) {
        long pos = Seek(file, 0, OFFSET_CURRENT);
        if (pos < 0) break;

        chunkId = ReadLE32(file);
        chunkSize = ReadLE32(file);

        if (chunkId == FMT_ID) {
            /* Parse format chunk */
            audioFormat = ReadLE16(file);
            numChannels = ReadLE16(file);
            sampleRate = ReadLE32(file);
            byteRate = ReadLE32(file);
            blockAlign = ReadLE16(file);
            bitsPerSample = ReadLE16(file);

            /* Validate format */
            if (audioFormat != WAVE_FORMAT_PCM) {
                return AUK_STREAM_ERROR_FORMAT;
            }

            if (numChannels != 1 && numChannels != 2) {
                return AUK_STREAM_ERROR_FORMAT;
            }

            if (bitsPerSample != 8 && bitsPerSample != 16 && bitsPerSample != 32) {
                return AUK_STREAM_ERROR_FORMAT;
            }

            /* Fill output info */
            outInfo->channels = numChannels;
            outInfo->sampleRate = sampleRate;

            /* Determine data type */
            if (bitsPerSample == 8) {
                outInfo->dataType = AUK_STREAM_8BIT_SIGNED;
            } else if (bitsPerSample == 16) {
                outInfo->dataType = AUK_STREAM_16BIT_SIGNED;
            } else {
                outInfo->dataType = AUK_STREAM_32BIT_SIGNED;
            }

            foundFmt = 1;

            /* Skip remaining format bytes */
            if (chunkSize > 16) {
                Seek(file, chunkSize - 16, OFFSET_CURRENT);
            }

        } else if (chunkId == DATA_ID) {
            /* Found data chunk */
            outInfo->dataOffset = Seek(file, 0, OFFSET_CURRENT);
            outInfo->dataSize = chunkSize;

            /* Calculate frame count */
            unsigned long bytesPerSample = bitsPerSample / 8;
            unsigned long bytesPerFrame = bytesPerSample * numChannels;
            outInfo->frameCount = chunkSize / bytesPerFrame;

            foundData = 1;

            /* Skip data for now */
            Seek(file, chunkSize, OFFSET_CURRENT);

        } else {
            /* Skip unknown chunk */
            Seek(file, chunkSize, OFFSET_CURRENT);
        }

        /* Align to word boundary */
        if (chunkSize & 1) {
            Seek(file, 1, OFFSET_CURRENT);
        }
    }

    if (!foundFmt || !foundData) {
        return AUK_STREAM_ERROR_FORMAT;
    }

    return AUK_STREAM_OK;
}

/*
 * Read audio frames from WAVE file
 */
AukStreamCacheResult aukwave_ReadFrames(BPTR file,
                                         const AukWaveInfo* info,
                                         unsigned long startFrame,
                                         unsigned long frameCount,
                                         void* buffer,
                                         unsigned long bufferSize,
                                         unsigned long* outRead) {
    unsigned long bytesPerSample;
    unsigned long bytesPerFrame;
    unsigned long bytesToRead;
    unsigned long bytesRead;

    if (!file || !info || !buffer || !outRead) {
        return AUK_STREAM_ERROR_INVALID;
    }

    *outRead = 0;

    /* Calculate bytes per frame */
    if (info->dataType == AUK_STREAM_8BIT_SIGNED) {
        bytesPerSample = 1;
    } else if (info->dataType == AUK_STREAM_16BIT_SIGNED) {
        bytesPerSample = 2;
    } else {
        bytesPerSample = 4;
    }

    bytesPerFrame = bytesPerSample * info->channels;

    /* Validate frame range */
    if (startFrame >= info->frameCount) {
        return AUK_STREAM_OK;  /* Beyond end of file */
    }

    /* Limit to available frames */
    if (startFrame + frameCount > info->frameCount) {
        frameCount = info->frameCount - startFrame;
    }

    /* Calculate bytes to read */
    bytesToRead = frameCount * bytesPerFrame;
    if (bytesToRead > bufferSize) {
        bytesToRead = bufferSize;
        frameCount = bytesToRead / bytesPerFrame;
    }

    /* Seek to start position */
    Seek(file, info->dataOffset + (startFrame * bytesPerFrame), OFFSET_BEGINNING);

    /* Read data */
    bytesRead = Read(file, buffer, bytesToRead);
    if (bytesRead < 0) {
        return AUK_STREAM_ERROR_FILE;
    }

    *outRead = bytesRead / bytesPerFrame;

    return AUK_STREAM_OK;
}
