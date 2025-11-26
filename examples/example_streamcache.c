/*
 * AukStreamCache Example
 * Demonstrates the stream cache loading and zero-copy access
 */

#include <stdio.h>
#include <stdlib.h>
#include "aukstream.h"
#include "aukfixed.h"

/* Helper to print stream info */
static void PrintStreamInfo(const AukStreamInfo* info) {
    const char* typeStr;

    switch (info->dataType) {
        case AUK_STREAM_8BIT_SIGNED:
            typeStr = "8-bit signed";
            break;
        case AUK_STREAM_16BIT_SIGNED:
            typeStr = "16-bit signed";
            break;
        case AUK_STREAM_32BIT_SIGNED:
            typeStr = "32-bit signed";
            break;
        default:
            typeStr = "unknown";
            break;
    }

    printf("  Data Type:    %s\n", typeStr);
    printf("  Channels:     %lu (%s)\n", info->channels,
           info->channels == 1 ? "mono" : "stereo");
    printf("  Sample Rate:  %lu Hz\n", info->sampleRate);
    printf("  Frame Count:  %lu\n", info->frameCount);
    printf("  Bytes/Frame:  %lu\n", info->bytesPerFrame);

    /* Calculate duration in seconds */
    if (info->sampleRate > 0) {
        double duration = (double)info->frameCount / (double)info->sampleRate;
        printf("  Duration:     %.2f seconds\n", duration);
    }
}

/* Helper to print cache statistics */
static void PrintCacheStats(AukStreamEngine* engine) {
    unsigned long usedBytes, totalBytes, streamCount;
    AukStreamCacheResult result;

    result = aukstream_GetCacheStats(engine, &usedBytes, &totalBytes, &streamCount);
    if (result == AUK_STREAM_OK) {
        double usagePercent = ((double)usedBytes / (double)totalBytes) * 100.0;
        printf("\nCache Statistics:\n");
        printf("  Used:         %lu / %lu bytes (%.1f%%)\n",
               usedBytes, totalBytes, usagePercent);
        printf("  Streams:      %lu\n", streamCount);
    }
}

int main(void) {
    AukStreamEngine* engine = NULL;
    AukStreamConfig config;
    AukStreamRequest request;
    AukCachedStream* stream1 = NULL;
    AukCachedStream* stream2 = NULL;
    AukStreamInfo info;
    AukStreamCacheResult result;
    const void* audioData;
    unsigned long available;

    printf("=== AukStreamCache Example ===\n\n");

    /* Initialize engine with 512KB cache */
    printf("1. Initializing stream cache engine...\n");
    config.cacheSize = 524288;  /* 512KB */
    config.conversionMode = AUK_STREAM_CONVERT_16BIT;
    config.soundDirectory = "sounds";  /* Relative to current directory */

    engine = aukstream_Init(&config);
    if (!engine) {
        printf("ERROR: Failed to initialize stream cache engine\n");
        return 1;
    }
    printf("   Engine initialized: 512KB cache, 16-bit conversion mode\n");

    /* Load first sound file */
    printf("\n2. Loading first sound (kick.wav)...\n");
    request.filename = "kick.wav";
    request.startTime = 0;
    request.endTime = 0;
    request.fileStartFrame = 0;
    request.fileEndFrame = 0;  /* Load entire file */

    result = aukstream_RequestStream(engine, &request, &stream1);
    if (result != AUK_STREAM_OK) {
        printf("   ERROR: Failed to load stream (error code: %d)\n", result);
        printf("   Make sure 'sounds/kick.wav' exists!\n");
    } else {
        printf("   Stream loaded successfully\n");

        /* Check if ready */
        if (aukstream_IsReady(stream1)) {
            printf("   Stream is ready for playback\n");

            /* Get stream info */
            result = aukstream_GetInfo(stream1, &info);
            if (result == AUK_STREAM_OK) {
                printf("\n   Stream Information:\n");
                PrintStreamInfo(&info);
            }

            /* Get audio data (zero-copy access) */
            printf("\n   Getting audio data (zero-copy)...\n");
            result = aukstream_GetAudioData(stream1, 0, 1024, &audioData, &available);
            if (result == AUK_STREAM_OK && audioData != NULL) {
                printf("   Got pointer to %lu frames of audio data\n", available);
                printf("   Pointer address: %p (const, zero-copy!)\n", audioData);

                /* Example: Read first few samples as 16-bit */
                if (info.dataType == AUK_STREAM_16BIT_SIGNED && available > 0) {
                    const short* samples = (const short*)audioData;
                    printf("   First 4 samples: %d, %d, %d, %d\n",
                           samples[0], samples[1], samples[2], samples[3]);
                }
            }
        }
    }

    PrintCacheStats(engine);

    /* Load second sound file */
    printf("\n3. Loading second sound (snare.wav)...\n");
    request.filename = "snare.wav";
    request.startTime = 0;
    request.endTime = 0;
    request.fileStartFrame = 0;
    request.fileEndFrame = 0;

    result = aukstream_RequestStream(engine, &request, &stream2);
    if (result != AUK_STREAM_OK) {
        printf("   ERROR: Failed to load stream (error code: %d)\n", result);
        printf("   Make sure 'sounds/snare.wav' exists!\n");
    } else {
        printf("   Stream loaded successfully\n");

        if (aukstream_IsReady(stream2)) {
            result = aukstream_GetInfo(stream2, &info);
            if (result == AUK_STREAM_OK) {
                printf("\n   Stream Information:\n");
                PrintStreamInfo(&info);
            }
        }
    }

    PrintCacheStats(engine);

    /* Test time-based loading with fixed-point time */
    printf("\n4. Loading partial stream using time-based request...\n");
    printf("   Loading first 0.5 seconds of kick.wav\n");

    /* 0.5 seconds in fixed-point (32.32 format) */
    AukFixed halfSecond = AukFixed_FromDouble(0.5);

    request.filename = "kick_partial.wav";  /* Different cache key */
    request.startTime = 0;
    request.endTime = halfSecond;
    request.fileStartFrame = 0;
    request.fileEndFrame = 0;

    /* This would load only the first 0.5 seconds */
    /* Note: This is just demonstrating the API - file may not exist */

    /* Request first stream again (should return cached version) */
    printf("\n5. Requesting kick.wav again (should use cache)...\n");
    AukCachedStream* stream1Again = NULL;
    request.filename = "kick.wav";
    request.startTime = 0;
    request.endTime = 0;
    request.fileStartFrame = 0;
    request.fileEndFrame = 0;

    result = aukstream_RequestStream(engine, &request, &stream1Again);
    if (result == AUK_STREAM_OK && stream1Again == stream1) {
        printf("   SUCCESS: Got same cached stream (no reload)\n");
    }

    /* Release streams */
    printf("\n6. Releasing streams...\n");
    if (stream1Again) {
        aukstream_ReleaseStream(stream1Again);
        printf("   Released stream1Again reference\n");
    }
    if (stream1) {
        aukstream_ReleaseStream(stream1);
        printf("   Released stream1 reference\n");
    }
    if (stream2) {
        aukstream_ReleaseStream(stream2);
        printf("   Released stream2 reference\n");
    }

    PrintCacheStats(engine);

    /* Shutdown engine */
    printf("\n7. Shutting down engine...\n");
    aukstream_Shutdown(engine);
    printf("   Engine shutdown complete\n");

    printf("\n=== Example Complete ===\n");
    return 0;
}
