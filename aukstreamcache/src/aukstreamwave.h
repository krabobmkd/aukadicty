#ifndef AUKSTREAMWAVE_H
#define AUKSTREAMWAVE_H

/*
 * AukStreamWave - RIFF WAVE File Parser
 * Parses WAVE files and extracts audio data
 */

#include "aukstreamtypes.h"
#include <dos/dos.h>

#ifdef __cplusplus
extern "C" {
#endif

/* WAVE file information */
typedef struct {
    AukStreamDataType dataType;
    unsigned long channels;
    unsigned long sampleRate;
    unsigned long frameCount;
    unsigned long dataOffset;      /* Offset to PCM data in file */
    unsigned long dataSize;        /* Size of PCM data in bytes */
} AukWaveInfo;

/*
 * Parse WAVE file header
 *
 * Reads the WAVE file header and extracts format information.
 * File must be opened and positioned at the beginning.
 *
 * Parameters:
 *   file    - DOS file handle (already opened)
 *   outInfo - Receives WAVE format information
 *
 * Returns:
 *   AUK_STREAM_OK on success, error code otherwise
 */
AukStreamCacheResult aukwave_ParseHeader(BPTR file, AukWaveInfo* outInfo);

/*
 * Read audio frames from WAVE file
 *
 * Reads PCM data from the WAVE file starting at a specific frame.
 * File must be positioned correctly (use info->dataOffset).
 *
 * Parameters:
 *   file        - DOS file handle
 *   info        - WAVE format information from aukwave_ParseHeader
 *   startFrame  - Starting frame number (0-based)
 *   frameCount  - Number of frames to read
 *   buffer      - Buffer to receive audio data
 *   bufferSize  - Size of buffer in bytes
 *   outRead     - Receives actual number of frames read
 *
 * Returns:
 *   AUK_STREAM_OK on success, error code otherwise
 */
AukStreamCacheResult aukwave_ReadFrames(BPTR file,
                                         const AukWaveInfo* info,
                                         unsigned long startFrame,
                                         unsigned long frameCount,
                                         void* buffer,
                                         unsigned long bufferSize,
                                         unsigned long* outRead);

#ifdef __cplusplus
}
#endif

#endif /* AUKSTREAMWAVE_H */
