#include "auksound.h"
#include "auksoundfile.h"
#include "serializer.h"
#include <proto/exec.h>

/*
 * AukSound implementation
 * Represents an instance of a sound on a track
 */

void AukSound_New(AukObjectPtr* firstPtr) {
    AukSound* sound;

    if (!firstPtr) {
        return;
    }

    sound = (AukSound*)AllocVec(sizeof(AukSound), MEMF_CLEAR);
    if (sound) {
        AukSound_Init(sound);
        AukObjectPtr_Set(firstPtr, &sound->base);
    }
}

void AukSound_Delete(AukObject* This) {
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

const char* AukSound_GetTypeName(AukObject* This) {
    (void)This;
    return "AukSound";
}

void AukSound_Serialize(AukObject* This, ISerializer* ser, const char* pName) {
    AukSound* sound = (AukSound*)This;
    (void)pName;

    if (!sound || !ser) {
        return;
    }

    /* Serialize sound file reference */
    ser->t_object(ser, "soundFile", (AukObjectPtr*)&sound->soundFile);

    /* Serialize channel mask */
    ser->t_uint(ser, "chMask", (unsigned int*)&sound->channelMask);

    /* Serialize timing information */
    ser->t_fixed(ser, "startTime", &sound->startTime);
    ser->t_fixed(ser, "endTime", &sound->endTime);

    /* Serialize file range */
    ser->t_ulonglong(ser, "fileStartFrame", (unsigned long long*)&sound->fileStartFrame);
    ser->t_ulonglong(ser, "fileEndFrame", (unsigned long long*)&sound->fileEndFrame);

    /* Serialize loop count */
    ser->t_ulonglong(ser, "loopCount", (unsigned long long*)&sound->loopCount);
}

void AukSound_SetSoundFile(AukSound* sound, AukSoundFile *soundFile) {
    int changed;

    if (!sound ) {
        return;
    }

    /* Check if value actually changed */
    changed = (sound->soundFile != soundFile);

    if (!changed) {
        return; /* No change */
    }

    /* Retain new reference */
    AukObjectPtr_Set((AukObjectPtr*)&sound->soundFile, (AukObject*)soundFile);

    /* Set project context on soundFile if sound has project context */
    if (soundFile && sound->base._project) {
        soundFile->base._project = sound->base._project;
    }

    /* Send update notification */
    {
        AukMessage msg;
        msg.type = AUK_MSG_MODIFY;
        sound->base.SendUpdate(&sound->base, &msg);
    }
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
            {
                AukMessage msg;
                msg.type = AUK_MSG_MODIFY;
                sound->base.SendUpdate(&sound->base, &msg);
            }
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
            {
                AukMessage msg;
                msg.type = AUK_MSG_MODIFY;
                sound->base.SendUpdate(&sound->base, &msg);
            }
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
            {
                AukMessage msg;
                msg.type = AUK_MSG_MODIFY;
                sound->base.SendUpdate(&sound->base, &msg);
            }
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
        sound->channelMask = 3; /* Default stereo (bits 0 and 1) */
        sound->startTime = 0;
        sound->endTime = 0;
        sound->fileStartFrame = 0;
        sound->fileEndFrame = 0;
        sound->loopCount = 0;
    }
}

void AukSound_SetChannelMask(AukSound* sound, unsigned long mask)
{
    if (!sound) return;
    if (sound->channelMask == mask) return;

    sound->channelMask = mask;

    /* Send update notification */
    {
        AukMessage msg;
        msg.type = AUK_MSG_MODIFY;
        sound->base.SendUpdate(&sound->base, &msg);
    }
}

unsigned long AukSound_GetChannelMask(AukSound* sound)
{
    if (!sound) return 0;
    return sound->channelMask;
}
