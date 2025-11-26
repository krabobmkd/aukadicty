/*
 * AukStream8SVX - IFF 8SVX File Parser Implementation
 */

#include "aukstream8svx.h"
#include <proto/dos.h>
#include <string.h>

/* IFF chunk identifiers (big-endian) */
#define FORM_ID 0x464f524d  /* 'FORM' */
#define SVX8_ID 0x38535658  /* '8SVX' */
#define VHDR_ID 0x56484452  /* 'VHDR' */
#define BODY_ID 0x424f4459  /* 'BODY' */

/* Voice header structure (IFF 8SVX format) */
typedef struct {
    unsigned long oneShotHiSamples;
    unsigned long repeatHiSamples;
    unsigned long samplesPerHiCycle;
    unsigned short samplesPerSec;
    unsigned char ctOctave;
    unsigned char sCompression;
    long volume;
} Voice8Header;

/* Helper: Read 32-bit big-endian value */
static unsigned long ReadBE32(BPTR file) {
    unsigned char buf[4];
    if (Read(file, buf, 4) != 4) return 0;
    return ((unsigned long)buf[0] << 24) |
           ((unsigned long)buf[1] << 16) |
           ((unsigned long)buf[2] << 8) |
           (unsigned long)buf[3];
}

/* Helper: Read 16-bit big-endian value */
static unsigned short ReadBE16(BPTR file) {
    unsigned char buf[2];
    if (Read(file, buf, 2) != 2) return 0;
    return ((unsigned short)buf[0] << 8) | (unsigned short)buf[1];
}

/*
 * Parse IFF 8SVX file header
 */
AukStreamCacheResult auk8svx_ParseHeader(BPTR file, Auk8SVXInfo* outInfo) {
    unsigned long formId, formSize, svxId;
    unsigned long chunkId, chunkSize;
    Voice8Header vhdr;
    int foundVhdr = 0, foundBody = 0;

    if (!file || !outInfo) {
        return AUK_STREAM_ERROR_INVALID;
    }

    /* Seek to beginning */
    Seek(file, 0, OFFSET_BEGINNING);

    /* Read FORM header */
    formId = ReadBE32(file);
    formSize = ReadBE32(file);
    svxId = ReadBE32(file);

    if (formId != FORM_ID || svxId != SVX8_ID) {
        return AUK_STREAM_ERROR_FORMAT;
    }

    /* Parse chunks */
    while (!foundVhdr || !foundBody) {
        long pos = Seek(file, 0, OFFSET_CURRENT);
        if (pos < 0) break;

        chunkId = ReadBE32(file);
        chunkSize = ReadBE32(file);

        if (chunkId == VHDR_ID) {
            /* Parse voice header */
            if (chunkSize < 20) {
                return AUK_STREAM_ERROR_FORMAT;
            }

            vhdr.oneShotHiSamples = ReadBE32(file);
            vhdr.repeatHiSamples = ReadBE32(file);
            vhdr.samplesPerHiCycle = ReadBE32(file);
            vhdr.samplesPerSec = ReadBE16(file);
            Read(file, &vhdr.ctOctave, 1);
            Read(file, &vhdr.sCompression, 1);
            vhdr.volume = ReadBE32(file);

            /* Validate compression (0 = uncompressed) */
            if (vhdr.sCompression != 0) {
                return AUK_STREAM_ERROR_FORMAT;
            }

            /* Fill output info */
            outInfo->dataType = AUK_STREAM_8BIT_SIGNED;
            outInfo->channels = 1;  /* 8SVX is always mono */
            outInfo->sampleRate = vhdr.samplesPerSec;

            foundVhdr = 1;

            /* Skip remaining header bytes */
            if (chunkSize > 20) {
                Seek(file, chunkSize - 20, OFFSET_CURRENT);
            }

        } else if (chunkId == BODY_ID) {
            /* Found body chunk */
            outInfo->dataOffset = Seek(file, 0, OFFSET_CURRENT);
            outInfo->dataSize = chunkSize;
            outInfo->frameCount = chunkSize;  /* 8-bit mono = 1 byte per frame */

            foundBody = 1;

            /* Skip body for now */
            Seek(file, chunkSize, OFFSET_CURRENT);

        } else {
            /* Skip unknown chunk */
            Seek(file, chunkSize, OFFSET_CURRENT);
        }

        /* IFF chunks are word-aligned */
        if (chunkSize & 1) {
            Seek(file, 1, OFFSET_CURRENT);
        }
    }

    if (!foundVhdr || !foundBody) {
        return AUK_STREAM_ERROR_FORMAT;
    }

    return AUK_STREAM_OK;
}

/*
 * Read audio frames from 8SVX file
 */
AukStreamCacheResult auk8svx_ReadFrames(BPTR file,
                                         const Auk8SVXInfo* info,
                                         unsigned long startFrame,
                                         unsigned long frameCount,
                                         void* buffer,
                                         unsigned long bufferSize,
                                         unsigned long* outRead) {
    unsigned long bytesToRead;
    unsigned long bytesRead;

    if (!file || !info || !buffer || !outRead) {
        return AUK_STREAM_ERROR_INVALID;
    }

    *outRead = 0;

    /* Validate frame range */
    if (startFrame >= info->frameCount) {
        return AUK_STREAM_OK;  /* Beyond end of file */
    }

    /* Limit to available frames */
    if (startFrame + frameCount > info->frameCount) {
        frameCount = info->frameCount - startFrame;
    }

    /* For 8SVX, 1 frame = 1 byte (8-bit mono) */
    bytesToRead = frameCount;
    if (bytesToRead > bufferSize) {
        bytesToRead = bufferSize;
        frameCount = bytesToRead;
    }

    /* Seek to start position */
    Seek(file, info->dataOffset + startFrame, OFFSET_BEGINNING);

    /* Read data */
    bytesRead = Read(file, buffer, bytesToRead);
    if (bytesRead < 0) {
        return AUK_STREAM_ERROR_FILE;
    }

    *outRead = bytesRead;  /* 1 byte = 1 frame */

    return AUK_STREAM_OK;
}
