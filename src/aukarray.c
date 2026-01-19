#include "aukarray.h"
#include <proto/exec.h>
#include <string.h>
#include <stdio.h>
#include "serializer.h"

/*
 * AukArray implementation
 * Base class for managing dynamic arrays of AukObjectPtr pointers
 */

#define INITIAL_ARRAY_CAPACITY 8
// AukObjectNewFunc itemNewFunc, const char* (*itemGetTypeName)(AukObject*)
void AukArray_New(AukObjectPtr* firstPtr) {
    AukArray* array;
    if (!firstPtr) return;
    array = (AukArray*)AllocVec(sizeof(AukArray), MEMF_CLEAR);
    if (array) {
        AukArray_Init(array);
        AukObjectPtr_Set(firstPtr, &array->base);
    }
}

void AukArray_Delete(AukObject* This) {
    AukArray* array = (AukArray*)This;
    unsigned int i;

    if (array) {
        /* Release all items */
        if (array->items) {
            for (i = 0; i < array->count; i++) {
                AukObjectPtr_Release(&array->items[i]);
            }
            FreeVec(array->items);
        }

        /* Call base object delete */
        AukObject_Delete(&array->base);
    }
}

const char* AukArray_GetTypeName(AukObject* This) {
    (void)This;
    return "AukArray";
}

void AukArray_Serialize(AukObject* This, ISerializer* ser, const char* pName) {
    AukArray* array = (AukArray*)This;
    unsigned int i;
    unsigned int count;
    char temp[32];
    AukObjectPtr itemPtr;
    (void)pName;

    if (!array || !ser) {
        return;
    }

    if (IS_WRITING(ser)) {
        /* Save array count and each object */
        ser->t_uint(ser, "count", &array->count);

        for (i = 0; i < array->count; i++) {
            snprintf(temp,31,"i%d",i);
            itemPtr = array->items[i];
            ser->t_object(ser, temp, &itemPtr);
        }
    } else {
        /* Load array items */
        count = 0;
        ser->t_uint(ser, "count", &count);

        for (i = 0; i < count; i++) {
            snprintf(temp,31,"i%d",i);
            itemPtr = NULL;
            ser->t_object(ser, temp, &itemPtr);
            if (itemPtr) {
                array->Add(array, itemPtr);
                /* Release our temporary reference - array now owns it */
                AukObjectPtr_Release(&itemPtr);
            }
        }
    }
}

void AukArray_Init(AukArray* array) {
    if (array) {
        /* Initialize base object */
        AukObject_Init(&array->base);

        /* Override virtual methods */
        array->base.Delete = AukArray_Delete;
        array->base.GetTypeName = AukArray_GetTypeName;
        array->base.Serialize = AukArray_Serialize;

        /* Set AukArray specific methods */
        array->Add = AukArray_Add;
        array->Remove = AukArray_Remove;
        array->RemoveAt = AukArray_RemoveAt;
        array->Get = AukArray_Get;
        array->GetCount = AukArray_GetCount;
        array->Insert = AukArray_Insert;
        array->Clear = AukArray_Clear;

        /* Initialize data members */
        array->items = NULL;
        array->count = 0;
        array->capacity = 0;

    }
}
void AukArray_SetType(AukArray* array,AukObjectNewFunc itemNewFunc, const char*typename)
{
    array->itemNewFunc = itemNewFunc;
    array->typename = typename;
}


int AukArray_EnsureCapacity(AukArray* array, unsigned int minCapacity) {
    AukObjectPtr* newItems;
    unsigned int newCapacity;

    if (!array || array->capacity >= minCapacity) {
        return 1;
    }

    /* Calculate new capacity (double until we reach minCapacity) */
    newCapacity = array->capacity == 0 ? INITIAL_ARRAY_CAPACITY : array->capacity * 2;
    while (newCapacity < minCapacity) {
        newCapacity *= 2;
    }

    /* Allocate new array */
    newItems = (AukObjectPtr*)AllocVec(newCapacity * sizeof(AukObjectPtr), MEMF_CLEAR);
    if (!newItems) {
        return 0;
    }

    /* Copy existing items */
    if (array->items) {
        memcpy(newItems, array->items, array->count * sizeof(AukObjectPtr));
        FreeVec(array->items);
    }

    array->items = newItems;
    array->capacity = newCapacity;

    return 1;
}

int AukArray_Add(void* This, AukObject* item) {
    AukArray* array = (AukArray*)This;

    if (!array || !item) {
        return 0;
    }

    aukMutex_lock(&array->mutex);

    /* Ensure capacity */
    if (!AukArray_EnsureCapacity(array, array->count + 1)) {
        aukMutex_unlock(&array->mutex);
        return 0;
    }

    /* Add item (retain reference) */
    AukObjectPtr_Set(&array->items[array->count], item);
    array->count++;

    aukMutex_unlock(&array->mutex);

    /* Send update notification */
    {
        AukMessage msg;
        msg.type = AUK_MSG_MODIFY;
        array->base.SendUpdate(&array->base, &msg);
    }

    return 1;
}

int AukArray_Remove(void* This, AukObject* item) {
    AukArray* array = (AukArray*)This;
    unsigned int i;

    if (!array || !item) {
        return 0;
    }

    aukMutex_lock(&array->mutex);
    /* Find and remove item */
    for (i = 0; i < array->count; i++) {
        if (array->items[i] == item) {
            aukMutex_unlock(&array->mutex);
            return AukArray_RemoveAt(This, i);
        }
    }
    aukMutex_unlock(&array->mutex);
    return 0;
}

int AukArray_RemoveAt(void* This, unsigned int index) {
    AukArray* array = (AukArray*)This;

    if (!array || index >= array->count) {
        return 0;
    }
    aukMutex_lock(&array->mutex);
    /* Release the item at index */
    AukObjectPtr_Release(&array->items[index]);

    /* Shift remaining items down */
    if (index < array->count - 1) {
        memcpy(&array->items[index],
               &array->items[index + 1],
               (array->count - index - 1) * sizeof(AukObjectPtr));
    }

    /* Clear the last slot (now duplicate after shift) */
    array->items[array->count - 1] = NULL;
    array->count--;

    aukMutex_unlock(&array->mutex);

    /* Send update notification */
    {
        AukMessage msg;
        msg.type = AUK_MSG_MODIFY;
        array->base.SendUpdate(&array->base, &msg);
    }

    return 1;
}

void AukArray_Get(void* This, AukObjectPtr *ptr, unsigned int index) {
    AukArray* array = (AukArray*)This;
    AukObject* o;

    AukObjectPtr_Release(ptr);

    if (!array ) {
        return;
    }

    aukMutex_lock(&array->mutex);
    if(  index >= array->count) {
        aukMutex_unlock(&array->mutex);
        return;
    }

    AukObjectPtr_Set(ptr,array->items[index]);

    aukMutex_unlock(&array->mutex);

}

unsigned int AukArray_GetCount(void* This) {
    unsigned int l=0;
    AukArray* array = (AukArray*)This;
    aukMutex_lock(&array->mutex);
    l = array ? array->count : 0;
    aukMutex_unlock(&array->mutex);
    return l;
}

int AukArray_Insert(void* This, unsigned int index, AukObject* item) {
    AukArray* array = (AukArray*)This;

    if (!array || !item || index > array->count) {
        return 0;
    }
    aukMutex_lock(&array->mutex);
    /* Ensure capacity */
    if (!AukArray_EnsureCapacity(array, array->count + 1)) {
        aukMutex_unlock(&array->mutex);
        return 0;
    }

    /* Shift items up to make room */
    if (index < array->count) {
        memmove(&array->items[index + 1],
                &array->items[index],
                (array->count - index) * sizeof(AukObjectPtr));
    }

    /* Insert item (retain reference) */
    array->items[index] = NULL; /* Clear before Set */
    AukObjectPtr_Set(&array->items[index], item);
    array->count++;


    aukMutex_unlock(&array->mutex);
    /* Send update notification */
    array->base.SendUpdate(&array->base, NULL);

    return 1;
}

void AukArray_Clear(void* This) {
    AukArray* array = (AukArray*)This;
    unsigned int i;

    if (!array) {
        return;
    }
    aukMutex_lock(&array->mutex);
    /* Release all items */
    if (array->items) {
        for (i = 0; i < array->count; i++) {
            AukObjectPtr_Release(&array->items[i]);
        }
    }

    array->count = 0;
    aukMutex_unlock(&array->mutex);
    /* Send update notification */
    {
        AukMessage msg;
        msg.type = AUK_MSG_MODIFY;
        array->base.SendUpdate(&array->base, &msg);
    }
}
