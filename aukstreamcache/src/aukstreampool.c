/*
 * AukStreamPool - Memory Pool Implementation
 * Pre-allocated memory divided into fixed-size chunks
 */

#include "aukstreampool.h"
#include <exec/memory.h>
#include <proto/exec.h>

#include "aukstreamprivate.h"

/*
 * Create a memory pool
 */
AukStreamPool* aukstreampool_Create(unsigned long totalBytes) {
    AukStreamPool* pool;
    unsigned long i;
    AukStreamChunk* chunk;

    /* Minimum size check (256KB) */
    if (totalBytes < 262144) {
        return NULL;
    }

    /* Allocate pool structure */
    pool = (AukStreamPool*)AllocVec(sizeof(AukStreamPool), MEMF_CLEAR);
    if (!pool) {
        return NULL;
    }

    /* Allocate the main memory block */
    pool->memory = AllocVec(totalBytes, MEMF_CLEAR);
    if (!pool->memory) {
        FreeVec(pool);
        return NULL;
    }

    pool->totalBytes = totalBytes;
    pool->totalChunks = totalBytes / sizeof(AukStreamChunk);
    pool->usedChunks = 0;

    /* Initialize free list - link all chunks */
    pool->freeList = NULL;
    for (i = 0; i < pool->totalChunks; i++) {
        chunk = (AukStreamChunk*)((unsigned char*)pool->memory + (i * sizeof(AukStreamChunk)));
        chunk->next = pool->freeList;
        pool->freeList = chunk;
    }

    return pool;
}

/*
 * Destroy a memory pool
 */
void aukstreampool_Destroy(AukStreamPool* pool) {
    if (!pool) return;

    if (pool->memory) {
        FreeVec(pool->memory);
    }

    FreeVec(pool);
}

/*
 * Allocate a chunk from the pool
 */
AukStreamChunk* aukstreampool_AllocChunk(AukStreamPool* pool) {
    AukStreamChunk* chunk;

    if (!pool || !pool->freeList) {
        return NULL;
    }

    /* Remove from free list */
    chunk = pool->freeList;
    pool->freeList = chunk->next;
    chunk->next = NULL;

    pool->usedChunks++;

    return chunk;
}

/*
 * Free a chunk back to the pool
 */
void aukstreampool_FreeChunk(AukStreamPool* pool, AukStreamChunk* chunk) {
    if (!pool || !chunk) return;

    /* Add to free list */
    chunk->next = pool->freeList;
    pool->freeList = chunk;

    if (pool->usedChunks > 0) {
        pool->usedChunks--;
    }
}

/*
 * Get pointer to chunk data
 */
void* aukstreampool_GetChunkData(AukStreamChunk* chunk) {
    if (!chunk) return NULL;
    return chunk->data;
}

/*
 * Get pool statistics
 */
void aukstreampool_GetStats(AukStreamPool* pool,
                             unsigned long* outTotal,
                             unsigned long* outUsed,
                             unsigned long* outFree) {
    if (!pool) return;

    if (outTotal) {
        *outTotal = pool->totalChunks;
    }

    if (outUsed) {
        *outUsed = pool->usedChunks;
    }

    if (outFree) {
        *outFree = pool->totalChunks - pool->usedChunks;
    }
}
