#include "aukoperations.h"
#include <proto/exec.h>

/*
 * High-level data management operations implementation
 */

AukSound* AukOp_AddSoundToTrack(AukTrack* track, AukShared* soundFile,
                                 AukFixed startTime, AukFixed endTime) {
    AukSound* sound;

    if (!track || !soundFile) {
        return NULL;
    }

    /* Create new sound */
    sound = (AukSound*)AukSound_New();
    if (!sound) {
        return NULL;
    }

    /* Set properties */
    AukSound_SetSoundFile(sound, soundFile);
    AukSound_SetTimeRange(sound, startTime, endTime);

    /* Add to track */
    if (!track->AddSound(track, sound)) {
        AukSound_Delete(sound);
        return NULL;
    }

    return sound;
}

int AukOp_RemoveSound(AukSound* sound) {
    AukTrack* track;

    if (!sound) {
        return 0;
    }

    track = sound->track;
    if (!track) {
        return 0;
    }

    /* Remove from track */
    if (!track->RemoveSound(track, sound)) {
        return 0;
    }

    /* Delete sound */
    AukSound_Delete(sound);

    return 1;
}

int AukOp_MoveSoundToTrack(AukSound* sound, AukTrack* newTrack) {
    AukTrack* oldTrack;

    if (!sound || !newTrack) {
        return 0;
    }

    oldTrack = sound->track;
    if (!oldTrack) {
        /* Sound not on any track, just add it */
        return newTrack->AddSound(newTrack, sound);
    }

    if (oldTrack == newTrack) {
        return 1; /* Already on target track */
    }

    /* Remove from old track */
    if (!oldTrack->RemoveSound(oldTrack, sound)) {
        return 0;
    }

    /* Add to new track */
    if (!newTrack->AddSound(newTrack, sound)) {
        /* Failed to add, put back on old track */
        oldTrack->AddSound(oldTrack, sound);
        return 0;
    }

    return 1;
}

int AukOp_SetSoundTimeRange(AukSound* sound, AukFixed startTime, AukFixed endTime) {
    if (!sound) {
        return 0;
    }

    sound->SetTimeRange(sound, startTime, endTime);
    return 1;
}

int AukOp_SetSoundFileRange(AukSound* sound, unsigned long startFrame, unsigned long endFrame) {
    if (!sound) {
        return 0;
    }

    sound->SetFileRange(sound, startFrame, endFrame);
    return 1;
}

int AukOp_SetSoundLoopCount(AukSound* sound, unsigned long loopCount) {
    if (!sound) {
        return 0;
    }

    sound->SetLoopCount(sound, loopCount);
    return 1;
}

AukTrack* AukOp_AddTrackToProject(AukProject* project, const char* name) {
    AukTrack* track;

    if (!project) {
        return NULL;
    }

    /* Create new track */
    track = (AukTrack*)AukTrack_New();
    if (!track) {
        return NULL;
    }

    /* Set name if provided */
    if (name) {
        AukTrack_SetName(track, name);
    }

    /* Add to project */
    if (!project->AddTrack(project, track)) {
        AukTrack_Delete(track);
        return NULL;
    }

    return track;
}

int AukOp_RemoveTrack(AukTrack* track) {
    AukProject* project;

    if (!track) {
        return 0;
    }

    project = track->project;
    if (!project) {
        return 0;
    }

    /* Remove from project */
    if (!project->RemoveTrack(project, track)) {
        return 0;
    }

    /* Delete track (this also deletes all sounds on it) */
    AukTrack_Delete(track);

    return 1;
}

AukSound* AukOp_FindSoundAtTime(AukTrack* track, AukFixed time) {
    unsigned long i, count;
    AukSound* sound;

    if (!track) {
        return NULL;
    }

    count = track->GetSoundCount(track);

    for (i = 0; i < count; i++) {
        sound = track->GetSound(track, i);
        if (sound && time >= sound->startTime && time < sound->endTime) {
            return sound;
        }
    }

    return NULL;
}

unsigned long AukOp_GetSoundsInRange(AukTrack* track, AukFixed startTime, AukFixed endTime,
                                      AukSound** outSounds, unsigned long maxSounds) {
    unsigned long i, count, found;
    AukSound* sound;

    if (!track || !outSounds) {
        return 0;
    }

    count = track->GetSoundCount(track);
    found = 0;

    for (i = 0; i < count && found < maxSounds; i++) {
        sound = track->GetSound(track, i);
        if (sound) {
            /* Check if sound overlaps with range */
            if (sound->endTime > startTime && sound->startTime < endTime) {
                outSounds[found] = sound;
                found++;
            }
        }
    }

    return found;
}

AukShared* AukOp_CreateSoundFile(const char* filename, unsigned long sampleRate,
                                 unsigned long channels, unsigned long frameCount) {
    AukSoundFile* soundFile;
    AukShared* shared;

    if (!filename) {
        return NULL;
    }

    /* Create sound file object */
    soundFile = (AukSoundFile*)AukSoundFile_New();
    if (!soundFile) {
        return NULL;
    }

    /* Set properties */
    AukSoundFile_SetFilename(soundFile, filename);
    AukSoundFile_SetProperties(soundFile, sampleRate, channels, frameCount);

    /* Create shared pointer */
    shared = AukShared_Create(&soundFile->base);
    if (!shared) {
        AukSoundFile_Delete(soundFile);
        return NULL;
    }

    return shared;
}

AukProject* AukOp_CreateProject(const char* name, const char* path) {
    AukProject* project;

    /* Create project */
    project = (AukProject*)AukProject_New();
    if (!project) {
        return NULL;
    }

    /* Set name and path */
    if (name) {
        project->SetName(project, name);
    }

    if (path) {
        project->SetPath(project, path);
    }

    return project;
}

AukFixed AukOp_GetProjectDuration(AukProject* project) {
    unsigned long i, trackCount;
    unsigned long j, soundCount;
    AukTrack* track;
    AukSound* sound;
    AukFixed maxEndTime;
    AukFixed soundEndTime;

    if (!project) {
        return 0;
    }

    maxEndTime = 0;
    trackCount = project->GetTrackCount(project);

    /* Find latest end time across all tracks */
    for (i = 0; i < trackCount; i++) {
        track = project->GetTrack(project, i);
        if (track) {
            soundCount = track->GetSoundCount(track);

            for (j = 0; j < soundCount; j++) {
                sound = track->GetSound(track, j);
                if (sound) {
                    soundEndTime = sound->endTime;
                    if (soundEndTime > maxEndTime) {
                        maxEndTime = soundEndTime;
                    }
                }
            }
        }
    }

    return maxEndTime;
}
