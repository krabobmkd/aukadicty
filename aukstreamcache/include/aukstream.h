#ifndef AUKSTREAM_H
#define AUKSTREAM_H

/*
 * AukStreamCache - Public API
 * Sound data loading engine with memory-constrained caching
 *
 * This engine runs in a separate AmigaOS process and provides
 * zero-copy access to cached audio data for playback.
 *
 * Key Features:
 * - Runs in own process using CreateNewProc()
 * - Pre-allocated memory pool (no runtime AllocVec during playback)
 * - Time-based cache eviction
 * - Supports WAVE and IFF 8SVX formats
 * - Zero-copy API (const pointer access)
 * - Configurable conversion modes
 */

#include "aukstreamtypes.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Initialize the stream cache engine
 *
 * Creates a separate AmigaOS process and allocates the memory pool.
 * Must be called before any other stream cache functions.
 *
 * Parameters:
 *   config - Engine configuration (see AukStreamConfig)
 *
 * Returns:
 *   Opaque engine handle, or NULL on failure
 */
AukStreamEngine* aukstream_Init(const AukStreamConfig* config);

/*
 * Shutdown the stream cache engine
 *
 * Terminates the cache process and frees all resources.
 * All cached streams become invalid after this call.
 *
 * Parameters:
 *   engine - Engine handle from aukstream_Init()
 */
void aukstream_Shutdown(AukStreamEngine* engine);

/*
 * Request a stream to be loaded and cached
 *
 * Sends a load request to the cache process. The stream will be loaded
 * asynchronously. Use aukstream_IsReady() to check if loading is complete.
 *
 * Parameters:
 *   engine  - Engine handle
 *   request - Stream request parameters
 *   outStream - Receives opaque stream handle
 *
 * Returns:
 *   AUK_STREAM_OK on success, error code otherwise
 */
AukStreamCacheResult aukstream_RequestStream(AukStreamEngine* engine,
                                              const AukStreamRequest* request,
                                              AukCachedStream** outStream);

/*
 * Check if a stream is ready for playback
 *
 * Parameters:
 *   stream - Stream handle from aukstream_RequestStream()
 *
 * Returns:
 *   1 if ready, 0 if still loading or error
 */
int aukstream_IsReady(AukCachedStream* stream);

/*
 * Get information about a cached stream
 *
 * Parameters:
 *   stream  - Stream handle
 *   outInfo - Receives stream information
 *
 * Returns:
 *   AUK_STREAM_OK on success, error code otherwise
 */
AukStreamCacheResult aukstream_GetInfo(AukCachedStream* stream,
                                        AukStreamInfo* outInfo);

/*
 * Get zero-copy access to cached audio data
 *
 * Returns a const pointer directly into the cache memory.
 * NO COPYING - pointer is valid until stream is released or evicted.
 *
 * IMPORTANT: Do not modify the returned memory!
 *
 * Parameters:
 *   stream      - Stream handle
 *   frameOffset - Frame offset from stream start (0-based)
 *   frameCount  - Number of frames requested
 *   outData     - Receives const pointer to audio data
 *   outAvailable - Receives actual number of frames available
 *
 * Returns:
 *   AUK_STREAM_OK on success, error code otherwise
 */
AukStreamCacheResult aukstream_GetAudioData(AukCachedStream* stream,
                                             unsigned long frameOffset,
                                             unsigned long frameCount,
                                             const void** outData,
                                             unsigned long* outAvailable);

/*
 * Release a stream reference
 *
 * Decrements the reference count. When count reaches 0, the stream
 * becomes eligible for cache eviction.
 *
 * Parameters:
 *   stream - Stream handle to release
 */
void aukstream_ReleaseStream(AukCachedStream* stream);

/*
 * Touch a stream to update its last-used time
 *
 * Call this periodically for active streams to prevent eviction.
 * Typically called before each playback cycle.
 *
 * Parameters:
 *   stream - Stream handle
 */
void aukstream_Touch(AukCachedStream* stream);

/*
 * Get cache statistics
 *
 * Parameters:
 *   engine - Engine handle
 *   outUsedBytes  - Receives bytes currently in use
 *   outTotalBytes - Receives total cache capacity
 *   outStreamCount - Receives number of cached streams
 *
 * Returns:
 *   AUK_STREAM_OK on success, error code otherwise
 */
AukStreamCacheResult aukstream_GetCacheStats(AukStreamEngine* engine,
                                              unsigned long* outUsedBytes,
                                              unsigned long* outTotalBytes,
                                              unsigned long* outStreamCount);

#ifdef __cplusplus
}
#endif

#endif /* AUKSTREAM_H */
