#include "auksound.h"
#include "serializer.h"
#include <proto/exec.h>

/*
 * AukSound implementation
 * Represents an instance of a sound on a track
 */

void AukSound_New(AukSoundPtr* firstPtr) {
    AukSound* sound;

    if (!firstPtr) {
        return;
    }

    sound = (AukSound*)AllocVec(sizeof(AukSound), MEMF_CLEAR);
    if (sound) {
        AukSound_Init(sound);
        AukObjectPtr_Set((AukObjectPtr*)firstPtr, &sound->base);
    }
}

void AukSound_Delete(void* This) {
    AukSound* sound = (AukSound*)This;
    if (sound) {
        /* Release shared sound file reference */
        if (sound->soundFile) {
            AukObjectPtr_Release((AukObjectPtr*)&sound->soundFile);
        }

        /* Call base object delete (which will FreeVec) */
        AukObject_Delete(&sound->base);
    }
}

const char* AukSound_GetTypeName(void* This) {
    (void)This;
    return "AukSound";
}

void AukSound_Serialize(void* This, ISerializer* ser, const char* pName) {
    AukSound* sound = (AukSound*)This;
    (void)pName;

    if (!sound || !ser) {
        return;
    }

    /* Serialize sound file reference */
    ser->t_object(ser, "soundFile", (AukObjectPtr*)&sound->soundFile);

    /* Serialize timing information */
    ser->t_fixed(ser, "startTime", &sound->startTime);
    ser->t_fixed(ser, "endTime", &sound->endTime);

    /* Serialize file range */
    ser->t_ulonglong(ser, "fileStartFrame", (unsigned long long*)&sound->fileStartFrame);
    ser->t_ulonglong(ser, "fileEndFrame", (unsigned long long*)&sound->fileEndFrame);

    /* Serialize loop count */
    ser->t_ulonglong(ser, "loopCount", (unsigned long long*)&sound->loopCount);
}

void AukSound_SetSoundFile(AukSound* sound, AukSoundFilePtr soundFile) {
    int changed;

    if (!sound) {
        return;
    }

    /* Check if value actually changed */
    changed = (sound->soundFile != soundFile);

    if (!changed) {
        return; /* No change */
    }

    /* Release old reference */
    if (sound->soundFile) {
        AukObjectPtr_Release((AukObjectPtr*)&sound->soundFile);
    }

    /* Retain new reference */
    AukObjectPtr_Set((AukObjectPtr*)&sound->soundFile, (AukObject*)soundFile);

    /* Send update notification */
    sound->base.SendUpdate(&sound->base, NULL);
}

void AukSound_SetTimeRange(void* This, AukFixed start, AukFixed end) {
    AukSound* sound = (AukSound*)This;
    int changed;

    if (sound) {
        /* Check if values actually changed */
        changed = (sound->startTime != start || sound->endTime != end);

        if (changed) {
            sound->startTime = start;
            sound->endTime = end;

            /* Send update notification */
            sound->base.SendUpdate(&sound->base,NULL);
        }
    }
}

void AukSound_SetFileRange(void* This, unsigned long startFrame, unsigned long endFrame) {
    AukSound* sound = (AukSound*)This;
    int changed;

    if (sound) {
        /* Check if values actually changed */
        changed = (sound->fileStartFrame != startFrame || sound->fileEndFrame != endFrame);

        if (changed) {
            sound->fileStartFrame = startFrame;
            sound->fileEndFrame = endFrame;

            /* Send update notification */
            sound->base.SendUpdate(&sound->base,NULL);
        }
    }
}

void AukSound_SetLoopCount(void* This, unsigned long count) {
    AukSound* sound = (AukSound*)This;
    int changed;

    if (sound) {
        /* Check if value actually changed */
        changed = (sound->loopCount != count);

        if (changed) {
            sound->loopCount = count;

            /* Send update notification */
            sound->base.SendUpdate(&sound->base,NULL);
        }
    }
}

AukFixed AukSound_GetDuration(void* This) {
    AukSound* sound = (AukSound*)This;
    if (sound) {
        return AukFixed_Sub(sound->endTime, sound->startTime);
    }
    return 0;
}

void AukSound_Init(AukSound* sound) {
    if (sound) {
        /* Initialize base object */
        AukObject_Init(&sound->base);

        /* Override virtual methods */
        sound->base.New = AukSound_New;
        sound->base.Delete = AukSound_Delete;
        sound->base.GetTypeName = AukSound_GetTypeName;
        sound->base.Serialize = AukSound_Serialize;

        /* Set AukSound specific methods */
        sound->SetTimeRange = AukSound_SetTimeRange;
        sound->SetFileRange = AukSound_SetFileRange;
        sound->SetLoopCount = AukSound_SetLoopCount;
        sound->GetDuration = AukSound_GetDuration;

        /* Initialize data members */
        sound->soundFile = NULL;
        sound->startTime = 0;
        sound->endTime = 0;
        sound->fileStartFrame = 0;
        sound->fileEndFrame = 0;
        sound->loopCount = 0;
    }
}
