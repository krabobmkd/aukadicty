
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
    [MSG_EDITMODE_1] = "1",
    [MSG_EDITMODE_2] = "2",
    [MSG_EDITMODE_3] = "3",
    [MSG_EDITMODE_4] = "4",
    [MSG_EDITMODE_5] = "5",
    [MSG_EDITMODE_6] = "6",

    /* Footer view */
    [MSG_FOOTER_FREQUENCY] = "%lu Hz",
    [MSG_FOOTER_SELECTION_START] = "Start: %s",
    [MSG_FOOTER_SELECTION_END] = "End: %s",
    [MSG_FOOTER_PLAYPOSITION] = "Pos: %s",

    /* Track operations */
    [MSG_TRACK_NAME] = "Track",
    [MSG_TRACK_VOCALS] = "Vocals",
    [MSG_TRACK_MUSIC] = "Music",

    /* Menu: Project */
    [MSG_MENU_PROJECT] = "Project",
    [MSG_FILE_OPEN] = "Open",
    [MSG_FILE_SAVEAS] = "Save as",
    [MSG_FILE_SAVE] = "Save",
    [MSG_FILE_EXPORT] = "Export",
    [MSG_MENU_ABOUT] = "About",
    [MSG_MENU_QUIT] = "Quit",

    /* Menu: Edition */
    [MSG_MENU_EDITION] = "Edition",
    [MSG_EDIT_SELECTALL] = "Select All",
    [MSG_EDIT_SELECTNONE] = "Select None",
    [MSG_EDIT_COPY] = "Copy",
    [MSG_EDIT_CUT] = "Cut",
    [MSG_EDIT_PASTE] = "Paste",

    /* Menu: Tracks */
    [MSG_MENU_TRACKS] = "Tracks",
    [MSG_TRACKS_ADD] = "Add",

    /* Menu: Generate */
    [MSG_MENU_GENERATE] = "Generate",

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
