#ifndef AUKAPROJECT_H
#define AUKAPROJECT_H

/*
 * AukAProject - Audio Project Implementation
 * Concrete implementation of AukProject for audio mixing
 * Manages tracks, audio preferences, and audio-specific operations
 * Document layer - must not depend on GUI
 */

#include "aukproject.h"
#include "aukfixed.h"

#ifdef __cplusplus
extern "C" {
#endif


/* Audio project preferences */
struct AukProjectPrefs {
    AukObject base;          /* Must be first - inheritance */
    unsigned int sampleRate;    /* Audio mixing rate (e.g., 44100) */
    unsigned int maxTracks;     /* Maximum number of tracks */
};

void AukProjectPrefs_New(AukProjectPrefsPtr *firstPtr);
void AukProjectPrefs_Delete(AukObject* This);
const char* AukProjectPrefs_GetTypeName(AukObject* This);
void AukProjectPrefs_Init(AukProjectPrefs *prefs);

/* AukAProject structure - inherits from AukProject */
struct AukAProject {
    AukProject base;         /* Must be first - inheritance from abstract AukProject */

    /* Audio-specific data members */
    AukProjectPrefsPtr prefs;  /* Audio project preferences */
    AukArrayPtr tracks;        /* Array of audio tracks (AukArray) */

    /* Selection state (not serialized) */
    int hasSelection;          /* Boolean: 1 if time selection exists, 0 otherwise */
    AukFixed selectionStart;   /* Start time of selection */
    AukFixed selectionEnd;     /* End time of selection (exclusive) */

    /* Virtual methods specific to audio projects */
    AukTrack* (*CreateTrack)(void* This);
    int (*RemoveTrack)(void* This, AukTrack* track);
    /** uses aukArray->Get() with retained pointer, so need a pointer inited to NULL, and a call to AukObjectPtr_Release() before pointer dies. */
    void (*GetTrack)(void* This, AukTrack**ptr, unsigned int index);
    unsigned int (*GetTrackCount)(void* This);
    AukFixed (*GetDuration)(void* This);
};

/* Constructor/Destructor */
void AukAProject_New(AukObjectPtr *firstPtr);
const char* AukAProject_GetTypeName(AukObject* This);

/* Initialize AukAProject structure */
void AukAProject_Init(AukAProject* project);

/* Audio-specific methods */
void AukAProject_SetPreferences(AukAProject* project, unsigned int sampleRate, unsigned int maxTracks);
AukTrack* AukAProject_CreateTrack(void* This);
int AukAProject_RemoveTrack(void* This, AukTrack* track);
void AukAProject_GetTrack(void* This, AukTrack**ptr, unsigned int index);
unsigned int AukAProject_GetTrackCount(void* This);
AukFixed AukAProject_GetDuration(void* This);

/* Selection accessors (not serialized) */
void AukAProject_SetSelection(AukAProject* project, AukFixed start, AukFixed end);
void AukAProject_ClearSelection(AukAProject* project);
int AukAProject_HasSelection(AukAProject* project);
AukFixed AukAProject_GetSelectionStart(AukAProject* project);
AukFixed AukAProject_GetSelectionEnd(AukAProject* project);


/* Message type enumeration that are AukAProject specific  */
typedef enum {
    AUK_MSG_TRACKADDED = AUKPROJECT_MSG_STARTLOAD,
    AUK_MSG_TRACKMODIFIED_TIMECHANGE,
    AUK_MSG_TRACKMODIFIED_SOUNDADDED,
    AUK_MSG_TRACKMODIFIED_SOUNDREMOVED,
    AUK_MSG_TRACKMODIFIED_NAMECHANGE,
    AUK_MSG_TRACKMODIFIED_CHANGEVol,
    AUK_MSG_TRACKMODIFIED_CHANGEPan,
    AUK_MSG_TRACKMODIFIED_CHANGEFlags,
    AUK_MSG_TRACKMODIFIED_CHANGESelection,

    AUK_MSG_TRACKREMOVED,
    AUK_MSG_SELECTIONCHANGED,
} AukAProjectMessageType;

typedef struct AukMessage_AProject {
    ULONG type;                /* Discriminator - always access this first */
    int     _track_id;
    AukTrack    *_track;    /* weak reference of object currently modified */
    AukFixed    _timeStart; /* for AUK_MSG_TRACKMODIFIED_TIMECHANGE */
} AukMessage_AProject;


#ifdef __cplusplus
}
#endif

#endif /* AUKAPROJECT_H */
