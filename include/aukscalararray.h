#ifndef AUKSCALARARRAY_H
#define AUKSCALARARRAY_H

/*
 * AukScalarArray - Efficient multidimensional array for scalar types
 * Used for compact serialization of vectors, matrices, and tensors
 * Does not use AukObject overhead - plain data structure
 */

#ifdef __cplusplus
extern "C" {
#endif

/* Scalar type enumeration - by size */
typedef enum {
    AUK_SCALAR_CHAR = 0,      /* 1 byte signed */
    AUK_SCALAR_SHORT = 1,     /* 2 bytes signed */
    AUK_SCALAR_INT = 2,       /* 4 bytes signed */
    AUK_SCALAR_LONGLONG = 3   /* 8 bytes signed */
} AukScalarType;

/* ScalarArray structure */
typedef struct AukScalarArray {
    short scalarType;  /* Type of scalar elements */
    short ndim;                /* Number of dimensions */
    unsigned int* shape;       /* Array of dimension sizes [ndim] */
    unsigned int* strides;     /* Stride per dimension [ndim] (computed) */
    void* data;                /* Actual data buffer */
    unsigned int totalElements; /* Total number of elements (computed) */
} AukScalarArray;

/* Get size in bytes for a scalar type */
unsigned int AukScalarArray_GetScalarSize(AukScalarType type);

/* Create a new scalar array */
AukScalarArray* AukScalarArray_New(AukScalarType scalarType, short ndim, const unsigned int* shape);

/* Delete scalar array */
void AukScalarArray_Delete(AukScalarArray* array);

/* Get element at multidimensional index */
void* AukScalarArray_GetElement(AukScalarArray* array, const unsigned int* indices);

/* Set element at multidimensional index */
void AukScalarArray_SetElement(AukScalarArray* array, const unsigned int* indices, const void* value);

/* Get element at flat index */
void* AukScalarArray_GetElementFlat(AukScalarArray* array, unsigned int flatIndex);

/* Set element at flat index */
void AukScalarArray_SetElementFlat(AukScalarArray* array, unsigned int flatIndex, const void* value);

/* Compute strides from shape (internal use) */
void AukScalarArray_ComputeStrides(AukScalarArray* array);

/* Compute total elements from shape (internal use) */
unsigned int AukScalarArray_ComputeTotalElements(const unsigned int* shape, short ndim);

/* Helper: convert multidimensional indices to flat index */
unsigned int AukScalarArray_IndicesToFlat(AukScalarArray* array, const unsigned int* indices);

#ifdef __cplusplus
}
#endif

#endif /* AUKSCALARARRAY_H */
