#ifndef AUKJSONSERIALIZER_H
#define AUKJSONSERIALIZER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "serializer.h"
#include "cJSON.h"

/*
 * JSON Serializer Implementation
 * Uses cJSON library for JSON reading/writing
 */

/* Create a JSON writing serializer (for saving) */
ISerializer* AukJsonSerializer_CreateWriter(void);

/* Create a JSON reading serializer (for loading) */
ISerializer* AukJsonSerializer_CreateReader(const char* jsonString, const TypeNameToContructor* typeRegistry);

/* Get the resulting JSON string from a writer (caller must FreeVec the result) */
char* AukJsonSerializer_GetString(ISerializer* ser);

/* Get the root cJSON object from a writer (for advanced usage) */
cJSON* AukJsonSerializer_GetRoot(ISerializer* ser);

#ifdef __cplusplus
}
#endif

#endif /* AUKJSONSERIALIZER_H */
