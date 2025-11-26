#ifndef AUKSTREAMPOOL_H
#define AUKSTREAMPOOL_H

/*
 * AukStreamPool - Memory Pool Management
 * Pre-allocated memory pool divided into fixed-size chunks
 * Avoids runtime AllocVec/FreeVec to prevent playback freezes
 */

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declaration */
 struct sAukStreamPool;
typedef struct sAukStreamPool AukStreamPool;

/* Opaque chunk handle */
 struct sAukStreamChunk;
typedef struct sAukStreamChunk AukStreamChunk;

/*
 * Create a memory pool
 *
 * Allocates a single large block and divides it into chunks.
 *
 * Parameters:
 *   totalBytes - Total pool size in bytes (must be >= 256KB)
 *
 * Returns:
 *   Pool handle, or NULL on failure
 */
AukStreamPool* aukstreampool_Create(unsigned long totalBytes);

/*
 * Destroy a memory pool
 *
 * Frees the entire pool. All chunks become invalid.
 *
 * Parameters:
 *   pool - Pool handle
 */
void aukstreampool_Destroy(AukStreamPool* pool);

/*
 * Allocate a chunk from the pool
 *
 * Returns NULL if no free chunks available.
 *
 * Parameters:
 *   pool - Pool handle
 *
 * Returns:
 *   Chunk handle, or NULL if pool is full
 */
AukStreamChunk* aukstreampool_AllocChunk(AukStreamPool* pool);

/*
 * Free a chunk back to the pool
 *
 * Parameters:
 *   pool  - Pool handle
 *   chunk - Chunk to free
 */
void aukstreampool_FreeChunk(AukStreamPool* pool, AukStreamChunk* chunk);

/*
 * Get pointer to chunk data
 *
 * Parameters:
 *   chunk - Chunk handle
 *
 * Returns:
 *   Pointer to chunk data (AUK_STREAM_CHUNK_SIZE bytes)
 */
void* aukstreampool_GetChunkData(AukStreamChunk* chunk);

/*
 * Get pool statistics
 *
 * Parameters:
 *   pool       - Pool handle
 *   outTotal   - Receives total chunks
 *   outUsed    - Receives used chunks
 *   outFree    - Receives free chunks
 */
void aukstreampool_GetStats(AukStreamPool* pool,
                             unsigned long* outTotal,
                             unsigned long* outUsed,
                             unsigned long* outFree);

#ifdef __cplusplus
}
#endif

#endif /* AUKSTREAMPOOL_H */
