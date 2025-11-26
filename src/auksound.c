#include "auksound.h"
#include <proto/exec.h>

/*
 * AukSound implementation
 * Represents an instance of a sound on a track
 */

void* AukSound_New(void) {
    AukSound* sound = (AukSound*)AllocVec(sizeof(AukSound), MEMF_CLEAR);
    if (sound) {
        AukSound_Init(sound);
    }
    return sound;
}

void AukSound_Delete(void* This) {
    AukSound* sound = (AukSound*)This;
    if (sound) {
        /* Release shared sound file reference */
        if (sound->soundFile) {
            AukShared_Release(sound->soundFile);
        }

        /* Free the object itself */
        FreeVec(sound);
    }
}

const char* AukSound_GetTypeName(void* This) {
    (void)This;
    return "AukSound";
}

void AukSound_SetSoundFile(AukSound* sound, AukShared* soundFile) {
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
        AukShared_Release(sound->soundFile);
    }

    /* Retain new reference */
    sound->soundFile = soundFile;
    if (soundFile) {
        AukShared_Retain(soundFile);
    }

    /* Send update notification */
    sound->base.SendUpdate(sound);
}

void AukSound_SetTrack(AukSound* sound, AukTrack* track) {
    if (sound) {
        sound->track = track;
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
            sound->base.SendUpdate(sound);
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
            sound->base.SendUpdate(sound);
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
            sound->base.SendUpdate(sound);
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

        /* Set AukSound specific methods */
        sound->SetTimeRange = AukSound_SetTimeRange;
        sound->SetFileRange = AukSound_SetFileRange;
        sound->SetLoopCount = AukSound_SetLoopCount;
        sound->GetDuration = AukSound_GetDuration;

        /* Initialize data members */
        sound->soundFile = NULL;
        sound->track = NULL;
        sound->startTime = 0;
        sound->endTime = 0;
        sound->fileStartFrame = 0;
        sound->fileEndFrame = 0;
        sound->loopCount = 0;
    }
}
