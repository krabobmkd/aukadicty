#ifndef SERIALIZER_H
#define SERIALIZER_H

/*
 * ISerializer - Abstract serializer interface
 * Provides function pointers for serializing/deserializing different types
 * Used by AukObject::Serialize() methods for both saving and loading
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "aukfixed.h"

/* Forward declarations */
struct AukObject;
typedef struct AukObject AukObject;
typedef AukObject* AukObjectPtr;
/* Function pointer type for creating new objects of a specific type */
typedef void (*AukObjectNewFunc)(AukObjectPtr* firstPtr);

struct AukArray;
typedef struct AukArray AukArray;

struct AukScalarArray;
typedef struct AukScalarArray AukScalarArray;

/* TypeNameToContructor structure -
 * reading serializers will need a table of this to be able to reconstruct objects from their type names.
 * sTypeNameToContructor tables will be terminated with last member typename being NULL.  */
typedef struct sTypeNameToContructor {
    const char *typename;
    void (*NewConstructor)(AukObjectPtr *firstPtr);
} TypeNameToContructor;

/* ISerializer structure - function pointers for each type */
typedef struct sISerializer {
    /* Context data */
    void* context;              /* Implementation-specific context (e.g., cJSON object, FILE*) */
    int _isReading;             /* 1 if deserializing (loading), 0 if serializing (saving) */

    /* Type registry for reading (NULL for writing serializers) */
    const TypeNameToContructor* typeRegistry;

    /* Primitive types */
    void (*t_int)(struct sISerializer* This, const char* name, int* value);
    void (*t_uint)(struct sISerializer* This, const char* name, unsigned int* value);
    void (*t_longlong)(struct sISerializer* This, const char* name, long long* value);
    void (*t_ulonglong)(struct sISerializer* This, const char* name, unsigned long long* value);
    void (*t_fixed)(struct sISerializer* This, const char* name, AukFixed* value);
    void (*t_bool)(struct sISerializer* This, const char* name, int* value);

    /* String types */
    void (*t_string)(struct sISerializer* This, const char* name, const char** value);
    void (*t_string_mutable)(struct sISerializer* This, const char* name, char** value);

    /* Object types */
    void (*t_object)(struct sISerializer* This, const char* name, AukObjectPtr* object);
    void (*t_arrayobj)(struct sISerializer* This, const char* name, AukArray** array,
                AukObjectNewFunc itemNewFunc, const char* (*itemGetTypeName)(AukObject*));

    /* Scalar array - efficient multidimensional arrays */
    void (*t_scalararray)(struct sISerializer* This, const char* name, AukScalarArray** array);

    /* Cleanup */
    void (*Destroy)(struct sISerializer* This);

} ISerializer;

/* Helper macros for cleaner syntax in Serialize methods */
#define SERIALIZE_INT(ser, name, value) \
    if ((ser)->t_int) (ser)->t_int((ser), (name), (value))

#define SERIALIZE_UINT(ser, name, value) \
    if ((ser)->t_uint) (ser)->t_uint((ser), (name), (value))

#define SERIALIZE_LONGLONG(ser, name, value) \
    if ((ser)->t_longlong) (ser)->t_longlong((ser), (name), (value))

#define SERIALIZE_ULONGLONG(ser, name, value) \
    if ((ser)->t_ulonglong) (ser)->t_ulonglong((ser), (name), (value))

#define SERIALIZE_FIXED(ser, name, value) \
    if ((ser)->t_fixed) (ser)->t_fixed((ser), (name), (value))

#define SERIALIZE_STRING(ser, name, value) \
    if ((ser)->t_string) (ser)->t_string((ser), (name), (value))

#define SERIALIZE_OBJECT(ser, name, object) \
    if ((ser)->t_object) (ser)->t_object((ser), (name), (object))

#define SERIALIZE_ARRAY(ser, name, array) \
    if ((ser)->t_arrayobj) (ser)->t_arrayobj((ser), (name), (array))

/* Check if serializer is in read or write mode */
#define IS_READING(ser) ((ser)->_isReading)
#define IS_WRITING(ser) (!(ser)->_isReading)

#ifdef __cplusplus
}
#endif

#endif /* SERIALIZER_H */
