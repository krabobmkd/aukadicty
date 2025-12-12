#include "aukaproject.h"
#include "auktrack.h"
#include "auksound.h"
#include "auksoundfile.h"
#include "aukstring.h"
#include "aukjson.h"
#include "aukarray.h"
#include <proto/exec.h>
#include <string.h>
#include "serializer.h"

#ifdef Remove
#undef Remove
#endif

/*
 * AukAProject implementation
 * Audio project concrete implementation
 */

#define INITIAL_TRACK_CAPACITY 8

void AukAProject_New(AukAProjectPtr *firstPtr) {
    if(!firstPtr) return;
    AukAProject* project = (AukAProject*)AllocVec(sizeof(AukAProject), MEMF_CLEAR);
    if (project) {
        AukAProject_Init(project);
        AukObjectPtr_Set((AukObjectPtr*)firstPtr, &project->base.base);
    }
}

void AukAProject_Delete(void* This) {
    AukAProject* project = (AukAProject*)This;

    if (project) {
        /* Release audio-specific members */
        AukObjectPtr_Release((AukObjectPtr*)&project->prefs);
        AukObjectPtr_Release((AukObjectPtr*)&project->tracks);

        /* Call base project delete (which deletes name/path and calls AukObject_Delete) */
        AukProject_Delete(&project->base);
    }
}

const char* AukAProject_GetTypeName(void* This) {
    (void)This;
    return "AukAProject";
}

void AukAProject_Serialize(void* This, ISerializer* ser, const char* pName) {
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
    ser->t_object(ser, "prefs", &project->prefs);

    /* Serialize tracks array */
    ser->t_arrayobj(ser, "tracks", &project->tracks, AukTrack_New, AukTrack_GetTypeName);
}

void AukAProject_SetPreferences(AukAProject* project, unsigned int sampleRate, unsigned int maxTracks) {
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
        project->base.base.SendUpdate(&project->base.base, NULL);
    }
}

static int AukAProject_AddTrack(void* This, AukTrack* track) {
    AukAProject* project = (AukAProject*)This;

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
    AukTrack_SetProject(track, (AukProject*)project);

    /* Send update notification */
    project->base.base.SendUpdate(&project->base.base, NULL);

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
    AukTrack_New(&trackPtr);
    track = trackPtr;
    if (!track) {
        return NULL;
    }

    /* Add to project - this retains the track */
    if (!AukAProject_AddTrack(project, track)) {
        /* Failed to add - release our reference */
        AukObjectPtr_Release(&trackPtr);
        return NULL;
    }
    /* Release our local reference, now it is retained by the array. */
    AukObjectPtr_Release(&trackPtr);
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
        AukTrack_SetProject(track, NULL);

        /* Send update notification */
        project->base.base.SendUpdate(&project->base.base, NULL);

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
    tracksArray->Get(tracksArray, ptr, index);
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
                AukSound* sound=NULL;
                track->GetSound(track, &sound, j);
                if (sound) {
                    soundEndTime = sound->endTime;
                    if (soundEndTime > maxEndTime) {
                        maxEndTime = soundEndTime;
                    }
                    AukObjectPtr_Release(&sound);
                }
            }
        }
    }
    AukObjectPtr_Release((AukObjectPtr*)&track);

    return maxEndTime;
}

int AukAProject_Save(void* This, const char* filename) {
    AukAProject* project = (AukAProject*)This;
    return AukJson_SaveProject((AukProject*)project, filename);
}

int AukAProject_Load(void* This, const char* filename) {
    /* Note: Load creates a new project, doesn't modify existing one */
    /* This method signature doesn't fit well with load pattern */
    /* Use AukJson_LoadProject() directly instead */
    (void)This;
    (void)filename;
    return 0;
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

        /* Override base project methods */
        project->base.Save = AukAProject_Save;
        project->base.Load = AukAProject_Load;

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
        AukArray_New(&project->tracks);
        if(project->tracks) {
            AukArray_SetType(project->tracks, AukTrack_New, AukTrack_GetTypeName);
            project->tracks->base._project = (AukProject*)project;
        }
    }
}

/* AukProjectPrefs implementation */

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
                        soundsArray->Get(soundsArray, &sound, j);
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
