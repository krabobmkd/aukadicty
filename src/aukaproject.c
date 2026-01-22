#include "aukaproject.h"
#include "auktrack.h"
#include "auksound.h"
#include "auksoundfile.h"
#include "aukstring.h"
#include "aukarray.h"
#include <proto/exec.h>
#include <string.h>
#include "serializer.h"

#include <stdio.h>
#ifdef Remove
#undef Remove
#endif

/*
 * AukAProject implementation
 * Audio project concrete implementation
 */

#define INITIAL_TRACK_CAPACITY 8

void AukAProject_New(AukObjectPtr *firstPtr) {
    AukAProject* project;
    if(!firstPtr) return;
    project = (AukAProject*)AllocVec(sizeof(AukAProject), MEMF_CLEAR);
    if (project) {
        AukAProject_Init(project);
        AukObjectPtr_Set(firstPtr, &project->base.base);
    }
}

void AukAProject_Delete(AukObject* This) {
    AukAProject* project = (AukAProject*)This;

    if (project) {
        /* Release audio-specific members */
        AukObjectPtr_Release((AukObjectPtr*)&project->prefs);
        AukObjectPtr_Release((AukObjectPtr*)&project->tracks);

        /* Call base project delete (which deletes name/path and calls AukObject_Delete) */
        AukProject_Delete(&project->base.base);
    }
}

const char* AukAProject_GetTypeName(AukObject* This) {
    (void)This;
    return "AukAProject";
}

static void reattributeTrackIndex(AukArray* tracksArray)
{
    int nbtracks;
    int i;

    aukMutex_lock(&tracksArray->mutex);
        nbtracks = tracksArray->count;
        for(i=0;i<nbtracks;i++)
        {
           AukTrack *track = (AukTrack *)tracksArray->items[i];
           if(track) track->trackIndex = i;
        }
    aukMutex_unlock(&tracksArray->mutex);
}


void AukAProject_Serialize(AukObject* This, ISerializer* ser, const char* pName) {
    AukAProject* project = (AukAProject*)This;
    (void)pName;

    if (!project || !ser) {
        return;
    }

    /* Write version only when saving */
    if (IS_WRITING(ser)) {
        const char* version = "0.1";
        ser->t_string(ser, "version", &version);
    }

    /* Serialize name and path from base */
    ser->t_string_mutable(ser, "name", &project->base.name);
    ser->t_string_mutable(ser, "path", &project->base.path);

    /* Serialize audio-specific preferences object */
    ser->t_object(ser, "prefs", ( AukObjectPtr* )&project->prefs);

    /* Serialize tracks array */
    ser->t_arrayobj(ser, "tracks", &project->tracks,(AukObjectNewFunc) AukTrack_New, AukTrack_GetTypeName(NULL));

    if(IS_READING(ser) && project->tracks)
    {
        reattributeTrackIndex((AukArray*) project->tracks);
    }
}

void AukAProject_SetPreferences(AukAProject* project, unsigned int sampleRate, unsigned int maxTracks) {
    int changed;
    AukProjectPrefs* prefs;
    AukMessage msg;

    if (!project) return;

    if(!project->prefs)
    {
        AukProjectPrefs_New(&project->prefs);
        if(!project->prefs) return;
    }

    /* Get typed pointer to prefs */
    prefs = (AukProjectPrefs*)project->prefs;

    /* Check if values actually changed */
    changed = (prefs->sampleRate != sampleRate || prefs->maxTracks != maxTracks);

    if (changed) {
        prefs->sampleRate = sampleRate;
        prefs->maxTracks = maxTracks;

        /* Send update notification */
        msg.type = AUK_MSG_MODIFY;
        project->base.base.SendUpdate(&project->base.base, &msg);
    }
}

static int AukAProject_AddTrack(void* This, AukTrack* track) {
    AukAProject* project = (AukAProject*)This;
    int nbtracks;
    AukProjectPrefs* prefs;
    AukArray* tracksArray;
    AukMessage_AProject msg;

    if (!project || !track) {
        return 0;
    }

    if (!project->tracks) {
        return 0; /* No tracks array */
    }

    /* Check max tracks limit */
    prefs = (AukProjectPrefs*)project->prefs;
    tracksArray = (AukArray*)project->tracks;

    nbtracks = tracksArray->GetCount(tracksArray);

    if (prefs && nbtracks >= prefs->maxTracks) {
        return 0; /* Maximum tracks reached */
    }

    track->trackIndex = nbtracks;

    /* Add track to array using AukArray */

    if (!tracksArray->Add(tracksArray, &track->base)) {
        return 0;
    }

    /* Set project reference in track */
    AukTrack_SetProject(track, (AukProject*)project);

    /* Send update notification */
    msg.type = AUK_MSG_TRACKADDED;
    msg._track = track;
    msg._track_id = tracksArray->GetCount(tracksArray) -1;
    msg._timeStart = 0;
    project->base.base.SendUpdate(&project->base.base,(AukMessage*) &msg);

    return 1;
}

AukTrack* AukAProject_CreateTrack(void* This) {
    AukAProject* project = (AukAProject*)This;
    AukTrackPtr trackPtr = NULL;
    AukTrack* track;

    if (!project) {
        return NULL;
    }

    /* Create new track */
    AukTrack_New((AukObjectPtr*)&trackPtr);
    track = trackPtr;
    if (!track) {
        return NULL;
    }

    /* Add to project - this retains the track */
    if (!AukAProject_AddTrack(project, track)) {
        /* Failed to add - release our reference */
        AukObjectPtr_Release((AukObjectPtr*)&trackPtr);
        return NULL;
    }
    /* Release our local reference, now it is retained by the array. */
    AukObjectPtr_Release((AukObjectPtr*)&trackPtr);
    /* Return raw pointer - the project owns the reference, caller doesn't */
    return track;
}

int AukAProject_RemoveTrack(void* This, AukTrack* track) {
    AukAProject* project = (AukAProject*)This;
    AukArray* tracksArray;

    if (!project || !track) {
        return 0;
    }

    if (!project->tracks) {
        return 0; /* No tracks array */
    }

    tracksArray = (AukArray*)project->tracks;

    /* Remove track using AukArray */
    if (tracksArray->Remove(tracksArray, &track->base)) {

        reattributeTrackIndex(tracksArray);

        /* Send update notification */
        {
            AukMessage_AProject msg;
            msg.type = AUK_MSG_TRACKREMOVED;
            msg._track = track;
            msg._track_id =0 ; // tracksArray->GetCount(tracksArray) -1;
            msg._timeStart = 0;
            project->base.base.SendUpdate(&project->base.base,(AukMessage*) &msg);
        }

        return 1;
    }

    return 0;
}

void AukAProject_GetTrack(void* This, AukTrack**ptr, unsigned int index) {
    AukAProject* project = (AukAProject*)This;
    AukArray* tracksArray;

    if(!ptr) return;
    AukObjectPtr_Release((AukObjectPtr*)ptr);

    if (!project || !project->tracks) {
        return;
    }

    tracksArray = (AukArray*)project->tracks;

    tracksArray->Get(tracksArray, (AukObjectPtr*)ptr, index);

}

unsigned int AukAProject_GetTrackCount(void* This) {
    AukAProject* project = (AukAProject*)This;
    AukArray* tracksArray;

    if (!project || !project->tracks) {
        return 0;
    }

    tracksArray = (AukArray*)project->tracks;
    return tracksArray->GetCount(tracksArray);
}

AukFixed AukAProject_GetDuration(void* This) {
    AukAProject* project = (AukAProject*)This;
    unsigned int i, trackCount;
    unsigned int j, soundCount;
    AukTrack* track=NULL;
    AukSound* sound=NULL;
    AukFixed maxEndTime;
    AukFixed soundEndTime;

    if (!project) {
        return 0;
    }

    maxEndTime = 0;
    trackCount = project->GetTrackCount(project);

    /* Find latest end time across all tracks */
    for (i = 0; i < trackCount; i++) {
        project->GetTrack(project, &track, i);
        if (track) {
            soundCount = track->GetSoundCount(track);

            for (j = 0; j < soundCount; j++) {
                sound = NULL;
                track->GetSound(track, &sound, j);
                if (sound) {
                    soundEndTime = sound->endTime;
                    if (soundEndTime > maxEndTime) {
                        maxEndTime = soundEndTime;
                    }
                    AukObjectPtr_Release((AukObjectPtr*)&sound);
                }
            }
        }
    }
    AukObjectPtr_Release((AukObjectPtr*)&track);

    return maxEndTime;
}

void AukAProject_Init(AukAProject* project) {
    if (project) {
        /* Initialize base project */
        AukProject_Init(&project->base);

        /* Override virtual methods */
        project->base.base.New = (void (*)(AukObjectPtr*))AukAProject_New;
        project->base.base.Delete = AukAProject_Delete;
        project->base.base.GetTypeName = AukAProject_GetTypeName;
        project->base.base.Serialize = AukAProject_Serialize;

        /* Set AukAProject specific methods */
        project->CreateTrack = AukAProject_CreateTrack;
        project->RemoveTrack = AukAProject_RemoveTrack;
        project->GetTrack = AukAProject_GetTrack;
        project->GetTrackCount = AukAProject_GetTrackCount;
        project->GetDuration = AukAProject_GetDuration;

        /* Initialize audio-specific data members */
        project->prefs = NULL;
        AukProjectPrefs_New(&project->prefs);
        if (project->prefs) {
            project->prefs->base._project = (AukProject*)project;
        }

        /* Initialize tracks array using AukArray */
        project->tracks = NULL;
        AukArray_New((AukObjectPtr*)&project->tracks);
        if(project->tracks) {
            AukArray_SetType(project->tracks, AukTrack_New, AukTrack_GetTypeName(NULL));
            project->tracks->base._project = (AukProject*)project;
        }

        /* Initialize selection state (not serialized) */
        project->hasSelection = 0;
        project->selectionStart = 0;
        project->selectionEnd = 0;

        project->soloTrack = -1;
    }
}

/* AukProjectPrefs implementation */

void AukProjectPrefs_Delete(AukObject* This) {
    AukProjectPrefs* prefs = (AukProjectPrefs*)This;

    if (prefs) {
        /* Call base object delete */
        AukObject_Delete(&prefs->base);
    }
}

const char* AukProjectPrefs_GetTypeName(AukObject* This) {
    (void)This;
    return "AukProjectPrefs";
}

void AukProjectPrefs_Serialize(AukObject* This, ISerializer* ser, const char* pName) {
    AukProjectPrefs* prefs = (AukProjectPrefs*)This;
    (void)pName;

    if (!prefs || !ser) {
        return;
    }

    ser->t_uint(ser, "sampleRate", &prefs->sampleRate);
    ser->t_uint(ser, "maxTracks", &prefs->maxTracks);
}

void AukProjectPrefs_New(AukProjectPrefsPtr *firstPtr)
{
    AukProjectPrefs* prefs;
    if(!firstPtr) return;
    prefs = (AukProjectPrefs*)AllocVec(sizeof(AukProjectPrefs), MEMF_CLEAR);
    if (prefs) {
        AukProjectPrefs_Init(prefs);
        AukObjectPtr_Set((AukObjectPtr*)firstPtr, &prefs->base);
    }
}

void AukProjectPrefs_Init(AukProjectPrefs *prefs)
{
    if(!prefs) return;
    AukObject_Init(&prefs->base);

    /* Override virtual methods */
    prefs->base.Delete = AukProjectPrefs_Delete;
    prefs->base.GetTypeName = AukProjectPrefs_GetTypeName;
    prefs->base.Serialize = AukProjectPrefs_Serialize;

    /* Initialize with defaults */
    prefs->sampleRate = 44100;
    prefs->maxTracks = 64;
}

/* Set project context on all objects after deserialization */
void AukProject_SetProjectContext(AukProject* project) {
    AukAProject* aproject;
    unsigned int i, trackCount;
    unsigned int j, soundCount;
    AukTrack* track = NULL;
    AukSound* sound = NULL;
    AukArray* soundsArray;

    if (!project) {
        return;
    }

    /* Check if this is an AukAProject (audio project) */
    if (AukString_Compare(project->base.GetTypeName(&project->base), "AukAProject") != 0) {
        return; /* Not an audio project, nothing to do */
    }

    aproject = (AukAProject*)project;

    /* Set project context on prefs */
    if (aproject->prefs) {
        aproject->prefs->base._project = project;
    }

    /* Set project context on tracks array */
    if (aproject->tracks) {
        aproject->tracks->base._project = project;

        /* Set project context on each track and its contents */
        trackCount = aproject->GetTrackCount(aproject);
        for (i = 0; i < trackCount; i++) {
            aproject->GetTrack(aproject, &track, i);
            if (track) {
                /* Use AukTrack_SetProject which sets both track->project and track->base._project */
                AukTrack_SetProject(track, project);

                /* Set project context on sounds array */
                soundsArray = (AukArray*)track->sounds;
                if (soundsArray) {
                    soundsArray->base._project = project;

                    /* Set project context on each sound and its soundFile */
                    soundCount = soundsArray->GetCount(soundsArray);
                    for (j = 0; j < soundCount; j++) {
                        soundsArray->Get(soundsArray, (AukObjectPtr*)&sound, j);
                        if (sound) {
                            sound->base._project = project;

                            /* Set project context on the soundFile */
                            if (sound->soundFile) {
                                sound->soundFile->base._project = project;
                            }

                            AukObjectPtr_Release((AukObjectPtr*)&sound);
                        }
                    }
                }

                AukObjectPtr_Release((AukObjectPtr*)&track);
            }
        }
    }
}

/* Selection accessors (not serialized) */

void AukAProject_SetSelection(AukAProject* project, AukFixed start, AukFixed end)
{
    AukMessage_AProject msg;

    if (!project) return;

    /* Check if values actually changed */
    if (project->hasSelection &&
        project->selectionStart == start &&
        project->selectionEnd == end) {
        return; /* No change */
    }

    project->hasSelection = 1;
    project->selectionStart = start;
    project->selectionEnd = end;

    /* Send update notification */
    msg.type = AUK_MSG_SELECTIONCHANGED;
    msg._track_id = -1;
    msg._track = NULL;
    msg._timeStart = start;
    project->base.base.SendUpdate(&project->base.base, (AukMessage*)&msg);
}

void AukAProject_ClearSelection(AukAProject* project)
{
    AukMessage_AProject msg;

    if (!project) return;

    /* Check if already cleared */
    if (!project->hasSelection) {
        return; /* No change */
    }

    project->hasSelection = 0;
    project->selectionStart = 0;
    project->selectionEnd = 0;

    /* Send update notification */
    msg.type = AUK_MSG_SELECTIONCHANGED;
    msg._track_id = -1;
    msg._track = NULL;
    msg._timeStart = 0;
    project->base.base.SendUpdate(&project->base.base, (AukMessage*)&msg);
}

int AukAProject_HasSelection(AukAProject* project)
{
    if (!project) return 0;
    return project->hasSelection;
}

AukFixed AukAProject_GetSelectionStart(AukAProject* project)
{
    if (!project) return 0;
    return project->selectionStart;
}

AukFixed AukAProject_GetSelectionEnd(AukAProject* project)
{
    if (!project) return 0;
    return project->selectionEnd;
}

/* -1 means no solo, else track id */
void AukAProject_SetSoloTrack(AukAProject* project, int soloTrackId)
{

    AukMessage_AProject msg;
    if (!project || !project->tracks) return;
    if(project->soloTrack == soloTrackId) return;
    if(soloTrackId<-1 || soloTrackId>=(int)project->tracks->count) return;
     project->soloTrack = soloTrackId;

    /* Send update notification */
    msg.type = AUK_MSG_TRACKMODIFIED_CHANGESoloTrack;
    msg._track_id = soloTrackId;
    msg._track = NULL;
    msg._timeStart = 0;
    if(project->tracks && soloTrackId>-1)
    {
         msg._track = (AukTrack *)project->tracks->items[soloTrackId];
    }

    project->base.base.SendUpdate(&project->base.base, (AukMessage*)&msg);

}
int AukAProject_SoloTrack(AukAProject* project)
{
    if (!project) return -1;
    return project->soloTrack;
}
