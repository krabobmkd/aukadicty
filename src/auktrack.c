#include "auktrack.h"
#include "auksound.h"
#include "auksoundfile.h"
#include "aukenvelopepoint.h"
#include "aukstring.h"
#include "serializer.h"
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

    if (track) {
        /* Free name */
        if (track->name) {
            AukString_Free(track->name);
        }

        /* Release all sounds using reference counting */
        AukObjectPtr_Release((AukObjectPtr*)&track->sounds);

        /* Release all envelope points using reference counting */
        AukObjectPtr_Release((AukObjectPtr*)&track->envelopePoints);

        /* Call base object delete */
        AukObject_Delete(This);
    }
}

const char* AukTrack_GetTypeName(void* This) {
    (void)This;
    return "AukTrack";
}

void AukTrack_Serialize(void* This, ISerializer* ser, const char* pName) {
    AukTrack* track = (AukTrack*)This;
    (void)pName;

    if (!track || !ser) {
        return;
    }

    /* Serialize track name */
    ser->t_string_mutable(ser, "name", &track->name);

    /* Serialize sounds array */
    ser->t_arrayobj(ser, "sounds", &track->sounds, AukSound_New, AukSound_GetTypeName);

    /* Serialize envelope points array */
    ser->t_arrayobj(ser, "envelopePoints", &track->envelopePoints, AukEnvelopePoint_New, AukEnvelopePoint_GetTypeName);
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
    if (!track->sounds->Add(track->sounds, sound)) {
        /* Failed to add - release our reference */
        AukObjectPtr_Release(&soundPtr);
        return NULL;
    }
    AukObjectPtr_Release(&soundPtr);
    track->base.SendUpdate(&track->base, NULL);
    /* Return raw pointer - the track owns the reference, caller doesn't */
    return sound;
}


int AukTrack_MoveSoundToTrack(void* This, AukSound* sound, AukTrack* destTrack) {
    AukTrack* srcTrack = (AukTrack*)This;

    if (!srcTrack || !sound || !destTrack) {
        return 0;
    }

    /* If already on destination track, nothing to do */
    if (srcTrack == destTrack) {
        return 1;
    }

    /* Add to destination track first */
    if (!destTrack->sounds->Add(&destTrack->sounds, sound)) {
        return 0;
    }

    /* Remove from source track */
    if (!srcTrack->sounds->Remove(&srcTrack->sounds, sound)) {
        /* Failed to remove - this shouldn't happen, but try to undo */
        srcTrack->sounds->Add(&srcTrack->sounds, sound);
        return 0;
    }
    /* Send update notification */
    srcTrack->base.SendUpdate(&srcTrack->base, NULL);
    destTrack->base.SendUpdate(&destTrack->base, NULL);

    return 1;
}

unsigned int AukTrack_GetSoundCount(void* This) {
    AukTrack* track = (AukTrack*)This;
    return (track && track->sounds)? track->sounds->count : 0;
}
void AukTrack_GetSound(void* This,AukSound**ptr, unsigned int index) {
    AukTrack* track = (AukTrack*)This;
    AukArray* tracksArray;

    if(!ptr) return;
    AukObjectPtr_Release((AukObjectPtr*)ptr);

    if (!track || !track->sounds) {
        return;
    }

    tracksArray = (AukArray*)track->sounds;
    tracksArray->Get(tracksArray,ptr, index);
}
int AukTrack_RemoveSound(void* This, AukSound* sound){
    AukTrack* track = (AukTrack*)This;
    AukArray* tracksArray;

    if (!track || !sound || !track->sounds) {
        return 0;
    }

    tracksArray = (AukArray*)track->sounds;

    /* Remove track using AukArray */
    if (tracksArray->Remove(tracksArray, &sound->base)) {

        /* Send update notification */
        track->base.SendUpdate(&track->base, NULL);

        return 1;
    }

    return 0;
}



AukEnvelopePoint* AukTrack_CreateEnvelopePoint(void* This, AukFixed time, AukFixed value) {
    AukTrack* track = (AukTrack*)This;
    AukEnvelopePointPtr pointPtr = NULL;
    AukEnvelopePoint* point;
    unsigned int i;
    unsigned int insertIndex = 0;

    if (!track || !track->envelopePoints) {
        return NULL;
    }

    /* Create new envelope point */
    AukEnvelopePoint_New(&pointPtr);
    point = (AukEnvelopePoint*)pointPtr;
    if (!point) {
        return NULL;
    }

    /* Set properties */
    AukEnvelopePoint_SetTime(point, time);
    AukEnvelopePoint_SetValue(point, value);

    /* Find insertion position to keep sorted by time */
    for (i = 0; i < track->envelopePoints->count; i++) {
        AukEnvelopePointPtr existingPtr = NULL;
        AukEnvelopePoint* existing;

        track->envelopePoints->Get(track->envelopePoints, &existingPtr, i);
        existing = (AukEnvelopePoint*)existingPtr;

        if (existing && existing->time > time) {
            AukObjectPtr_Release(&existingPtr);
            break;
        }

        AukObjectPtr_Release(&existingPtr);
        insertIndex = i + 1;
    }

    /* Insert at the found position */
    if (!track->envelopePoints->Insert(track->envelopePoints, insertIndex, &point->base)) {
        /* Failed to add - release our reference */
        AukObjectPtr_Release(&pointPtr);
        return NULL;
    }

    AukObjectPtr_Release(&pointPtr);
    track->base.SendUpdate(&track->base, NULL);

    /* Return raw pointer - the track owns the reference, caller doesn't */
    return point;
}

int AukTrack_RemoveEnvelopePoint(void* This, AukEnvelopePoint* point) {
    AukTrack* track = (AukTrack*)This;

    if (!track || !point || !track->envelopePoints) {
        return 0;
    }

    /* Remove point using AukArray */
    if (track->envelopePoints->Remove(track->envelopePoints, &point->base)) {
        /* Send update notification */
        track->base.SendUpdate(&track->base, NULL);
        return 1;
    }

    return 0;
}

void AukTrack_GetEnvelopePoint(void* This, AukEnvelopePoint** ptr, unsigned int index) {
    AukTrack* track = (AukTrack*)This;

    if (!ptr) return;
    AukObjectPtr_Release((AukObjectPtr*)ptr);

    if (!track || !track->envelopePoints) {
        return;
    }

    track->envelopePoints->Get(track->envelopePoints, (AukObjectPtr*)ptr, index);
}

unsigned int AukTrack_GetEnvelopePointCount(void* This) {
    AukTrack* track = (AukTrack*)This;
    return (track && track->envelopePoints) ? track->envelopePoints->count : 0;
}

AukFixed AukTrack_GetEnvelopeValue(void* This, AukFixed time) {
    AukTrack* track = (AukTrack*)This;
    AukEnvelopePointPtr p0Ptr = NULL;
    AukEnvelopePointPtr p1Ptr = NULL;
    AukEnvelopePoint* p0;
    AukEnvelopePoint* p1;
    AukFixed result;
    AukFixed t;
    unsigned int i;

    if (!track || !track->envelopePoints || track->envelopePoints->count == 0) {
        return AUK_FIXED_ONE; /* Default to full volume */
    }

    /* Get first point */
    track->envelopePoints->Get(track->envelopePoints, &p0Ptr, 0);
    p0 = (AukEnvelopePoint*)p0Ptr;

    /* Before first point */
    if (time < p0->time) {
        result = p0->value;
        AukObjectPtr_Release(&p0Ptr);
        return result;
    }

    /* Find interval containing time */
    for (i = 0; i < track->envelopePoints->count - 1; i++) {
        AukObjectPtr_Release(&p0Ptr);
        track->envelopePoints->Get(track->envelopePoints, &p0Ptr, i);
        track->envelopePoints->Get(track->envelopePoints, &p1Ptr, i + 1);

        p0 = (AukEnvelopePoint*)p0Ptr;
        p1 = (AukEnvelopePoint*)p1Ptr;

        if (time >= p0->time && time <= p1->time) {
            /* Linear interpolation between p0 and p1 */
            t = AukFixed_Div(AukFixed_Sub(time, p0->time),
                            AukFixed_Sub(p1->time, p0->time));
            result = AukFixed_Lerp(p0->value, p1->value, t);
            AukObjectPtr_Release(&p0Ptr);
            AukObjectPtr_Release(&p1Ptr);
            return result;
        }

        AukObjectPtr_Release(&p1Ptr);
    }

    /* After last point - p0 is still the last point from the loop */
    result = p0->value;
    AukObjectPtr_Release(&p0Ptr);
    return result;
}

void AukTrack_Init(AukTrack* track) {
    if (track) {
        /* Initialize base object */
        AukObject_Init(&track->base);

        /* Override virtual methods */
        track->base.New = AukTrack_New;
        track->base.Delete = AukTrack_Delete;
        track->base.GetTypeName = AukTrack_GetTypeName;
        track->base.Serialize = AukTrack_Serialize;

        /* Set AukTrack specific methods */
        track->CreateSound = AukTrack_CreateSound;
        track->RemoveSound = AukTrack_RemoveSound;
        track->MoveSoundToTrack = AukTrack_MoveSoundToTrack;
        track->GetSound = AukTrack_GetSound;
        track->GetSoundCount = AukTrack_GetSoundCount;

        /* Set envelope management methods */
        track->CreateEnvelopePoint = AukTrack_CreateEnvelopePoint;
        track->RemoveEnvelopePoint = AukTrack_RemoveEnvelopePoint;
        track->GetEnvelopePoint = AukTrack_GetEnvelopePoint;
        track->GetEnvelopePointCount = AukTrack_GetEnvelopePointCount;
        track->GetEnvelopeValue = AukTrack_GetEnvelopeValue;

        /* Initialize data members */
        track->project = NULL;
        track->name = NULL;

        /* Initialize sounds array */
        AukArray_New(&track->sounds);
        if (track->sounds) {
            AukArray_SetType(track->sounds, AukSound_New, AukSound_GetTypeName);
        }

        /* Initialize envelope points array */
        AukArray_New(&track->envelopePoints);
        if (track->envelopePoints) {
            AukArray_SetType(track->envelopePoints, AukEnvelopePoint_New, AukEnvelopePoint_GetTypeName);
        }
    }
}
