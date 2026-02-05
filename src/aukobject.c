#include "aukobject.h"
#include <proto/exec.h>
#include <string.h>


int AukObjectCount = 0;

/*
 * Base object implementation
 * This is the root of our object hierarchy
 * Includes listener/observer pattern implementation
 */

void AukObject_New(AukObjectPtr *firstPtr) {
    AukObject* obj;
    if(!firstPtr) return;
    obj = (AukObject*)AllocVec(sizeof(AukObject), MEMF_CLEAR);
    if (obj) {
        AukObject_Init(obj);
        AukObjectPtr_Set(firstPtr,obj);
    }
}

void AukObject_Delete(AukObject* obj) {
    AukListener* listener;
    AukListener* nextListener;
    AukMessage m;

    if (obj) {

         m.type = AUK_MSG_WILL_DELETE;
         AukObject_SendUpdate(obj,&m);

        /* Free all listeners */
        listener = obj->listeners;
        while (listener) {
            nextListener = listener->next;

            /* Release reference to listener object */
            if (listener->listenerObject) {
                AukObjectPtr_Release(&listener->listenerObject);
            }

            FreeVec(listener);
            listener = nextListener;
        }

        AukObjectCount--;
        FreeVec(obj);
    }
}

const char* AukObject_GetTypeName(AukObject* This) {
    (void)This; /* Unused */
    return "AukObject";
}

void AukObject_Serialize(AukObject* This, ISerializer* ser, const char* pName) {
    /* Base class has no members to serialize */
    /* Listeners are NOT serialized - they are runtime-only connections */
    (void)This;
    (void)ser;
    (void)pName;
}

int AukObject_AddListener(AukObject* obj, AukObject* listenerObject, void* userData, AukUpdateCallback callback) {
    AukListener* newListener;
    AukListener* current;

    if (!obj || !listenerObject || !callback) {
        return 0;
    }

    aukMutex_lock( &obj->listeners_mutex );

    /* Check if listener already exists */
    current = obj->listeners;
    while (current) {
        if (AukObjectPtr_GetObject(&current->listenerObject) == listenerObject) {
            /* Already registered */
            aukMutex_unlock( &obj->listeners_mutex );
            return 1;
        }
        current = current->next;
    }

    /* Allocate new listener node */
    newListener = (AukListener*)AllocVec(sizeof(AukListener), MEMF_CLEAR);
    if (!newListener) {
        aukMutex_unlock( &obj->listeners_mutex );
        return 0;
    }

    /* Retain reference to listener object */
    AukObjectPtr_Set(&newListener->listenerObject,listenerObject);
    newListener->userData = userData;
    newListener->callback = callback;
    newListener->next = obj->listeners;

    /* Add to front of list */
    obj->listeners = newListener;
    aukMutex_unlock( &obj->listeners_mutex );
    return 1;
}

int AukObject_RemoveListener(AukObject* obj, AukObject* listenerObject) {
    AukListener* current;
    AukListener* prev;

    if (!obj || !listenerObject) {
        return 0;
    }
    aukMutex_lock( &obj->listeners_mutex );
    prev = NULL;
    current = obj->listeners;

    /* Find and remove listener */
    while (current) {
        if (AukObjectPtr_GetObject(&current->listenerObject) == listenerObject) {
            /* Remove from list */
            if (prev) {
                prev->next = current->next;
            } else {
                obj->listeners = current->next;
            }
            /* Release reference and free node */
            AukObjectPtr_Release(&current->listenerObject);
            FreeVec(current);
            aukMutex_unlock( &obj->listeners_mutex );
            return 1;
        }
        prev = current;
        current = current->next;
    }
    aukMutex_unlock( &obj->listeners_mutex );
    return 0;
}

void AukObject_SendUpdate(AukObject* obj, AukMessage* message) {
    AukListener* current;
    AukObject* listenerPtr;

    if (!obj || obj->_blockUpdates) {
        return;
    }
    if(obj->listeners_mutex.n>0) return; // recursive message shouldnt happen !
    aukMutex_lock( &obj->listeners_mutex );
    /* Notify all listeners */
    current = obj->listeners;
    while (current) {
        listenerPtr = AukObjectPtr_GetObject(&current->listenerObject);
        if (listenerPtr && current->callback) {
            current->callback(listenerPtr, obj, current->userData, message);
        }
        current = current->next;
    }
    aukMutex_unlock( &obj->listeners_mutex );
}

void AukObject_Init(AukObject* obj) {
    if (obj) {
        obj->New = AukObject_New;
        obj->Delete = AukObject_Delete;
        obj->GetTypeName = AukObject_GetTypeName;
        obj->Serialize = AukObject_Serialize;

        /* Listener management */
        obj->AddListener = AukObject_AddListener;
        obj->RemoveListener = AukObject_RemoveListener;
        obj->SendUpdate = AukObject_SendUpdate;

        /* Initialize listener list */
        obj->listeners = NULL;

        /* Initialize project context */
        obj->_project = NULL;

        AukObjectCount++;
    }
}


/*
 * Typed pointer implementation
 * Reference counted smart pointer for automatic memory management
 */

/*  Set pointer, retaining the object. If previous exists, it is released. object can be NULL to just release. */

void AukObjectPtr_Set(AukObjectPtr* ptr, AukObject* object)
{
    AukObject *previous;
    if(!ptr) return;
    if((*ptr) == object) return;

    previous = *ptr;
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
    (*ptr) = object;
    if(object)
    {
        object->refcount++;
    }
}


AukObject* AukObjectPtr_GetObject(AukObjectPtr* ptr) {
    return (ptr && (*ptr))  ? (*ptr) : NULL;
}

unsigned int AukObjectPtr_GetRefCount(AukObjectPtr* ptr) {
    return (ptr && (*ptr)) ? (*ptr)->refcount : 0;
}

