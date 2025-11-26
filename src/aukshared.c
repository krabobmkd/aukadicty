#include "aukshared.h"
#include <proto/exec.h>

/*
 * Shared pointer implementation
 * Reference counted smart pointer for automatic memory management
 */

AukShared* AukShared_Create(void* object, AukObject* vtable) {
    AukShared* shared;

    if (!object || !vtable) {
        return NULL;
    }

    shared = (AukShared*)AllocVec(sizeof(AukShared), MEMF_CLEAR);
    if (!shared) {
        return NULL;
    }

    shared->object = object;
    shared->vtable = vtable;
    shared->refcount = 1;

    return shared;
}

AukShared* AukShared_Retain(AukShared* shared) {
    if (shared) {
        shared->refcount++;
    }
    return shared;
}

void AukShared_Release(AukShared* shared) {
    if (!shared) {
        return;
    }

    shared->refcount--;

    if (shared->refcount == 0) {
        /* Call object's Delete method */
        if (shared->object && shared->vtable && shared->vtable->Delete) {
            shared->vtable->Delete(shared->object);
        }

        /* Free the shared pointer structure itself */
        FreeVec(shared);
    }
}

void* AukShared_GetObject(AukShared* shared) {
    return shared ? shared->object : NULL;
}

unsigned long AukShared_GetRefCount(AukShared* shared) {
    return shared ? shared->refcount : 0;
}
