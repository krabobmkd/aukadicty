
#include <stdio.h>
#include <string.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/gadtools.h>
#include <libraries/gadtools.h>

#include "aukmenu.h"
#include "aukaction.h"
#include "compilers.h"

#include "auklocale.h"

extern struct Library *GadToolsBase;
/*
struct NewMenu
{
    UBYTE nm_Type;		/* See below
     Compiler inserts a PAD byte here
    CONST_STRPTR nm_Label;	/* Menu's label
    CONST_STRPTR nm_CommKey;	/* MenuItem Command Key Equiv
    UWORD nm_Flags;		/* Menu or MenuItem flags (see note)
    LONG nm_MutualExclude;	/* MenuItem MutualExclude word
    APTR nm_UserData;		/* For your own use, see note
};
*/
/* NewMenu template for GadTools menus
   UserData points to action ID (cast to APTR)
   Labels will be filled in from action names during initialization
   Watch out:
    - NM_TITLE (menu entry with just name) nm_UserData is the message enum
    - NM_ITEM (menu entry with an actual action) nm_UserData is the action enum, action already hold the name.

*/
static struct NewMenu menuTemplate[] = {
    /* Project menu */
    {NM_TITLE, NULL, 0, 0, 0, (APTR)MSG_MENU_PROJECT},  /* Title uses string ID */
        {NM_ITEM, NULL, 0, 0, 0, (APTR)ACTION_PROJECT_OPEN},
        {NM_ITEM, NULL, 0, 0, 0, (APTR)ACTION_PROJECT_SAVEAS},
        {NM_ITEM, NULL, 0, 0, 0, (APTR)ACTION_PROJECT_SAVE},
        {NM_ITEM, NULL, 0, 0, 0, (APTR)ACTION_PROJECT_EXPORT},
        {NM_ITEM, NM_BARLABEL, 0, 0, 0, NULL},
        {NM_ITEM, NULL, 0, 0, 0, (APTR)ACTION_PROJECT_ABOUT},
        {NM_ITEM, NM_BARLABEL, 0, 0, 0, NULL},
        {NM_ITEM, NULL, 0, 0, 0, (APTR)ACTION_PROJECT_QUIT},

    /* Edition menu */
    {NM_TITLE, NULL, 0, 0, 0, (APTR)MSG_MENU_EDITION},
        {NM_ITEM, NULL, 0, 0, 0, (APTR)ACTION_EDIT_SELECTALL},
        {NM_ITEM, NULL, 0, 0, 0, (APTR)ACTION_EDIT_SELECTNONE},
        {NM_ITEM, NM_BARLABEL, 0, 0, 0, NULL},
        {NM_ITEM, NULL, 0, 0, 0, (APTR)ACTION_EDIT_COPY},
        {NM_ITEM, NULL, 0, 0, 0, (APTR)ACTION_EDIT_CUT},
        {NM_ITEM, NULL, 0, 0, 0, (APTR)ACTION_EDIT_PASTE},

    /* Tracks menu */
    {NM_TITLE, NULL, 0, 0, 0, (APTR)MSG_MENU_TRACKS},
        {NM_ITEM, NULL, 0, 0, 0, (APTR)ACTION_TRACKS_ADD},

    /* Generate menu */
    {NM_TITLE, NULL, 0, 0, 0, (APTR)MSG_MENU_GENERATE},

    /* Settings menu */
    {NM_TITLE, NULL, 0, 0, 0, (APTR)MSG_MENU_SETTINGS},
        {NM_ITEM, NULL, 0, 0, 0, (APTR)ACTION_SETTINGS_PROJECT},
        {NM_ITEM, NULL, 0, 0, 0, (APTR)ACTION_SETTINGS_VIEW},

    /* Help menu */
    {NM_TITLE, NULL, 0, 0, 0, (APTR)MSG_MENU_HELP},
        {NM_ITEM, NULL, 0, 0, 0, (APTR)ACTION_HELP_HELP},

    {NM_END, NULL, 0, 0, 0, NULL}
};

BOOL AukMenu_Create(AukMenu *am, struct Screen *screen, struct Window *window)
{
    int i;
 printf(" ////////////AukMenu_Create1\n");
    if (!am || !screen || !window || !GadToolsBase) {
        return FALSE;
    }

    /* Get visual info for the screen */
    am->visualInfo = GetVisualInfo(screen, TAG_END);
    if (!am->visualInfo) {
        printf("Failed to get visual info for menus\n");
        return FALSE;
    }

    /* Set menu labels from actions or localized strings */
    for (i = 0; menuTemplate[i].nm_Type != NM_END; i++) {
        if (menuTemplate[i].nm_Label != NM_BARLABEL) {
            if (menuTemplate[i].nm_Type == NM_TITLE) {
                /* Title: UserData contains string ID */
                ULONG msgID = (ULONG)menuTemplate[i].nm_UserData;
                menuTemplate[i].nm_Label = (STRPTR)LOC(msgID);
            } else {
                /* Menu item: UserData contains action ID */
                ULONG actionID = (ULONG)menuTemplate[i].nm_UserData;
                AukAction *action = AukAction_Get(actionID);
                if (action && action->name) {
                    menuTemplate[i].nm_Label = (STRPTR)action->name;
                } else {
                    menuTemplate[i].nm_Label = (STRPTR)"???";
                }
            }
            printf("Menu: %s\n", menuTemplate[i].nm_Label);
        }
    }
 printf(" ////////////AukMenu_Create4\n");
    /* Create the menus */
    am->menu = CreateMenus(menuTemplate, TAG_END);
    if (!am->menu) {
        printf("Failed to create menus\n");
        FreeVisualInfo(am->visualInfo);
        am->visualInfo = NULL;
        return FALSE;
    }

    /* Layout the menus */
    if (!LayoutMenus(am->menu, am->visualInfo,
                     GTMN_NewLookMenus, TRUE,
                     TAG_END)) {
        printf("Failed to layout menus\n");
        FreeMenus(am->menu);
        FreeVisualInfo(am->visualInfo);
        am->menu = NULL;
        am->visualInfo = NULL;
        return FALSE;
    }

    /* Attach menus to window */
    SetMenuStrip(window, am->menu);

    printf("Menus created successfully\n");
    return TRUE;
}

void AukMenu_Close(AukMenu *am, struct Window *window)
{
    if (!am) return;

    /* Remove menus from window */
    if (window && am->menu) {
        ClearMenuStrip(window);
    }

    /* Free menu structures */
    if (am->menu) {
        FreeMenus(am->menu);
        am->menu = NULL;
    }

    if (am->visualInfo) {
        FreeVisualInfo(am->visualInfo);
        am->visualInfo = NULL;
    }
}

LONG AukMenu_HandleEvent(AukMenu *am, UWORD menuNumber)
{
    struct MenuItem *item;
    LONG actionID = -1;

    if (!am || !am->menu) return -1;

    /* Get the selected menu item */
    if (menuNumber != MENUNULL) {
        item = ItemAddress(am->menu, menuNumber);
        if (item) {
            /* Get the action ID from UserData */
            actionID = (LONG)GTMENUITEM_USERDATA(item);
            printf("Menu selected, action ID: %ld\n", actionID);
            return actionID;
        }
    }

    return -1;
}
