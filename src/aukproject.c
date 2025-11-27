#include "aukproject.h"
#include "auktrack.h"
#include "aukstring.h"
#include "aukjson.h"
#include <proto/exec.h>
#include <string.h>
#include "serializer.h"

/*
 * AukProject implementation
 * Root object that manages the entire project graph
 */

#define INITIAL_TRACK_CAPACITY 8

void AukProject_New(AukShared *firstPtr) {
    if(!firstPtr) return;
    AukProject* project = (AukProject*)AllocVec(sizeof(AukProject), MEMF_CLEAR);
    if (project) {    
        AukProject_Init(project);
        AukShared_Set(firstPtr,&project->base);
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
        AukShared_Release(&project->prefs);
        AukShared_Release(&project->tracks);

        // /* Delete all tracks */
        // if (project->tracks.tracks) {
        //     for (i = 0; i < project->tracks.count; i++) {
        //         if (project->tracks.tracks[i]) {
        //             AukTrack_Delete(project->tracks.tracks[i]);
        //         }
        //     }
        //     FreeVec(project->tracks.tracks);
        // }

        /* Free the object itself */
        FreeVec(project);
    }
}

const char* AukProject_GetTypeName(void* This) {
    (void)This;
    return "AukProject";
}

// tell what to be load and saved
void AukProject_Serialize(void* This,ISerializer *ser,const char *pName)
{
    AukProject* project = (AukProject*)This;
    // ISerializer
    // force writting values when
    if(!ser->_isReading)
    {
        const char *version="0.1";
        ser->t_string(ser,"version",&version);
    }
    ser->t_object(ser,"prefs",&project->prefs);
    ser->t_arrayobj(ser,"prefs",&project->tracks);

    // ser->t_int(ser,"sampleRate",&project->prefs.sampleRate);
    // ser->t_int(ser,"maxTracks",&project->prefs.maxTracks);



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

void AukProject_SetPreferences(AukProject* project, unsigned long sampleRate, unsigned long maxTracks) {
    int changed;

    if (!project) return;

    if(!project->prefs)
    {
        AukProjectPrefs_New(&project->prefs);
        if(!project->prefs) return;
    }

    /* Check if values actually changed */
    changed = (project->prefs->sampleRate != sampleRate || project->prefs->maxTracks != maxTracks);

    if (changed) {
        project->prefs->sampleRate = sampleRate;
        project->prefs->maxTracks = maxTracks;

        /* Send update notification */
        project->base.SendUpdate(project,NULL);
    }

}

int AukProject_AddTrack(void* This, AukTrack* track) {
    AukProject* project = (AukProject*)This;
    AukTrack** newTracks;
    unsigned long newCapacity;

    if (!project || !track) {
        return 0;
    }

    /* Check max tracks limit */
    if (project->tracks.count >= project->prefs.maxTracks) {
        return 0; /* Maximum tracks reached */
    }

    /* Grow array if needed */
    if (project->tracks.count >= project->tracks.capacity) {
        newCapacity = project->tracks.capacity == 0 ? INITIAL_TRACK_CAPACITY : project->tracks.capacity * 2;

        /* Don't exceed max tracks */
        if (newCapacity > project->prefs.maxTracks) {
            newCapacity = project->prefs.maxTracks;
        }

        newTracks = (AukTrack**)AllocVec(newCapacity * sizeof(AukTrack*), MEMF_CLEAR);
        if (!newTracks) {
            return 0;
        }

        /* Copy existing tracks */
        if (project->tracks.tracks) {
            memcpy(newTracks, project->tracks.tracks, project->tracks.count * sizeof(AukTrack*));
            FreeVec(project->tracks.tracks);
        }

        project->tracks.tracks = newTracks;
        project->tracks.capacity = newCapacity;
    }

    /* Add track to array */
    project->tracks.tracks[project->tracks.count] = track;
    project->tracks.count++;

    /* Set project reference in track */
    AukTrack_SetProject(track, project);

    /* Send update notification */
    project->base.SendUpdate(project);

    return 1;
}

int AukProject_RemoveTrack(void* This, AukTrack* track) {
    AukProject* project = (AukProject*)This;
    unsigned long i;

    if (!project || !track) {
        return 0;
    }

    /* Find and remove track */
    for (i = 0; i < project->tracks.count; i++) {
        if (project->tracks.tracks[i] == track) {
            /* Shift remaining tracks down */
            if (i < project->tracks.count - 1) {
                memcpy(&project->tracks.tracks[i],
                       &project->tracks.tracks[i + 1],
                       (project->tracks.count - i - 1) * sizeof(AukTrack*));
            }

            project->tracks.count--;
            AukTrack_SetProject(track, NULL);

            /* Send update notification */
            project->base.SendUpdate(project);

            return 1;
        }
    }

    return 0;
}

AukTrack* AukProject_GetTrack(void* This, unsigned long index) {
    AukProject* project = (AukProject*)This;

    if (!project || index >= project->tracks.count) {
        return NULL;
    }

    return project->tracks.tracks[index];
}

unsigned long AukProject_GetTrackCount(void* This) {
    AukProject* project = (AukProject*)This;
    return project ? project->tracks.count : 0;
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
        project->AddTrack = AukProject_AddTrack;
        project->RemoveTrack = AukProject_RemoveTrack;
        project->GetTrack = AukProject_GetTrack;
        project->GetTrackCount = AukProject_GetTrackCount;
        project->Save = AukProject_Save;
        project->Load = AukProject_Load;

        /* Initialize data members */
        project->name = NULL;
        project->path = NULL;
        project->prefs.sampleRate = 44100;  /* Default */
        project->prefs.maxTracks = 64;      /* Default */
        project->tracks.tracks = NULL;
        project->tracks.count = 0;
        project->tracks.capacity = 0;
    }
}

void AukProjectPrefs_New(AukShared *firstPtr)
{
    if(!firstPtr) return;
    AukProjectPrefs* prefs = (AukProjectPrefs*)AllocVec(sizeof(AukProjectPrefs), MEMF_CLEAR);
    if (prefs) {
        AukProjectPrefs_Init(prefs);
        AukShared_Set(firstPtr,&prefs->base);
    }

}
void AukProjectPrefs_Init(AukProjectPrefs *prefs)
{
    if(!prefs) return;
    AukObject_Init(&prefs->base);
    prefs->maxTracks = 0;
    prefs->sampleRate = 0;

}

