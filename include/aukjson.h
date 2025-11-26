#ifndef AUKJSON_H
#define AUKJSON_H

/*
 * JSON serialization/deserialization for Aukadicty
 * Uses cJSON library for save/load operations
 */

#include "aukproject.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Save project to JSON file */
int AukJson_SaveProject(AukProject* project, const char* filename);

/* Load project from JSON file */
AukProject* AukJson_LoadProject(const char* filename);

/* Serialize project to JSON string (caller must free with AukString_Free) */
char* AukJson_SerializeProject(AukProject* project);

/* Deserialize project from JSON string */
AukProject* AukJson_DeserializeProject(const char* jsonString);

#ifdef __cplusplus
}
#endif

#endif /* AUKJSON_H */
