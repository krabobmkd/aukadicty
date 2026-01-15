#include "aukproject.h"
#include "aukstring.h"
#include <proto/exec.h>
#include <string.h>

/*
 * AukProject implementation
 * Abstract base class for all project types
 */

void AukProject_New(AukObjectPtr *firstPtr) {
    AukProject* project;
    if(!firstPtr) return;
    project = (AukProject*)AllocVec(sizeof(AukProject), MEMF_CLEAR);
    if (project) {
        AukProject_Init(project);
        AukObjectPtr_Set(firstPtr, &project->base);
    }
}

void AukProject_Delete(AukObject* This) {
    AukProject* project = (AukProject*)This;

    if (project) {
        /* Free name and path */
        if (project->name) {
            AukString_Free(project->name);
        }
        if (project->path) {
            AukString_Free(project->path);
        }

        /* Call base object delete (which will FreeVec) */
        AukObject_Delete(&project->base);
    }
}

const char* AukProject_GetTypeName(AukObject* This) {
    (void)This;
    return "AukProject";
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
        {
            AukMessage msg;
            msg.type = AUK_MSG_MODIFY;
            project->base.SendUpdate(&project->base, &msg);
        }
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
        {
            AukMessage msg;
            msg.type = AUK_MSG_MODIFY;
            project->base.SendUpdate(&project->base, &msg);
        }
    }

    return project->path != NULL;
}

const char* AukProject_GetPath(void* This) {
    AukProject* project = (AukProject*)This;
    return project ? project->path : NULL;
}


void AukProject_Init(AukProject* project) {
    if (project) {
        /* Initialize base object */
        AukObject_Init(&project->base);

        /* Override virtual methods */
        project->base.New = (void (*)(AukObjectPtr*))AukProject_New;
        project->base.Delete = AukProject_Delete;
        project->base.GetTypeName = AukProject_GetTypeName;
        project->base.Serialize = NULL; /* Abstract - subclasses must implement */

        /* Set AukProject specific methods */
        project->SetName = AukProject_SetName;
        project->GetName = AukProject_GetName;
        project->SetPath = AukProject_SetPath;
        project->GetPath = AukProject_GetPath;

        /* Initialize data members */
        project->name = NULL;
        project->path = NULL;
    }
}
