#ifndef BOOPSIMAINWINDOW_H
#define BOOPSIMAINWINDOW_H

#include <exec/types.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <intuition/classusr.h>
#include "aukmenu.h"
// to get recent menu list
#include "appsettings.h"

/*
    Extends a bit Window Boopsi behaviour, so:
     - We can switch WB Window to fullscreen window
     - Find the window position back when returning to WB.
     - have a single function to set WB Window Title, or screen title if fullscreen.(TODO)
     - Manage recreating the menu each time we change mode or iconify/uniconify.
     - manage color remap issues when changing screen.(TODO)

  Window boopsi object and attached gadgets are persistant and reconfigurable.
   when Intuition Screens and Windows are transient.

*/

/* structure */
typedef struct BoopsiMainWindow {
    WORD top,left,width,height;
    int fullscreen; // keep state when hidding.

    struct Screen *lockedscreen; // when using window on WB or else
    struct Screen *fullPubScreen;  // when using our own public screen, reallocated. (experimental)

    AukMenu appMenu; /* GadTools menu */

    //struct DrawInfo *drawInfo; // informations on how to draw on the screen, passed to gagdets.

} BoopsiMainWindow;

/* once at init */
void BMainWindow_Init(struct BoopsiMainWindow *mw);
void BMainWindow_SwitchToFullScreen(struct BoopsiMainWindow *mw,Object *window_obj, AppSettings *appSettings);
void BMainWindow_SwitchToWB(struct BoopsiMainWindow *mw,Object *window_obj, AppSettings *appSettings);
/* at iconify */
//void BMainWindow_Hide(struct BoopsiMainWindow *mw,Object *window_obj);
void BMainWindow_Iconify(struct BoopsiMainWindow *mw,Object *window_obj);
/* at uniconify */
void BMainWindow_Show(struct BoopsiMainWindow *mw,Object *window_obj, AppSettings *appSettings);
/* at quitting */
void BMainWindow_Close(struct BoopsiMainWindow *mw,Object *window_obj);

extern struct Window *CurrentMainWindow;

#endif /* AUKMENU_H */
