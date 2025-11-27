#ifndef AUKTRACK_H
#define AUKTRACK_H

/*
 * AukTrack - Represents a track in the project
 * Contains a list of sounds with their timing
 * Has envelope for volume/pan control (B-spline interpolation)
 */

#include "aukobject.h"
#include "aukfixed.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct sAukTrack AukTrack;
typedef struct AukSound AukSound;
typedef struct AukProject AukProject;
typedef struct AukEnvelopePoint AukEnvelopePoint;

/* Envelope point for B-spline interpolation */
struct AukEnvelopePoint {
    AukFixed time;         /* Time position */
    AukFixed value;        /* Value at this point (0.0 to 1.0 in fixed-point) */
    AukEnvelopePoint* next; /* Next point in linked list */
};

/* Dynamic array for sounds */
typedef struct AukSoundArray {
    AukSound** sounds;      /* Array of sound pointers */
    unsigned long count;    /* Current number of sounds */
    unsigned long capacity; /* Allocated capacity */
} AukSoundArray;

/* AukTrack structure - inherits from AukObject */
struct sAukTrack {
    AukObject base;          /* Must be first - inheritance */

    /* Data members */
    AukProject* project;     /* Pointer to parent project (weak reference) */
    char* name;              /* Track name */
    AukSoundArray sounds;    /* Array of sounds on this track */
    AukEnvelopePoint* envelope; /* Envelope points (linked list) */

    /* Virtual methods specific to AukTrack */
    int (*AddSound)(void* This, AukSound* sound);
    int (*RemoveSound)(void* This, AukSound* sound);
    AukSound* (*GetSound)(void* This, unsigned long index);
    unsigned long (*GetSoundCount)(void* This);
    int (*AddEnvelopePoint)(void* This, AukFixed time, AukFixed value);
    AukFixed (*GetEnvelopeValue)(void* This, AukFixed time);
};
typedef struct sAukTrack AukTrack;
/* Constructor/Destructor */
void* AukTrack_New(void);
void AukTrack_Delete(void* This);
const char* AukTrack_GetTypeName(void* This);

/* Initialize AukTrack structure */
void AukTrack_Init(AukTrack* track);

/* Methods */
void AukTrack_SetProject(AukTrack* track, AukProject* project);
int AukTrack_SetName(AukTrack* track, const char* name);
const char* AukTrack_GetName(AukTrack* track);
int AukTrack_AddSound(void* This, AukSound* sound);
int AukTrack_RemoveSound(void* This, AukSound* sound);
AukSound* AukTrack_GetSound(void* This, unsigned long index);
unsigned long AukTrack_GetSoundCount(void* This);
int AukTrack_AddEnvelopePoint(void* This, AukFixed time, AukFixed value);
AukFixed AukTrack_GetEnvelopeValue(void* This, AukFixed time);

#ifdef __cplusplus
}
#endif

#endif /* AUKTRACK_H */
