#ifndef AUKSOUNDFILE_H
#define AUKSOUNDFILE_H

/*
 * AukSoundFile - Represents a sound file on disk
 *
 * Shared resource - multiple Sound objects can reference same file.
 * The center object to get sound buffers with asynchronous AukSoundFileEngine.
 * AukSoundFile are modified by an external Process and consumed by multiple other processes.
 *
 * Get them from AukSoundFileEngine, ask for buffers when needed, and receive
 * asynchronous messages from AukSoundFileEngine when data is ready.
 *
 * Buffer organization:
 * - buffers[iChannel][iPart] where each part is SOUNDBUFFERPARTSIZE samples
 * - Each channel is independent to allow selective loading (e.g., only left channel)
 * - Parts are loaded on-demand via consumer lock/unlock mechanism
 */

#include "aukobject.h"
#include "auksoundfileengine.h"

#ifdef __cplusplus
extern "C" {
#endif

/* File status - mirrors AukSFEFileStatus for convenience */
typedef enum {
    AUKSF_STATUS_PENDING = 0,       /* Waiting to be processed, no data usable yet */
    AUKSF_STATUS_STATED_PHASE1,     /* fileformat/sampleRate/channels/frameCount ready */
    AUKSF_STATUS_STATED_PHASE2,     /* Min/max computation in progress */
    AUKSF_STATUS_STATED_PHASE3,     /* All min/max ready */
    AUKSF_STATUS_ERROR              /* Error occurred */
} AukSFFileStatus;

/*
 * Min/Max statistics for waveform display.
 * Saves memory: -1..1 range mapped to 0..255 (128 = zero).
 * Used for:
 * - Waveform display in TrackArea BOOPSI gadget
 * - Potentially for mixing/equalizer quick preview
 */
typedef struct {
    unsigned char min;
    unsigned char max;
} AukSFMinMax;

/*
 * Sound buffer part - one chunk of audio data for one channel.
 * Size is fixed at SOUNDBUFFERPARTSIZE samples.
 */

/* Part size: 32768 samples (64KB for 16-bit mono, 128KB for stereo interleaved) */
#define SOUNDBUFFERPARTSIZEL2 15
#define SOUNDBUFFERPARTSIZE (1UL << SOUNDBUFFERPARTSIZEL2)
#define SOUNDBUFFERPARTMASK (SOUNDBUFFERPARTSIZE - 1)

typedef struct SoundBufferPart {
    /*
     * Loading state:
     * 0 = not loaded / incomplete
     * 1 = loaded and available
     */
    unsigned short _state;

    /*
     * Reference count from consumers.
     * When > 0: buffer must be loaded (triggers loading if _state == 0)
     * When back to 0: buffer may be flushed on memory shortage
     */
    unsigned short _nblocks;

    /* Audio format (mirrors AukSoundFile but stored per-part for thread safety) */
    unsigned int _sampleRate;
    unsigned int _nbSamples;        /* Actual samples in this part (last part may be smaller) */
    unsigned int _sampleoffset;     /* Offset in file (in samples from start) */

    /*
     * Buffer data pointer.
     * NULL until loaded, points to pool chunk when ready.
     * Format: signed 16-bit samples, interleaved if stereo.
     *
     * IMPORTANT: This comes from the engine's buffer pool, not runtime AllocVec.
     */
    signed short *_buffer;

    /* For LRU cache management */
    unsigned long _lastAccessTime;  /* Timestamp of last access */

} SoundBufferPart;

struct AukSoundFile;

/*
 * Sound reader plugin interface.
 * Plugins handle format-specific file I/O (WAV, 8SVX, MP3, etc.)
 */
typedef struct SoundReaderPlugin {
    /* Format identifier for display (e.g., "WAV", "8SVX") */
    const char* formatName;

    /*
     * Open file and read header.
     * Returns plugin-specific data stored in soundReaderPluginData.
     * Should fill: sampleRate, channels, frameCount, fileformat
     * Returns NULL on error.
     */
    void* (*open)(struct AukSoundFile *s, const char *absFilePath);

    /*
     * Close file and free plugin data.
     */
    void (*close)(struct AukSoundFile *s);

    /*
     * Read samples into a buffer part.
     * Reads from file, converts to 16-bit signed if needed.
     * Part's _sampleoffset and _nbSamples define what to read.
     */
    int (*read)(struct AukSoundFile *s, SoundBufferPart *part, int iChannel);

} SoundReaderPlugin;

/*
 * Format detection result
 */
typedef enum {
    AUKSF_FORMAT_UNKNOWN = 0,
    AUKSF_FORMAT_WAVE,
    AUKSF_FORMAT_8SVX
    /* Future: AUKSF_FORMAT_MP3, AUKSF_FORMAT_FLAC, etc. */
} AukSFFileFormat;

/*
 * Detect file format from filename extension.
 */
AukSFFileFormat AukSoundFile_DetectFormat(const char* filename);

/*
 * Get plugin for a format.
 */
SoundReaderPlugin* AukSoundFile_GetPlugin(AukSFFileFormat format);

/*
 * AukSoundFile structure - inherits from AukObject.
 * All data are updated asynchronously by the engine worker thread.
 */
struct AukSoundFile {
    AukObject base;                 /* Must be first - inheritance */

    /* Current loading status - check before reading other members */
    int status;                     /* AukSFFileStatus */

    /* Format string for display (e.g., "WAVE 16-bit", "8SVX 8-bit") */
    char fileformat[32];

    /* --- Available when status >= AUKSF_STATUS_STATED_PHASE1 --- */

    char* filename;                 /* Absolute path to sound file */
    unsigned long sampleRate;       /* Sample rate in Hz (e.g., 44100) */
    unsigned long channels;         /* Number of channels (1=mono, 2=stereo) */
    unsigned long frameCount;       /* Total number of sample frames */
    unsigned long bytesPerSample;   /* 1 for 8-bit, 2 for 16-bit, 4 for 32-bit */

    /* --- Available when status >= AUKSF_STATUS_STATED_PHASE2 --- */

    /*
     * Min/max statistics per 256 samples, per channel.
     * Array layout: minmaxdiv256[iChannel * minmaxStride + iChunk]
     * Length per channel: (frameCount + 255) >> 8
     * Total length: channels * ((frameCount + 255) >> 8)
     *
     * For a 5-minute stereo 44100Hz file: ~206KB
     */
    AukSFMinMax *minmaxdiv256;
    int minmaxdiv256_allocated;     /* Allocated size */
    int minmaxdiv256_length;        /* Valid entries per channel */
    int minmaxStride;               /* Stride between channels */

    /* --- Buffer parts for audio data --- */

    /*
     * 2D array of buffer parts: buffers[iChannel][iPart]
     * Structure is allocated in Phase 1, buffers loaded on demand.
     *
     * Access pattern:
     *   sample = buffers[iChannel][iSample >> SOUNDBUFFERPARTSIZEL2]
     *            ._buffer[iSample & SOUNDBUFFERPARTMASK]
     *   (after checking _state == 1)
     */
    SoundBufferPart **buffers;
    int nbBufferParts;              /* (frameCount + SOUNDBUFFERPARTSIZE - 1) >> SOUNDBUFFERPARTSIZEL2 */

    /* --- Internal: Plugin state --- */

    SoundReaderPlugin *fileformatreader;    /* Plugin used to read this file */
    void *soundReaderPluginData;            /* Plugin-specific data (file handle, etc.) */

    /* --- Virtual methods specific to AukSoundFile --- */

    int (*SetFilename)(void* This, const char* filename);
    const char* (*GetFilename)(void* This);
};

typedef struct AukSoundFile AukSoundFile;
typedef AukSoundFile* AukSoundFilePtr;

/* ============================================================
 * Constructor/Destructor
 * ============================================================ */

void AukSoundFile_New(AukObjectPtr* firstPtr);
void AukSoundFile_Delete(AukObject* This);
const char* AukSoundFile_GetTypeName(AukObject* This);
void AukSoundFile_Init(AukSoundFile* soundFile);

/* ============================================================
 * Property Methods
 * ============================================================ */

int AukSoundFile_SetFilename(void* This, const char* filename);
const char* AukSoundFile_GetFilename(void* This);

/*
 * Set audio properties (called by engine after Phase 1).
 * Also allocates buffer part structure.
 */
void AukSoundFile_SetProperties(AukSoundFile* soundFile,
                                 unsigned long sampleRate,
                                 unsigned long channels,
                                 unsigned long frameCount,
                                 unsigned long bytesPerSample);

/*
 * Allocate min/max array (called by engine before Phase 2).
 */
int AukSoundFile_AllocMinMax(AukSoundFile* soundFile);

/*
 * Allocate buffer parts structure (called by engine after Phase 1).
 */
int AukSoundFile_AllocBufferParts(AukSoundFile* soundFile);

/*
 * Free buffer parts and their data.
 */
void AukSoundFile_FreeBufferParts(AukSoundFile* soundFile);

/* ============================================================
 * Consumer API - Lock/Unlock Buffer Parts
 *
 * Consumers (mixer, player, exporter, UI) use these to request
 * and release buffer ranges. The engine handles actual loading.
 * ============================================================ */

/*
 * Consumer context - tracks which parts a consumer has locked.
 * Each consumer (mixer, player, etc.) should have its own context.
 */
typedef struct SoundFileConsumer {
    AukSoundFile* soundFile;
    int iPartStart;                 /* First locked part (inclusive) */
    int iPartEnd;                   /* Last locked part (inclusive), -1 if none */
    unsigned int channelMask;       /* Which channels are locked (bit 0 = chan 0, etc.) */
} SoundFileConsumer;

/*
 * Initialize a consumer context.
 */
void SoundFileConsumer_Init(SoundFileConsumer* consumer, AukSoundFile* soundFile);

/*
 * Request buffers for a sample range.
 *
 * Increments _nblocks for needed parts, decrements for parts no longer needed.
 * If parts aren't loaded, they will be queued for loading by the engine.
 *
 * @param consumer     Consumer context
 * @param sampleStart  First sample index needed
 * @param sampleEnd    Last sample index needed (inclusive)
 * @param channelMask  Which channels to lock (bit 0 = channel 0, etc.)
 *
 * @return 1 if all parts are ready, 0 if some are still loading
 */
int SoundFileConsumer_LockSampleRange(SoundFileConsumer* consumer,
                                       unsigned long sampleStart,
                                       unsigned long sampleEnd,
                                       unsigned int channelMask);

/*
 * Request buffers for a time range (fixed-point seconds).
 *
 * @param consumer     Consumer context
 * @param tstart       Start time in 32.32 fixed-point seconds
 * @param tend         End time in 32.32 fixed-point seconds
 * @param channelMask  Which channels to lock
 *
 * @return 1 if all parts are ready, 0 if some are still loading
 */
int SoundFileConsumer_LockTimeRange(SoundFileConsumer* consumer,
                                     long long tstart,
                                     long long tend,
                                     unsigned int channelMask);

/*
 * Release all locked parts.
 * Decrements _nblocks for all currently locked parts.
 */
void SoundFileConsumer_Unlock(SoundFileConsumer* consumer);

/*
 * Check if all currently locked parts are ready.
 *
 * @return 1 if all ready, 0 if still loading
 */
int SoundFileConsumer_IsReady(SoundFileConsumer* consumer);

#ifdef __cplusplus
}
#endif

#endif /* AUKSOUNDFILE_H */
