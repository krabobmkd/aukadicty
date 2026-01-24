#ifndef AUKLOCALE_H
#define AUKLOCALE_H

#include <exec/types.h>

/*
    Localization support for Aukadicty using AmigaOS locale.library
    Uses catalog description (.cd) files for translation management.
*/

/* String IDs for all localizable strings in the GUI */
enum {
    /* Window and general UI */
    MSG_WINDOW_TITLE = 0,
    MSG_ABOUT_TITLE,
    MSG_ABOUT_TEXT,

    /* Transport controls */
    MSG_TRANSPORT_REWIND,
    MSG_TRANSPORT_STOP,
    MSG_TRANSPORT_PLAY,
    MSG_TRANSPORT_PAUSE,
    MSG_TRANSPORT_FORWARD,

    /* Edit mode buttons */
    MSG_EDITMODE_1,
    MSG_EDITMODE_2,
    MSG_EDITMODE_3,
    MSG_EDITMODE_4,
    MSG_EDITMODE_5,
    MSG_EDITMODE_6,

    /* Footer view */
    MSG_FOOTER_FREQUENCY,
    MSG_FOOTER_SELECTION_START,
    MSG_FOOTER_SELECTION_END,
    MSG_FOOTER_PLAYPOSITION,

    /* Track operations */
    MSG_TRACK_NAME,
    MSG_TRACK_VOCALS,
    MSG_TRACK_MUSIC,
    MSG_TRACK_RENAME_TITLE,
    MSG_TRACK_RENAME_PROMPT,

    /* Menu: Project */
    MSG_MENU_PROJECT,
    MSG_FILE_NEW,
    MSG_FILE_OPEN,
    MSG_FILE_SAVEAS,
    MSG_FILE_SAVE,
    MSG_FILE_EXPORT,
    MSG_MENU_ABOUT,
    MSG_MENU_QUIT,

    /* Menu: Edition */
    MSG_MENU_EDITION,
    MSG_EDIT_UNDO,
    MSG_EDIT_REDO,
    MSG_EDIT_SELECTALL,
    MSG_EDIT_SELECTNONE,
    MSG_EDIT_COPY,
    MSG_EDIT_CUT,
    MSG_EDIT_PASTE,

    /* Menu: Tracks */
    MSG_MENU_TRACKS,
    MSG_TRACKS_ADD,

    /* Menu: Generate */
    MSG_MENU_GENERATE,

    /* Menu: Settings */
    MSG_MENU_SETTINGS,
    MSG_SETTINGS_PROJECT,
    MSG_SETTINGS_VIEW,

    /* Menu: Help */
    MSG_MENU_HELP,

    /* Status messages */
    MSG_STATUS_READY,
    MSG_STATUS_LOADING,
    MSG_STATUS_SAVING,
    MSG_STATUS_ERROR,

    /* Error messages */
    MSG_ERROR_OPENFILE,
    MSG_ERROR_SAVEFILE,
    MSG_ERROR_NOMEMORY,
    MSG_ERROR_INVALIDFILE,

    /* GUI Error/Log messages (used by aukerrors.h) */
    MSG_GUI_TRACKLIST_CAPACITY_REACHED,
    MSG_GUI_TRACKLIST_ALLOC_FAILED,
    MSG_GUI_TRACKLIST_INVALID_INDEX,
    MSG_GUI_TRACKLIST_INSERT_FAILED,
    MSG_GUI_TRACKLIST_REMOVE_FAILED,
    MSG_GUI_TRACKLIST_SWAP_FAILED,
    MSG_GUI_GADGET_CREATE_FAILED,
    MSG_GUI_LAYOUT_FAILED,

    /* Action messages - Project */
    MSG_GUI_ACTION_PROJECT_NEW,
    MSG_GUI_ACTION_PROJECT_CLEARED,
    MSG_GUI_ACTION_PROJECT_OPEN,
    MSG_GUI_ACTION_PROJECT_OPEN_CANCELLED,
    MSG_GUI_ACTION_PROJECT_OPENED,
    MSG_GUI_ACTION_PROJECT_SAVE,
    MSG_GUI_ACTION_PROJECT_SAVE_CANCELLED,
    MSG_GUI_ACTION_PROJECT_SAVED,

    /* Action error messages */
    MSG_GUI_ACTION_NO_PROJECT,
    MSG_GUI_ACTION_NO_ASLBASE,
    MSG_GUI_ACTION_FILE_ALLOC_FAILED,
    MSG_GUI_ACTION_FILE_INVALID,
    MSG_GUI_ACTION_FILE_OPEN_FAILED,
    MSG_GUI_ACTION_FILE_WRITE_FAILED,
    MSG_GUI_ACTION_IFF_READ_FAILED,
    MSG_GUI_ACTION_IFF_WRITE_FAILED,
    MSG_GUI_ACTION_IFF_FINALIZE_FAILED,

    /* Must be last */
    MSG_COUNT
};

/* Initialize locale system - opens locale.library and loads catalog */
BOOL AukLocale_Init(const char *catalogName, ULONG version);

/* Close locale system - closes catalog and library */
void AukLocale_Close(void);

/* Get localized string by ID - returns default English string if catalog not loaded */
const char *AukLocale_GetString(ULONG stringID);

/* Convenience macro for getting strings */
#define LOC(id) AukLocale_GetString(id)

#endif /* AUKLOCALE_H */
