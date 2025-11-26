#ifndef AUKSTREAMCONVERT_H
#define AUKSTREAMCONVERT_H

/*
 * AukStreamConvert - Audio Format Conversion
 * Converts between 8/16/32-bit audio formats
 */

#include "aukstreamtypes.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Convert audio data to target format
 *
 * Converts audio samples from source format to target format.
 * Supports 8-bit, 16-bit, and 32-bit signed PCM.
 *
 * Parameters:
 *   srcData      - Source audio data
 *   srcType      - Source data type
 *   srcFrames    - Number of frames in source
 *   srcChannels  - Number of channels in source (1 or 2)
 *   dstData      - Destination buffer
 *   dstType      - Destination data type
 *   dstSize      - Size of destination buffer in bytes
 *   outFrames    - Receives number of frames written
 *
 * Returns:
 *   AUK_STREAM_OK on success, error code otherwise
 */
AukStreamCacheResult aukconvert_Convert(const void* srcData,
                                         AukStreamDataType srcType,
                                         unsigned long srcFrames,
                                         unsigned long srcChannels,
                                         void* dstData,
                                         AukStreamDataType dstType,
                                         unsigned long dstSize,
                                         unsigned long* outFrames);

/*
 * Get bytes per sample for a data type
 */
unsigned long aukconvert_GetBytesPerSample(AukStreamDataType type);

/*
 * Get bytes per frame (sample * channels)
 */
unsigned long aukconvert_GetBytesPerFrame(AukStreamDataType type, unsigned long channels);

#ifdef __cplusplus
}
#endif

#endif /* AUKSTREAMCONVERT_H */
