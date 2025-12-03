/*
 * Type Registry Implementation
 * Provides mapping from type names to constructors for deserialization
 */

#include "auktyperegistry.h"
#include "aukproject.h"
#include "auktrack.h"
#include "auksound.h"
#include "auksoundfile.h"
#include "aukarray.h"

/* Type registry for AukProject object graph
 * Last entry must have typename = NULL to terminate the table */
static const TypeNameToContructor g_projectTypeRegistry[] = {
    { "AukProject", (void (*)(AukObjectPtr*))AukProject_New },
    { "AukProjectPrefs", (void (*)(AukObjectPtr*))AukProjectPrefs_New },
    { "AukTrack", (void (*)(AukObjectPtr*))AukTrack_New },
    { "AukSound", (void (*)(AukObjectPtr*))AukSound_New },
    { "AukSoundFile", (void (*)(AukObjectPtr*))AukSoundFile_New },
    { "AukArray", (void (*)(AukObjectPtr*))AukArray_New },
    { NULL, NULL }  /* Terminator */
};

const TypeNameToContructor* AukProject_GetTypeRegistry(void) {
    return g_projectTypeRegistry;
}
