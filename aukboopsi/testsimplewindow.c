#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <clib/alib_protos.h>

#include <intuition/screens.h>
#include <intuition/icclass.h>

#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/utility.h>

#include <proto/dos.h>

#include <proto/window.h>
#include <classes/window.h>

#include <proto/layout.h>
#include <gadgets/layout.h>

#include <proto/button.h>
#include <gadgets/button.h>

#include "compilers.h"

INLINE struct Window *boopsi_OpenWindow(Object *owin) {
    return (struct Window *)DoMethod(owin, WM_OPEN, NULL);
}


/* Library bases */
struct IntuitionBase *IntuitionBase = NULL;
struct GfxBase *GfxBase = NULL;
struct Library *UtilityBase = NULL;
struct Library *LayersBase = NULL;
struct Library *WindowBase = NULL;
struct Library *LayoutBase = NULL;
struct Library *ButtonBase = NULL;

/* Library table for automated opening/closing */
typedef struct {
    const char *name;
    ULONG version;
    struct Library **base;
} LibraryEntry;

static LibraryEntry libraryTable[] = {
    /* System libraries */
    {"intuition.library", 39, (struct Library **)&IntuitionBase},
    {"graphics.library", 39, (struct Library **)&GfxBase},
    {"utility.library", 39, &UtilityBase},
    {"layers.library", 39, &LayersBase},
    /* BOOPSI class libraries - version 45 for OS3.9 */
    {"window.class", 45, &WindowBase},
    {"gadgets/layout.gadget", 45, &LayoutBase},
    {"gadgets/button.gadget", 45, &ButtonBase},
    {NULL, 0, NULL} /* Terminator */
};

/* Global variables */
Object *window_obj = NULL;
struct Window *win = NULL;
struct MsgPort *app_port = NULL;
struct Screen *lockedscreen = NULL;
struct DrawInfo *drawInfo = NULL;
Object *mainlayout = NULL;
Object *testbutton = NULL;

void cleanexit(const char *pmessage) {
    if (pmessage) printf("%s\n", pmessage);
    exit(0);
}

void exitclose(void);

int main(int argc, char **argv) {
    atexit(&exitclose);

    /* Open all libraries via table */
    {
        LibraryEntry *entry;
        char errorMsg[80];

        for (entry = libraryTable; entry->name != NULL; entry++) {
            *(entry->base) = OpenLibrary(entry->name, entry->version);
            if (!*(entry->base)) {
                snprintf(errorMsg, 79, "Can't open %s", entry->name);
                cleanexit(errorMsg);
            }
        }
    }

    /* Lock the public screen */
    lockedscreen = LockPubScreen(NULL);
    if (!lockedscreen) cleanexit("Can't lock screen");

    drawInfo = GetScreenDrawInfo(lockedscreen);

    /* Create a simple button */
    testbutton = NewObject(BUTTON_GetClass(), NULL,
        GA_Text, "Test Button",
        GA_RelVerify, TRUE,
        TAG_END);
    if (!testbutton) cleanexit("Can't create button");

    /* Create layout with the button */
    mainlayout = NewObject(LAYOUT_GetClass(), NULL,
        GA_DrawInfo, drawInfo,
        LAYOUT_DeferLayout, TRUE,
        LAYOUT_SpaceOuter, TRUE,
        LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
        LAYOUT_AddChild, testbutton,
        TAG_END);
    if (!mainlayout) cleanexit("Can't create layout");

    /* Create message port */
    app_port = CreateMsgPort();
    if (!app_port) cleanexit("Can't create message port");

    /* Create window object */
    window_obj = NewObject(WINDOW_GetClass(), NULL,
        WA_Left, 100,
        WA_Top, 100,
        WA_Width, 320,
        WA_Height, 200,
        WA_CustomScreen, (ULONG)lockedscreen,
        WA_IDCMP, IDCMP_CLOSEWINDOW | IDCMP_RAWKEY,
        WA_Flags, WFLG_DRAGBAR | WFLG_DEPTHGADGET | WFLG_CLOSEGADGET |
                  WFLG_SIZEGADGET | WFLG_ACTIVATE | WFLG_SMART_REFRESH,
        WA_Title, (ULONG)"Simple Test Window",
        WINDOW_ParentGroup, (ULONG)mainlayout,
        WINDOW_AppPort, (ULONG)app_port,
        TAG_END);
    if (!window_obj) cleanexit("Can't create window");

    /* Open the window */
    win = boopsi_OpenWindow(window_obj);
    if (!win) cleanexit("Can't open window");

    printf("Window opened. Press ESC to quit.\n");

    /* Main event loop */
    {
        ULONG winsignal;
        BOOL ok = TRUE;

        GetAttr(WINDOW_SigMask, window_obj, &winsignal);

        while (ok) {
            ULONG result, currentSignal;

            currentSignal = Wait(winsignal | (1L << app_port->mp_SigBit) | SIGBREAKF_CTRL_C);
            if (currentSignal & SIGBREAKF_CTRL_C) exit(0);

            while ((result = DoMethod(window_obj, WM_HANDLEINPUT, NULL)) != WMHI_LASTMSG) {
                switch (result & WMHI_CLASSMASK) {
                    case WMHI_RAWKEY:
                        /* Quit on ESC key (0x45) */
                        if ((result & WMHI_KEYMASK) == 0x45) ok = FALSE;
                        break;

                    case WMHI_CLOSEWINDOW:
                        ok = FALSE;
                        break;

                    default:
                        break;
                }
            }
        }
    }

    return 0;
}

void exitclose(void) {
    printf("Closing application...\n");

    /* Dispose window object (cascades to children) */
    if (window_obj) {
        DoMethod(window_obj, WM_CLOSE);
        DisposeObject(window_obj);
    }

    /* Delete message port */
    if (app_port) DeleteMsgPort(app_port);

    /* Free screen resources */
    if (drawInfo) FreeScreenDrawInfo(lockedscreen, drawInfo);
    if (lockedscreen) UnlockPubScreen(0, lockedscreen);

    /* Close all libraries in reverse order */
    {
        LibraryEntry *entry;
        int i;

        /* Find last entry */
        for (i = 0; libraryTable[i].name != NULL; i++);

        /* Close in reverse order */
        for (i = i - 1; i >= 0; i--) {
            entry = &libraryTable[i];
            if (*(entry->base)) {
                CloseLibrary(*(entry->base));
                *(entry->base) = NULL;
            }
        }
    }
}
