#include "auktrack.h"
#include "auksound.h"
#include "aukstring.h"
#include <proto/exec.h>
#include <string.h>

/*
 * AukTrack implementation
 * Manages a track with sounds and envelope
 */

#define INITIAL_SOUND_CAPACITY 16

void AukTrack_New(AukTrackPtr *firstPtr) {
    if (!firstPtr) return;
    AukTrack* track = (AukTrack*)AllocVec(sizeof(AukTrack), MEMF_CLEAR);
    if (track) {
        AukTrack_Init(track);
        AukObjectPtr_Set((AukObjectPtr*)firstPtr, &track->base);
    }
}

void AukTrack_Delete(void* This) {
    AukTrack* track = (AukTrack*)This;
    AukEnvelopePoint* point;
    AukEnvelopePoint* nextPoint;
    unsigned int i;

    if (track) {
        /* Free name */
        if (track->name) {
            AukString_Free(track->name);
        }

        /* Release all sounds using reference counting */
        if (track->sounds.sounds) {
            for (i = 0; i < track->sounds.count; i++) {
                if (track->sounds.sounds[i]) {
                    AukObjectPtr_Release((AukObjectPtr*)&track->sounds.sounds[i]);
                }
            }
            FreeVec(track->sounds.sounds);
        }

        /* Free envelope points */
        point = track->envelope;
        while (point) {
            nextPoint = point->next;
            FreeVec(point);
            point = nextPoint;
        }

        /* Call base object delete */
        AukObject_Delete(This);
    }
}

const char* AukTrack_GetTypeName(void* This) {
    (void)This;
    return "AukTrack";
}

void AukTrack_SetProject(AukTrack* track, AukProject* project) {
    if (track) {
        track->project = project;
    }
}

int AukTrack_SetName(AukTrack* track, const char* name) {
    int changed;

    if (!track || !name) {
        return 0;
    }

    /* Check if value actually changed */
    changed = (track->name == NULL || AukString_Compare(track->name, name) != 0);

    if (!changed) {
        return 1; /* No change, but success */
    }

    /* Free old name */
    if (track->name) {
        AukString_Free(track->name);
    }

    /* Duplicate new name */
    track->name = AukString_Duplicate(name);

    if (track->name) {
        /* Send update notification */
        track->base.SendUpdate(&track->base,NULL);
    }

    return track->name != NULL;
}

const char* AukTrack_GetName(AukTrack* track) {
    return track ? track->name : NULL;
}

AukSound* AukTrack_CreateSound(void* This, AukSoundFilePtr soundFile, AukFixed startTime, AukFixed endTime) {
    AukTrack* track = (AukTrack*)This;
    AukSoundPtr soundPtr = NULL;
    AukSound* sound;

    if (!track || !soundFile) {
        return NULL;
    }

    /* Create new sound */
    AukSound_New(&soundPtr);
    sound = soundPtr;
    if (!sound) {
        return NULL;
    }

    /* Set properties */
    AukSound_SetSoundFile(sound, soundFile);
    AukSound_SetTimeRange(sound, startTime, endTime);

    /* Add to track - this retains the sound */
    if (!track->AddSound(track, sound)) {
        /* Failed to add - release our reference */
        AukObjectPtr_Release(&soundPtr);
        return NULL;
    }

    /* Return raw pointer - the track owns the reference, caller doesn't */
    return sound;
}

int AukTrack_AddSound(void* This, AukSound* sound) {
    AukTrack* track = (AukTrack*)This;
    AukSound** newSounds;
    unsigned int newCapacity;

    if (!track || !sound) {
        return 0;
    }

    /* Grow array if needed */
    if (track->sounds.count >= track->sounds.capacity) {
        newCapacity = track->sounds.capacity == 0 ? INITIAL_SOUND_CAPACITY : track->sounds.capacity * 2;
        newSounds = (AukSound**)AllocVec(newCapacity * sizeof(AukSound*), MEMF_CLEAR);

        if (!newSounds) {
            return 0;
        }

        /* Copy existing sounds */
        if (track->sounds.sounds) {
            memcpy(newSounds, track->sounds.sounds, track->sounds.count * sizeof(AukSound*));
            FreeVec(track->sounds.sounds);
        }

        track->sounds.sounds = newSounds;
        track->sounds.capacity = newCapacity;
    }

    /* Add sound to array */
    track->sounds.sounds[track->sounds.count] = sound;
    track->sounds.count++;

    /* Set track reference in sound */
    AukSound_SetTrack(sound, track);

    /* Send update notification */
    track->base.SendUpdate(&track->base,NULL);

    return 1;
}

int AukTrack_RemoveSound(void* This, AukSound* sound) {
    AukTrack* track = (AukTrack*)This;
    unsigned int i;

    if (!track || !sound) {
        return 0;
    }

    /* Find and remove sound */
    for (i = 0; i < track->sounds.count; i++) {
        if (track->sounds.sounds[i] == sound) {
            /* Shift remaining sounds down */
            if (i < track->sounds.count - 1) {
                memcpy(&track->sounds.sounds[i],
                       &track->sounds.sounds[i + 1],
                       (track->sounds.count - i - 1) * sizeof(AukSound*));
            }

            track->sounds.count--;
            AukSound_SetTrack(sound, NULL);

            /* Send update notification */
            track->base.SendUpdate(&track->base,NULL);

            return 1;
        }
    }

    return 0;
}

int AukTrack_MoveSoundToTrack(void* This, AukSound* sound, AukTrack* destTrack) {
    AukTrack* srcTrack = (AukTrack*)This;

    if (!srcTrack || !sound || !destTrack) {
        return 0;
    }

    /* If sound isn't on source track, fail */
    if (sound->track != srcTrack) {
        return 0;
    }

    /* If already on destination track, nothing to do */
    if (srcTrack == destTrack) {
        return 1;
    }

    /* Add to destination track first */
    if (!destTrack->AddSound(destTrack, sound)) {
        return 0;
    }

    /* Remove from source track */
    if (!srcTrack->RemoveSound(srcTrack, sound)) {
        /* Failed to remove - this shouldn't happen, but try to undo */
        destTrack->RemoveSound(destTrack, sound);
        return 0;
    }

    return 1;
}

AukSound* AukTrack_GetSound(void* This, unsigned int index) {
    AukTrack* track = (AukTrack*)This;

    if (!track || index >= track->sounds.count) {
        return NULL;
    }

    return track->sounds.sounds[index];
}

unsigned int AukTrack_GetSoundCount(void* This) {
    AukTrack* track = (AukTrack*)This;
    return track ? track->sounds.count : 0;
}

int AukTrack_AddEnvelopePoint(void* This, AukFixed time, AukFixed value) {
    AukTrack* track = (AukTrack*)This;
    AukEnvelopePoint* newPoint;
    AukEnvelopePoint* current;
    AukEnvelopePoint* prev;

    if (!track) {
        return 0;
    }

    /* Allocate new point */
    newPoint = (AukEnvelopePoint*)AllocVec(sizeof(AukEnvelopePoint), MEMF_CLEAR);
    if (!newPoint) {
        return 0;
    }

    newPoint->time = time;
    newPoint->value = value;
    newPoint->next = NULL;

    /* Insert in sorted order by time */
    if (!track->envelope || time < track->envelope->time) {
        /* Insert at beginning */
        newPoint->next = track->envelope;
        track->envelope = newPoint;
    } else {
        /* Find insertion point */
        prev = track->envelope;
        current = track->envelope->next;

        while (current && current->time < time) {
            prev = current;
            current = current->next;
        }

        /* Insert after prev */
        newPoint->next = current;
        prev->next = newPoint;
    }

    /* Send update notification */
    track->base.SendUpdate(&track->base,NULL);

    return 1;
}

AukFixed AukTrack_GetEnvelopeValue(void* This, AukFixed time) {
    AukTrack* track = (AukTrack*)This;
    AukEnvelopePoint* p0;
    AukEnvelopePoint* p1;
    AukFixed t;

    if (!track || !track->envelope) {
        return AUK_FIXED_ONE; /* Default to full volume */
    }

    /* Find surrounding points */
    p0 = track->envelope;
    p1 = p0->next;

    /* Before first point */
    if (time < p0->time) {
        return p0->value;
    }

    /* Find interval containing time */
    while (p1) {
        if (time >= p0->time && time <= p1->time) {
            /* Linear interpolation between p0 and p1 */
            t = AukFixed_Div(AukFixed_Sub(time, p0->time),
                            AukFixed_Sub(p1->time, p0->time));
            return AukFixed_Lerp(p0->value, p1->value, t);
        }
        p0 = p1;
        p1 = p1->next;
    }

    /* After last point */
    return p0->value;
}

void AukTrack_Init(AukTrack* track) {
    if (track) {
        /* Initialize base object */
        AukObject_Init(&track->base);

        /* Override virtual methods */
        track->base.New = AukTrack_New;
        track->base.Delete = AukTrack_Delete;
        track->base.GetTypeName = AukTrack_GetTypeName;

        /* Set AukTrack specific methods */
        track->CreateSound = AukTrack_CreateSound;
        track->AddSound = AukTrack_AddSound;
        track->RemoveSound = AukTrack_RemoveSound;
        track->MoveSoundToTrack = AukTrack_MoveSoundToTrack;
        track->GetSound = AukTrack_GetSound;
        track->GetSoundCount = AukTrack_GetSoundCount;
        track->AddEnvelopePoint = AukTrack_AddEnvelopePoint;
        track->GetEnvelopeValue = AukTrack_GetEnvelopeValue;

        /* Initialize data members */
        track->project = NULL;
        track->name = NULL;
        track->sounds.sounds = NULL;
        track->sounds.count = 0;
        track->sounds.capacity = 0;
        track->envelope = NULL;
    }
}
