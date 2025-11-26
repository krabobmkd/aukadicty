#ifndef AUKSTREAMINTERNAL_H
#define AUKSTREAMINTERNAL_H

/*
 * AukStreamCache - Internal Structures
 * Not exposed in public API
 */

#include "aukstream.h"
#include "aukstreampool.h"
#include <exec/ports.h>
#include <exec/tasks.h>
#include <dos/dos.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Message types for inter-process communication */
typedef enum {
    AUK_MSG_LOAD_STREAM,      /* Request to load a stream */
    AUK_MSG_RELEASE_STREAM,   /* Release a stream reference */
    AUK_MSG_TOUCH_STREAM,     /* Update stream last-used time */
    AUK_MSG_SHUTDOWN          /* Shutdown the cache process */
} AukStreamMessageType;

/* Message structure for cache process communication */
typedef struct {
    struct Message execMsg;           /* AmigaOS message header */
    AukStreamMessageType type;        /* Message type */
    void* data;                       /* Message-specific data */
    AukStreamCacheResult result;      /* Result code (filled by cache process) */
} AukStreamMessage;

/* Cached stream structure */
struct AukCachedStream {
    char* filename;                   /* Allocated filename */
    AukStreamInfo info;               /* Stream information */

    AukStreamChunk** chunks;          /* Array of chunk pointers */
    unsigned long chunkCount;         /* Number of chunks */

    unsigned long refCount;           /* Reference count */
    struct DateStamp lastUsedTime;    /* Last access time */

    int ready;                        /* 1 if fully loaded, 0 if loading */

    AukCachedStream* next;            /* For linked list */
};

/* Engine structure */
struct AukStreamEngine {
    AukStreamConfig config;           /* Configuration copy */

    struct MsgPort* mainPort;         /* Message port for main process */
    struct MsgPort* cachePort;        /* Message port for cache process */
    struct Process* cacheProcess;     /* Cache process handle */

    AukStreamPool* pool;              /* Memory pool */

    AukCachedStream* streamList;      /* Linked list of cached streams */
    unsigned long streamCount;        /* Number of cached streams */

    int shutdownFlag;                 /* 1 when shutting down */
};

/* Cache process entry point */
void aukstream_CacheProcessMain(void);

/* Internal helper functions */
AukCachedStream* aukstream_FindStream(AukStreamEngine* engine, const char* filename);
AukStreamCacheResult aukstream_EvictLRU(AukStreamEngine* engine, unsigned long bytesNeeded);

#ifdef __cplusplus
}
#endif

#endif /* AUKSTREAMINTERNAL_H */
