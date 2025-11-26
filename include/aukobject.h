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

/* Forward declarations */
typedef struct AukObject AukObject;
typedef struct AukListener AukListener;
typedef struct AukShared AukShared;

/* Update callback function type */
/* Parameters: listenerObject, modifiedObject */
typedef void (*AukUpdateCallback)(void* listenerObject, void* modifiedObject);

/* Listener node in linked list */
struct AukListener {
    AukShared* listenerObject;      /* Shared pointer to listener object */
    AukUpdateCallback callback;     /* Update notification callback */
    AukListener* next;              /* Next listener in list */
};

/* Base object vtable - all objects must implement these */
struct AukObject {
    /* Virtual methods - all take void* This as first parameter */
    void* (*New)(void);                    /* Constructor */
    void (*Delete)(void* This);            /* Destructor */
    const char* (*GetTypeName)(void* This); /* Get object type name */

    /* Listener management - inherited by all objects */
    int (*AddListener)(void* This, AukShared* listenerObject, AukUpdateCallback callback);
    int (*RemoveListener)(void* This, void* listenerObject);
    void (*SendUpdate)(void* This);        /* Notify all listeners of change */

    /* Listener list - managed by base object */
    AukListener* listeners;
};

/* Helper macros for calling virtual methods */
#define AUK_NEW(obj) ((obj)->New())
#define AUK_DELETE(obj, this) ((obj)->Delete(this))
#define AUK_GET_TYPE_NAME(obj, this) ((obj)->GetTypeName(this))
#define AUK_SEND_UPDATE(obj, this) ((obj)->SendUpdate(this))

/* Base object functions */
void* AukObject_New(void);
void AukObject_Delete(void* This);
const char* AukObject_GetTypeName(void* This);

/* Listener management functions */
int AukObject_AddListener(void* This, AukShared* listenerObject, AukUpdateCallback callback);
int AukObject_RemoveListener(void* This, void* listenerObject);
void AukObject_SendUpdate(void* This);

/* Initialize base object vtable */
void AukObject_Init(AukObject* obj);

#ifdef __cplusplus
}
#endif

#endif /* AUKOBJECT_H */
