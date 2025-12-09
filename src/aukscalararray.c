#include "aukscalararray.h"
#include <proto/exec.h>
#include <string.h>

/*
 * AukScalarArray implementation
 * Efficient multidimensional arrays for scalar types
 */

unsigned int AukScalarArray_GetScalarSize(AukScalarType type) {
    switch (type) {
        case AUK_SCALAR_CHAR:
            return 1;
        case AUK_SCALAR_SHORT:
            return 2;
        case AUK_SCALAR_INT:
            return 4;
        case AUK_SCALAR_LONGLONG:
            return 8;
        default:
            return 0;
    }
}

unsigned int AukScalarArray_ComputeTotalElements(const unsigned int* shape, short ndim) {
    unsigned int total = 1;
    short i;

    if (!shape || ndim <= 0) {
        return 0;
    }

    for (i = 0; i < ndim; i++) {
        total *= shape[i];
    }

    return total;
}

void AukScalarArray_ComputeStrides(AukScalarArray* array) {
    short i;
    unsigned int stride;

    if (!array || !array->strides || !array->shape) {
        return;
    }

    /* Strides in row-major order (C-style) */
    stride = 1;
    for (i = array->ndim - 1; i >= 0; i--) {
        array->strides[i] = stride;
        stride *= array->shape[i];
    }
}

AukScalarArray* AukScalarArray_New(AukScalarType scalarType, short ndim, const unsigned int* shape) {
    AukScalarArray* array;
    unsigned int scalarSize;
    unsigned int totalElements;
    unsigned int dataSize;

    if (ndim <= 0 || !shape) {
        return NULL;
    }

    array = (AukScalarArray*)AllocVec(sizeof(AukScalarArray), MEMF_CLEAR);
    if (!array) {
        return NULL;
    }

    /* Allocate shape array */
    array->shape = (unsigned int*)AllocVec(ndim * sizeof(unsigned int), MEMF_CLEAR);
    if (!array->shape) {
        FreeVec(array);
        return NULL;
    }

    /* Allocate strides array */
    array->strides = (unsigned int*)AllocVec(ndim * sizeof(unsigned int), MEMF_CLEAR);
    if (!array->strides) {
        FreeVec(array->shape);
        FreeVec(array);
        return NULL;
    }

    /* Copy shape */
    memcpy(array->shape, shape, ndim * sizeof(unsigned int));

    /* Set basic properties */
    array->scalarType = scalarType;
    array->ndim = ndim;

    /* Compute total elements */
    totalElements = AukScalarArray_ComputeTotalElements(shape, ndim);
    array->totalElements = totalElements;

    /* Compute strides */
    AukScalarArray_ComputeStrides(array);

    /* Allocate data buffer */
    scalarSize = AukScalarArray_GetScalarSize(scalarType);
    dataSize = totalElements * scalarSize;

    if (dataSize > 0) {
        array->data = AllocVec(dataSize, MEMF_CLEAR);
        if (!array->data) {
            FreeVec(array->strides);
            FreeVec(array->shape);
            FreeVec(array);
            return NULL;
        }
    } else {
        array->data = NULL;
    }

    return array;
}

void AukScalarArray_Delete(AukScalarArray* array) {
    if (!array) {
        return;
    }

    if (array->data) {
        FreeVec(array->data);
    }

    if (array->strides) {
        FreeVec(array->strides);
    }

    if (array->shape) {
        FreeVec(array->shape);
    }

    FreeVec(array);
}

unsigned int AukScalarArray_IndicesToFlat(AukScalarArray* array, const unsigned int* indices) {
    unsigned int flatIndex = 0;
    short i;

    if (!array || !indices) {
        return 0;
    }

    for (i = 0; i < array->ndim; i++) {
        flatIndex += indices[i] * array->strides[i];
    }

    return flatIndex;
}

void* AukScalarArray_GetElement(AukScalarArray* array, const unsigned int* indices) {
    unsigned int flatIndex;
    unsigned int scalarSize;
    unsigned char* dataPtr;

    if (!array || !array->data || !indices) {
        return NULL;
    }

    flatIndex = AukScalarArray_IndicesToFlat(array, indices);

    if (flatIndex >= array->totalElements) {
        return NULL;
    }

    scalarSize = AukScalarArray_GetScalarSize(array->scalarType);
    dataPtr = (unsigned char*)array->data;

    return (void*)(dataPtr + flatIndex * scalarSize);
}

void AukScalarArray_SetElement(AukScalarArray* array, const unsigned int* indices, const void* value) {
    void* element;
    unsigned int scalarSize;

    if (!array || !value) {
        return;
    }

    element = AukScalarArray_GetElement(array, indices);
    if (!element) {
        return;
    }

    scalarSize = AukScalarArray_GetScalarSize(array->scalarType);
    memcpy(element, value, scalarSize);
}

void* AukScalarArray_GetElementFlat(AukScalarArray* array, unsigned int flatIndex) {
    unsigned int scalarSize;
    unsigned char* dataPtr;

    if (!array || !array->data || flatIndex >= array->totalElements) {
        return NULL;
    }

    scalarSize = AukScalarArray_GetScalarSize(array->scalarType);
    dataPtr = (unsigned char*)array->data;

    return (void*)(dataPtr + flatIndex * scalarSize);
}

void AukScalarArray_SetElementFlat(AukScalarArray* array, unsigned int flatIndex, const void* value) {
    void* element;
    unsigned int scalarSize;

    if (!array || !value) {
        return;
    }

    element = AukScalarArray_GetElementFlat(array, flatIndex);
    if (!element) {
        return;
    }

    scalarSize = AukScalarArray_GetScalarSize(array->scalarType);
    memcpy(element, value, scalarSize);
}
