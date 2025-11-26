#ifndef AUKPROJECT_H
#define AUKPROJECT_H

/*
 * AukProject - Root object of the project graph
 * Manages tracks, preferences, and project metadata
 * Document layer - must not depend on GUI
 */

#include "aukobject.h"
#include "aukfixed.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct AukProject AukProject;
typedef struct AukTrack AukTrack;

/* Project preferences */
typedef struct AukProjectPrefs {
    unsigned long sampleRate;    /* Audio mixing rate (e.g., 44100) */
    unsigned long maxTracks;     /* Maximum number of tracks */
} AukProjectPrefs;

/* Dynamic array for tracks */
typedef struct AukTrackArray {
    AukTrack** tracks;       /* Array of track pointers */
    unsigned long count;     /* Current number of tracks */
    unsigned long capacity;  /* Allocated capacity */
} AukTrackArray;

/* AukProject structure - inherits from AukObject */
struct AukProject {
    AukObject base;          /* Must be first - inheritance */

    /* Data members */
    char* name;              /* Project name */
    char* path;              /* Project file path (directory) */
    AukProjectPrefs prefs;   /* Project preferences */
    AukTrackArray tracks;    /* Array of tracks */

    /* Virtual methods specific to AukProject */
    int (*SetName)(void* This, const char* name);
    const char* (*GetName)(void* This);
    int (*SetPath)(void* This, const char* path);
    const char* (*GetPath)(void* This);
    int (*AddTrack)(void* This, AukTrack* track);
    int (*RemoveTrack)(void* This, AukTrack* track);
    AukTrack* (*GetTrack)(void* This, unsigned long index);
    unsigned long (*GetTrackCount)(void* This);
    int (*Save)(void* This, const char* filename);
    int (*Load)(void* This, const char* filename);
};

/* Constructor/Destructor */
void* AukProject_New(void);
void AukProject_Delete(void* This);
const char* AukProject_GetTypeName(void* This);

/* Initialize AukProject structure */
void AukProject_Init(AukProject* project);

/* Methods */
int AukProject_SetName(void* This, const char* name);
const char* AukProject_GetName(void* This);
int AukProject_SetPath(void* This, const char* path);
const char* AukProject_GetPath(void* This);
void AukProject_SetPreferences(AukProject* project, unsigned long sampleRate, unsigned long maxTracks);
int AukProject_AddTrack(void* This, AukTrack* track);
int AukProject_RemoveTrack(void* This, AukTrack* track);
AukTrack* AukProject_GetTrack(void* This, unsigned long index);
unsigned long AukProject_GetTrackCount(void* This);
int AukProject_Save(void* This, const char* filename);
int AukProject_Load(void* This, const char* filename);

#ifdef __cplusplus
}
#endif

#endif /* AUKPROJECT_H */
