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

#include "aukdefs.h"
#include "aukfixed.h"

/* TypeNameToContructor structure -
 * reading serializers will need a table of this to be able to reconstruct objects from their type names.
 * sTypeNameToContructor tables will be terminated with last member typename being NULL.  */
typedef struct sTypeNameToContructor {
    const char *typename;
    void (*NewConstructor)(AukObjectPtr *firstPtr);
} TypeNameToContructor;

/* ISerializer structure - function pointers for each type */
struct ISerializer {
    /* Context data */
    void* context;              /* Implementation-specific context (e.g., cJSON object, FILE*) */
    int _isReading;             /* 1 if deserializing (loading), 0 if serializing (saving) */

    /* Type registry for reading (NULL for writing serializers) */
    const TypeNameToContructor* typeRegistry;

    /* Primitive types */
    void (*t_int)(struct ISerializer* This, const char* name, int* value);
    void (*t_uint)(struct ISerializer* This, const char* name, unsigned int* value);
    void (*t_longlong)(struct ISerializer* This, const char* name, long long* value);
    void (*t_ulonglong)(struct ISerializer* This, const char* name, unsigned long long* value);
    void (*t_fixed)(struct ISerializer* This, const char* name, AukFixed* value);
    void (*t_bool)(struct ISerializer* This, const char* name, int* value);

    /* String types */
    void (*t_string)(struct ISerializer* This, const char* name, const char** value);
    void (*t_string_mutable)(struct ISerializer* This, const char* name, char** value);

    /* Object types */
    void (*t_object)(struct ISerializer* This, const char* name, AukObjectPtr* object);
    void (*t_arrayobj)(struct ISerializer* This, const char* name, AukArray** array,
                AukObjectNewFunc itemNewFunc, const char* classname);

    /* Scalar array - efficient multidimensional arrays */
    void (*t_scalararray)(struct ISerializer* This, const char* name, AukScalarArray** array);

    /* Cleanup */
    void (*Destroy)(struct ISerializer* This);

};

/* Check if serializer is in read or write mode */
#define IS_READING(ser) ((ser)->_isReading)
#define IS_WRITING(ser) (!(ser)->_isReading)

#ifdef __cplusplus
}
#endif

#endif /* SERIALIZER_H */
