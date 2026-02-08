#ifndef AUKSOUNDFILE_H
#define AUKSOUNDFILE_H

/*
 * AukSoundFile - Represents a sound file on disk
 * Shared resource - multiple Sound objects can reference same file
 * Also The center object to get sounds buffers with Asynchronous AukSoundFileEngine.
 * AukSoundFile are modified by an external Process and consumed by multiple other processes.
 * get them from AukSoundFileEngine, ask for buffers when needed, and receive
 * asynchronous messages from AukSoundFileEngine when some data are ready.
 */

#include "aukobject.h"

#ifdef __cplusplus
extern "C" {
#endif

/* File status */
typedef enum {
    AUKSF_STATUS_PENDING=0,    /* Waiting to be processed, no data usable yet */
    AUKSF_STATUS_STATED_PHASE1, /* fileformat/sampleRate/channels/frameCount ready, stats continue */
    AUKSF_STATUS_STATED_PHASE2, /* at least one min/max piece ready, stats continue  */
    AUKSF_STATUS_STATED_PHASE3, /* all min/max piece ready */
    AUKSF_STATUS_ERROR         /* Error occurred */
} AukSFFileStatus;

/* we aim to save memory about this struct. -1,1 range ported to 0,255 128 being 0.
 Basically this (will) be used for sound wave display in boopsi gadget TrackArea,
 But also may be used in some equalizer mixing equalizer routine.
 */
typedef struct {
    unsigned char min,max;
} AUKSFMinMax;

// 32768b
#define SOUNDBUFFERPARTSIZEL2 15
#define SOUNDBUFFERPARTSIZE (1UL<<SOUNDBUFFERPARTSIZEL2)
#define SOUNDBUFFERPARTMASK (SOUNDBUFFERPARTSIZE-1)

typedef struct {
    /* 0 incomplete/notloaded, 1 loaded and available.
        _nblocks can grow from consumer processes,
        _nblocks>0 triggers loading, then later when buffer is ready we'll have _state==1.
        when _nblocks is back to zero *and* buffer shortage, flush will get it back to 1.

        -> One per channel
    */
    unsigned short _state;

    unsigned short _nblocks; /* when get 0 back, may release buffer (or only on flush when shortage ?). */
    unsigned int _sampleRate; /* mirror AukSoundFile sampleRate */
    unsigned int _nbSamples; /* for this part, some power of 2 thing, except for the last one which have rest. */
    unsigned int _sampleoffset; /* offset to start of sample */
    unsigned int _bufferlength; /* _nbSamples */

    /* watch out we'll assume this comes from a "buffer cache", we're not going to alloc in real time during mixing. */
    signed short *_buffer;   /* may be NULL. interleaved channels */

    /* todo may have members about cache priority to manage flush and max buffer in mem. */
} SoundBufferPart;

struct AukSoundFile;

/* define what is used to read */
typedef struct SoundReaderPlugin {
    /*return what is stored in AukSoundFile.soundReaderPluginData */
    void *(*open)(struct AukSoundFile *s,const char *absfilepath);
    void (*close)(struct AukSoundFile *s);
    void (*read)(struct AukSoundFile *s,SoundBufferPart *p);
} SoundReaderPlugin;

/* AukSoundFile structure - inherits from AukObject
  All data are updated asynchronously.
 */
struct AukSoundFile {
    AukObject base;          /* Must be first - inheritance */

    int status; /* AukSFFileStatus see enum up there, must be checked before any further member read */

    char fileformat[12]; /* to display rather than test, because file format are abstract because of plugin interface */

    /* informations ready when AUKSF_STATUS_STATED_PHASE1 */
    char* filename;          /* Relative path to sound file. Always absolute path at this level. */
    unsigned long sampleRate; /* Sample rate in Hz (e.g., 44100) */
    unsigned long channels;   /* Number of channels (1=mono, 2=stereo) */
    unsigned long frameCount; /* Total number of sample frames */

    /* informations ready when AUKSF_STATUS_STATED_PHASE2.
     * length is channels * ((frameCount+255)>>8)
     *  stride is channels*sizeof(AUKSFEMinMax)
     * for a 5min stereo 44100 wave this would weights 206kb
     */
    AUKSFMinMax *minmaxdiv256;
    int         minmaxdiv256_l; /* allocated */
    int         minmaxdiv256_length; /* available */

    /* access to complete mixable data buffers
      Watch out AukSoundFileEngine read dynamically sound parts when needed,
      and multiple track sounds can asks for different moments in the sample (because of cuts and copy paste editions)
        You can just index a sample with buffers[i>>14]._buffer[(i&0x3fff)*nbchan],
        by testing buffers[i>>14]._state , of course.

        Now read buffer are divided in channels.
        This way if we support reading 3 ,4 or 5 channels files,
        but only need to process one or 2, we'll cache only this two.
        Hence, have a "channel mask" in SoundFileConsumer down there.


        buffers[iChannel][iPart]

        struct table is allocated before AUKSF_STATUS_STATED_PHASE1,
        then buffers[iChannel][iPart]._buffer is attributed later

     */
     SoundBufferPart **buffers;
     int        nbBufferParts; /* given by (frameCount+ ((1<<14)-1))>>14 */

    /* internal use, the plugin used to load this file, if null file format unknown.
        This is set by AukSoundFileEngine.
     */
    SoundReaderPlugin *fileformatreader;
    void            *soundReaderPluginData;

    /* Virtual methods specific to AukSoundFile */
    int (*SetFilename)(void* This, const char* filename);
    const char* (*GetFilename)(void* This);
};

/* Constructor/Destructor */
void AukSoundFile_New(AukObjectPtr* firstPtr);
void AukSoundFile_Delete(AukObject* This);
const char* AukSoundFile_GetTypeName(AukObject* This);

/* Initialize AukSoundFile structure */
void AukSoundFile_Init(AukSoundFile* soundFile);

/* Methods */
int AukSoundFile_SetFilename(void* This, const char* filename);
const char* AukSoundFile_GetFilename(void* This);
void AukSoundFile_SetProperties(AukSoundFile* soundFile,
                                 unsigned long sampleRate,
                                 unsigned long channels,
                                 unsigned long frameCount);


/* extra tools for consumer, to assume locks are always unlocked.
 * (experimental)
 * should acts like a "guard" for file part streaming.
 * Should be used by:
 * - AukSound (-> and then players/mixers/ui would use this.)
 * - stats
 */
typedef struct SoundFileConsumerSpan {
    AukSoundFile* soundFile;
    /* span for which we have locked parts in that soundfile. 0,0 means none. */
    int iPartStart,iPartEnd;
    int channelMask;
} SoundFileConsumer;

/* for a single SoundFileConsumer, we must only lock once a part */
void SoundFileConsumer_ReadAndLockTimeSpan(SoundFileConsumer *sfc,
                            long long tstart,long long tend, unsigned int channelMask );
void SoundFileConsumer_UnlockTimeSpan(SoundFileConsumer *sfc );


#ifdef __cplusplus
}
#endif

#endif /* AUKSOUNDFILE_H */
