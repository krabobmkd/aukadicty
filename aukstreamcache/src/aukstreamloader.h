#ifndef AUKSTREAMLOADER_H
#define AUKSTREAMLOADER_H

/*
 * AukStreamLoader - Stream Loading and Caching
 * Handles file loading, format detection, and cache management
 */

#include "aukstreaminternal.h"
#include "aukstreampool.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Detect file format from filename extension
 *
 * Parameters:
 *   filename - Path to audio file
 *
 * Returns:
 *   Detected format, or AUK_STREAM_FORMAT_UNKNOWN
 */
AukStreamFileFormat aukloader_DetectFormat(const char* filename);

/*
 * Load a stream from file into cache
 *
 * Opens the file, parses the format, loads audio data into chunks,
 * and converts to the target format if needed.
 *
 * Parameters:
 *   engine   - Engine handle
 *   request  - Stream request parameters
 *   outStream - Receives cached stream handle
 *
 * Returns:
 *   AUK_STREAM_OK on success, error code otherwise
 */
AukStreamCacheResult aukloader_LoadStream(AukStreamEngine* engine,
                                           const AukStreamRequest* request,
                                           AukCachedStream** outStream);

/*
 * Unload a stream from cache
 *
 * Frees all chunks and removes stream from cache.
 *
 * Parameters:
 *   engine - Engine handle
 *   stream - Stream to unload
 */
void aukloader_UnloadStream(AukStreamEngine* engine, AukCachedStream* stream);

#ifdef __cplusplus
}
#endif

#endif /* AUKSTREAMLOADER_H */
