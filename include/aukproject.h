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
typedef AukProject* AukProjectPtr;

struct sAukTrack;
typedef struct sAukTrack AukTrack;
typedef AukTrack* AukTrackPtr;

typedef struct AukArray AukArray;
typedef AukArray* AukArrayPtr;

/* Project preferences */
typedef struct AukProjectPrefs {
    AukObject base;          /* Must be first - inheritance */
    unsigned int sampleRate;    /* Audio mixing rate (e.g., 44100) */
    unsigned int maxTracks;     /* Maximum number of tracks */
} AukProjectPrefs;

typedef AukProjectPrefs* AukProjectPrefsPtr;


void AukProjectPrefs_New(AukProjectPrefsPtr *firstPtr);
void AukProjectPrefs_Delete(void* This);
const char* AukProjectPrefs_GetTypeName(void* This);
void AukProjectPrefs_Init(AukProjectPrefs *prefs);

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
    AukProjectPrefsPtr prefs;  /* Project preferences (AukProjectPrefs) */
    AukArrayPtr tracks;        /* Array of tracks (AukArray) */

    /* Virtual methods specific to AukProject */
    int (*SetName)(void* This, const char* name);
    const char* (*GetName)(void* This);
    int (*SetPath)(void* This, const char* path);
    const char* (*GetPath)(void* This);
    AukTrack* (*CreateTrack)(void* This);
    int (*RemoveTrack)(void* This, AukTrack* track);
    /** uses aukArray->Get() with retained pointer, so need a pointer inited to NULL, and a call to AukObjectPtr_Release() before pointer dies. */
    void (*GetTrack)(void* This,AukTrack**ptr, unsigned int index);
    unsigned long (*GetTrackCount)(void* This);
    AukFixed (*GetDuration)(void* This);
    int (*Save)(void* This, const char* filename);
    int (*Load)(void* This, const char* filename);
};

/* Constructor/Destructor */
void AukProject_New(AukProjectPtr *firstPtr);
//private void AukProject_Delete(void* This);
const char* AukProject_GetTypeName(void* This);

/* Initialize AukProject structure */
void AukProject_Init(AukProject* project);

/* Methods */
int AukProject_SetName(void* This, const char* name);
const char* AukProject_GetName(void* This);
int AukProject_SetPath(void* This, const char* path);
const char* AukProject_GetPath(void* This);
void AukProject_SetPreferences(AukProject* project, unsigned int sampleRate, unsigned int maxTracks);
AukTrack* AukProject_CreateTrack(void* This);
int AukProject_RemoveTrack(void* This, AukTrack* track);
void AukProject_GetTrack(void* This,AukTrack**ptr, unsigned int index) ;
unsigned int AukProject_GetTrackCount(void* This);
AukFixed AukProject_GetDuration(void* This);
int AukProject_Save(void* This, const char* filename);
int AukProject_Load(void* This, const char* filename);

#ifdef __cplusplus
}
#endif

#endif /* AUKPROJECT_H */
