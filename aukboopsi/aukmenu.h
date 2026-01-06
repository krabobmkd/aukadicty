#ifndef AUKMENU_H
#define AUKMENU_H

#include <exec/types.h>
#include <intuition/intuition.h>

/*
    Menu management for Aukadicty using GadTools library
    Creates and manages the application menu bar
*/

/* Menu item IDs for identifying menu selections */
enum {
    /* Project menu */
    MENU_PROJECT_OPEN = 1,
    MENU_PROJECT_SAVE,
    MENU_PROJECT_SAVEAS,
    MENU_PROJECT_EXPORT,
    MENU_PROJECT_ABOUT,
    MENU_PROJECT_QUIT,

    /* Edition menu */
    MENU_EDIT_SELECTALL,
    MENU_EDIT_SELECTNONE,
    MENU_EDIT_COPY,
    MENU_EDIT_PASTE,

    /* Settings menu */
    MENU_SETTINGS_PREFERENCES,

    /* Plugins menu */
    MENU_PLUGINS_EFFECT,
    MENU_PLUGINS_GENERATE,

    /* Help menu */
    MENU_HELP_HELP,
    MENU_HELP_ABOUT
};

/* Menu state structure */
typedef struct AukMenu {
    struct Menu *menu;          /* GadTools menu structure */
    APTR visualInfo;            /* Visual info for menu rendering */
} AukMenu;

/* Create menus and attach to window */
BOOL AukMenu_Create(AukMenu *am, struct Screen *screen, struct Window *window);

/* Close and free menus */
void AukMenu_Close(AukMenu *am, struct Window *window);

/* Process menu selection - returns menu item ID or 0 */
ULONG AukMenu_HandleEvent(AukMenu *am, UWORD menuNumber);

#endif /* AUKMENU_H */
