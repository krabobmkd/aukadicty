#ifndef AUKTYPEREGISTRY_H
#define AUKTYPEREGISTRY_H

/*
 * Type Registry for AukProject Object Graph
 * Maps type names to constructors for deserialization
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "serializer.h"

/* Get the type registry for AukProject and its contained objects */
const TypeNameToContructor* AukProject_GetTypeRegistry(void);

#ifdef __cplusplus
}
#endif

#endif /* AUKTYPEREGISTRY_H */
