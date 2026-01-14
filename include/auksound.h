#ifndef AUKSOUND_H
#define AUKSOUND_H

/*
 * AukSound - Represents a sound instance on a track
 * Abstract playable that references a shared AukSoundFile
 * Can represent a portion of the file with loop count
 */

#include "aukobject.h"
#include "aukfixed.h"

#ifdef __cplusplus
extern "C" {
#endif

/* AukSound structure - inherits from AukObject */
struct AukSound {
    AukObject base;          /* Must be first - inheritance */

    /* Data members */
    AukSoundFile *soundFile;   /* Shared pointer to AukSoundFile */

    AukFixed startTime;      /* Start time in project timeline (inclusive) */
    AukFixed endTime;        /* End time in project timeline (exclusive - first value NOT in sound) */

    unsigned long fileStartFrame; /* Start frame in source file */
    unsigned long fileEndFrame;   /* End frame in source file (0 = use all) */

    unsigned long loopCount;  /* Number of times to loop (0 = no loop) */

    /* Virtual methods specific to AukSound */
    void (*SetTimeRange)(void* This, AukFixed start, AukFixed end);
    void (*SetFileRange)(void* This, unsigned long startFrame, unsigned long endFrame);
    void (*SetLoopCount)(void* This, unsigned long count);
    AukFixed (*GetDuration)(void* This);
};

/* Constructor/Destructor */
void AukSound_New(AukSoundPtr* firstPtr);
void AukSound_Delete(void* This);
const char* AukSound_GetTypeName(void* This);

/* Initialize AukSound structure */
void AukSound_Init(AukSound* sound);

/* Methods */
void AukSound_SetSoundFile(AukSound* sound, AukSoundFile *soundFile);
void AukSound_SetTimeRange(void* This, AukFixed start, AukFixed end);
void AukSound_SetFileRange(void* This, unsigned long startFrame, unsigned long endFrame);
void AukSound_SetLoopCount(void* This, unsigned long count);
AukFixed AukSound_GetDuration(void* This);

#ifdef __cplusplus
}
#endif

#endif /* AUKSOUND_H */
