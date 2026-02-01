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

/* Structure for sound slide notification (used by UI to communicate slide events) */
typedef struct {
    int itrack;              /* Track index */
    AukSound *sound;         /* Sound being slid (not retained) */
    AukFixed newStartTime;   /* New start time for the sound */
    UBYTE isEnd;             /* 1 if slide ended (mouse up), 0 if sliding */
    UBYTE reserved[3];
} AukSoundSlideInfo;

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

    AukFixed16  stereoPan; /* 0 Right, 32768 middle 65536 Left */
    AukFixed16  ownVolume; /* 0 silent, 65536 Full. Multiply envelope signal */

    /* Number of audio channels this track manages (1=mono, 2=stereo, etc.)
        This is set at the first AddSound and shouldnt change (or heavy op.)
        If another sound copied, may need conversion. 1 by default.
    */
    unsigned int    channelCount;
    /* Sample rate set at the first AddSound, from the sound file.
       sounds may differ, but we keep the first one for display.
    */
    unsigned long   sampleRate;
    #define AukTrackFlag_Silent 1

    int         stateFlags;

    /* Selection state (not serialized) */
    #define AukTrackSelFlag_Selected 1

    int         selectionFlags;

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
void AukTrack_Delete(AukObject* This);
const char* AukTrack_GetTypeName(AukObject* This);

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

/* Find sound at a given time point. Returns sound if found (caller must release), NULL otherwise.
 * Also outputs the sound's index and the allowed slide range (min/max start times).
 */
AukSound* AukTrack_FindSoundAtTime(void* This, AukFixed time, unsigned int* outIndex,
                                   AukFixed* outMinSlide, AukFixed* outMaxSlide);

/* Slide a sound to a new start time, constrained by minTime and maxTime.
 * Returns 1 on success, 0 on failure. The sound's end time is adjusted to maintain duration.
 */
int AukTrack_SlideSound(void* This, AukSound* sound, AukFixed newStartTime,
                        AukFixed minTime, AukFixed maxTime);

/* Envelope management */
int AukTrack_AddEnvelopePoint(void* This, AukFixed time, unsigned short value);
int AukTrack_RemoveEnvelopePointAt(void* This, unsigned int index);
int AukTrack_GetEnvelopePointAt(void* This, unsigned int index, AukFixed* time, unsigned short* value);
unsigned int AukTrack_GetEnvelopePointCount(void* This);
AukFixed AukTrack_GetEnvelopeValue(void* This, AukFixed time);

/* accessors */
void AukTrack_SetOwnVolume(AukTrack* track,AukFixed16 v);
void AukTrack_SetStereoPan(AukTrack* track,AukFixed16 v);
AukFixed16 AukTrack_GetOwnVolume(AukTrack* track);
AukFixed16 AukTrack_GetStereoPan(AukTrack* track);

void AukTrack_SetChannelCount(AukTrack* track, int count);
int AukTrack_GetChannelCount(AukTrack* track);

void AukTrack_SetSampleRate(AukTrack* track, unsigned long rate);
unsigned long AukTrack_GetSampleRate(AukTrack* track);

/* These 2 are exclusives, bool is passed */
void AukTrack_SetSilent(AukTrack* track, int isSilent);
int AukTrack_isSilent(AukTrack* track);

/* Selection accessors (not serialized) */
void AukTrack_SetSelected(AukTrack* track, int isSelected);
int AukTrack_isSelected(AukTrack* track);

#ifdef __cplusplus
}
#endif

#endif /* AUKTRACK_H */
