/*
 * AukStreamConvert - Audio Format Conversion Implementation
 */

#include "aukstreamconvert.h"

/*
 * Get bytes per sample for a data type
 */
unsigned long aukconvert_GetBytesPerSample(AukStreamDataType type) {
    switch (type) {
        case AUK_STREAM_8BIT_SIGNED:
            return 1;
        case AUK_STREAM_16BIT_SIGNED:
            return 2;
        case AUK_STREAM_32BIT_SIGNED:
            return 4;
        default:
            return 0;
    }
}

/*
 * Get bytes per frame (sample * channels)
 */
unsigned long aukconvert_GetBytesPerFrame(AukStreamDataType type, unsigned long channels) {
    return aukconvert_GetBytesPerSample(type) * channels;
}

/*
 * Convert 8-bit to 16-bit (shift left by 8)
 */
static void convert_8to16(const char* src, short* dst, unsigned long samples) {
    unsigned long i;
    for (i = 0; i < samples; i++) {
        dst[i] = (short)src[i] << 8;
    }
}

/*
 * Convert 8-bit to 32-bit (shift left by 24)
 */
static void convert_8to32(const char* src, long* dst, unsigned long samples) {
    unsigned long i;
    for (i = 0; i < samples; i++) {
        dst[i] = (long)src[i] << 24;
    }
}

/*
 * Convert 16-bit to 8-bit (shift right by 8)
 */
static void convert_16to8(const short* src, char* dst, unsigned long samples) {
    unsigned long i;
    for (i = 0; i < samples; i++) {
        dst[i] = (char)(src[i] >> 8);
    }
}

/*
 * Convert 16-bit to 32-bit (shift left by 16)
 */
static void convert_16to32(const short* src, long* dst, unsigned long samples) {
    unsigned long i;
    for (i = 0; i < samples; i++) {
        dst[i] = (long)src[i] << 16;
    }
}

/*
 * Convert 32-bit to 8-bit (shift right by 24)
 */
static void convert_32to8(const long* src, char* dst, unsigned long samples) {
    unsigned long i;
    for (i = 0; i < samples; i++) {
        dst[i] = (char)(src[i] >> 24);
    }
}

/*
 * Convert 32-bit to 16-bit (shift right by 16)
 */
static void convert_32to16(const long* src, short* dst, unsigned long samples) {
    unsigned long i;
    for (i = 0; i < samples; i++) {
        dst[i] = (short)(src[i] >> 16);
    }
}

/*
 * Copy samples without conversion
 */
static void convert_copy(const void* src, void* dst, unsigned long bytes) {
    const char* s = (const char*)src;
    char* d = (char*)dst;
    unsigned long i;
    for (i = 0; i < bytes; i++) {
        d[i] = s[i];
    }
}

/*
 * Convert audio data to target format
 */
AukStreamCacheResult aukconvert_Convert(const void* srcData,
                                         AukStreamDataType srcType,
                                         unsigned long srcFrames,
                                         unsigned long srcChannels,
                                         void* dstData,
                                         AukStreamDataType dstType,
                                         unsigned long dstSize,
                                         unsigned long* outFrames) {
    unsigned long srcBytesPerFrame;
    unsigned long dstBytesPerFrame;
    unsigned long totalSamples;
    unsigned long maxFrames;

    if (!srcData || !dstData || !outFrames) {
        return AUK_STREAM_ERROR_INVALID;
    }

    if (srcChannels == 0) {
        return AUK_STREAM_ERROR_INVALID;
    }

    *outFrames = 0;

    /* Calculate bytes per frame */
    srcBytesPerFrame = aukconvert_GetBytesPerFrame(srcType, srcChannels);
    dstBytesPerFrame = aukconvert_GetBytesPerFrame(dstType, srcChannels);

    if (srcBytesPerFrame == 0 || dstBytesPerFrame == 0) {
        return AUK_STREAM_ERROR_INVALID;
    }

    /* Calculate maximum frames that fit in destination */
    maxFrames = dstSize / dstBytesPerFrame;
    if (srcFrames > maxFrames) {
        srcFrames = maxFrames;
    }

    if (srcFrames == 0) {
        return AUK_STREAM_OK;
    }

    /* Total samples = frames * channels */
    totalSamples = srcFrames * srcChannels;

    /* Perform conversion based on source and destination types */
    if (srcType == dstType) {
        /* No conversion needed - direct copy */
        convert_copy(srcData, dstData, srcFrames * srcBytesPerFrame);

    } else if (srcType == AUK_STREAM_8BIT_SIGNED && dstType == AUK_STREAM_16BIT_SIGNED) {
        convert_8to16((const char*)srcData, (short*)dstData, totalSamples);

    } else if (srcType == AUK_STREAM_8BIT_SIGNED && dstType == AUK_STREAM_32BIT_SIGNED) {
        convert_8to32((const char*)srcData, (long*)dstData, totalSamples);

    } else if (srcType == AUK_STREAM_16BIT_SIGNED && dstType == AUK_STREAM_8BIT_SIGNED) {
        convert_16to8((const short*)srcData, (char*)dstData, totalSamples);

    } else if (srcType == AUK_STREAM_16BIT_SIGNED && dstType == AUK_STREAM_32BIT_SIGNED) {
        convert_16to32((const short*)srcData, (long*)dstData, totalSamples);

    } else if (srcType == AUK_STREAM_32BIT_SIGNED && dstType == AUK_STREAM_8BIT_SIGNED) {
        convert_32to8((const long*)srcData, (char*)dstData, totalSamples);

    } else if (srcType == AUK_STREAM_32BIT_SIGNED && dstType == AUK_STREAM_16BIT_SIGNED) {
        convert_32to16((const long*)srcData, (short*)dstData, totalSamples);

    } else {
        return AUK_STREAM_ERROR_FORMAT;
    }

    *outFrames = srcFrames;
    return AUK_STREAM_OK;
}
