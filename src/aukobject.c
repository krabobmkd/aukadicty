#include "aukobject.h"
#include <proto/exec.h>
#include <string.h>

/*
 * Base object implementation
 * This is the root of our object hierarchy
 * Includes listener/observer pattern implementation
 */

void AukObject_New(AukShared *firstPtr) {
    if(!firstPtr) return;
    AukObject* obj = (AukObject*)AllocVec(sizeof(AukObject), MEMF_CLEAR);
    if (obj) {
        AukObject_Init(obj);
        AukShared_Set(firstPtr,obj);
    }
}

void AukObject_Delete(AukObject* obj) {
    AukListener* listener;
    AukListener* nextListener;

    if (obj) {
        /* Free all listeners */
        listener = obj->listeners;
        while (listener) {
            nextListener = listener->next;

            /* Release reference to listener object */
            if (listener->listenerObject) {
                AukShared_Release(&listener->listenerObject);
            }

            FreeVec(listener);
            listener = nextListener;
        }

        FreeVec(obj);
    }
}

const char* AukObject_GetTypeName(AukObject* This) {
    (void)This; /* Unused */
    return "AukObject";
}

int AukObject_AddListener(AukObject* obj, AukObject* listenerObject, AukUpdateCallback callback) {
    AukListener* newListener;

    if (!obj || !listenerObject || !callback) {
        return 0;
    }

    /* Check if listener already exists */
    AukListener* current = obj->listeners;
    while (current) {
        if (AukShared_GetObject(&current->listenerObject) == listenerObject) {
            /* Already registered */
            return 1;
        }
        current = current->next;
    }

    /* Allocate new listener node */
    newListener = (AukListener*)AllocVec(sizeof(AukListener), MEMF_CLEAR);
    if (!newListener) {
        return 0;
    }

    /* Retain reference to listener object */
    AukShared_Set(&newListener->listenerObject,listenerObject);
    newListener->callback = callback;
    newListener->next = obj->listeners;

    /* Add to front of list */
    obj->listeners = newListener;

    return 1;
}

int AukObject_RemoveListener(AukObject* obj, AukObject* listenerObject) {
    AukListener* current;
    AukListener* prev;

    if (!obj || !listenerObject) {
        return 0;
    }

    prev = NULL;
    current = obj->listeners;

    /* Find and remove listener */
    while (current) {
        if (AukShared_GetObject(&current->listenerObject) == listenerObject) {
            /* Remove from list */
            if (prev) {
                prev->next = current->next;
            } else {
                obj->listeners = current->next;
            }
            /* Release reference and free node */
            AukShared_Release(&current->listenerObject);
            FreeVec(current);

            return 1;
        }
        prev = current;
        current = current->next;
    }

    return 0;
}

void AukObject_SendUpdate(AukObject* obj,void *message) {
    AukListener* current;
    AukObject* listenerPtr;

    if (!obj) {
        return;
    }

    /* Notify all listeners */
    current = obj->listeners;
    while (current) {
        listenerPtr = AukShared_GetObject(&current->listenerObject);
        if (listenerPtr && current->callback) {
            current->callback(listenerPtr, obj,message);
        }
        current = current->next;
    }
}

void AukObject_Init(AukObject* obj) {
    if (obj) {
        obj->New = AukObject_New;
        obj->Delete = AukObject_Delete;
        obj->GetTypeName = AukObject_GetTypeName;

        /* Listener management */
        obj->AddListener = AukObject_AddListener;
        obj->RemoveListener = AukObject_RemoveListener;
        obj->SendUpdate = AukObject_SendUpdate;

        /* Initialize listener list */
        obj->listeners = NULL;
    }
}


/*
 * Shared pointer implementation
 * Reference counted smart pointer for automatic memory management
 */

/*  shared retain the object, if previous, previous is released. object can be NULL to just release. */

void AukShared_Set(AukShared* shared, AukObject* object )
{
    AukObject *previous;
    if(!shared) return;
    if((*shared) == object) return;

    previous = *shared;
    if(previous)
    {
        previous->refcount--;
        if (previous->refcount == 0) {
            /* Call object's Delete method */
            if (previous->Delete) {
                previous->Delete(previous);
            }
        }
    }
    /* note: can be null */
    (*shared) = object;
    if(object)
    {
        object->refcount++;
    }
}


AukObject* AukShared_GetObject(AukShared* shared) {
    return (shared && (*shared))  ? (*shared) : NULL;
}

unsigned long AukShared_GetRefCount(AukShared* shared) {
    return (shared && (*shared)) ? (*shared)->refcount : 0;
}

