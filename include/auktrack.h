#ifndef AUKTRACK_H
#define AUKTRACK_H

/*
 * AukTrack - Represents a track in the project
 * Contains a list of sounds with their timing
 * Has envelope for volume/pan control (B-spline interpolation)
 */

#include "aukobject.h"
#include "aukfixed.h"
#include "aukarray.h"
#include "aukscalararray.h"
#ifdef __cplusplus
extern "C" {
#endif

/* AukTrack structure - inherits from AukObject */
struct AukTrack {
    AukObject base;          /* Must be first - inheritance */

    /* Data members */
    char* name;              /* Track name */
    AukArray *sounds;        /* Array of sounds on this track */

    /* index index in project track list tracks. */
    int trackIndex;

    /* Envelope data - parallel arrays for efficient storage */
    AukScalarArray *envelopeTime;  /* Time points (8-byte AukFixed) */
    AukScalarArray *envelopeValue; /* Volume values (2-byte fixed point, 0x0100 = 1.0) */

    /* Virtual methods specific to AukTrack */
    AukSound* (*CreateSound)(void* This, AukSoundFilePtr soundFile, AukFixed startTime, AukFixed endTime);
    int (*RemoveSound)(void* This, AukSound* sound);
    int (*MoveSoundToTrack)(void* This, AukSound* sound, AukTrack* destTrack);
    void (*GetSound)(void* This, AukSound**ptr, unsigned int index);
    unsigned int (*GetSoundCount)(void* This);

    /* Envelope management methods */
    int (*AddEnvelopePoint)(void* This, AukFixed time, unsigned short value);
    int (*RemoveEnvelopePointAt)(void* This, unsigned int index);
    int (*GetEnvelopePointAt)(void* This, unsigned int index, AukFixed* time, unsigned short* value);
    unsigned int (*GetEnvelopePointCount)(void* This);
    AukFixed (*GetEnvelopeValue)(void* This, AukFixed time);
};


/* Constructor/Destructor */
void AukTrack_New(AukObjectPtr *firstPtr);
void AukTrack_Delete(void* This);
const char* AukTrack_GetTypeName(void* This);

/* Initialize AukTrack structure */
void AukTrack_Init(AukTrack* track);

/* Methods */
void AukTrack_SetProject(AukTrack* track, AukProject* project);
int AukTrack_SetName(AukTrack* track, const char* name);
const char* AukTrack_GetName(AukTrack* track);

/* Sound management */
AukSound* AukTrack_CreateSound(void* This, AukSoundFilePtr soundFile, AukFixed startTime, AukFixed endTime);
int AukTrack_AddSound(void* This, AukSound* sound);
int AukTrack_RemoveSound(void* This, AukSound* sound);
int AukTrack_MoveSound(void* This, AukSound* sound, AukFixed newStartTime);
int AukTrack_MoveSoundToTrack(void* This, AukSound* sound, AukTrack* destTrack);
void AukTrack_GetSound(void* This, AukSound** ptr, unsigned int index);
unsigned int AukTrack_GetSoundCount(void* This);

/* Envelope management */
int AukTrack_AddEnvelopePoint(void* This, AukFixed time, unsigned short value);
int AukTrack_RemoveEnvelopePointAt(void* This, unsigned int index);
int AukTrack_GetEnvelopePointAt(void* This, unsigned int index, AukFixed* time, unsigned short* value);
unsigned int AukTrack_GetEnvelopePointCount(void* This);
AukFixed AukTrack_GetEnvelopeValue(void* This, AukFixed time);

#ifdef __cplusplus
}
#endif

#endif /* AUKTRACK_H */
