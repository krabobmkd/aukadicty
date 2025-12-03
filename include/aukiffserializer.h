#ifndef AUKIFFSERIALIZER_H
#define AUKIFFSERIALIZER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "serializer.h"
#include <dos/dos.h>

/*
 * IFF Serializer Implementation
 * Uses IFF chunk format with big-endian byte order (Amiga standard)
 *
 * IFF Format Structure:
 * - FORM chunk (container)
 *   - Contains FourCC type identifier
 *   - Contains nested chunks for data
 * - Each chunk has: FourCC ID (4 bytes), Size (4 bytes big-endian), Data, [Pad byte if odd size]
 *
 * Chunk IDs used:
 * - FORM - Top-level container
 * - AUPJ - Aukadicty Project type
 * - OBJ  - Object chunk (contains __type and nested data)
 * - INT  - 32-bit signed integer
 * - UINT - 32-bit unsigned integer
 * - I64  - 64-bit signed integer
 * - U64  - 64-bit unsigned integer
 * - FIX  - 64-bit fixed-point (AukFixed)
 * - BOOL - Boolean (4 bytes)
 * - STR  - String (null-terminated)
 * - ARRY - Array of primitives
 */

/* Create an IFF writing serializer (for saving to file) */
ISerializer* AukIFFSerializer_CreateWriter(BPTR file);

/* Create an IFF reading serializer (for loading from file) */
ISerializer* AukIFFSerializer_CreateReader(BPTR file, const TypeNameToContructor* typeRegistry);

/* Finalize IFF writer - writes any pending data, returns 1 on success */
int AukIFFSerializer_Finalize(ISerializer* ser);

#ifdef __cplusplus
}
#endif

#endif /* AUKIFFSERIALIZER_H */
