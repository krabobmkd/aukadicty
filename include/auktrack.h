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
#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct sAukTrack AukTrack;
typedef struct AukSound AukSound;
typedef struct AukProject AukProject;
typedef struct AukEnvelopePoint AukEnvelopePoint;

typedef struct AukSoundFile AukSoundFile;
typedef AukSoundFile* AukSoundFilePtr;

/* Dynamic array for sounds */
// typedef struct AukSoundArray {
//     AukSound** sounds;      /* Array of sound pointers */
//     unsigned int count;     /* Current number of sounds */
//     unsigned int capacity;  /* Allocated capacity */
// } AukSoundArray;

/* AukTrack structure - inherits from AukObject */
struct sAukTrack {
    AukObject base;          /* Must be first - inheritance */

    /* Data members */
    AukProject* project;     /* Pointer to parent project (weak reference) */
    char* name;              /* Track name */
    AukArray *sounds;        /* Array of sounds on this track */
    AukArray *envelopePoints; /* Array of envelope points */

    /* Virtual methods specific to AukTrack */
    AukSound* (*CreateSound)(void* This, AukSoundFilePtr soundFile, AukFixed startTime, AukFixed endTime);
    int (*RemoveSound)(void* This, AukSound* sound);
    int (*MoveSoundToTrack)(void* This, AukSound* sound, AukTrack* destTrack);
    void (*GetSound)(void* This, AukSound**ptr, unsigned int index);
    unsigned int (*GetSoundCount)(void* This);

    /* Envelope management methods - matching sound management pattern */
    AukEnvelopePoint* (*CreateEnvelopePoint)(void* This, AukFixed time, AukFixed value);
    int (*RemoveEnvelopePoint)(void* This, AukEnvelopePoint* point);
    void (*GetEnvelopePoint)(void* This, AukEnvelopePoint** ptr, unsigned int index);
    unsigned int (*GetEnvelopePointCount)(void* This);
    AukFixed (*GetEnvelopeValue)(void* This, AukFixed time);
};
typedef struct sAukTrack AukTrack;
typedef AukTrack* AukTrackPtr;

/* Constructor/Destructor */
void AukTrack_New(AukTrackPtr *firstPtr);
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
int AukTrack_MoveSoundToTrack(void* This, AukSound* sound, AukTrack* destTrack);
void AukTrack_GetSound(void* This, AukSound** ptr, unsigned int index);
unsigned int AukTrack_GetSoundCount(void* This);

/* Envelope management */
AukEnvelopePoint* AukTrack_CreateEnvelopePoint(void* This, AukFixed time, AukFixed value);
int AukTrack_RemoveEnvelopePoint(void* This, AukEnvelopePoint* point);
void AukTrack_GetEnvelopePoint(void* This, AukEnvelopePoint** ptr, unsigned int index);
unsigned int AukTrack_GetEnvelopePointCount(void* This);
AukFixed AukTrack_GetEnvelopeValue(void* This, AukFixed time);

#ifdef __cplusplus
}
#endif

#endif /* AUKTRACK_H */
