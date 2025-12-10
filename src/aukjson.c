#include "aukjson.h"
#include "aukjsonserializer.h"
#include "auktyperegistry.h"
#include "aukproject.h"
#include "aukstring.h"
#include <proto/exec.h>
#include <proto/dos.h>
#include <dos/dos.h>
#include <string.h>

/*
 * JSON serialization implementation
 * Now uses the ISerializer abstraction with AukJsonSerializer
 */

char* AukJson_SerializeProject(AukProject* project) {
    ISerializer* ser;
    char* result;

    if (!project) {
        return NULL;
    }

    /* Create JSON writer serializer */
    ser = AukJsonSerializer_CreateWriter();
    if (!ser) {
        return NULL;
    }

    /* Serialize the project using its Serialize method */
    // if (project->base.Serialize) {
    //     project->base.Serialize(project, ser, "project");
    // }
    /* Serialize the project */
    ser->t_object(ser, "project", (AukObjectPtr*)&project);


    /* Get JSON string */
    result = AukJsonSerializer_GetString(ser);

    /* Clean up serializer */
    ser->Destroy(ser);

    return result;
}

void AukJson_DeserializeProject(AukProjectPtr* projectPtr, const char* jsonString) {
    ISerializer* ser;
    const TypeNameToContructor* typeRegistry;

    if (!projectPtr || !jsonString) {
        return;
    }

    /* Get type registry */
    typeRegistry = AukProject_GetTypeRegistry();

    /* Create JSON reader serializer */
    ser = AukJsonSerializer_CreateReader(jsonString, typeRegistry);
    if (!ser) {
        return;
    }

    /* Deserialize the project */
    ser->t_object(ser, "project", (AukObjectPtr*)projectPtr);

    /* Clean up serializer */
    ser->Destroy(ser);
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

        /* Set project context on all loaded objects */
        AukProject_SetProjectContext(project);
    }
}
