#include "aukjson.h"
#include "auktrack.h"
#include "auksound.h"
#include "auksoundfile.h"
#include "aukstring.h"
#include "cJSON.h"
#include <proto/exec.h>
#include <proto/dos.h>
#include <dos/dos.h>
#include <string.h>

/*
 * JSON serialization implementation
 * Handles saving and loading entire project graph
 */

/* Helper: Convert fixed-point to integer for JSON (scaled by 1000 for precision) */
static long AukJson_FixedToInt(AukFixed value) {
    /* Convert to milliseconds or similar integer representation */
    return AukFixed_ToInt(AukFixed_Mul(value, AukFixed_FromInt(1000)));
}

/* Helper: Convert integer from JSON to fixed-point */
static AukFixed AukJson_IntToFixed(long value) {
    return AukFixed_Div(AukFixed_FromInt(value), AukFixed_FromInt(1000));
}

/* Serialize envelope points to JSON array */
static cJSON* AukJson_SerializeEnvelope(AukEnvelopePoint* envelope) {
    cJSON* array;
    cJSON* point;
    AukEnvelopePoint* current;

    array = cJSON_CreateArray();
    if (!array) {
        return NULL;
    }

    current = envelope;
    while (current) {
        point = cJSON_CreateObject();
        if (point) {
            cJSON_AddNumberToObjectFixed(point, "time", current->time);
            cJSON_AddNumberToObjectFixed(point, "value", current->value);
            cJSON_AddItemToArray(array, point);
        }
        current = current->next;
    }

    return array;
}

/* Deserialize envelope points from JSON array */
static void AukJson_DeserializeEnvelope(AukTrack* track, cJSON* array) {
    cJSON* point;
    AukFixed time, value;

    if (!track || !array || !cJSON_IsArray(array)) {
        return;
    }

    cJSON_ArrayForEach(point, array) {
        if (cJSON_IsObject(point)) {
            time = cJSON_GetNumberFixed(cJSON_GetObjectItem(point, "time"));
            value = cJSON_GetNumberFixed(cJSON_GetObjectItem(point, "value"));
            track->AddEnvelopePoint(track, time, value);
        }
    }
}

/* Serialize sound to JSON object */
static cJSON* AukJson_SerializeSound(AukSound* sound, AukProject* project) {
    cJSON* obj;
    AukSoundFile* soundFile;
    const char* filename;

    if (!sound) {
        return NULL;
    }

    obj = cJSON_CreateObject();
    if (!obj) {
        return NULL;
    }

    /* Get sound file info */
    soundFile = sound->soundFile;
    if (soundFile) {
        filename = soundFile->GetFilename(soundFile);
        if (filename) {
            /* Convert to relative path if needed */
            if (project && project->path) {
                char* relPath = AukString_MakeRelativePath(project->path, filename);
                if (relPath) {
                    cJSON_AddStringToObject(obj, "file", relPath);
                    AukString_Free(relPath);
                } else {
                    cJSON_AddStringToObject(obj, "file", filename);
                }
            } else {
                cJSON_AddStringToObject(obj, "file", filename);
            }
        }

        cJSON_AddNumberToObjectInt(obj, "sampleRate", soundFile->sampleRate);
        cJSON_AddNumberToObjectInt(obj, "channels", soundFile->channels);
        cJSON_AddNumberToObjectInt(obj, "frameCount", soundFile->frameCount);
    }

    /* Sound properties */
    cJSON_AddNumberToObjectFixed(obj, "startTime", sound->startTime);
    cJSON_AddNumberToObjectFixed(obj, "endTime", sound->endTime);
    cJSON_AddNumberToObjectInt(obj, "fileStartFrame", sound->fileStartFrame);
    cJSON_AddNumberToObjectInt(obj, "fileEndFrame", sound->fileEndFrame);
    cJSON_AddNumberToObjectInt(obj, "loopCount", sound->loopCount);

    return obj;
}

/* Deserialize sound from JSON object */
static AukSound* AukJson_DeserializeSound(cJSON* obj, AukProject* project) {
    AukSound* sound;
    AukSoundFilePtr soundFile = NULL;
    cJSON* item;
    const char* filename;
    char* absPath;
    unsigned long sampleRate, channels, frameCount;
    unsigned long fileStartFrame, fileEndFrame, loopCount;
    AukFixed startTime, endTime;

    if (!obj || !cJSON_IsObject(obj)) {
        return NULL;
    }

    /* Get file info */
    item = cJSON_GetObjectItem(obj, "file");
    if (!item || !cJSON_IsString(item)) {
        return NULL;
    }
    filename = item->valuestring;

    /* Convert to absolute path */
    if (project && project->path) {
        absPath = AukString_MakeAbsolutePath(project->path, filename);
    } else {
        absPath = AukString_Duplicate(filename);
    }

    /* Get sound file properties */
    sampleRate = cJSON_GetObjectItem(obj, "sampleRate")->valueint;
    channels = cJSON_GetObjectItem(obj, "channels")->valueint;
    frameCount = cJSON_GetObjectItem(obj, "frameCount")->valueint;

    /* Create sound file */
    AukSoundFile_New(&soundFile);
    if (!soundFile) {
        AukString_Free(absPath);
        return NULL;
    }
    AukSoundFile_SetFilename(soundFile, absPath);
    AukSoundFile_SetProperties(soundFile, sampleRate, channels, frameCount);
    AukString_Free(absPath);

    /* Create sound */
    AukSoundPtr soundPtr = NULL;
    AukSound_New(&soundPtr);
    sound = soundPtr;
    if (!sound) {
        AukObjectPtr_Release((AukObjectPtr*)&soundFile);
        return NULL;
    }

    /* Set sound file */
    AukSound_SetSoundFile(sound, soundFile);

    /* Get sound properties */
    startTime = cJSON_GetNumberFixed(cJSON_GetObjectItem(obj, "startTime"));
    endTime = cJSON_GetNumberFixed(cJSON_GetObjectItem(obj, "endTime"));
    fileStartFrame = cJSON_GetObjectItem(obj, "fileStartFrame")->valueint;
    fileEndFrame = cJSON_GetObjectItem(obj, "fileEndFrame")->valueint;
    loopCount = cJSON_GetObjectItem(obj, "loopCount")->valueint;

    /* Set properties */
    sound->SetTimeRange(sound, startTime, endTime);
    sound->SetFileRange(sound, fileStartFrame, fileEndFrame);
    sound->SetLoopCount(sound, loopCount);

    /* Release our reference (sound now owns it) */
    AukObjectPtr_Release((AukObjectPtr*)&soundFile);

    return sound;
}

/* Serialize track to JSON object */
static cJSON* AukJson_SerializeTrack(AukTrack* track, AukProject* project) {
    cJSON* obj;
    cJSON* soundsArray;
    cJSON* soundObj;
    unsigned long i, count;
    AukSound* sound;

    if (!track) {
        return NULL;
    }

    obj = cJSON_CreateObject();
    if (!obj) {
        return NULL;
    }

    /* Track name */
    if (track->name) {
        cJSON_AddStringToObject(obj, "name", track->name);
    }

    /* Sounds array */
    soundsArray = cJSON_CreateArray();
    if (soundsArray) {
        count = track->GetSoundCount(track);
        for (i = 0; i < count; i++) {
            sound = track->GetSound(track, i);
            if (sound) {
                soundObj = AukJson_SerializeSound(sound, project);
                if (soundObj) {
                    cJSON_AddItemToArray(soundsArray, soundObj);
                }
            }
        }
        cJSON_AddItemToObject(obj, "sounds", soundsArray);
    }

    /* Envelope */
    if (track->envelope) {
        cJSON* envelope = AukJson_SerializeEnvelope(track->envelope);
        if (envelope) {
            cJSON_AddItemToObject(obj, "envelope", envelope);
        }
    }

    return obj;
}

/* Deserialize track from JSON object */
static void AukJson_DeserializeTrack(AukTrack*track, cJSON* obj, AukProject* project) {
    cJSON* item;
    cJSON* soundsArray;
    cJSON* soundObj;
    cJSON* envelope;
    AukSound* sound;

    if (!track || !obj || !cJSON_IsObject(obj)) {
        return;
    }

    /* Set name */
    item = cJSON_GetObjectItem(obj, "name");
    if (item && cJSON_IsString(item)) {
        AukTrack_SetName(track, item->valuestring);
    }

    /* Load sounds */
    soundsArray = cJSON_GetObjectItem(obj, "sounds");
    if (soundsArray && cJSON_IsArray(soundsArray)) {
        cJSON_ArrayForEach(soundObj, soundsArray) {
            sound = AukJson_DeserializeSound(soundObj, project);
            if (sound) {
                track->AddSound(track, sound);
            }
        }
    }

    /* Load envelope */
    envelope = cJSON_GetObjectItem(obj, "envelope");
    if (envelope) {
        AukJson_DeserializeEnvelope(track, envelope);
    }

}

char* AukJson_SerializeProject(AukProject* project) {
    cJSON* root;
    cJSON* prefs;
    cJSON* tracksArray;
    cJSON* trackObj;
    unsigned long i, count;
    AukTrack* track;
    char* jsonString;

    if (!project) {
        return NULL;
    }

    root = cJSON_CreateObject();
    if (!root) {
        return NULL;
    }

    /* Project metadata */
    cJSON_AddStringToObject(root, "version", "1.0");
    if (project->name) {
        cJSON_AddStringToObject(root, "name", project->name);
    }

    /* Preferences */
    prefs = cJSON_CreateObject();
    if (prefs) {
        cJSON_AddNumberToObjectInt(prefs, "sampleRate", project->prefs->sampleRate);
        cJSON_AddNumberToObjectInt(prefs, "maxTracks", project->prefs->maxTracks);
        cJSON_AddItemToObject(root, "preferences", prefs);
    }

    /* Tracks */
    tracksArray = cJSON_CreateArray();
    if (tracksArray) {
        count = project->GetTrackCount(project);
        for (i = 0; i < count; i++) {
            track = project->GetTrack(project, i);
            if (track) {
                trackObj = AukJson_SerializeTrack(track, project);
                if (trackObj) {
                    cJSON_AddItemToArray(tracksArray, trackObj);
                }
            }
        }
        cJSON_AddItemToObject(root, "tracks", tracksArray);
    }

    /* Convert to string */
    jsonString = cJSON_Print(root);
    cJSON_Delete(root);

    return jsonString;
}

void AukJson_DeserializeProject(AukProjectPtr* projectPtr, const char* jsonString) {
    cJSON* root;
    cJSON* prefs;
    cJSON* tracksArray;
    cJSON* trackObj;
    cJSON* item;
    AukProject* project;
    AukTrack* track;
    unsigned long sampleRate, maxTracks;

    if (!projectPtr || !jsonString) {
        return;
    }

    root = cJSON_Parse(jsonString);
    if (!root) {
        return;
    }

    /* Create project */
    AukProject_New(projectPtr);
    project = *projectPtr;
    if (!project) {
        cJSON_Delete(root);
        return;
    }

    /* Load name */
    item = cJSON_GetObjectItem(root, "name");
    if (item && cJSON_IsString(item)) {
        project->SetName(project, item->valuestring);
    }

    /* Load preferences */
    prefs = cJSON_GetObjectItem(root, "preferences");
    if (prefs && cJSON_IsObject(prefs)) {
        sampleRate = cJSON_GetObjectItem(prefs, "sampleRate")->valueint;
        maxTracks = cJSON_GetObjectItem(prefs, "maxTracks")->valueint;
        AukProject_SetPreferences(project, sampleRate, maxTracks);
    }

    /* Load tracks */
    tracksArray = cJSON_GetObjectItem(root, "tracks");
    if (tracksArray && cJSON_IsArray(tracksArray)) {
        cJSON_ArrayForEach(trackObj, tracksArray) {
            track = project->CreateTrack(project);
            if (track) {
                AukJson_DeserializeTrack(track, trackObj, project);
            }
        }
    }

    cJSON_Delete(root);
}

int AukJson_SaveProject(AukProject* project, const char* filename) {
    char* jsonString;
    BPTR file;
    long length;
    long written;

    if (!project || !filename) {
        return 0;
    }

    /* Serialize to JSON */
    jsonString = AukJson_SerializeProject(project);
    if (!jsonString) {
        return 0;
    }

    /* Open file for writing */
    file = Open((STRPTR)filename, MODE_NEWFILE);
    if (!file) {
        FreeVec(jsonString);
        return 0;
    }

    /* Write JSON string */
    length = strlen(jsonString);
    written = Write(file, jsonString, length);

    Close(file);
    FreeVec(jsonString);

    return (written == length);
}

void AukJson_LoadProject(AukProjectPtr* projectPtr, const char* filename) {
    BPTR file;
    long size;
    char* buffer;
    AukProject* project;

    if (!projectPtr || !filename) {
        return;
    }

    /* Open file for reading */
    file = Open((STRPTR)filename, MODE_OLDFILE);
    if (!file) {
        return;
    }

    /* Get file size */
    Seek(file, 0, OFFSET_END);
    size = Seek(file, 0, OFFSET_BEGINNING);

    /* Allocate buffer */
    buffer = (char*)AllocVec(size + 1, MEMF_CLEAR);
    if (!buffer) {
        Close(file);
        return;
    }

    /* Read file */
    if (Read(file, buffer, size) != size) {
        Close(file);
        FreeVec(buffer);
        return;
    }

    Close(file);

    /* Deserialize */
    AukJson_DeserializeProject(projectPtr, buffer);
    project = *projectPtr;

    FreeVec(buffer);

    /* Set project path from filename */
    if (project) {
        /* Extract directory from filename */
        char* lastSlash = AukString_Find(filename, "/");
        char* lastColon = AukString_Find(filename, ":");
        char* pathEnd = lastSlash > lastColon ? lastSlash : lastColon;

        if (pathEnd) {
            unsigned long pathLen = (unsigned long)(pathEnd - filename + 1);
            char* path = (char*)AllocVec(pathLen + 1, MEMF_CLEAR);
            if (path) {
                memcpy(path, filename, pathLen);
                project->SetPath(project, path);
                FreeVec(path);
            }
        }
    }
}
