/*
 * AukSoundFile Format Reader Plugins
 *
 * Implements WAV and 8SVX file readers adapted from aukstreamcache.
 * These plugins are used by AukSoundFileEngine to read different audio formats.
 */

#include "auksoundfile.h"
#include "aukstring.h"

#ifdef AMIGA
#include <proto/dos.h>
#include <proto/exec.h>
#else
/* PC stubs for file I/O */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* BPTR is a file handle on PC */
static BPTR pc_Open(const char* path, int mode) {
    (void)mode;
    return (BPTR)fopen(path, "rb");
}

static void pc_Close(BPTR file) {
    if (file) fclose((FILE*)file);
}

static long pc_Read(BPTR file, void* buffer, long length) {
    if (!file) return -1;
    return (long)fread(buffer, 1, length, (FILE*)file);
}

static long pc_Seek(BPTR file, long offset, long mode) {
    if (!file) return -1;
    int whence = SEEK_SET;
    if (mode == OFFSET_CURRENT) whence = SEEK_CUR;
    else if (mode == OFFSET_BEGINNING) whence = SEEK_SET;
    fseek((FILE*)file, offset, whence);
    return ftell((FILE*)file);
}

#define Open(path, mode) pc_Open(path, mode)
#define Close(file) pc_Close(file)
#define Read(file, buf, len) pc_Read(file, buf, len)
#define Seek(file, off, mode) pc_Seek(file, off, mode)
#define MODE_OLDFILE 1005

#ifndef MEMF_CLEAR
#define MEMF_CLEAR 0x10000
#define MEMF_PUBLIC 0x1
#endif

static void* pc_AllocVec(unsigned long size, unsigned long flags) {
    void* p = malloc(size);
    if (p && (flags & MEMF_CLEAR)) memset(p, 0, size);
    return p;
}

static void pc_FreeVec(void* p) {
    free(p);
}

#define AllocVec(size, flags) pc_AllocVec(size, flags)
#define FreeVec(p) pc_FreeVec(p)

#endif /* !AMIGA */

/* ============================================================
 * WAVE File Reader
 * ============================================================ */

/* RIFF chunk identifiers (little-endian as stored in file) */
#define RIFF_ID 0x46464952  /* 'RIFF' stored little-endian */
#define WAVE_ID 0x45564157  /* 'WAVE' stored little-endian */
#define FMT_ID  0x20746D66  /* 'fmt ' stored little-endian */
#define DATA_ID 0x61746164  /* 'data' stored little-endian */

/* WAVE format codes */
#define WAVE_FORMAT_PCM 1

/* Plugin-specific data for WAV files */
typedef struct {
    BPTR file;
    unsigned long dataOffset;
    unsigned long dataSize;
    unsigned long bytesPerFrame;
    unsigned long bytesPerSample;
    int dataType;  /* 8, 16, or 32 bits */
} WavePluginData;

/* Helper: Read 32-bit little-endian value */
static unsigned long ReadLE32(BPTR file) {
    unsigned char buf[4];
    if (Read(file, buf, 4) != 4) return 0;
    return (unsigned long)buf[0] |
           ((unsigned long)buf[1] << 8) |
           ((unsigned long)buf[2] << 16) |
           ((unsigned long)buf[3] << 24);
}

/* Helper: Read 16-bit little-endian value */
static unsigned short ReadLE16(BPTR file) {
    unsigned char buf[2];
    if (Read(file, buf, 2) != 2) return 0;
    return (unsigned short)buf[0] | ((unsigned short)buf[1] << 8);
}

static void* WavePlugin_Open(AukSoundFile* s, const char* absFilePath) {
    BPTR file;
    WavePluginData* data;
    unsigned long riffId, fileSize, waveId;
    unsigned long chunkId, chunkSize;
    unsigned short audioFormat, numChannels, bitsPerSample;
    unsigned long sampleRate, byteRate;
    unsigned short blockAlign;
    int foundFmt = 0, foundData = 0;

    file = Open(absFilePath, MODE_OLDFILE);
    if (!file) return NULL;

    /* Seek to beginning */
    Seek(file, 0, OFFSET_BEGINNING);

    /* Read RIFF header */
    riffId = ReadLE32(file);
    fileSize = ReadLE32(file);
    waveId = ReadLE32(file);

    if (riffId != RIFF_ID || waveId != WAVE_ID) {
        Close(file);
        return NULL;
    }

    /* Allocate plugin data */
    data = (WavePluginData*)AllocVec(sizeof(WavePluginData), MEMF_CLEAR | MEMF_PUBLIC);
    if (!data) {
        Close(file);
        return NULL;
    }
    data->file = file;

    /* Parse chunks */
    while (!foundFmt || !foundData) {
        long pos = Seek(file, 0, OFFSET_CURRENT);
        if (pos < 0 || pos >= (long)fileSize + 8) break;

        chunkId = ReadLE32(file);
        chunkSize = ReadLE32(file);

        if (chunkId == 0) break;  /* End of file */

        if (chunkId == FMT_ID) {
            /* Parse format chunk */
            audioFormat = ReadLE16(file);
            numChannels = ReadLE16(file);
            sampleRate = ReadLE32(file);
            byteRate = ReadLE32(file);
            blockAlign = ReadLE16(file);
            bitsPerSample = ReadLE16(file);

            (void)byteRate;
            (void)blockAlign;

            /* Validate format */
            if (audioFormat != WAVE_FORMAT_PCM) {
                FreeVec(data);
                Close(file);
                return NULL;
            }

            /* Fill soundfile info */
            s->sampleRate = sampleRate;
            s->channels = numChannels;

            if (bitsPerSample == 8) {
                data->dataType = 8;
                s->bytesPerSample = 1;
            } else if (bitsPerSample == 16) {
                data->dataType = 16;
                s->bytesPerSample = 2;
            } else if (bitsPerSample == 32) {
                data->dataType = 32;
                s->bytesPerSample = 4;
            } else {
                FreeVec(data);
                Close(file);
                return NULL;
            }

            data->bytesPerSample = s->bytesPerSample;
            data->bytesPerFrame = s->bytesPerSample * numChannels;

            foundFmt = 1;

            /* Skip remaining format bytes */
            if (chunkSize > 16) {
                Seek(file, chunkSize - 16, OFFSET_CURRENT);
            }

        } else if (chunkId == DATA_ID) {
            /* Found data chunk */
            data->dataOffset = Seek(file, 0, OFFSET_CURRENT);
            data->dataSize = chunkSize;

            /* Calculate frame count */
            if (data->bytesPerFrame > 0) {
                s->frameCount = chunkSize / data->bytesPerFrame;
            }

            foundData = 1;

            /* Skip data for now */
            Seek(file, chunkSize, OFFSET_CURRENT);

        } else {
            /* Skip unknown chunk */
            Seek(file, chunkSize, OFFSET_CURRENT);
        }

        /* Align to word boundary */
        if (chunkSize & 1) {
            Seek(file, 1, OFFSET_CURRENT);
        }
    }

    if (!foundFmt || !foundData) {
        FreeVec(data);
        Close(file);
        return NULL;
    }

    /* Set format string */
    if (data->dataType == 8) {
        strcpy(s->fileformat, "WAVE 8-bit PCM");
    } else if (data->dataType == 16) {
        strcpy(s->fileformat, "WAVE 16-bit PCM");
    } else {
        strcpy(s->fileformat, "WAVE 32-bit PCM");
    }

    return data;
}

static void WavePlugin_Close(AukSoundFile* s) {
    WavePluginData* data = (WavePluginData*)s->soundReaderPluginData;
    if (data) {
        if (data->file) {
            Close(data->file);
        }
        FreeVec(data);
        s->soundReaderPluginData = NULL;
    }
}

static int WavePlugin_Read(AukSoundFile* s, SoundBufferPart* part, int iChannel) {
    WavePluginData* data = (WavePluginData*)s->soundReaderPluginData;
    unsigned long bytesToRead, framesRead;
    unsigned long i;
    signed short* dest;

    if (!data || !data->file || !part || !part->_buffer) return -1;

    /* Calculate file position */
    unsigned long fileOffset = data->dataOffset + (part->_sampleoffset * data->bytesPerFrame);
    Seek(data->file, fileOffset, OFFSET_BEGINNING);

    /* Read frames into part buffer */
    bytesToRead = part->_nbSamples * data->bytesPerFrame;

    /* Allocate temporary buffer for reading if format conversion needed */
    if (data->dataType != 16 || s->channels > 1) {
        unsigned char* tempBuf = (unsigned char*)AllocVec(bytesToRead, MEMF_CLEAR);
        if (!tempBuf) return -1;

        long bytesRead = Read(data->file, tempBuf, bytesToRead);
        if (bytesRead <= 0) {
            FreeVec(tempBuf);
            return -1;
        }

        framesRead = bytesRead / data->bytesPerFrame;
        dest = part->_buffer;

        /* Convert to 16-bit, extract channel */
        if (data->dataType == 8) {
            /* 8-bit unsigned to 16-bit signed */
            for (i = 0; i < framesRead; i++) {
                unsigned char sample = tempBuf[i * s->channels + iChannel];
                dest[i] = ((signed short)sample - 128) << 8;
            }
        } else if (data->dataType == 16) {
            /* 16-bit signed, extract channel */
            signed short* src16 = (signed short*)tempBuf;
            for (i = 0; i < framesRead; i++) {
                dest[i] = src16[i * s->channels + iChannel];
            }
        } else {
            /* 32-bit to 16-bit */
            signed int* src32 = (signed int*)tempBuf;
            for (i = 0; i < framesRead; i++) {
                dest[i] = (signed short)(src32[i * s->channels + iChannel] >> 16);
            }
        }

        FreeVec(tempBuf);
    } else {
        /* Direct read for mono 16-bit */
        long bytesRead = Read(data->file, part->_buffer, bytesToRead);
        if (bytesRead <= 0) return -1;
        framesRead = bytesRead / data->bytesPerFrame;
    }

    /* Clear remaining samples if we read less than expected */
    if (framesRead < part->_nbSamples) {
        dest = part->_buffer + framesRead;
        for (i = framesRead; i < part->_nbSamples; i++) {
            dest[i - framesRead] = 0;
        }
    }

    return 0;
}

/* WAVE Plugin instance */
static SoundReaderPlugin g_WavePlugin = {
    "WAVE",
    WavePlugin_Open,
    WavePlugin_Close,
    WavePlugin_Read
};

/* ============================================================
 * IFF 8SVX File Reader
 * ============================================================ */

/* IFF chunk identifiers (big-endian) */
#define FORM_ID 0x464F524D  /* 'FORM' */
#define SVX8_ID 0x38535658  /* '8SVX' */
#define VHDR_ID 0x56484452  /* 'VHDR' */
#define BODY_ID 0x424F4459  /* 'BODY' */

/* Plugin-specific data for 8SVX files */
typedef struct {
    BPTR file;
    unsigned long dataOffset;
    unsigned long dataSize;
} SVX8PluginData;

/* Helper: Read 32-bit big-endian value */
static unsigned long ReadBE32(BPTR file) {
    unsigned char buf[4];
    if (Read(file, buf, 4) != 4) return 0;
    return ((unsigned long)buf[0] << 24) |
           ((unsigned long)buf[1] << 16) |
           ((unsigned long)buf[2] << 8) |
           (unsigned long)buf[3];
}

/* Helper: Read 16-bit big-endian value */
static unsigned short ReadBE16(BPTR file) {
    unsigned char buf[2];
    if (Read(file, buf, 2) != 2) return 0;
    return ((unsigned short)buf[0] << 8) | (unsigned short)buf[1];
}

static void* SVX8Plugin_Open(AukSoundFile* s, const char* absFilePath) {
    BPTR file;
    SVX8PluginData* data;
    unsigned long formId, formSize, svxId;
    unsigned long chunkId, chunkSize;
    unsigned long oneShotHiSamples, repeatHiSamples, samplesPerHiCycle;
    unsigned short samplesPerSec;
    unsigned char ctOctave, sCompression;
    int foundVhdr = 0, foundBody = 0;

    file = Open(absFilePath, MODE_OLDFILE);
    if (!file) return NULL;

    /* Seek to beginning */
    Seek(file, 0, OFFSET_BEGINNING);

    /* Read FORM header */
    formId = ReadBE32(file);
    formSize = ReadBE32(file);
    svxId = ReadBE32(file);

    if (formId != FORM_ID || svxId != SVX8_ID) {
        Close(file);
        return NULL;
    }

    /* Allocate plugin data */
    data = (SVX8PluginData*)AllocVec(sizeof(SVX8PluginData), MEMF_CLEAR | MEMF_PUBLIC);
    if (!data) {
        Close(file);
        return NULL;
    }
    data->file = file;

    /* Parse chunks */
    while (!foundVhdr || !foundBody) {
        long pos = Seek(file, 0, OFFSET_CURRENT);
        if (pos < 0 || pos >= (long)formSize + 8) break;

        chunkId = ReadBE32(file);
        chunkSize = ReadBE32(file);

        if (chunkId == 0) break;

        if (chunkId == VHDR_ID) {
            /* Parse voice header */
            if (chunkSize < 20) {
                FreeVec(data);
                Close(file);
                return NULL;
            }

            oneShotHiSamples = ReadBE32(file);
            repeatHiSamples = ReadBE32(file);
            samplesPerHiCycle = ReadBE32(file);
            samplesPerSec = ReadBE16(file);
            Read(file, &ctOctave, 1);
            Read(file, &sCompression, 1);
            /* Skip volume (4 bytes) */
            ReadBE32(file);

            (void)oneShotHiSamples;
            (void)repeatHiSamples;
            (void)samplesPerHiCycle;
            (void)ctOctave;

            /* Validate compression (0 = uncompressed) */
            if (sCompression != 0) {
                FreeVec(data);
                Close(file);
                return NULL;
            }

            /* Fill soundfile info */
            s->sampleRate = samplesPerSec;
            s->channels = 1;  /* 8SVX is always mono */
            s->bytesPerSample = 1;

            foundVhdr = 1;

            /* Skip remaining header bytes */
            if (chunkSize > 20) {
                Seek(file, chunkSize - 20, OFFSET_CURRENT);
            }

        } else if (chunkId == BODY_ID) {
            /* Found body chunk */
            data->dataOffset = Seek(file, 0, OFFSET_CURRENT);
            data->dataSize = chunkSize;
            s->frameCount = chunkSize;  /* 8-bit mono = 1 byte per frame */

            foundBody = 1;

            /* Skip body for now */
            Seek(file, chunkSize, OFFSET_CURRENT);

        } else {
            /* Skip unknown chunk */
            Seek(file, chunkSize, OFFSET_CURRENT);
        }

        /* IFF chunks are word-aligned */
        if (chunkSize & 1) {
            Seek(file, 1, OFFSET_CURRENT);
        }
    }

    if (!foundVhdr || !foundBody) {
        FreeVec(data);
        Close(file);
        return NULL;
    }

    /* Set format string */
    strcpy(s->fileformat, "IFF 8SVX");

    return data;
}

static void SVX8Plugin_Close(AukSoundFile* s) {
    SVX8PluginData* data = (SVX8PluginData*)s->soundReaderPluginData;
    if (data) {
        if (data->file) {
            Close(data->file);
        }
        FreeVec(data);
        s->soundReaderPluginData = NULL;
    }
}

static int SVX8Plugin_Read(AukSoundFile* s, SoundBufferPart* part, int iChannel) {
    SVX8PluginData* data = (SVX8PluginData*)s->soundReaderPluginData;
    unsigned long bytesToRead, bytesRead;
    unsigned char* tempBuf;
    signed short* dest;
    unsigned long i;

    (void)iChannel;  /* 8SVX is always mono, channel 0 */

    if (!data || !data->file || !part || !part->_buffer) return -1;

    /* Calculate file position */
    unsigned long fileOffset = data->dataOffset + part->_sampleoffset;
    Seek(data->file, fileOffset, OFFSET_BEGINNING);

    /* Read samples */
    bytesToRead = part->_nbSamples;

    /* Allocate temp buffer for 8-bit data */
    tempBuf = (unsigned char*)AllocVec(bytesToRead, MEMF_CLEAR);
    if (!tempBuf) return -1;

    bytesRead = Read(data->file, tempBuf, bytesToRead);
    if (bytesRead <= 0) {
        FreeVec(tempBuf);
        return -1;
    }

    /* Convert 8-bit signed to 16-bit signed */
    dest = part->_buffer;
    for (i = 0; i < bytesRead; i++) {
        /* 8SVX uses signed 8-bit */
        dest[i] = ((signed char)tempBuf[i]) << 8;
    }

    /* Clear remaining samples */
    for (i = bytesRead; i < part->_nbSamples; i++) {
        dest[i] = 0;
    }

    FreeVec(tempBuf);
    return 0;
}

/* 8SVX Plugin instance */
static SoundReaderPlugin g_SVX8Plugin = {
    "8SVX",
    SVX8Plugin_Open,
    SVX8Plugin_Close,
    SVX8Plugin_Read
};

/* ============================================================
 * Format Detection and Plugin Registry
 * ============================================================ */

AukSFFileFormat AukSoundFile_DetectFormat(const char* filename) {
    const char* ext;
    int len;

    if (!filename) return AUKSF_FORMAT_UNKNOWN;

    /* Find last dot */
    len = strlen(filename);
    ext = filename + len;
    while (ext > filename && *(ext - 1) != '.') {
        ext--;
    }

    if (ext == filename) return AUKSF_FORMAT_UNKNOWN;

    /* Case-insensitive extension comparison */
    if ((ext[0] == 'w' || ext[0] == 'W') &&
        (ext[1] == 'a' || ext[1] == 'A') &&
        (ext[2] == 'v' || ext[2] == 'V') &&
        (ext[3] == '\0' || ((ext[3] == 'e' || ext[3] == 'E') && ext[4] == '\0'))) {
        return AUKSF_FORMAT_WAVE;
    }

    if ((ext[0] == '8' || ext[0] == '8') &&
        (ext[1] == 's' || ext[1] == 'S') &&
        (ext[2] == 'v' || ext[2] == 'V') &&
        (ext[3] == 'x' || ext[3] == 'X') &&
        ext[4] == '\0') {
        return AUKSF_FORMAT_8SVX;
    }

    /* Also check for .iff extension (often used for 8SVX) */
    if ((ext[0] == 'i' || ext[0] == 'I') &&
        (ext[1] == 'f' || ext[1] == 'F') &&
        (ext[2] == 'f' || ext[2] == 'F') &&
        ext[3] == '\0') {
        return AUKSF_FORMAT_8SVX;
    }

    return AUKSF_FORMAT_UNKNOWN;
}

SoundReaderPlugin* AukSoundFile_GetPlugin(AukSFFileFormat format) {
    switch (format) {
        case AUKSF_FORMAT_WAVE:
            return &g_WavePlugin;
        case AUKSF_FORMAT_8SVX:
            return &g_SVX8Plugin;
        default:
            return NULL;
    }
}
