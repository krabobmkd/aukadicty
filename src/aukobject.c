#include "aukobject.h"
#include "aukshared.h"
#include <proto/exec.h>
#include <string.h>

/*
 * Base object implementation
 * This is the root of our object hierarchy
 * Includes listener/observer pattern implementation
 */

void* AukObject_New(void) {
    AukObject* obj = (AukObject*)AllocVec(sizeof(AukObject), MEMF_CLEAR);
    if (obj) {
        AukObject_Init(obj);
    }
    return obj;
}

void AukObject_Delete(void* This) {
    AukObject* obj = (AukObject*)This;
    AukListener* listener;
    AukListener* nextListener;

    if (obj) {
        /* Free all listeners */
        listener = obj->listeners;
        while (listener) {
            nextListener = listener->next;

            /* Release reference to listener object */
            if (listener->listenerObject) {
                AukShared_Release(listener->listenerObject);
            }

            FreeVec(listener);
            listener = nextListener;
        }

        FreeVec(obj);
    }
}

const char* AukObject_GetTypeName(void* This) {
    (void)This; /* Unused */
    return "AukObject";
}

int AukObject_AddListener(void* This, AukShared* listenerObject, AukUpdateCallback callback) {
    AukObject* obj = (AukObject*)This;
    AukListener* newListener;
    void* listenerPtr;

    if (!obj || !listenerObject || !callback) {
        return 0;
    }

    listenerPtr = AukShared_GetObject(listenerObject);
    if (!listenerPtr) {
        return 0;
    }

    /* Check if listener already exists */
    AukListener* current = obj->listeners;
    while (current) {
        if (AukShared_GetObject(current->listenerObject) == listenerPtr) {
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
    newListener->listenerObject = AukShared_Retain(listenerObject);
    newListener->callback = callback;
    newListener->next = obj->listeners;

    /* Add to front of list */
    obj->listeners = newListener;

    return 1;
}

int AukObject_RemoveListener(void* This, void* listenerObject) {
    AukObject* obj = (AukObject*)This;
    AukListener* current;
    AukListener* prev;

    if (!obj || !listenerObject) {
        return 0;
    }

    prev = NULL;
    current = obj->listeners;

    /* Find and remove listener */
    while (current) {
        if (AukShared_GetObject(current->listenerObject) == listenerObject) {
            /* Remove from list */
            if (prev) {
                prev->next = current->next;
            } else {
                obj->listeners = current->next;
            }

            /* Release reference and free node */
            AukShared_Release(current->listenerObject);
            FreeVec(current);

            return 1;
        }

        prev = current;
        current = current->next;
    }

    return 0;
}

void AukObject_SendUpdate(void* This) {
    AukObject* obj = (AukObject*)This;
    AukListener* current;
    void* listenerPtr;

    if (!obj) {
        return;
    }

    /* Notify all listeners */
    current = obj->listeners;
    while (current) {
        listenerPtr = AukShared_GetObject(current->listenerObject);
        if (listenerPtr && current->callback) {
            current->callback(listenerPtr, This);
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
