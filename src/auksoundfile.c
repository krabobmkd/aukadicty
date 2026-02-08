/*
 * AukSoundFile implementation
 * Manages sound file metadata, buffer parts, and consumer lock/unlock mechanism.
 */

#include "auksoundfile.h"
#include "aukstring.h"
#include "serializer.h"

#ifdef AMIGA
#include <proto/exec.h>
#else
#include <stdlib.h>
#include <string.h>

static void* pc_AllocVec(unsigned long size, unsigned long flags) {
    void* p = malloc(size);
    if (p && (flags & MEMF_CLEAR)) memset(p, 0, size);
    return p;
}
static void pc_FreeVec(void* p) { free(p); }

#define AllocVec(size, flags) pc_AllocVec(size, flags)
#define FreeVec(p) pc_FreeVec(p)
#endif

/* ============================================================
 * Constructor / Destructor
 * ============================================================ */

void AukSoundFile_New(AukObjectPtr* firstPtr) {
    AukSoundFile* soundFile;

    if (!firstPtr) {
        return;
    }

    /* MEMF_PUBLIC for multi-process access */
    soundFile = (AukSoundFile*)AllocVec(sizeof(AukSoundFile), MEMF_CLEAR | MEMF_PUBLIC);
    if (soundFile) {
        AukSoundFile_Init(soundFile);
        AukObjectPtr_Set(firstPtr, &soundFile->base);
    }
}

void AukSoundFile_Delete(AukObject* This) {
    AukSoundFile* soundFile = (AukSoundFile*)This;
    if (soundFile) {
        /* Close plugin if open */
        if (soundFile->fileformatreader && soundFile->soundReaderPluginData) {
            soundFile->fileformatreader->close(soundFile);
        }

        /* Free buffer parts */
        AukSoundFile_FreeBufferParts(soundFile);

        /* Free min/max array */
        if (soundFile->minmaxdiv256) {
            FreeVec(soundFile->minmaxdiv256);
            soundFile->minmaxdiv256 = NULL;
        }

        /* Free filename string */
        if (soundFile->filename) {
            AukString_Free(soundFile->filename);
        }

        /* Call base object delete (which will FreeVec) */
        AukObject_Delete(&soundFile->base);
    }
}

const char* AukSoundFile_GetTypeName(AukObject* This) {
    (void)This;
    return "AukSoundFile";
}

void AukSoundFile_Serialize(AukObject* This, ISerializer* ser, const char* pName) {
    AukSoundFile* soundFile = (AukSoundFile*)This;
    (void)pName;

    if (!soundFile || !ser) {
        return;
    }

    /* Serialize filename (relative path) - this is the only persisted data */
    ser->t_string_mutable(ser, "filename", &soundFile->filename);

    /* Note: sampleRate, channels, frameCount are read from file, not serialized */
}

void AukSoundFile_Init(AukSoundFile* soundFile) {
    if (soundFile) {
        /* Initialize base object */
        AukObject_Init(&soundFile->base);

        /* Override virtual methods */
        soundFile->base.New = AukSoundFile_New;
        soundFile->base.Delete = AukSoundFile_Delete;
        soundFile->base.GetTypeName = AukSoundFile_GetTypeName;
        soundFile->base.Serialize = AukSoundFile_Serialize;

        /* Set AukSoundFile specific methods */
        soundFile->SetFilename = AukSoundFile_SetFilename;
        soundFile->GetFilename = AukSoundFile_GetFilename;

        /* Initialize data members */
        soundFile->status = AUKSF_STATUS_PENDING;
        soundFile->filename = NULL;
        soundFile->sampleRate = 0;
        soundFile->channels = 0;
        soundFile->frameCount = 0;
        soundFile->bytesPerSample = 0;

        soundFile->minmaxdiv256 = NULL;
        soundFile->minmaxdiv256_allocated = 0;
        soundFile->minmaxdiv256_length = 0;
        soundFile->minmaxStride = 0;

        soundFile->buffers = NULL;
        soundFile->nbBufferParts = 0;

        soundFile->fileformatreader = NULL;
        soundFile->soundReaderPluginData = NULL;

        soundFile->fileformat[0] = '\0';
    }
}

/* ============================================================
 * Property Methods
 * ============================================================ */

int AukSoundFile_SetFilename(void* This, const char* filename) {
    AukSoundFile* soundFile = (AukSoundFile*)This;
    int changed;

    if (!soundFile || !filename) {
        return 0;
    }

    /* Check if value actually changed */
    changed = (soundFile->filename == NULL || AukString_Compare(soundFile->filename, filename) != 0);

    if (!changed) {
        return 1; /* No change, but success */
    }

    /* Free old filename if exists */
    if (soundFile->filename) {
        AukString_Free(soundFile->filename);
    }

    /* Duplicate new filename */
    soundFile->filename = AukString_Duplicate(filename);

    if (soundFile->filename) {
        /* Send update notification */
        AukMessage msg;
        msg.type = AUK_MSG_MODIFY;
        soundFile->base.SendUpdate(&soundFile->base, &msg);
    }

    return soundFile->filename != NULL;
}

const char* AukSoundFile_GetFilename(void* This) {
    AukSoundFile* soundFile = (AukSoundFile*)This;
    return soundFile ? soundFile->filename : NULL;
}

void AukSoundFile_SetProperties(AukSoundFile* soundFile,
                                 unsigned long sampleRate,
                                 unsigned long channels,
                                 unsigned long frameCount,
                                 unsigned long bytesPerSample) {
    if (!soundFile) return;

    soundFile->sampleRate = sampleRate;
    soundFile->channels = channels;
    soundFile->frameCount = frameCount;
    soundFile->bytesPerSample = bytesPerSample;

    /* Allocate buffer parts structure */
    AukSoundFile_AllocBufferParts(soundFile);

    /* Send update notification */
    {
        AukMessage msg;
        msg.type = AUK_MSG_MODIFY;
        soundFile->base.SendUpdate(&soundFile->base, &msg);
    }
}

/* ============================================================
 * Buffer Parts Management
 * ============================================================ */

int AukSoundFile_AllocBufferParts(AukSoundFile* soundFile) {
    unsigned long nParts, iChan, iPart;

    if (!soundFile || soundFile->frameCount == 0 || soundFile->channels == 0) {
        return 0;
    }

    /* Free existing if any */
    AukSoundFile_FreeBufferParts(soundFile);

    /* Calculate number of parts */
    nParts = (soundFile->frameCount + SOUNDBUFFERPARTSIZE - 1) >> SOUNDBUFFERPARTSIZEL2;
    soundFile->nbBufferParts = nParts;

    /* Allocate channel array */
    soundFile->buffers = (SoundBufferPart**)AllocVec(
        sizeof(SoundBufferPart*) * soundFile->channels,
        MEMF_CLEAR | MEMF_PUBLIC
    );
    if (!soundFile->buffers) return 0;

    /* Allocate parts array for each channel */
    for (iChan = 0; iChan < soundFile->channels; iChan++) {
        soundFile->buffers[iChan] = (SoundBufferPart*)AllocVec(
            sizeof(SoundBufferPart) * nParts,
            MEMF_CLEAR | MEMF_PUBLIC
        );
        if (!soundFile->buffers[iChan]) {
            AukSoundFile_FreeBufferParts(soundFile);
            return 0;
        }

        /* Initialize each part */
        for (iPart = 0; iPart < nParts; iPart++) {
            SoundBufferPart* part = &soundFile->buffers[iChan][iPart];
            part->_state = 0;
            part->_nblocks = 0;
            part->_sampleRate = soundFile->sampleRate;
            part->_sampleoffset = iPart << SOUNDBUFFERPARTSIZEL2;

            /* Last part may have fewer samples */
            if (iPart == nParts - 1) {
                unsigned long remaining = soundFile->frameCount - part->_sampleoffset;
                part->_nbSamples = remaining;
            } else {
                part->_nbSamples = SOUNDBUFFERPARTSIZE;
            }

            part->_buffer = NULL;
            part->_lastAccessTime = 0;
        }
    }

    return 1;
}

void AukSoundFile_FreeBufferParts(AukSoundFile* soundFile) {
    unsigned long iChan, iPart;

    if (!soundFile || !soundFile->buffers) return;

    for (iChan = 0; iChan < soundFile->channels; iChan++) {
        if (soundFile->buffers[iChan]) {
            for (iPart = 0; iPart < (unsigned long)soundFile->nbBufferParts; iPart++) {
                SoundBufferPart* part = &soundFile->buffers[iChan][iPart];
                /* Note: _buffer comes from pool, not freed here */
                part->_buffer = NULL;
            }
            FreeVec(soundFile->buffers[iChan]);
        }
    }

    FreeVec(soundFile->buffers);
    soundFile->buffers = NULL;
    soundFile->nbBufferParts = 0;
}

/* ============================================================
 * Min/Max Array Allocation
 * ============================================================ */

int AukSoundFile_AllocMinMax(AukSoundFile* soundFile) {
    unsigned long chunksPerChannel, totalChunks;

    if (!soundFile || soundFile->frameCount == 0 || soundFile->channels == 0) {
        return 0;
    }

    /* Free existing if any */
    if (soundFile->minmaxdiv256) {
        FreeVec(soundFile->minmaxdiv256);
        soundFile->minmaxdiv256 = NULL;
    }

    /* Calculate array size */
    chunksPerChannel = (soundFile->frameCount + 255) >> 8;
    totalChunks = chunksPerChannel * soundFile->channels;

    soundFile->minmaxdiv256 = (AukSFMinMax*)AllocVec(
        sizeof(AukSFMinMax) * totalChunks,
        MEMF_CLEAR | MEMF_PUBLIC
    );
    if (!soundFile->minmaxdiv256) return 0;

    soundFile->minmaxdiv256_allocated = totalChunks;
    soundFile->minmaxdiv256_length = 0;  /* Filled during Phase 2 */
    soundFile->minmaxStride = chunksPerChannel;

    return 1;
}

/* ============================================================
 * Consumer API Implementation
 * ============================================================ */

void SoundFileConsumer_Init(SoundFileConsumer* consumer, AukSoundFile* soundFile) {
    if (!consumer) return;

    consumer->soundFile = soundFile;
    consumer->iPartStart = 0;
    consumer->iPartEnd = -1;  /* -1 = no parts locked */
    consumer->channelMask = 0;
}

int SoundFileConsumer_LockSampleRange(SoundFileConsumer* consumer,
                                       unsigned long sampleStart,
                                       unsigned long sampleEnd,
                                       unsigned int channelMask) {
    AukSoundFile* sf;
    int newPartStart, newPartEnd;
    int iPart, iChan;
    unsigned int chanBit;
    int allReady;

    if (!consumer || !consumer->soundFile) return 0;

    sf = consumer->soundFile;

    /* Must be at least in Phase 1 */
    if (sf->status < AUKSF_STATUS_STATED_PHASE1 || !sf->buffers) return 0;

    /* Clamp range */
    if (sampleEnd >= sf->frameCount) {
        sampleEnd = sf->frameCount - 1;
    }
    if (sampleStart > sampleEnd) return 0;

    /* Calculate part range */
    newPartStart = sampleStart >> SOUNDBUFFERPARTSIZEL2;
    newPartEnd = sampleEnd >> SOUNDBUFFERPARTSIZEL2;

    if (newPartEnd >= sf->nbBufferParts) {
        newPartEnd = sf->nbBufferParts - 1;
    }

    /* If same range and mask, just check readiness */
    if (newPartStart == consumer->iPartStart &&
        newPartEnd == consumer->iPartEnd &&
        channelMask == consumer->channelMask) {
        return SoundFileConsumer_IsReady(consumer);
    }

    /* Unlock old parts that are no longer needed */
    if (consumer->iPartEnd >= 0) {
        for (iChan = 0, chanBit = 1; iChan < (int)sf->channels; iChan++, chanBit <<= 1) {
            if (!(consumer->channelMask & chanBit)) continue;

            for (iPart = consumer->iPartStart; iPart <= consumer->iPartEnd; iPart++) {
                /* If this part is outside new range or channel not in new mask */
                if (iPart < newPartStart || iPart > newPartEnd ||
                    !(channelMask & chanBit)) {
                    SoundBufferPart* part = &sf->buffers[iChan][iPart];
                    if (part->_nblocks > 0) {
                        part->_nblocks--;
                    }
                }
            }
        }
    }

    /* Lock new parts */
    allReady = 1;
    for (iChan = 0, chanBit = 1; iChan < (int)sf->channels; iChan++, chanBit <<= 1) {
        if (!(channelMask & chanBit)) continue;

        for (iPart = newPartStart; iPart <= newPartEnd; iPart++) {
            /* If this part wasn't previously locked */
            if (consumer->iPartEnd < 0 ||
                iPart < consumer->iPartStart || iPart > consumer->iPartEnd ||
                !(consumer->channelMask & chanBit)) {
                SoundBufferPart* part = &sf->buffers[iChan][iPart];
                part->_nblocks++;
            }

            /* Check if ready */
            if (sf->buffers[iChan][iPart]._state == 0) {
                allReady = 0;
            }
        }
    }

    /* Update consumer state */
    consumer->iPartStart = newPartStart;
    consumer->iPartEnd = newPartEnd;
    consumer->channelMask = channelMask;

    return allReady;
}

int SoundFileConsumer_LockTimeRange(SoundFileConsumer* consumer,
                                     long long tstart,
                                     long long tend,
                                     unsigned int channelMask) {
    AukSoundFile* sf;
    unsigned long sampleStart, sampleEnd;

    if (!consumer || !consumer->soundFile) return 0;

    sf = consumer->soundFile;

    /* Must be at least in Phase 1 */
    if (sf->status < AUKSF_STATUS_STATED_PHASE1 || sf->sampleRate == 0) return 0;

    /* Validate time range */
    if (tend < tstart || tstart < 0) return 0;

    /*
     * Convert 32.32 fixed-point seconds to samples.
     * tstart/tend are in 32.32 format: upper 32 bits = seconds, lower 32 bits = fraction
     *
     * samples = time * sampleRate
     *         = (time_hi + time_lo/2^32) * sampleRate
     *         = time_hi * sampleRate + (time_lo * sampleRate) / 2^32
     */
    {
        unsigned long tstart_hi = (unsigned long)(tstart >> 32);
        unsigned long tstart_lo = (unsigned long)tstart;
        unsigned long tend_hi = (unsigned long)(tend >> 32);
        unsigned long tend_lo = (unsigned long)tend;

        /* Compute start sample */
        sampleStart = tstart_hi * sf->sampleRate;
        /* Add fractional part: (tstart_lo * sampleRate) >> 32 */
        /* To avoid overflow, split the multiplication */
        sampleStart += ((tstart_lo >> 16) * sf->sampleRate) >> 16;

        /* Compute end sample */
        sampleEnd = tend_hi * sf->sampleRate;
        sampleEnd += ((tend_lo >> 16) * sf->sampleRate) >> 16;
    }

    return SoundFileConsumer_LockSampleRange(consumer, sampleStart, sampleEnd, channelMask);
}

void SoundFileConsumer_Unlock(SoundFileConsumer* consumer) {
    AukSoundFile* sf;
    int iPart, iChan;
    unsigned int chanBit;

    if (!consumer || !consumer->soundFile) return;

    sf = consumer->soundFile;

    if (consumer->iPartEnd < 0 || !sf->buffers) return;

    /* Decrement nblocks for all locked parts */
    for (iChan = 0, chanBit = 1; iChan < (int)sf->channels; iChan++, chanBit <<= 1) {
        if (!(consumer->channelMask & chanBit)) continue;

        for (iPart = consumer->iPartStart; iPart <= consumer->iPartEnd; iPart++) {
            SoundBufferPart* part = &sf->buffers[iChan][iPart];
            if (part->_nblocks > 0) {
                part->_nblocks--;
            }
        }
    }

    /* Reset consumer state */
    consumer->iPartStart = 0;
    consumer->iPartEnd = -1;
    consumer->channelMask = 0;
}

int SoundFileConsumer_IsReady(SoundFileConsumer* consumer) {
    AukSoundFile* sf;
    int iPart, iChan;
    unsigned int chanBit;

    if (!consumer || !consumer->soundFile) return 0;

    sf = consumer->soundFile;

    if (consumer->iPartEnd < 0 || !sf->buffers) return 1;  /* Nothing locked = ready */

    for (iChan = 0, chanBit = 1; iChan < (int)sf->channels; iChan++, chanBit <<= 1) {
        if (!(consumer->channelMask & chanBit)) continue;

        for (iPart = consumer->iPartStart; iPart <= consumer->iPartEnd; iPart++) {
            if (sf->buffers[iChan][iPart]._state == 0) {
                return 0;  /* At least one part not ready */
            }
        }
    }

    return 1;  /* All locked parts are ready */
}
