#include "auktrack.h"
#include "auksound.h"
#include "auksoundfile.h"
#include "aukstring.h"
#include "aukscalararray.h"
#include "serializer.h"
#include <proto/exec.h>
#include <string.h>

#ifdef Remove
#undef Remove
#endif

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

        /* Delete envelope scalar arrays */
        if (track->envelopeTime) {
            AukScalarArray_Delete(track->envelopeTime);
        }
        if (track->envelopeValue) {
            AukScalarArray_Delete(track->envelopeValue);
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
    (void)pName;

    if (!track || !ser) {
        return;
    }

    /* Serialize track name */
    ser->t_string_mutable(ser, "name", &track->name);

    /* Serialize sounds array */
    ser->t_arrayobj(ser, "sounds", &track->sounds, AukSound_New, AukSound_GetTypeName);

    /* Serialize envelope scalar arrays */
    ser->t_scalararray(ser, "envelopeTime", &track->envelopeTime);
    ser->t_scalararray(ser, "envelopeValue", &track->envelopeValue);
}

void AukTrack_SetProject(AukTrack* track, AukProject* project) {
    if (track) {
        track->project = project;
        track->base._project = project;

        /* Also set project on the sounds array */
        if (track->sounds) {
            track->sounds->base._project = project;
        }
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

    /* Set project context */
    sound->base._project = track->project;

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



int AukTrack_AddEnvelopePoint(void* This, AukFixed time, unsigned short value) {
    AukTrack* track = (AukTrack*)This;
    unsigned int currentCount;
    unsigned int newCount;
    unsigned int insertIndex = 0;
    unsigned int i;
    unsigned int shape[1];
    AukScalarArray* newTimeArray;
    AukScalarArray* newValueArray;
    long long* timeData;
    short* valueData;

    if (!track) {
        return 0;
    }

    /* Get current count */
    currentCount = track->envelopeTime ? track->envelopeTime->totalElements : 0;
    newCount = currentCount + 1;

    /* Find insertion position to keep sorted by time */
    if (track->envelopeTime) {
        timeData = (long long*)track->envelopeTime->data;
        for (i = 0; i < currentCount; i++) {
            if (timeData[i] > time) {
                break;
            }
            insertIndex = i + 1;
        }
    }

    /* Create new arrays with increased size */
    shape[0] = newCount;
    newTimeArray = AukScalarArray_New(AUK_SCALAR_LONGLONG, 1, shape);
    newValueArray = AukScalarArray_New(AUK_SCALAR_SHORT, 1, shape);

    if (!newTimeArray || !newValueArray) {
        if (newTimeArray) AukScalarArray_Delete(newTimeArray);
        if (newValueArray) AukScalarArray_Delete(newValueArray);
        return 0;
    }

    /* Copy existing data and insert new point */
    timeData = (long long*)newTimeArray->data;
    valueData = (short*)newValueArray->data;

    if (track->envelopeTime && track->envelopeValue) {
        long long* oldTimeData = (long long*)track->envelopeTime->data;
        short* oldValueData = (short*)track->envelopeValue->data;

        /* Copy elements before insertion point */
        for (i = 0; i < insertIndex; i++) {
            timeData[i] = oldTimeData[i];
            valueData[i] = oldValueData[i];
        }

        /* Insert new point */
        timeData[insertIndex] = time;
        valueData[insertIndex] = (short)value;

        /* Copy elements after insertion point */
        for (i = insertIndex; i < currentCount; i++) {
            timeData[i + 1] = oldTimeData[i];
            valueData[i + 1] = oldValueData[i];
        }
    } else {
        /* First point */
        timeData[0] = time;
        valueData[0] = (short)value;
    }

    /* Replace old arrays */
    if (track->envelopeTime) {
        AukScalarArray_Delete(track->envelopeTime);
    }
    if (track->envelopeValue) {
        AukScalarArray_Delete(track->envelopeValue);
    }

    track->envelopeTime = newTimeArray;
    track->envelopeValue = newValueArray;

    track->base.SendUpdate(&track->base, NULL);
    return 1;
}

int AukTrack_RemoveEnvelopePointAt(void* This, unsigned int index) {
    AukTrack* track = (AukTrack*)This;
    unsigned int currentCount;
    unsigned int newCount;
    unsigned int i;
    unsigned int shape[1];
    AukScalarArray* newTimeArray;
    AukScalarArray* newValueArray;
    long long* timeData;
    short* valueData;
    long long* oldTimeData;
    short* oldValueData;

    if (!track || !track->envelopeTime || !track->envelopeValue) {
        return 0;
    }

    currentCount = track->envelopeTime->totalElements;

    if (index >= currentCount) {
        return 0;
    }

    newCount = currentCount - 1;

    /* If removing last point, just delete arrays */
    if (newCount == 0) {
        AukScalarArray_Delete(track->envelopeTime);
        AukScalarArray_Delete(track->envelopeValue);
        track->envelopeTime = NULL;
        track->envelopeValue = NULL;
        track->base.SendUpdate(&track->base, NULL);
        return 1;
    }

    /* Create new arrays with decreased size */
    shape[0] = newCount;
    newTimeArray = AukScalarArray_New(AUK_SCALAR_LONGLONG, 1, shape);
    newValueArray = AukScalarArray_New(AUK_SCALAR_SHORT, 1, shape);

    if (!newTimeArray || !newValueArray) {
        if (newTimeArray) AukScalarArray_Delete(newTimeArray);
        if (newValueArray) AukScalarArray_Delete(newValueArray);
        return 0;
    }

    /* Copy data, skipping the removed index */
    timeData = (long long*)newTimeArray->data;
    valueData = (short*)newValueArray->data;
    oldTimeData = (long long*)track->envelopeTime->data;
    oldValueData = (short*)track->envelopeValue->data;

    for (i = 0; i < index; i++) {
        timeData[i] = oldTimeData[i];
        valueData[i] = oldValueData[i];
    }

    for (i = index + 1; i < currentCount; i++) {
        timeData[i - 1] = oldTimeData[i];
        valueData[i - 1] = oldValueData[i];
    }

    /* Replace old arrays */
    AukScalarArray_Delete(track->envelopeTime);
    AukScalarArray_Delete(track->envelopeValue);

    track->envelopeTime = newTimeArray;
    track->envelopeValue = newValueArray;

    track->base.SendUpdate(&track->base, NULL);
    return 1;
}

int AukTrack_GetEnvelopePointAt(void* This, unsigned int index, AukFixed* time, unsigned short* value) {
    AukTrack* track = (AukTrack*)This;
    long long* timeData;
    short* valueData;

    if (!track || !track->envelopeTime || !track->envelopeValue) {
        return 0;
    }

    if (index >= track->envelopeTime->totalElements) {
        return 0;
    }

    timeData = (long long*)track->envelopeTime->data;
    valueData = (short*)track->envelopeValue->data;

    if (time) {
        *time = (AukFixed)timeData[index];
    }

    if (value) {
        *value = (unsigned short)valueData[index];
    }

    return 1;
}

unsigned int AukTrack_GetEnvelopePointCount(void* This) {
    AukTrack* track = (AukTrack*)This;
    return (track && track->envelopeTime) ? track->envelopeTime->totalElements : 0;
}

AukFixed AukTrack_GetEnvelopeValue(void* This, AukFixed time) {
    AukTrack* track = (AukTrack*)This;
    long long* timeData;
    short* valueData;
    unsigned int count;
    unsigned int i;
    AukFixed t0, t1, v0, v1;
    AukFixed t, result;

    if (!track || !track->envelopeTime || !track->envelopeValue) {
        return AUK_FIXED_ONE; /* Default to full volume */
    }

    count = track->envelopeTime->totalElements;
    if (count == 0) {
        return AUK_FIXED_ONE;
    }

    timeData = (long long*)track->envelopeTime->data;
    valueData = (short*)track->envelopeValue->data;

    /* Before first point */
    if (time < timeData[0]) {
        /* Convert from 8-bit fixed point (0x0100 = 1.0) to AukFixed */
        return ((AukFixed)valueData[0]) << 24; /* 0x0100 << 24 = AUK_FIXED_ONE */
    }

    /* After last point */
    if (time >= timeData[count - 1]) {
        return ((AukFixed)valueData[count - 1]) << 24;
    }

    /* Find interval containing time */
    for (i = 0; i < count - 1; i++) {
        t0 = (AukFixed)timeData[i];
        t1 = (AukFixed)timeData[i + 1];

        if (time >= t0 && time <= t1) {
            /* Linear interpolation between points */
            v0 = ((AukFixed)valueData[i]) << 24;
            v1 = ((AukFixed)valueData[i + 1]) << 24;

            t = AukFixed_Div(AukFixed_Sub(time, t0), AukFixed_Sub(t1, t0));
            result = AukFixed_Lerp(v0, v1, t);
            return result;
        }
    }

    /* Shouldn't reach here, but return last point value */
    return ((AukFixed)valueData[count - 1]) << 24;
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
        track->AddEnvelopePoint = AukTrack_AddEnvelopePoint;
        track->RemoveEnvelopePointAt = AukTrack_RemoveEnvelopePointAt;
        track->GetEnvelopePointAt = AukTrack_GetEnvelopePointAt;
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

        /* Initialize envelope scalar arrays (start empty) */
        track->envelopeTime = NULL;
        track->envelopeValue = NULL;
    }
}
