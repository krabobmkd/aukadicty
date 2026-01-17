#ifndef AUKERRORS_H
#define AUKERRORS_H

#include <exec/types.h>

/*
    GUI Error/Log system for Aukadicty
    Provides centralized error reporting with localized messages.
*/

/* Log message levels */
typedef enum {
    AUKLOG_INFO = 0,    /* Informational message */
    AUKLOG_WARNING,     /* Warning - operation may have issues */
    AUKLOG_ERROR        /* Error - operation failed */
} AukLogLevel;

/* GUI Error/Log message IDs - these map to MSG_GUI_xxx in auklocale.h */
typedef enum {
    /* TrackListArea messages */
    AUKERR_TRACKLIST_CAPACITY_REACHED = 0,  /* Maximum track count reached */
    AUKERR_TRACKLIST_ALLOC_FAILED,          /* Track array allocation failed */
    AUKERR_TRACKLIST_INVALID_INDEX,         /* Invalid track index */
    AUKERR_TRACKLIST_INSERT_FAILED,         /* Failed to insert track */
    AUKERR_TRACKLIST_REMOVE_FAILED,         /* Failed to remove track */
    AUKERR_TRACKLIST_SWAP_FAILED,           /* Failed to swap tracks */

    /* General GUI messages */
    AUKERR_GUI_GADGET_CREATE_FAILED,        /* Failed to create gadget */
    AUKERR_GUI_LAYOUT_FAILED,               /* Layout operation failed */

    /* Must be last */
    AUKERR_COUNT
} AukErrorID;

/*
 * Log a GUI message with localization support.
 *
 * @param level    Log level (info/warning/error)
 * @param errorID  Error ID from AukErrorID enum
 *
 * The actual message text is retrieved from the locale system.
 * Messages are output via bdbprintf for debug builds.
 */
void AukLog_Message(AukLogLevel level, AukErrorID errorID);

/*
 * Log a GUI message with an additional integer parameter.
 *
 * @param level    Log level (info/warning/error)
 * @param errorID  Error ID from AukErrorID enum
 * @param param    Integer parameter (e.g., index value, count)
 */
void AukLog_MessageInt(AukLogLevel level, AukErrorID errorID, LONG param);

#endif /* AUKERRORS_H */
