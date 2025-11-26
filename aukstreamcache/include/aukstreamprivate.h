#ifndef AUKSTREAMPRIVATE_H
#define AUKSTREAMPRIVATE_H

#ifdef __cplusplus
extern "C" {
#endif

/* Chunk size: 16KB for efficient disk I/O and cache management */
#define AUK_STREAM_CHUNK_SIZE 16384

/* Chunk structure - includes both data and free list linkage */
struct sAukStreamChunk {
    unsigned char data[AUK_STREAM_CHUNK_SIZE];
    struct sAukStreamChunk* next;  /* For free list */
};

/* Pool structure */
struct sAukStreamPool {
    void* memory;               /* Base address of allocated memory */
    unsigned long totalBytes;   /* Total pool size */
    unsigned long totalChunks;  /* Total number of chunks */
    unsigned long usedChunks;   /* Number of chunks in use */
    struct sAukStreamChunk* freeList;   /* Linked list of free chunks */
};


#ifdef __cplusplus
}
#endif

#endif /* AUKSTREAMPRIVATE_H */
