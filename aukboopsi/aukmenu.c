
#include <stdio.h>
#include <string.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/gadtools.h>
#include <libraries/gadtools.h>

#include "aukmenu.h"
#include "aukaction.h"
#include "appsettings.h"
#include "compilers.h"

#include "auklocale.h"

extern struct Library *GadToolsBase;

void cleanexit(const char *pmessage);

/*
 * Base menu template (without recent files).
 * Recent file items are injected dynamically in the Project menu.
 */
static struct NewMenu baseTemplate[] = {
    /* Project menu */
    {NM_TITLE, NULL, 0, 0, 0, (APTR)MSG_MENU_PROJECT},
        {NM_ITEM, NULL,"N",0, 0, (APTR)ACTION_PROJECT_NEW},
        {NM_ITEM, NULL,"O",0, 0, (APTR)ACTION_PROJECT_OPEN},
        {NM_ITEM, NULL,"S",0, 0, (APTR)ACTION_PROJECT_SAVEAS},
        {NM_ITEM, NULL,"s",0, 0, (APTR)ACTION_PROJECT_SAVE},
        {NM_ITEM, NULL, 0, 0, 0, (APTR)ACTION_PROJECT_EXPORT},
        /* --- recent files will be inserted here --- */
        {NM_ITEM, NM_BARLABEL, 0, 0, 0, NULL},
        {NM_ITEM, NULL, 0, 0, 0, (APTR)ACTION_PROJECT_ABOUT},
        {NM_ITEM, NM_BARLABEL, 0, 0, 0, NULL},
        {NM_ITEM, NULL,"Q", 0, 0, (APTR)ACTION_PROJECT_QUIT},

    /* Edition menu */
    {NM_TITLE, NULL, 0, 0, 0, (APTR)MSG_MENU_EDITION},
        {NM_ITEM, NULL,"Z",0, 0, (APTR)ACTION_EDIT_UNDO},
        {NM_ITEM, NULL,"Y",0, 0, (APTR)ACTION_EDIT_REDO},
        {NM_ITEM, NM_BARLABEL, 0, 0, 0, NULL},
        {NM_ITEM, NULL, 0, 0, 0, (APTR)ACTION_EDIT_SELECTALL},
        {NM_ITEM, NULL, 0, 0, 0, (APTR)ACTION_EDIT_SELECTNONE},
        {NM_ITEM, NM_BARLABEL, 0, 0, 0, NULL},
        {NM_ITEM, NULL,"C",0, 0, (APTR)ACTION_EDIT_COPY},
        {NM_ITEM, NULL,"X",0, 0, (APTR)ACTION_EDIT_CUT},
        {NM_ITEM, NULL,"P",0, 0, (APTR)ACTION_EDIT_PASTE},
        {NM_ITEM, NM_BARLABEL, 0, 0, 0, NULL},
        {NM_ITEM, NULL,"T",0, 0, (APTR)ACTION_TRACKS_ADD},

    /* View menu */
    {NM_TITLE, NULL, 0, 0, 0, (APTR)MSG_MENU_VIEW},
        {NM_ITEM, NULL,"+",0, 0, (APTR)ACTION_VIEW_ZOOMIN},
        {NM_ITEM, NULL,"-",0, 0, (APTR)ACTION_VIEW_ZOOMOUT},
        {NM_ITEM, NULL,"0",0, 0, (APTR)ACTION_VIEW_ZOOM_PROJECT},
        {NM_ITEM, NM_BARLABEL, 0, 0, 0, NULL},
        {NM_ITEM, NULL, 0, 0, 0, (APTR)ACTION_VIEW_COLLAPSE_TRACKS},
        {NM_ITEM, NULL, 0, 0, 0, (APTR)ACTION_VIEW_EXPAND_TRACKS},
        {NM_ITEM, NM_BARLABEL, 0, 0, 0, NULL},
        {NM_ITEM, NULL, /*"F10"*/ 0, 0, 0, (APTR)ACTION_VIEW_SWITCH_TO_FULLSCREEN},
        {NM_ITEM, NULL, 0, 0, 0, (APTR)ACTION_VIEW_ICONIFY},

    /* Modify menu */
    {NM_TITLE, NULL, 0, 0, 0, (APTR)MSG_MENU_MODIFY},

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

/* Index in baseTemplate after which recent files are inserted (after Export) */
#define RECENT_INSERT_INDEX 6  /* baseTemplate[6] is the separator before About */

/* Static buffers for recent file menu labels ("1. filename") */
static char recentLabels[APPSETTINGS_MAX_RECENT][64];

/* Dynamic menu template (NULL when using static) */
static struct NewMenu *dynTemplate = NULL;

/*
 * Extract just the filename from a full path.
 * Returns pointer into the original string (no allocation).
 */
static const char *extractFilename(const char *path)
{
    const char *p;
    const char *last = path;

    if(!path) return "???";

    for(p = path; *p; p++) {
        if(*p == '/' || *p == ':') {
            last = p + 1;
        }
    }
    return last;
}

/*
 * Build a dynamic NewMenu template with recent files injected.
 * If recentCount == 0, just returns baseTemplate.
 */
static struct NewMenu *buildMenuTemplate(AppSettings *appSettings)
{
    int baseCount;
    int recentCount;
    int dynCount;
    int i, di;
    struct NewMenu *tmpl;

    recentCount = appSettings ? AppSettings_GetRecentCount(appSettings) : 0;

    if(recentCount <= 0) {
        /* No recent files, free any previous dynamic template */
        if(dynTemplate) {
            FreeVec(dynTemplate);
            dynTemplate = NULL;
        }
        return baseTemplate;
    }

    /* Count base template entries (including NM_END) */
    for(baseCount = 0; baseTemplate[baseCount].nm_Type != NM_END; baseCount++)
        ;
    baseCount++; /* Include NM_END */

    /* Dynamic size: base + separator + recentCount items */
    dynCount = baseCount + 1 + recentCount;

    /* Free previous dynamic template */
    if(dynTemplate) {
        FreeVec(dynTemplate);
        dynTemplate = NULL;
    }

    tmpl = (struct NewMenu *)AllocVec(dynCount * sizeof(struct NewMenu), MEMF_CLEAR);
    if(!tmpl) return baseTemplate;

    /* Copy base template up to insert point */
    di = 0;
    for(i = 0; i < RECENT_INSERT_INDEX; i++) {
        tmpl[di++] = baseTemplate[i];
    }

    /* Insert separator before recent files */
    tmpl[di].nm_Type = NM_ITEM;
    tmpl[di].nm_Label = NM_BARLABEL;
    tmpl[di].nm_CommKey = 0;
    tmpl[di].nm_Flags = 0;
    tmpl[di].nm_MutualExclude = 0;
    tmpl[di].nm_UserData = NULL;
    di++;

    /* Insert recent file items */
    for(i = 0; i < recentCount; i++) {
        const char *path = AppSettings_GetRecentFile(appSettings, i);
        const char *fname = extractFilename(path);

        /* Build label: "1. filename" */
        sprintf(recentLabels[i], "%d. %.58s", i + 1, fname);

        tmpl[di].nm_Type = NM_ITEM;
        tmpl[di].nm_Label = (STRPTR)recentLabels[i];
        tmpl[di].nm_CommKey = 0;
        tmpl[di].nm_Flags = 0;
        tmpl[di].nm_MutualExclude = 0;
        tmpl[di].nm_UserData = (APTR)(ACTION_RECENT_FILE_0 + i);
        di++;
    }

    /* Copy rest of base template from insert point */
    for(i = RECENT_INSERT_INDEX; baseTemplate[i].nm_Type != NM_END; i++) {
        tmpl[di++] = baseTemplate[i];
    }

    /* NM_END */
    tmpl[di].nm_Type = NM_END;
    tmpl[di].nm_Label = NULL;

    dynTemplate = tmpl;
    return tmpl;
}

/*
 * Resolve labels in a menu template from actions/locale.
 * Recent file items already have their labels set.
 */
static void resolveMenuLabels(struct NewMenu *tmpl)
{
    int i;

    for(i = 0; tmpl[i].nm_Type != NM_END; i++) {
        if(tmpl[i].nm_Label != NM_BARLABEL && tmpl[i].nm_Label == NULL) {
            if(tmpl[i].nm_Type == NM_TITLE) {
                /* Title: UserData contains string ID */
                ULONG msgID = (ULONG)tmpl[i].nm_UserData;
                tmpl[i].nm_Label = (STRPTR)LOC(msgID);
            } else {
                /* Menu item: UserData contains action ID */
                ULONG actionID = (ULONG)tmpl[i].nm_UserData;
                AukAction *action = AukAction_Get(actionID);
                if(action && action->name) {
                    tmpl[i].nm_Label = (STRPTR)action->name;
                } else {
                    tmpl[i].nm_Label = (STRPTR)"???";
                }
            }
        }
    }
}

BOOL AukMenu_Create(AukMenu *am, struct Screen *screen, struct Window *window)
{
    if (!am || !screen || !window || !GadToolsBase) {
        return FALSE;
    }

    /* Get visual info for the screen */
    am->visualInfo = GetVisualInfo(screen, TAG_END);
    if (!am->visualInfo) {
        cleanexit("Failed to get visual info for menus\n");
        return FALSE;
    }

    /* Resolve labels in base template (no recent files at initial create) */
    resolveMenuLabels(baseTemplate);

    /* Create the menus */
    am->menu = CreateMenus(baseTemplate, TAG_END);
    if (!am->menu) {
        cleanexit("Failed to create menus\n");
        FreeVisualInfo(am->visualInfo);
        am->visualInfo = NULL;
        return FALSE;
    }

    /* Layout the menus */
    if (!LayoutMenus(am->menu, am->visualInfo,
                     GTMN_NewLookMenus, TRUE,
                     TAG_END)) {
        cleanexit("Failed to layout menus\n");
        FreeMenus(am->menu);
        FreeVisualInfo(am->visualInfo);
        am->menu = NULL;
        am->visualInfo = NULL;
        return FALSE;
    }

    /* Attach menus to window */
    SetMenuStrip(window, am->menu);

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

    /* Free dynamic template if any */
    if (dynTemplate) {
        FreeVec(dynTemplate);
        dynTemplate = NULL;
    }
}

void AukMenu_Rebuild(AukMenu *am, struct Screen *screen, struct Window *window,
                     AppSettings *appSettings)
{
    struct NewMenu *tmpl;

    if (!am || !screen || !window) return;

    /* Remove current menus from window */
    if (am->menu) {
        ClearMenuStrip(window);
        FreeMenus(am->menu);
        am->menu = NULL;
    }

    /* Build dynamic template with recent files */
    tmpl = buildMenuTemplate(appSettings);
    resolveMenuLabels(tmpl);

    /* Create new menus */
    am->menu = CreateMenus(tmpl, TAG_END);
    if (!am->menu) return;

    /* Layout */
    if (!LayoutMenus(am->menu, am->visualInfo,
                     GTMN_NewLookMenus, TRUE,
                     TAG_END)) {
        FreeMenus(am->menu);
        am->menu = NULL;
        return;
    }

    /* Attach to window */
    SetMenuStrip(window, am->menu);
}

AukAction *AukMenu_ToAction(AukMenu *am, UWORD menuNumber)
{
    struct MenuItem *item;
    LONG actionID = -1;

    if (!am || !am->menu) return NULL;

    /* Get the selected menu item */
    if (menuNumber != MENUNULL) {
        item = ItemAddress(am->menu, menuNumber);
        if (item) {
            /* Get the action ID from UserData */
            actionID = (LONG)GTMENUITEM_USERDATA(item);
            return AukAction_Get(actionID);
        }
    }

    return NULL;
}

LONG AukMenu_ToActionID(AukMenu *am, UWORD menuNumber)
{
    struct MenuItem *item;

    if (!am || !am->menu) return -1;

    if (menuNumber != MENUNULL) {
        item = ItemAddress(am->menu, menuNumber);
        if (item) {
            return (LONG)GTMENUITEM_USERDATA(item);
        }
    }

    return -1;
}
