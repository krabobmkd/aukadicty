#include "auktrack.h"
#include "auksound.h"
#include "auksoundfile.h"
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
    AukEnvelopePoint* point;
    AukEnvelopePoint* nextPoint;
    unsigned int i;

    if (track) {
        /* Free name */
        if (track->name) {
            AukString_Free(track->name);
        }

        /* Release all sounds using reference counting */
        AukObjectPtr_Release((AukObjectPtr*)&track->sounds);

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

void AukTrack_Serialize(void* This, ISerializer* ser, const char* pName) {
    AukTrack* track = (AukTrack*)This;
    unsigned int i;
    (void)pName;

    if (!track || !ser) {
        return;
    }

    /* Serialize track name */
    ser->t_string_mutable(ser, "name", &track->name);

    ser->t_arrayobj(ser, "sounds", &track->sounds,AukSound_New,AukSound_GetTypeName);

    /* Serialize envelope points */
    if (IS_WRITING(ser)) {
        /* Count envelope points */
        AukEnvelopePoint* point = track->envelope;
        unsigned int envelopeCount = 0;
        while (point) {
            envelopeCount++;
            point = point->next;
        }

        ser->t_uint(ser, "envelopeCount", &envelopeCount);

        /* Write each envelope point */
        point = track->envelope;
        while (point) {
            ser->t_fixed(ser, "envelopeTime", &point->time);
            ser->t_fixed(ser, "envelopeValue", &point->value);
            point = point->next;
        }
    } else {
        /* Load envelope points */
        unsigned int envelopeCount = 0;
        ser->t_uint(ser, "envelopeCount", &envelopeCount);

        for (i = 0; i < envelopeCount; i++) {
            AukFixed time, value;
            ser->t_fixed(ser, "envelopeTime", &time);
            ser->t_fixed(ser, "envelopeValue", &value);
            track->AddEnvelopePoint(track, time, value);
        }
    }
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
        track->base.Serialize = AukTrack_Serialize;

        /* Set AukTrack specific methods */
        track->CreateSound = AukTrack_CreateSound;
        track->RemoveSound = AukTrack_RemoveSound;
        track->MoveSoundToTrack = AukTrack_MoveSoundToTrack;
        track->GetSound = AukTrack_GetSound;
        track->GetSoundCount = AukTrack_GetSoundCount;
        track->AddEnvelopePoint = AukTrack_AddEnvelopePoint;
        track->GetEnvelopeValue = AukTrack_GetEnvelopeValue;

        /* Initialize data members */
        track->project = NULL;
        track->name = NULL;

        AukArray_New(&track->sounds);
        // set the type managed by the array
        if(track->sounds) AukArray_SetType(track->sounds,AukSound_New, AukSound_GetTypeName);

        track->envelope = NULL;


    }
}
