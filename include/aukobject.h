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
/* Forward declarations */
typedef struct AukObject AukObject;
typedef struct AukListener AukListener;
typedef AukObject* AukShared;

/* Update callback function type */
/* Parameters: listenerObject, modifiedObject */
typedef void (*AukUpdateCallback)(AukObject* listenerObject, AukObject* modifiedObject, void *message);

/* Listener node in linked list */
struct AukListener {
    AukShared listenerObject;      /* Shared pointer to listener object */
    AukUpdateCallback callback;     /* Update notification callback */
    AukListener* next;              /* Next listener in list */
};

struct sISerializer;
typedef struct sISerializer ISerializer;

/* Base object vtable - all objects must implement these */
struct AukObject {
    /* Virtual methods - all take void* This as first parameter */
    void (*New)(AukShared *firstPtr);     /* Constructor */
    void (*Delete)(AukObject* This);            /* Destructor */
    const char* (*GetTypeName)(AukObject* This); /* Get object type name */
    void (*Serialize)(void* This,ISerializer *ser,const char *pName); /* both load/save */

    /* Listener management - inherited by all objects */
    int (*AddListener)(AukObject* This, AukObject* listenerObject, AukUpdateCallback callback);
    int (*RemoveListener)(AukObject* This, AukObject* listenerObject);
    void (*SendUpdate)(AukObject* This,void *message);        /* Notify all listeners of change */

    /* Listener list - managed by base object */
    AukListener* listeners;
    unsigned long refcount;
};

/* Helper macros for calling virtual methods */
#define AUK_NEW(obj) ((obj)->New())
#define AUK_DELETE(obj, this) ((obj)->Delete(this))
#define AUK_GET_TYPE_NAME(obj, this) ((obj)->GetTypeName(this))
#define AUK_SEND_UPDATE(obj, this) ((obj)->SendUpdate(this))

/* Base object functions */
void AukObject_New(AukShared *firstPtr);
/* internal, used externaly just to used as super method.
 * Should only be used by internal release mecanism, and root object delete. */
void AukObject_Delete(AukObject* This);


/* Listener management functions */
int AukObject_AddListener(AukObject* This, AukObject* listenerObject, AukUpdateCallback callback);
int AukObject_RemoveListener(AukObject* This, AukObject* listenerObject);
void AukObject_SendUpdate(AukObject* This, void *message);

/* Initialize base object vtable */
void AukObject_Init(AukObject* obj);




/* Shared pointer structure,all AukObject pointers should use. */



/*  shared retain the object, if previous, previous is released. object can be NULL to just release. */
void AukShared_Set(AukShared* shared,AukObject* object );
/* release object, can be inline */
INLINE void AukShared_Release(AukShared* shared) {  AukShared_Set(shared,0L); }

/* Get the managed object pointer */
AukObject* AukShared_GetObject(AukShared* shared);

/* Get current reference count */
unsigned long AukShared_GetRefCount(AukShared* shared);


#ifdef __cplusplus
}
#endif

#endif /* AUKOBJECT_H */
