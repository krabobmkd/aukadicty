#include "boopsimainwindow.h"

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/alib.h>

#include <intuition/intuition.h>
#include <intuition/gadgetclass.h>

#include <proto/window.h>
#include <classes/window.h>

#include "aukmenu.h"
#include "appsettings.h"

#include <stdio.h>
#include <string.h>
// This is the intuition level Window, on OS3 it's recreated when iconizing/reopening !
// when  iconizing/reopening BOOPSI objects are kept, but Intuition level instances and buffers are wiped out.
// Yet, it's needed for most Gadget method calls, and this is not retained by boopsi objects.
// note there could be many windows.
struct Window *CurrentMainWindow=NULL;

/* can be either the WB locked screen, or our private screen */
struct Screen *CurrentMainScreen=NULL;


extern void CloseSettingsWindow();

void BMainWindow_Init(struct BoopsiMainWindow *mw)
{
    mw->title[0] = 0;
//    if(!mw) return;
//    mw->lockedScreen = LockPubScreen(NULL);
//    if(mw->lockedScreen)
//    {
//        mw->drawInfo = GetScreenDrawInfo(app->lockedScreen);
//    }

}

/* would either set the window title or Screen title according to mode */
void BMainWindow_SetTitle(struct BoopsiMainWindow *mw, const char *title)
{
    strncpy(&mw->title[0],title,sizeof(mw->title)-1);
    mw->title[sizeof(mw->title)-1] = 0;

    /* if fullscreen open, update */
    if(mw->fullscreen && mw->fullPubScreen && CurrentMainScreen)
    {
        /* seriously ? validate this. */
        SetAttrs((Object *)CurrentMainScreen,
                        SA_Title,&mw->title[0],NULL);

    } else if(!mw->fullscreen && mw->lockedScreen && CurrentMainWindow)
    {
        /* if wb window open, update */
        SetWindowTitles(CurrentMainWindow,&mw->title[0],NULL);
    }

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

/* at iconify or close */
void BMainWindow_Close(struct BoopsiMainWindow *mw,Object *window_obj, int iconify)
{
    if(!mw) return;

    CloseSettingsWindow();

    if(CurrentMainWindow)
    {
        // todo save window position
        AukMenu_Close(&mw->appMenu, CurrentMainWindow);
        if(iconify)
        {
            DoMethod(window_obj, WM_ICONIFY, NULL);
        } else
        {
            DoMethod(window_obj, WM_CLOSE );
        }
        CurrentMainWindow = NULL;
    }

    /* if was in fullscreen mode */
    if(mw->fullPubScreen)
    {
        CloseScreen(mw->fullPubScreen);
        mw->fullPubScreen = NULL;
    }
    if(mw->lockedScreen)
    {
        UnlockPubScreen(0, mw->lockedScreen);
        mw->lockedScreen = NULL;
    }
    CurrentMainScreen = NULL;
}

/* Used in both WB window and fullscreen cases. doing WM_OPEN implies:
    - recreating and refreshing the menu.
*/
void GenericOpenWindow(BoopsiMainWindow *mw,Object *window_obj, AppSettings *appSettings)
{
    if(CurrentMainWindow) return; /* if already exists don't open */
    if(!CurrentMainScreen) return; /* need an active screen */

    CurrentMainWindow = (struct Window *)DoMethod(window_obj, WM_OPEN, NULL);

    if(!CurrentMainWindow) return;

    /* Create and attach menus */
    if (!AukMenu_Create(&mw->appMenu, CurrentMainScreen, CurrentMainWindow)) {
       // printf("Warning: Could not create menus\n");
       return;
    }

    /* Rebuild menus with recent files from loaded settings */
    if (AppSettings_GetRecentCount(appSettings) > 0) {
        AukMenu_Rebuild(&mw->appMenu, CurrentMainScreen, CurrentMainWindow, appSettings );
    }

}
/*got to do that better */
extern void UpdatePensToCurrentMainScreen();

void BMainWindow_SwitchToFullScreen(struct BoopsiMainWindow *mw,Object *window_obj, AppSettings *appSettings)
{
        int x1,y1,w,h;

    struct Screen *myScreen;
    printf("go fs\n");
    if(!mw || !window_obj) return;
    if(mw->fullPubScreen) return; // already ok

    CloseSettingsWindow();

    myScreen = OpenScreenTags(NULL,
        SA_Type,       PUBLICSCREEN,
        SA_PubName,   (ULONG) "Aukadicty",  // Optional: custom name
        SA_LikeWorkbench, TRUE,             // Inherit Workbench settings
        SA_Title,     (ULONG) &mw->title[0],
       // SA_Colors,(ULONG)&colspec[0],
        TAG_DONE);
    if(!myScreen) return;

    mw->fullPubScreen = myScreen;
    /* You may set these while the window is NOT open, at NewObject() time or SetAttrs() - between WM_CLOSE and WM_OPEN for example.
        WA_PubScreen WA_CustomScreen, ...
     */     
    if(CurrentMainWindow)
    {
        /* save window position if there is one */
        GetAttr(WA_Top,window_obj,&mw->top);
        GetAttr(WA_Left,window_obj,&mw->left);
        GetAttr(WA_Width,window_obj,&mw->width);
        GetAttr(WA_Height,window_obj,&mw->height);
        if(mw->width<128) mw->width=128;
        if(mw->height<64) mw->height=64;

        AukMenu_Close(&mw->appMenu, CurrentMainWindow);
        DoMethod(window_obj, WM_CLOSE );
        CurrentMainWindow = NULL;

        if(mw->lockedScreen)
        {
            UnlockPubScreen(0, mw->lockedScreen);
            mw->lockedScreen = NULL;
        }
        //CurrentMainScreen = NULL;

    }
    CurrentMainScreen = myScreen;

    /* need to reattribute pens on this screen */
    UpdatePensToCurrentMainScreen();


    /* reconfigure persistant boopsi window object while closed */
    {
        /* get screen dimension */
        x1 =0;
        y1 = myScreen->BarHeight;
        w = myScreen->Width;
        h = myScreen->Height - y1;

        SetAttrs(window_obj,
            WA_CustomScreen,(ULONG)myScreen,
            WA_Borderless, TRUE,
            WA_SizeGadget,FALSE,
            WA_DepthGadget,FALSE,
            WA_CloseGadget,FALSE,
            WA_DragBar,FALSE,
            WA_Title,NULL,
            WA_Flags,WFLG_ACTIVATE | WFLG_SMART_REFRESH ,
            WA_Backdrop,TRUE,
            WINDOW_IconifyGadget, FALSE,
            WA_Top,y1,
            WA_Left,x1,
            WA_Width,w+16,
            WA_Height,h+16,
            WA_MaxWidth,w+16,
            WA_MaxHeight,h+16,
            // WFLG_DRAGBAR | WFLG_DEPTHGADGET | WFLG_CLOSEGADGET | WFLG_SIZEGADGET | WFLG_ACTIVATE | WFLG_SMART_REFRESH,

            TAG_END);
    }
    /* re-open */
    GenericOpenWindow( mw, window_obj, appSettings );

    if(CurrentMainWindow)
    {
        struct Gadget *mlayout=NULL;
        GetAttr(WINDOW_ParentGroup,window_obj,(ULONG *)&mlayout);
        //printf("got WINDOW_ParentGroup:%08x\n",(int)mlayout);
        if(mlayout)
        {
            SetGadgetAttrs(mlayout,CurrentMainWindow,NULL,
                GA_Width,w,
                GA_Height,h,
                TAG_END
                    );

        }

    }



    mw->fullscreen = TRUE;
}


void BMainWindow_SwitchToWB(struct BoopsiMainWindow *mw,Object *window_obj, AppSettings *appSettings)
{
    if(!mw || !window_obj) return;

    CloseSettingsWindow();

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
        CurrentMainScreen = NULL;
    }

    if(!mw->lockedScreen)
    {
        mw->lockedScreen = LockPubScreen(NULL);
    }
    if(! mw->lockedScreen) return;

    CurrentMainScreen =  mw->lockedScreen;

    /* need to reattribute pens on this screen */
    UpdatePensToCurrentMainScreen();

    {
        int x1,y1,w,h;
        /* if dimension has been kept by settings or screen switch, recover them */
        if(mw->width>0)
        {
            x1 = mw->left;
            y1 = mw->top;
            w = mw->width;
            h = mw->height;
        } else
        {
            /* else some default */
            x1 = 40;
            y1 = 40;
            w = 320;
            h= 240;
        }

        /* reconfigure persistant boopsi window object while closed */
        SetAttrs(window_obj,
            WA_CustomScreen,(ULONG)mw->lockedScreen,
            WA_Borderless, FALSE,
            WA_Backdrop,FALSE,
            WA_Flags,WFLG_ACTIVATE | WFLG_SMART_REFRESH ,
           // WA_Flags, WFLG_DRAGBAR | WFLG_DEPTHGADGET | WFLG_CLOSEGADGET | WFLG_SIZEGADGET | WFLG_ACTIVATE | WFLG_SMART_REFRESH,
            WA_DragBar,TRUE,
            WA_SizeGadget,TRUE,
            WA_DepthGadget,TRUE,
            WA_CloseGadget,TRUE,
            WA_Title,(ULONG)&mw->title[0],
            WINDOW_IconifyGadget, TRUE,
            WA_Top,y1,
            WA_Left,x1,
            WA_Width,w,
            WA_Height,h,
            TAG_END);
    }
    mw->fullscreen = FALSE;

    /* re-open */
    GenericOpenWindow( mw, window_obj, appSettings );


}
