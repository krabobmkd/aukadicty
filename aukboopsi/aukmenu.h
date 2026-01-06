#ifndef AUKMENU_H
#define AUKMENU_H

#include <exec/types.h>
#include <intuition/intuition.h>
#include "aukaction.h"

/*
    Menu management for Aukadicty using GadTools library
    Creates and manages the application menu bar
    Menu items are linked to actions defined in aukaction.h
*/

/* Menu state structure */
typedef struct AukMenu {
    struct Menu *menu;          /* GadTools menu structure */
    APTR visualInfo;            /* Visual info for menu rendering */
} AukMenu;

/* Create menus and attach to window */
BOOL AukMenu_Create(AukMenu *am, struct Screen *screen, struct Window *window);

/* Close and free menus */
void AukMenu_Close(AukMenu *am, struct Window *window);

/* Process menu selection - returns action ID or -1 if none */
LONG AukMenu_HandleEvent(AukMenu *am, UWORD menuNumber);

#endif /* AUKMENU_H */
