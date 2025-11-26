#ifndef AUKSTREAMTYPES_H
#define AUKSTREAMTYPES_H

/*
 * AukStreamCache - Types and Enumerations
 * Defines all types used by the stream cache engine
 */

#include "aukfixed.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Audio data type enumeration */
typedef enum {
    AUK_STREAM_8BIT_SIGNED,   /* 8-bit signed PCM */
    AUK_STREAM_16BIT_SIGNED,  /* 16-bit signed PCM */
    AUK_STREAM_32BIT_SIGNED   /* 32-bit signed PCM */
} AukStreamDataType;

/* Conversion mode for cache storage */
typedef enum {
    AUK_STREAM_CONVERT_NONE,      /* Store as-is, no conversion */
    AUK_STREAM_CONVERT_16BIT,     /* Convert all to 16-bit (balanced) */
    AUK_STREAM_CONVERT_32BIT      /* Convert all to 32-bit (highest quality) */
} AukStreamConversionMode;

/* File format types */
typedef enum {
    AUK_STREAM_FORMAT_UNKNOWN,
    AUK_STREAM_FORMAT_WAVE,       /* RIFF WAVE format */
    AUK_STREAM_FORMAT_8SVX        /* IFF 8SVX format */
} AukStreamFileFormat;

/* Result codes */
typedef enum {
    AUK_STREAM_OK = 0,
    AUK_STREAM_ERROR_INIT,        /* Engine initialization failed */
    AUK_STREAM_ERROR_MEMORY,      /* Memory allocation failed */
    AUK_STREAM_ERROR_FILE,        /* File I/O error */
    AUK_STREAM_ERROR_FORMAT,      /* Unsupported format */
    AUK_STREAM_ERROR_NOTFOUND,    /* Stream not found */
    AUK_STREAM_ERROR_INVALID,     /* Invalid parameters */
    AUK_STREAM_ERROR_PROCESS      /* Process creation failed */
} AukStreamCacheResult;

/* Opaque handle for stream cache engine */
typedef struct AukStreamEngine AukStreamEngine;

/* Opaque handle for cached stream */
typedef struct AukCachedStream AukCachedStream;

/* Engine configuration */
typedef struct {
    unsigned long cacheSize;              /* Total cache memory in bytes (min 256KB) */
    AukStreamConversionMode conversionMode; /* How to store audio in cache */
    char* soundDirectory;                 /* Directory for sound files */
} AukStreamConfig;

/* Stream request parameters */
typedef struct {
    const char* filename;                 /* Relative path to sound file */
    AukFixed startTime;                   /* Start time in seconds (fixed-point) */
    AukFixed endTime;                     /* End time in seconds (fixed-point) */
    unsigned long fileStartFrame;         /* Start frame in file (0 = beginning) */
    unsigned long fileEndFrame;           /* End frame in file (0 = end) */
} AukStreamRequest;

/* Stream information */
typedef struct {
    AukStreamDataType dataType;           /* Audio sample format */
    unsigned long channels;               /* 1=mono, 2=stereo */
    unsigned long sampleRate;             /* Samples per second */
    unsigned long frameCount;             /* Total frames available */
    unsigned long bytesPerFrame;          /* Bytes per frame (based on type & channels) */
} AukStreamInfo;

#ifdef __cplusplus
}
#endif

#endif /* AUKSTREAMTYPES_H */
