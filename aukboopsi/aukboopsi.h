#ifndef AUKBOOPSI_H
#define AUKBOOPSI_H

/*
    Main header for Aukadicty BOOPSI GUI module
    Exposes public functions for use by other modules
*/

#include <exec/types.h>
#include "aukerrors.h"

/* Logging functions - display messages with localization support */

/*
 * Log a GUI message with localization support.
 * @param level    Log level (AUKLOG_INFO, AUKLOG_WARNING, AUKLOG_ERROR)
 * @param errorID  Error ID from AukErrorID enum
 */
void AukLog_Message(AukLogLevel level, AukErrorID errorID);

/*
 * Log a GUI message with an additional integer parameter.
 * @param level    Log level
 * @param errorID  Error ID from AukErrorID enum
 * @param param    Integer parameter (e.g., index value, count)
 */
void AukLog_MessageInt(AukLogLevel level, AukErrorID errorID, LONG param);

/*
 * Log a GUI message with an additional string parameter.
 * @param level    Log level
 * @param errorID  Error ID from AukErrorID enum
 * @param param    String parameter (e.g., filename)
 */
void AukLog_MessageStr(AukLogLevel level, AukErrorID errorID, const char *param);

#endif /* AUKBOOPSI_H */
