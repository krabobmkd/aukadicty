#ifndef AUKARRAY_H
#define AUKARRAY_H

/*
 * AukArray - Base class for managing arrays of AukObjectPtr pointers
 * Provides dynamic array functionality with automatic reference counting
 * Derived classes specify what object type they contain
 */

#include "aukobject.h"

#ifdef __cplusplus
extern "C" {
#endif


/* AukArray structure - inherits from AukObject */
struct AukArray {
    AukObject base;          /* Must be first - inheritance */

    /* Data members */
    AukObjectPtr* items;     /* Array of object pointers */
    unsigned int count;      /* Current number of items */
    unsigned int capacity;   /* Allocated capacity */
    AukMutex    mutex;

    /* Object creation function pointer for this array type */
    AukObjectNewFunc itemNewFunc;      /* Function to create new items */

    /* TypeName for the item type */
    const char *typename;

    /* Virtual methods specific to AukArray */
    int (*Add)(void* This, AukObject* item);
    int (*Remove)(void* This, AukObject* item);
    int (*RemoveAt)(void* This, unsigned int index);
    void (*Get)(void* This, AukObjectPtr *ptr, unsigned int index);
    unsigned int (*GetCount)(void* This);
    int (*Insert)(void* This, unsigned int index, AukObject* item);
    void (*Clear)(void* This);
};

/* Constructor/Destructor */
/* Parameters: firstPtr, itemNewFunc, itemGetTypeName */
void AukArray_New(AukObjectPtr* firstPtr);
void AukArray_SetType(AukArray* array,AukObjectNewFunc itemNewFunc, const char*typename );

void AukArray_Delete(AukObject* This);
const char* AukArray_GetTypeName(AukObject* This);
void AukArray_Serialize(AukObject* This, ISerializer* ser, const char* pName);

/* Initialize AukArray structure */
void AukArray_Init(AukArray* array);

/* Methods */
int AukArray_Add(void* This, AukObject* item);
int AukArray_Remove(void* This, AukObject* item);
int AukArray_RemoveAt(void* This, unsigned int index);
/** retain indexed object to a pointer, so need a pointer inited to NULL, and a call to AukObjectPtr_Release() before pointer dies. */
void AukArray_Get(void* This, AukObjectPtr *ptr, unsigned int index);
unsigned int AukArray_GetCount(void* This);
int AukArray_Insert(void* This, unsigned int index, AukObject* item);
void AukArray_Clear(void* This);

/* Helper function to grow array capacity */
int AukArray_EnsureCapacity(AukArray* array, unsigned int minCapacity);

#ifdef __cplusplus
}
#endif

#endif /* AUKARRAY_H */
