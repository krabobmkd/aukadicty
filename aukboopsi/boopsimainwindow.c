#include "boopsimainwindow.h"

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/alib.h>

#include <intuition/intuition.h>


#include <proto/window.h>
#include <classes/window.h>

#include "aukmenu.h"
#include "appsettings.h"
// This is the intuition level Window, on OS3 it's recreated when iconizing/reopening !
// when  iconizing/reopening BOOPSI objects are kept, but Intuition level instances and buffers are wiped out.
// Yet, it's needed for most Gadget method calls, and this is not retained by boopsi objects.
// note there could be many windows.
struct Window *CurrentMainWindow=NULL;

void BMainWindow_Init(struct BoopsiMainWindow *mw)
{
    if(!mw) return;
    mw->lockedscreen = LockPubScreen(NULL);
//    if(mw->lockedscreen)
//    {
//        mw->drawInfo = GetScreenDrawInfo(app->lockedscreen);
//    }

}
void BMainWindow_Show(struct BoopsiMainWindow *mw,Object *window_obj, AppSettings *appSettings)
{
    if(!mw) return;
    if(mw->fullscreen)
    {
        BMainWindow_SwitchToFullScreen(mw,window_obj,appSettings);
    } else
    {   // WB window
        BMainWindow_SwitchToWB(mw,window_obj,appSettings);
    }
}

/* at iconify */
void BMainWindow_Iconify(struct BoopsiMainWindow *mw,Object *window_obj)
{
    if(!mw) return;
    if(CurrentMainWindow)
    {
        // todo save window position
        AukMenu_Close(&mw->appMenu, CurrentMainWindow);
        DoMethod(window_obj, WM_ICONIFY, NULL);

        CurrentMainWindow = NULL;
    }

    /* if was in fullscreen mode */
    if(mw->fullPubScreen)
    {
        CloseScreen(mw->fullPubScreen);
        mw->fullPubScreen = NULL;
    }
}
/* at quitting */
void BMainWindow_Close(struct BoopsiMainWindow *mw,Object *window_obj)
{
    if(CurrentMainWindow)
    {
        // todo save window position
        AukMenu_Close(&mw->appMenu, CurrentMainWindow);
        DoMethod(window_obj, WM_CLOSE );
        CurrentMainWindow = NULL;
    }

    /* if was in fullscreen mode */
    if(mw->fullPubScreen)
    {
        CloseScreen(mw->fullPubScreen);
        mw->fullPubScreen = NULL;
    }

    if(mw->lockedscreen)
    {
        //if(mw->drawInfo) FreeScreenDrawInfo(mw->lockedscreen, mw->drawInfo);
        UnlockPubScreen(0, mw->lockedscreen);
        mw->lockedscreen = NULL;
    }
}

extern void UpdateRecentMenu();
/* Used in both WB window and fullscreen cases. doing WM_OPEN implies:
    - recreating and refreshing the menu.
*/
void GenericOpenWindow(BoopsiMainWindow *mw,Object *window_obj, AppSettings *appSettings)
{
    struct Screen *appliedScreen;

    if(CurrentMainWindow) return;

    CurrentMainWindow = (struct Window *)DoMethod(window_obj, WM_OPEN, NULL);
    if(!CurrentMainWindow) return;

    appliedScreen = (mw->fullPubScreen!=NULL)?(mw->fullPubScreen):(mw->lockedscreen);

    /* Create and attach menus */
    if (!AukMenu_Create(&mw->appMenu, appliedScreen, CurrentMainWindow)) {
       // printf("Warning: Could not create menus\n");
    }

    /* Rebuild menus with recent files from loaded settings */
    if (AppSettings_GetRecentCount(appSettings) > 0) {
        AukMenu_Rebuild(&mw->appMenu, appliedScreen, CurrentMainWindow, appSettings );
    }

}


void BMainWindow_SwitchToFullScreen(struct BoopsiMainWindow *mw,Object *window_obj, AppSettings *appSettings)
{
    struct Screen *myScreen;

    if(!mw || !window_obj) return;
    if(mw->fullPubScreen) return; // already ok

    myScreen = OpenScreenTags(NULL,
        SA_Type,       PUBLICSCREEN,
        SA_PubName,   (ULONG) "Aukadicty",  // Optional: custom name
        SA_LikeWorkbench, TRUE,             // Inherit Workbench settings
        SA_Title,     (ULONG) "Aukadicty",
        TAG_DONE);
    if(!myScreen) return;

    mw->fullPubScreen = myScreen;
    /* You may set these while the window is NOT open, at NewObject() time or SetAttrs() - between WM_CLOSE and WM_OPEN for example.
        WA_PubScreen WA_CustomScreen, ...
     */
    if(CurrentMainWindow)
    {
        // todo save window position

        AukMenu_Close(&mw->appMenu, CurrentMainWindow);
        DoMethod(window_obj, WM_CLOSE );
        CurrentMainWindow = NULL;
    }
    /* reconfigure persistant boopsi window object while closed */
    SetAttrs(window_obj,
            WA_CustomScreen,(ULONG)myScreen,
        //  WA_PubScreen,(ULONG)myScreen,
            WA_Backdrop,TRUE,
            TAG_END);

    /* re-open */
    GenericOpenWindow( mw, window_obj, appSettings );

    mw->fullscreen = TRUE;
}


void BMainWindow_SwitchToWB(struct BoopsiMainWindow *mw,Object *window_obj, AppSettings *appSettings)
{
    if(!mw || !window_obj) return;

    /* close backdrop window at boopsi level */
    if(CurrentMainWindow)
    {
        AukMenu_Close(&mw->appMenu, CurrentMainWindow);
        DoMethod(window_obj, WM_CLOSE );
        CurrentMainWindow = NULL;
    }

    /* close extra screen */
    if(mw->fullPubScreen)
    {
        CloseScreen(mw->fullPubScreen);
        mw->fullPubScreen = NULL;
    }
    /* reconfigure persistant boopsi window object while closed */
    SetAttrs(window_obj,
            WA_CustomScreen,(ULONG)mw->lockedscreen,
        //  WA_PubScreen,(ULONG)myScreen,
            WA_Backdrop,FALSE,
            TAG_END);

    /* re-open */
    GenericOpenWindow( mw, window_obj, appSettings );

    mw->fullscreen = FALSE;
}
