#ifndef AUKOBJECT_H
#define AUKOBJECT_H

/*
 * Base object system for Aukadicty
 * Provides root object structure with New/Delete methods
 * All objects inherit from AukObject by placing it as first member
 *
 * Includes listener/observer pattern for Document/View architecture
 */

#ifdef __cplusplus
extern "C" {
#endif
#include "compilers.h"
#include "aukmutex.h"
/* Forward declarations */
typedef struct AukObject AukObject;
typedef struct AukListener AukListener;

/* Typed pointer system - replaces generic AukShared */
typedef AukObject* AukObjectPtr;

/* Update callback function type */
/* Parameters: listenerObject, modifiedObject */
typedef void (*AukUpdateCallback)(AukObject* listenerObject, AukObject* modifiedObject, void *message);

/* Listener node in linked list */
struct AukListener {
    AukObjectPtr listenerObject;    /* Shared pointer to listener object */
    AukUpdateCallback callback;     /* Update notification callback */
    AukListener* next;              /* Next listener in list */
};

struct sISerializer;
typedef struct sISerializer ISerializer;

/* Base object vtable - all objects must implement these */
struct AukObject {
    /* Virtual methods - all take void* This as first parameter */
    void (*New)(AukObjectPtr *firstPtr);     /* Constructor */
    void (*Delete)(AukObject* This);            /* Destructor */
    const char* (*GetTypeName)(AukObject* This); /* Get object type name */
    void (*Serialize)(AukObject* This,ISerializer *ser,const char *pName); /* both load/save */

    /* Listener management - inherited by all objects */
    int (*AddListener)(AukObject* This, AukObject* listenerObject, AukUpdateCallback callback);
    int (*RemoveListener)(AukObject* This, AukObject* listenerObject);
    void (*SendUpdate)(AukObject* This,void *message);        /* Notify all listeners of change */

    /* Listener list - managed by base object */
    AukListener* listeners;
    AukMutex    listeners_mutex;
    unsigned int refcount;
};

/* Helper macros for calling virtual methods */
#define AUK_NEW(obj) ((obj)->New())
#define AUK_DELETE(obj, this) ((obj)->Delete(this))
#define AUK_GET_TYPE_NAME(obj, this) ((obj)->GetTypeName(this))
#define AUK_SEND_UPDATE(obj, this) ((obj)->SendUpdate(this))

/* Base object functions */
void AukObject_New(AukObjectPtr *firstPtr);
/* internal, used externaly just to used as super method.
 * Should only be used by internal release mecanism, and root object delete. */
void AukObject_Delete(AukObject* This);


/* Listener management functions */
int AukObject_AddListener(AukObject* This, AukObject* listenerObject, AukUpdateCallback callback);
int AukObject_RemoveListener(AukObject* This, AukObject* listenerObject);
void AukObject_SendUpdate(AukObject* This, void *message);

/* Initialize base object vtable */
void AukObject_Init(AukObject* obj);




/* Typed pointer management functions - all AukObject pointers should use these */

/*  Set pointer, retaining the object. If previous exists, it is released. object can be NULL to just release. */
void AukObjectPtr_Set(AukObjectPtr* ptr, AukObject* object);

/* Release object, can be inline */
INLINE void AukObjectPtr_Release(AukObjectPtr* ptr) { AukObjectPtr_Set(ptr, 0L); }

/* Get the managed object pointer */
AukObject* AukObjectPtr_GetObject(AukObjectPtr* ptr);

/* Get current reference count */
unsigned int AukObjectPtr_GetRefCount(AukObjectPtr* ptr);


#ifdef __cplusplus
}
#endif

#endif /* AUKOBJECT_H */
