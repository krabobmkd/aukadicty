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

    /* Action messages - Project */
    AUKERR_ACTION_PROJECT_NEW,              /* New project action */
    AUKERR_ACTION_PROJECT_CLEARED,          /* Project cleared */
    AUKERR_ACTION_PROJECT_OPEN,             /* Open project action */
    AUKERR_ACTION_PROJECT_OPEN_CANCELLED,   /* Open cancelled by user */
    AUKERR_ACTION_PROJECT_OPENED,           /* Project opened successfully */
    AUKERR_ACTION_PROJECT_SAVE,             /* Save project action */
    AUKERR_ACTION_PROJECT_SAVE_CANCELLED,   /* Save cancelled by user */
    AUKERR_ACTION_PROJECT_SAVED,            /* Project saved successfully */

    /* Action error messages */
    AUKERR_ACTION_NO_PROJECT,               /* No project in context */
    AUKERR_ACTION_NO_ASLBASE,               /* ASL library not initialized */
    AUKERR_ACTION_FILE_ALLOC_FAILED,        /* File requester alloc failed */
    AUKERR_ACTION_FILE_INVALID,             /* Invalid file selection */
    AUKERR_ACTION_FILE_OPEN_FAILED,         /* Failed to open file */
    AUKERR_ACTION_FILE_WRITE_FAILED,        /* Failed to write file */
    AUKERR_ACTION_IFF_READ_FAILED,          /* Failed to create IFF reader */
    AUKERR_ACTION_IFF_WRITE_FAILED,         /* Failed to create IFF writer */
    AUKERR_ACTION_IFF_FINALIZE_FAILED,      /* Failed to finalize IFF */

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

/*
 * Log a GUI message with an additional string parameter.
 *
 * @param level    Log level (info/warning/error)
 * @param errorID  Error ID from AukErrorID enum
 * @param param    String parameter (e.g., filename)
 */
void AukLog_MessageStr(AukLogLevel level, AukErrorID errorID, const char *param);

#endif /* AUKERRORS_H */
