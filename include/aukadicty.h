#ifndef AUKADICTY_H
#define AUKADICTY_H

/*
 * Aukadicty - Main header file
 * Include this file to access all Aukadicty functionality
 */

#ifdef __cplusplus
extern "C" {
#endif

/* Core object system */
#include "aukobject.h"
#include "aukshared.h"

/* Utility libraries */
#include "aukstring.h"
#include "aukfixed.h"

/* Data model objects */
#include "aukproject.h"
#include "auktrack.h"
#include "auksound.h"
#include "auksoundfile.h"

/* High-level operations */
#include "aukoperations.h"

/* JSON serialization */
#include "aukjson.h"

#ifdef __cplusplus
}
#endif

#endif /* AUKADICTY_H */
