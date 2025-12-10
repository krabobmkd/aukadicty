#ifndef AUKPROJECT_H
#define AUKPROJECT_H

/*
 * AukProject - Abstract Project Base Class
 * Base class for all project types (audio, video, etc.)
 * Provides common project operations: name, path, save, load
 * Document layer - must not depend on GUI
 */

#include "aukobject.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declaration */
typedef struct AukProject AukProject;
typedef AukProject* AukProjectPtr;

/* Abstract AukProject structure - inherits from AukObject */
struct AukProject {
    AukObject base;          /* Must be first - inheritance */

    /* Common project data members */
    char* name;              /* Project name */
    char* path;              /* Project file path (directory) */

    /* Virtual methods common to all projects */
    int (*SetName)(void* This, const char* name);
    const char* (*GetName)(void* This);
    int (*SetPath)(void* This, const char* path);
    const char* (*GetPath)(void* This);
    int (*Save)(void* This, const char* filename);
    int (*Load)(void* This, const char* filename);
};

/* Constructor/Destructor */
void AukProject_New(AukProjectPtr *firstPtr);
void AukProject_Delete(void* This);
const char* AukProject_GetTypeName(void* This);

/* Initialize AukProject structure */
void AukProject_Init(AukProject* project);

/* Common project methods */
int AukProject_SetName(void* This, const char* name);
const char* AukProject_GetName(void* This);
int AukProject_SetPath(void* This, const char* path);
const char* AukProject_GetPath(void* This);
int AukProject_Save(void* This, const char* filename);
int AukProject_Load(void* This, const char* filename);

/* Helper to set project context on all objects after deserialization */
void AukProject_SetProjectContext(AukProject* project);

#ifdef __cplusplus
}
#endif

#endif /* AUKPROJECT_H */
