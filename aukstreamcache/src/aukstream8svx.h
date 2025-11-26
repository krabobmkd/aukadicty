#ifndef AUKSTREAM8SVX_H
#define AUKSTREAM8SVX_H

/*
 * AukStream8SVX - IFF 8SVX File Parser
 * Parses Amiga IFF 8SVX files and extracts audio data
 */

#include "aukstreamtypes.h"
#include <dos/dos.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 8SVX file information */
typedef struct {
    AukStreamDataType dataType;    /* Always 8-bit signed for 8SVX */
    unsigned long channels;         /* Always 1 (mono) for standard 8SVX */
    unsigned long sampleRate;
    unsigned long frameCount;
    unsigned long dataOffset;       /* Offset to audio data in file */
    unsigned long dataSize;         /* Size of audio data in bytes */
} Auk8SVXInfo;

/*
 * Parse IFF 8SVX file header
 *
 * Reads the 8SVX file header and extracts format information.
 * File must be opened and positioned at the beginning.
 *
 * Parameters:
 *   file    - DOS file handle (already opened)
 *   outInfo - Receives 8SVX format information
 *
 * Returns:
 *   AUK_STREAM_OK on success, error code otherwise
 */
AukStreamCacheResult auk8svx_ParseHeader(BPTR file, Auk8SVXInfo* outInfo);

/*
 * Read audio frames from 8SVX file
 *
 * Reads PCM data from the 8SVX file starting at a specific frame.
 *
 * Parameters:
 *   file        - DOS file handle
 *   info        - 8SVX format information from auk8svx_ParseHeader
 *   startFrame  - Starting frame number (0-based)
 *   frameCount  - Number of frames to read
 *   buffer      - Buffer to receive audio data
 *   bufferSize  - Size of buffer in bytes
 *   outRead     - Receives actual number of frames read
 *
 * Returns:
 *   AUK_STREAM_OK on success, error code otherwise
 */
AukStreamCacheResult auk8svx_ReadFrames(BPTR file,
                                         const Auk8SVXInfo* info,
                                         unsigned long startFrame,
                                         unsigned long frameCount,
                                         void* buffer,
                                         unsigned long bufferSize,
                                         unsigned long* outRead);

#ifdef __cplusplus
}
#endif

#endif /* AUKSTREAM8SVX_H */
