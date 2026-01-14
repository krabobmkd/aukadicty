#include "auksoundfile.h"
#include "aukstring.h"
#include "serializer.h"
#include <proto/exec.h>

/*
 * AukSoundFile implementation
 * Manages sound file metadata and filename
 */

void AukSoundFile_New(AukObjectPtr* firstPtr) {
    AukSoundFile* soundFile;

    if (!firstPtr) {
        return;
    }

    soundFile = (AukSoundFile*)AllocVec(sizeof(AukSoundFile), MEMF_CLEAR);
    if (soundFile) {
        AukSoundFile_Init(soundFile);
        AukObjectPtr_Set(firstPtr, &soundFile->base);
    }
}

void AukSoundFile_Delete(void* This) {
    AukSoundFile* soundFile = (AukSoundFile*)This;
    if (soundFile) {
        /* Free filename string */
        if (soundFile->filename) {
            AukString_Free(soundFile->filename);
        }

        /* Call base object delete (which will FreeVec) */
        AukObject_Delete(&soundFile->base);
    }
}

const char* AukSoundFile_GetTypeName(void* This) {
    (void)This;
    return "AukSoundFile";
}

void AukSoundFile_Serialize(AukObject* This, ISerializer* ser, const char* pName) {
    AukSoundFile* soundFile = (AukSoundFile*)This;
    (void)pName;

    if (!soundFile || !ser) {
        return;
    }

    /* Serialize filename (relative path) */
    ser->t_string_mutable(ser, "filename", &soundFile->filename);

    /* Serialize audio properties */
    ser->t_ulonglong(ser, "sampleRate", (unsigned long long*)&soundFile->sampleRate);
    ser->t_ulonglong(ser, "channels", (unsigned long long*)&soundFile->channels);
    ser->t_ulonglong(ser, "frameCount", (unsigned long long*)&soundFile->frameCount);
}

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
        {
            AukMessage msg;
            msg.type = AUK_MSG_MODIFY;
            soundFile->base.SendUpdate(&soundFile->base, &msg);
        }
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
                                 unsigned long frameCount) {
    int changed;

    if (soundFile) {
        /* Check if values actually changed */
        changed = (soundFile->sampleRate != sampleRate ||
                   soundFile->channels != channels ||
                   soundFile->frameCount != frameCount);

        if (changed) {
            soundFile->sampleRate = sampleRate;
            soundFile->channels = channels;
            soundFile->frameCount = frameCount;

            /* Send update notification */
            {
                AukMessage msg;
                msg.type = AUK_MSG_MODIFY;
                soundFile->base.SendUpdate(&soundFile->base, &msg);
            }
        }
    }
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
        soundFile->filename = NULL;
        soundFile->sampleRate = 44100; /* Default */
        soundFile->channels = 2;       /* Default stereo */
        soundFile->frameCount = 0;
    }
}
