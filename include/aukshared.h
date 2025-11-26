#ifndef AUKSHARED_H
#define AUKSHARED_H

/*
 * Shared pointer system for Aukadicty
 * C99 equivalent of std::shared_ptr with reference counting
 * Objects are automatically deleted when last reference is released
 */

#include "aukobject.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Shared pointer structure */
typedef struct AukShared {
    void* object;           /* Pointer to managed object */
    AukObject* vtable;      /* Pointer to object's vtable for Delete call */
    unsigned long refcount; /* Reference count */
} AukShared;

/* Create a new shared pointer to an object */
AukShared* AukShared_Create(void* object, AukObject* vtable);

/* Increment reference count and return the shared pointer */
AukShared* AukShared_Retain(AukShared* shared);

/* Decrement reference count, delete object if count reaches 0 */
void AukShared_Release(AukShared* shared);

/* Get the managed object pointer */
void* AukShared_GetObject(AukShared* shared);

/* Get current reference count */
unsigned long AukShared_GetRefCount(AukShared* shared);

#ifdef __cplusplus
}
#endif

#endif /* AUKSHARED_H */
