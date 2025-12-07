#include "aukproject.h"
#include "auktrack.h"
#include "auksound.h"
#include "auksoundfile.h"
#include "aukstring.h"
#include "aukjson.h"
#include "aukarray.h"
#include <proto/exec.h>
#include <string.h>
#include "serializer.h"

/*
 * AukProject implementation
 * Root object that manages the entire project graph
 */

#define INITIAL_TRACK_CAPACITY 8

void AukProject_New(AukProjectPtr *firstPtr) {
    if(!firstPtr) return;
    AukProject* project = (AukProject*)AllocVec(sizeof(AukProject), MEMF_CLEAR);
    if (project) {
        AukProject_Init(project);
        AukObjectPtr_Set((AukObjectPtr*)firstPtr, &project->base);
    }
}

void AukProject_Delete(void* This) {
    AukProject* project = (AukProject*)This;
    unsigned long i;

    if (project) {
        /* Free name and path */
        if (project->name) {
            AukString_Free(project->name);
        }
        if (project->path) {
            AukString_Free(project->path);
        }
        AukObjectPtr_Release((AukObjectPtr*)&project->prefs);
        AukObjectPtr_Release((AukObjectPtr*)&project->tracks);

        // /* Delete all tracks */
        // if (project->tracks.tracks) {
        //     for (i = 0; i < project->tracks.count; i++) {
        //         if (project->tracks.tracks[i]) {
        //             AukTrack_Delete(project->tracks.tracks[i]);
        //         }
        //     }
        //     FreeVec(project->tracks.tracks);
        // }

        /* Call base object delete (which will FreeVec) */
        AukObject_Delete(&project->base);
    }
}

const char* AukProject_GetTypeName(void* This) {
    (void)This;
    return "AukProject";
}

// tell what to be load and saved
void AukProject_Serialize(void* This, ISerializer* ser, const char* pName) {
    AukProject* project = (AukProject*)This;
    (void)pName;

    if (!project || !ser) {
        return;
    }

    /* Write version only when saving */
    if (IS_WRITING(ser)) {
        const char* version = "0.1";
        ser->t_string(ser, "version", &version);
    }

    /* Serialize name and path */
    ser->t_string_mutable(ser, "name", &project->name);
    ser->t_string_mutable(ser, "path", &project->path);

    /* Serialize preferences object */
    ser->t_object(ser, "prefs", &project->prefs);

    /* Serialize tracks array */
    ser->t_arrayobj(ser, "tracks", &project->tracks, AukTrack_New, AukTrack_GetTypeName);

}

int AukProject_SetName(void* This, const char* name) {
    AukProject* project = (AukProject*)This;
    int changed;

    if (!project || !name) {
        return 0;
    }

    /* Check if value actually changed */
    changed = (project->name == NULL || AukString_Compare(project->name, name) != 0);

    if (!changed) {
        return 1; /* No change, but success */
    }

    /* Free old name */
    if (project->name) {
        AukString_Free(project->name);
    }

    /* Duplicate new name */
    project->name = AukString_Duplicate(name);

    if (project->name) {
        /* Send update notification */
        project->base.SendUpdate(project,NULL);
    }

    return project->name != NULL;
}

const char* AukProject_GetName(void* This) {
    AukProject* project = (AukProject*)This;
    return project ? project->name : NULL;
}

int AukProject_SetPath(void* This, const char* path) {
    AukProject* project = (AukProject*)This;
    int changed;

    if (!project || !path) {
        return 0;
    }

    /* Check if value actually changed */
    changed = (project->path == NULL || AukString_Compare(project->path, path) != 0);

    if (!changed) {
        return 1; /* No change, but success */
    }

    /* Free old path */
    if (project->path) {
        AukString_Free(project->path);
    }

    /* Duplicate new path */
    project->path = AukString_Duplicate(path);

    if (project->path) {
        /* Send update notification */
        project->base.SendUpdate(project,NULL);
    }

    return project->path != NULL;
}

const char* AukProject_GetPath(void* This) {
    AukProject* project = (AukProject*)This;
    return project ? project->path : NULL;
}

void AukProject_SetPreferences(AukProject* project, unsigned int sampleRate, unsigned int maxTracks) {
    int changed;

    if (!project) return;

    if(!project->prefs)
    {
        AukProjectPrefs_New(&project->prefs);
        if(!project->prefs) return;
    }

    /* Get typed pointer to prefs */
    AukProjectPrefs* prefs = (AukProjectPrefs*)project->prefs;

    /* Check if values actually changed */
    changed = (prefs->sampleRate != sampleRate || prefs->maxTracks != maxTracks);

    if (changed) {
        prefs->sampleRate = sampleRate;
        prefs->maxTracks = maxTracks;

        /* Send update notification */
        project->base.SendUpdate(project,NULL);
    }

}


static int AukProject_AddTrack(void* This, AukTrack* track) {
    AukProject* project = (AukProject*)This;

    if (!project || !track) {
        return 0;
    }

    if (!project->tracks) {
        return 0; /* No tracks array */
    }

    /* Check max tracks limit */
    AukProjectPrefs* prefs = (AukProjectPrefs*)project->prefs;
    AukArray* tracksArray = (AukArray*)project->tracks;

    if (prefs && tracksArray->GetCount(tracksArray) >= prefs->maxTracks) {
        return 0; /* Maximum tracks reached */
    }

    /* Add track to array using AukArray */
    if (!tracksArray->Add(tracksArray, &track->base)) {
        return 0;
    }

    /* Set project reference in track */
    AukTrack_SetProject(track, project);

    /* Send update notification */
    project->base.SendUpdate(&project->base, NULL);

    return 1;
}


AukTrack* AukProject_CreateTrack(void* This) {
    AukProject* project = (AukProject*)This;
    AukTrackPtr trackPtr = NULL;
    AukTrack* track;

    if (!project) {
        return NULL;
    }

    /* Create new track */
    AukTrack_New(&trackPtr);
    track = trackPtr;
    if (!track) {
        return NULL;
    }

    /* Add to project - this retains the track */
    if (!AukProject_AddTrack(project, track)) {
        /* Failed to add - release our reference */
        AukObjectPtr_Release(&trackPtr);
        return NULL;
    }
    /* Release our local reference, now it is retained by the array. */
    AukObjectPtr_Release(&trackPtr);
    /* Return raw pointer - the project owns the reference, caller doesn't */
    return track;
}

int AukProject_RemoveTrack(void* This, AukTrack* track) {
    AukProject* project = (AukProject*)This;
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
        AukTrack_SetProject(track, NULL);

        /* Send update notification */
        project->base.SendUpdate(&project->base, NULL);

        return 1;
    }

    return 0;
}

void AukProject_GetTrack(void* This,AukTrack**ptr, unsigned int index) {
    AukProject* project = (AukProject*)This;
    AukArray* tracksArray;

    if(!ptr) return;
    AukObjectPtr_Release((AukObjectPtr*)ptr);

    if (!project || !project->tracks) {
        return;
    }

    tracksArray = (AukArray*)project->tracks;
    tracksArray->Get(tracksArray,ptr, index);
}

unsigned int AukProject_GetTrackCount(void* This) {
    AukProject* project = (AukProject*)This;
    AukArray* tracksArray;

    if (!project || !project->tracks) {
        return 0;
    }

    tracksArray = (AukArray*)project->tracks;
    return tracksArray->GetCount(tracksArray);
}

AukFixed AukProject_GetDuration(void* This) {
    AukProject* project = (AukProject*)This;
    unsigned int i, trackCount;
    unsigned int j, soundCount;
    AukTrack* track=NULL;
    AukFixed maxEndTime;
    AukFixed soundEndTime;

    if (!project) {
        return 0;
    }

    maxEndTime = 0;
    trackCount = project->GetTrackCount(project);

    /* Find latest end time across all tracks */
    for (i = 0; i < trackCount; i++) {
        project->GetTrack(project,&track, i);
        if (track) {
            soundCount = track->GetSoundCount(track);

            for (j = 0; j < soundCount; j++) {
                AukSound* sound=NULL;
                track->GetSound(track,&sound, j);
                if (sound) {
                    soundEndTime = sound->endTime;
                    if (soundEndTime > maxEndTime) {
                        maxEndTime = soundEndTime;
                    }
                }
            }
        }
    }
    AukObjectPtr_Release((AukObjectPtr*)&track);

    return maxEndTime;
}

int AukProject_Save(void* This, const char* filename) {
    AukProject* project = (AukProject*)This;
    return AukJson_SaveProject(project, filename);
}

int AukProject_Load(void* This, const char* filename) {
    /* Note: Load creates a new project, doesn't modify existing one */
    /* This method signature doesn't fit well with load pattern */
    /* Use AukJson_LoadProject() directly instead */
    (void)This;
    (void)filename;
    return 0;
}

void AukProject_Init(AukProject* project) {
    if (project) {
        /* Initialize base object */
        AukObject_Init(&project->base);

        /* Override virtual methods */
        project->base.New = AukProject_New;
        project->base.Delete = AukProject_Delete;
        project->base.GetTypeName = AukProject_GetTypeName;
        project->base.Serialize = AukProject_Serialize;

        /* Set AukProject specific methods */
        project->SetName = AukProject_SetName;
        project->GetName = AukProject_GetName;
        project->SetPath = AukProject_SetPath;
        project->GetPath = AukProject_GetPath;
        project->CreateTrack = AukProject_CreateTrack;
        project->RemoveTrack = AukProject_RemoveTrack;
        project->GetTrack = AukProject_GetTrack;
        project->GetTrackCount = AukProject_GetTrackCount;
        project->GetDuration = AukProject_GetDuration;
        project->Save = AukProject_Save;
        project->Load = AukProject_Load;

        /* Initialize data members */
        project->name = NULL;
        project->path = NULL;

        /* Initialize prefs as AukProjectPrefsPtr pointer */
        project->prefs = NULL;
        AukProjectPrefs_New(&project->prefs);

        /* Initialize tracks array using AukArray */
        project->tracks = NULL;
        AukArray_New(&project->tracks);
        // set the type managed by the array
        if(project->tracks) AukArray_SetType( project->tracks,AukTrack_New, AukTrack_GetTypeName);

    }
}

void AukProjectPrefs_Delete(void* This) {
    AukProjectPrefs* prefs = (AukProjectPrefs*)This;

    if (prefs) {
        /* Call base object delete */
        AukObject_Delete(&prefs->base);
    }
}

const char* AukProjectPrefs_GetTypeName(void* This) {
    (void)This;
    return "AukProjectPrefs";
}

void AukProjectPrefs_Serialize(void* This, ISerializer* ser, const char* pName) {
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
    if(!firstPtr) return;
    AukProjectPrefs* prefs = (AukProjectPrefs*)AllocVec(sizeof(AukProjectPrefs), MEMF_CLEAR);
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

