
#include <stdio.h>
#include <string.h>

#include <proto/exec.h>
#include <proto/locale.h>
#include <libraries/locale.h>

#include "auklocale.h"
#include "compilers.h"

/* Default English strings - used as fallback if catalog not available */
static const char *defaultStrings[MSG_COUNT] = {
    /* Window and general UI */
    [MSG_WINDOW_TITLE] = "Aukadicty",
    [MSG_ABOUT_TITLE] = "About Aukadicty",
    [MSG_ABOUT_TEXT] = "Aukadicty Audio Editor\nVersion 0.1",

    /* Transport controls */
    [MSG_TRANSPORT_REWIND] = "|<<",
    [MSG_TRANSPORT_STOP] = "[]",
    [MSG_TRANSPORT_PLAY] = ">",
    [MSG_TRANSPORT_PAUSE] = "||",
    [MSG_TRANSPORT_FORWARD] = ">>|",

    /* Edit mode buttons */
    [MSG_EDITMODE_SELECTTOOL] = "Select",
    [MSG_EDITMODE_VOLUMEENV] = "Volume",
    [MSG_EDITMODE_COPY] = "Copy",
    [MSG_EDITMODE_ZOOMTOOL] = "Zoom",
    [MSG_EDITMODE_TIMESLIDE] = "Slide",
    [MSG_EDITMODE_PASTE] = "Paste",

    /* Footer view */
    [MSG_FOOTER_FREQUENCY] = "%lu Hz",
    [MSG_FOOTER_SELECTION_START] = "Start: %s",
    [MSG_FOOTER_SELECTION_END] = "End: %s",
    [MSG_FOOTER_PLAYPOSITION] = "Pos: %s",

    /* Track operations */
    [MSG_TRACK_NAME] = "Track",
    [MSG_TRACK_VOCALS] = "Vocals",
    [MSG_TRACK_MUSIC] = "Music",
    [MSG_TRACK_RENAME_TITLE] = "Rename Track",
    [MSG_TRACK_RENAME_PROMPT] = "Enter a new name for the track:",

    /* Menu: Project */
    [MSG_MENU_PROJECT] = "Project",
    [MSG_FILE_NEW] = "New",
    [MSG_FILE_OPEN] = "Open",
    [MSG_FILE_SAVEAS] = "Save as",
    [MSG_FILE_SAVE] = "Save",
    [MSG_FILE_EXPORT] = "Export",
    [MSG_MENU_ABOUT] = "About",
    [MSG_MENU_QUIT] = "Quit",

    /* Menu: Edition */
    [MSG_MENU_EDITION] = "Edition",
    [MSG_EDIT_UNDO] = "Undo",
    [MSG_EDIT_REDO] = "Redo",
    [MSG_EDIT_SELECTALL] = "Select All",
    [MSG_EDIT_SELECTNONE] = "Select None",
    [MSG_EDIT_COPY] = "Copy",
    [MSG_EDIT_CUT] = "Cut",
    [MSG_EDIT_PASTE] = "Paste",

    /* Menu: View */
    [MSG_MENU_VIEW] = "View",
    [MSG_VIEW_ZOOMIN] = "Zoom In",
    [MSG_VIEW_ZOOMOUT] = "Zoom Out",
    [MSG_VIEW_ZOOM_PROJECT] = "Zoom to Project",
    [MSG_VIEW_COLLAPSE_TRACKS] = "Collapse Tracks",
    [MSG_VIEW_EXPAND_TRACKS] = "Expand Tracks",
    [MSG_VIEW_SWITCH_FULLSCREEN] = "Toggle Full Screen",
    [MSG_VIEW_ICONIFY] = "Iconify",

    /* Menu: Modify */
    [MSG_MENU_MODIFY] = "Modify",

    /* Menu: Generate */
    [MSG_MENU_GENERATE] = "Generate",

    /* Menu: Tracks (items moved to Edition) */
    [MSG_TRACKS_ADD] = "Add Track",

    /* Menu: Settings */
    [MSG_MENU_SETTINGS] = "Settings",
    [MSG_SETTINGS_PROJECT] = "Project Settings",
    [MSG_SETTINGS_VIEW] = "View Settings",

    /* Menu: Help */
    [MSG_MENU_HELP] = "Help",

    /* Status messages */
    [MSG_STATUS_READY] = "Ready",
    [MSG_STATUS_LOADING] = "Loading...",
    [MSG_STATUS_SAVING] = "Saving...",
    [MSG_STATUS_ERROR] = "Error",

    /* Error messages */
    [MSG_ERROR_OPENFILE] = "Cannot open file",
    [MSG_ERROR_SAVEFILE] = "Cannot save file",
    [MSG_ERROR_NOMEMORY] = "Out of memory",
    [MSG_ERROR_INVALIDFILE] = "Invalid file format",

    /* GUI Error/Log messages */
    [MSG_GUI_TRACKLIST_CAPACITY_REACHED] = "Maximum track count reached",
    [MSG_GUI_TRACKLIST_ALLOC_FAILED] = "Track array allocation failed",
    [MSG_GUI_TRACKLIST_INVALID_INDEX] = "Invalid track index",
    [MSG_GUI_TRACKLIST_INSERT_FAILED] = "Failed to insert track",
    [MSG_GUI_TRACKLIST_REMOVE_FAILED] = "Failed to remove track",
    [MSG_GUI_TRACKLIST_SWAP_FAILED] = "Failed to swap tracks",
    [MSG_GUI_GADGET_CREATE_FAILED] = "Failed to create gadget",
    [MSG_GUI_LAYOUT_FAILED] = "Layout operation failed",
};

/* Global locale state */
static struct Catalog *catalog = NULL;

BOOL AukLocale_Init(const char *catalogName, ULONG version)
{
    if(LocaleBase)
    {
        /* Try to open the catalog */
        if (catalogName) {
            catalog = OpenCatalog(NULL, (STRPTR)catalogName,
                                 OC_Version, version,
                                 OC_BuiltInLanguage, (ULONG)"english",
                                 TAG_DONE);

            if (!catalog) {
                printf("Warning: Could not open catalog '%s', using default strings\n", catalogName);
                /* Not a fatal error - we still have defaults */
            } else {
                printf("Catalog '%s' opened successfully\n", catalogName);
            }
        }
    }else
    {
        //
        printf("locale.library not found\n");
    }

    return TRUE;
}

void AukLocale_Close(void)
{
    if (catalog) {
        CloseCatalog(catalog);
        catalog = NULL;
    }

}

const char *AukLocale_GetString(ULONG stringID)
{
    const char *str;

    /* Validate string ID */
    if (stringID >= MSG_COUNT) {
        return "???";
    }

    /* Try to get from catalog first */
    if (LocaleBase && catalog) {
        str = GetCatalogStr(catalog, stringID, (STRPTR)defaultStrings[stringID]);
        return str;
    } else
    {
        return defaultStrings[stringID];
    }

    /* Fall back to default English string */
    return defaultStrings[stringID];
}
