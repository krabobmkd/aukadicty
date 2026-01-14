#ifndef AUKSOUNDFILE_H
#define AUKSOUNDFILE_H

/*
 * AukSoundFile - Represents a sound file on disk
 * Shared resource - multiple Sound objects can reference same file
 * Manages filename (relative to project path)
 */

#include "aukobject.h"

#ifdef __cplusplus
extern "C" {
#endif

/* AukSoundFile structure - inherits from AukObject */
struct AukSoundFile {
    AukObject base;          /* Must be first - inheritance */

    /* Data members */
    char* filename;          /* Relative path to sound file */
    unsigned long sampleRate; /* Sample rate (e.g., 44100) */
    unsigned long channels;   /* Number of channels (1=mono, 2=stereo) */
    unsigned long frameCount; /* Total number of sample frames */

    /* Virtual methods specific to AukSoundFile */
    int (*SetFilename)(void* This, const char* filename);
    const char* (*GetFilename)(void* This);
};

/* Constructor/Destructor */
void AukSoundFile_New(AukSoundFilePtr* firstPtr);
void AukSoundFile_Delete(void* This);
const char* AukSoundFile_GetTypeName(void* This);

/* Initialize AukSoundFile structure */
void AukSoundFile_Init(AukSoundFile* soundFile);

/* Methods */
int AukSoundFile_SetFilename(void* This, const char* filename);
const char* AukSoundFile_GetFilename(void* This);
void AukSoundFile_SetProperties(AukSoundFile* soundFile,
                                 unsigned long sampleRate,
                                 unsigned long channels,
                                 unsigned long frameCount);

#ifdef __cplusplus
}
#endif

#endif /* AUKSOUNDFILE_H */
