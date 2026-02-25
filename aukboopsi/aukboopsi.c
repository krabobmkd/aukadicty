
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <clib/alib_protos.h>
//#include <clib/reaction_lib_protos.h>

#include <intuition/screens.h>
#include <intuition/icclass.h>

#include <proto/diskfont.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/diskfont.h>
#include <proto/locale.h>

#include <proto/dos.h>
#include <proto/icon.h>
#include <exec/alerts.h>

#include <proto/window.h>
#include <classes/window.h>

#include <proto/layout.h>
#include <gadgets/layout.h>

#include <proto/button.h>
#include <gadgets/button.h>

#include <proto/checkbox.h>
#include <gadgets/checkbox.h>

#include <proto/label.h>
#include <images/label.h>

#include <proto/bitmap.h>
#include <images/bitmap.h>

#include <proto/string.h>
#include <gadgets/string.h>

#include <proto/texteditor.h>
#include <gadgets/texteditor.h>

#include <proto/requester.h>
#include <classes/requester.h>

//#include <proto/scroller.h>
//#include <gadgets/scroller.h>

//#include <proto/slider.h>
//#include <gadgets/slider.h>

//#include <proto/getfile.h>
//#include <gadgets/getfile.h>

#include <proto/asl.h>
#include <libraries/asl.h>

#include "gadgetid.h"
#include "TrackListView.h"
#include "HeaderView.h"
#include "FooterView.h"
#include "ProjectSettingsView.h"
#include "auklocale.h"
#include "aukerrors.h"
#include "aukaction.h"
#include "aukstylesheet.h"
#include "aukmenu.h"
#include "boopsimainwindow.h"
#include "boopsimessage.h"
#include "boopsidispose.h"
#include "appsettings.h"

//#include "aukaproject.h"
#include <aukadicty.h>
#include <auksoundfileengine.h>

#include "compilers.h"
#include "bdbprintf.h"

struct Task	*myTask=NULL;

const char *pVersion="$VER: 0.1";

// DOSBase is already opened by C startup...
// struct DosLibrary *DOSBase=NULL;
struct IntuitionBase *IntuitionBase=NULL;
struct GfxBase *GfxBase=NULL;
struct Library *UtilityBase=NULL; // inlined DoMethod() may use CallHooksKpt().
struct Library *LayersBase=NULL; // only used by gadgets drawing in static link mode.
// used for appicon.
struct Library *IconBase=NULL;
struct Library *AslBase=NULL;
struct Library *DiskfontBase=NULL;
struct Library *GadToolsBase=NULL;

// this lib is optional, and allow using graphics cards and special RGB truecolor bitmaps drawing functions.
struct Library *CyberGfxBase = NULL;

// boopsi classes bases:
struct Library *WindowBase=NULL;
struct Library *LayoutBase=NULL;
struct Library *BitMapBase=NULL;
struct Library *ButtonBase=NULL;
struct Library *LabelBase=NULL;
//struct Library *VirtualBase=NULL;

struct Library *CheckBoxBase=NULL;
struct Library *StringBase=NULL;
struct Library *TextFieldBase=NULL;
struct Library *RequesterBase=NULL;
struct Library *ScrollerBase=NULL;
struct Library *SliderBase=NULL;
struct Library *GetFileBase=NULL;
struct LocaleBase *LocaleBase=NULL;

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
    {"icon.library", 39, &IconBase},
    {"asl.library", 39, &AslBase},
    {"diskfont.library", 39, &DiskfontBase},
    {"gadtools.library", 39, &GadToolsBase},
    {"locale.library", 38, (struct Library **)&LocaleBase},
    /* BOOPSI class libraries - version 45 for OS3.9 */
    /* class */
    {"window.class", 45, &WindowBase},
    {"requester.class", 45, &RequesterBase},
    /* images */
    {"images/bitmap.image", 45, &BitMapBase},
    {"images/label.image", 45, &LabelBase},
    /* gadgets */
    {"gadgets/layout.gadget", 45, &LayoutBase},
    {"gadgets/button.gadget", 45, &ButtonBase},
//    {"virtual.gadget", 45, &VirtualBase},
    {"gadgets/checkbox.gadget", 45, &CheckBoxBase},
    {"gadgets/string.gadget", 45, &StringBase},
    {"gadgets/texteditor.gadget", 45, &TextFieldBase},
    {"gadgets/scroller.gadget", 45, &ScrollerBase},
    {"gadgets/slider.gadget", 45, &SliderBase},
    {"gadgets/getfile.gadget", 45, &GetFileBase},
    {NULL, 0, NULL} /* Terminator */
};

void cleanexit(const char *pmessage)
{
    if(pmessage) printf("%s\n",pmessage);
    // will execute functions registered with atexit().
    // this way if C startup manages it, Ctrl-C will also close nicely.
    exit(0);
}
void exitclose(void);

void openAboutReq();

// all app related variables are here:
struct App
{
    Object *window_obj; // window as boopsi object

    BoopsiMainWindow mainwindow; /* main window management */

    ProjectSettingsView projectSettingsView; /* Project Settings window */

    struct MsgPort *app_port;
    AukStyleSheetPtr styleSheet; /* shared stylesheet instance pointer */

    Object *mainvlayout;

            Object* btAbout;

        HeaderView headerView;
        TrackListView tracksListView;
        FooterView footerView;

     Object *statusBarLayout;  /* Status bar container with separator */
     Object *statusBarLabel;   /* Status bar text label */

     Object *reportReq;

     // - - - retain document object
     AukAProjectPtr _project;


     // Application-level settings (temp dir, recent files)
     AppSettings appSettings;
};

// App Modelinstance as our private struct.
struct App *app=NULL;



// shared global state...
int CurrentEditMode = 0;

BoopsiDisposeQueue *ObjectLateDisposer=NULL;

static int testprojectinited=0;
int initProject();

void OpenSettingsWindow()
{
    if(!app) return;
    ProjectSettingsView_Open(&app->projectSettingsView);
}
void CloseSettingsWindow()
{
    if(!app) return;
    /* Sync temp dir from settings view back to AppSettings */
    {
        const char *tempDir = ProjectSettingsView_GetTempDir(&app->projectSettingsView);
        if (tempDir) {
            AppSettings_SetTempDir(&app->appSettings, tempDir);
        }
    }
    ProjectSettingsView_Close(&app->projectSettingsView);
}

//  - - - -- - - - -  end of App modelclass management.

int main(int argc, char **argv)
{
    int y;
    myTask = FindTask(NULL);
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
    // try optional libs, pointer will be null if missing, valid case.
    CyberGfxBase  = OpenLibrary("cybergraphics.library", 1);

    /* Initialize localization system */
    AukLocale_Init("aukadicty.catalog", 1);

    /* Initialize action system (after locale init) */
    AukAction_Init();

    ObjectLateDisposer = (BoopsiDisposeQueue *)AllocVec(sizeof(BoopsiDisposeQueue),MEMF_CLEAR);
    if(!ObjectLateDisposer) exit(0);

    if(!initMessageTargetModel())  cleanexit("Can't create appmodel");

    app = AllocVec(sizeof(struct App),MEMF_CLEAR);
    if(!app)  cleanexit("Can't create app");

    /* Initialize and load application settings from icon tooltypes */
    AppSettings_Init(&app->appSettings);
    AppSettings_Load(&app->appSettings, "aukadicty");

    /* Initialize sound file engine for background loading */

  //Re  soundFileEngine = AukSoundFileEngine_Init((struct Process *)myTask,"PROGDIR:",0);
    /* Note: Engine init failure is non-fatal - features that need it will be disabled */

    /* BOOPSI needs */
    BMainWindow_Init(&app->mainwindow);
    if (!app->mainwindow.lockedscreen) cleanexit("Can't lock screen");

    /* Create AukStyleSheet object */
    AukStyleSheet_New(&app->styleSheet);
    if (!app->styleSheet) cleanexit("Can't create stylesheet");

    /* Set stylesheet font specifications */
    app->styleSheet->SetFontTiny(app->styleSheet, "SevenAlone.font", 7);

    /* Open fonts from specifications */
    app->styleSheet->ApplyStyle( app->styleSheet,app->mainwindow.lockedscreen );
   //bdbprintf(" **** main init style:%08x fontTiny:%08x \n",(int)&app->styleSheet->style,(int)app->styleSheet->style.fontTiny);

    CreateHeaderView(&app->headerView, TargetInstance, &app->styleSheet->style);

    CreateTrackListView(&app->tracksListView, TargetInstance,&app->styleSheet->style);

    CreateFooterView(&app->footerView, TargetInstance, &app->styleSheet->style);

    /* Create status bar */
    {
        app->statusBarLabel = (Object *)NewObject(BUTTON_GetClass(), NULL,
          //  GA_DrawInfo, (ULONG)app->drawInfo,
            GA_ReadOnly, TRUE,
            BUTTON_BevelStyle, BVS_NONE,
            BUTTON_Transparent, TRUE,
            BUTTON_Justification, BCJ_LEFT,
            GA_Text, (ULONG)LOC(MSG_STATUS_READY),
            TAG_END);

        app->statusBarLayout = (Object *)NewObject(LAYOUT_GetClass(), NULL,
           //   GA_DrawInfo,(ULONG) app->drawInfo,
            LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
            LAYOUT_BevelStyle, BVS_SBAR_VERT,
            LAYOUT_AddChild,(ULONG) app->statusBarLabel,
            TAG_END);
    }

    /* create final layout */
    {
        app->mainvlayout = (Object *)NewObject( LAYOUT_GetClass(), NULL,
           //   GA_DrawInfo,(ULONG) app->drawInfo,
            LAYOUT_DeferLayout, TRUE, /* Layout refreshes done on task's context (by thewindow class)*/
            LAYOUT_SpaceOuter, TRUE,
            LAYOUT_BottomSpacing, 2,
            LAYOUT_TopSpacing,0,
            LAYOUT_LeftSpacing,0,
            LAYOUT_RightSpacing,0,
            LAYOUT_InnerSpacing,0,
            LAYOUT_Orientation, LAYOUT_ORIENT_VERT,

            LAYOUT_AddChild, app->headerView.mainHl,
                CHILD_WeightedHeight,0,
            LAYOUT_AddChild, app->tracksListView.mainVl,
                CHILD_WeightedHeight,4,
            LAYOUT_AddChild, app->footerView.mainHl,
                CHILD_WeightedHeight,0,
            LAYOUT_AddChild, app->statusBarLayout,
                CHILD_WeightedHeight,0,
            TAG_END);
        if (!app->mainvlayout) cleanexit("layout error 3");

        app->reportReq = NewObject(REQUESTER_GetClass(), NULL,
			// REQ_TitleText, "Project Generated",
			REQ_Image,REQIMAGE_INFO,
			REQ_BodyText,(ULONG)"....",
			REQ_GadgetText,(ULONG)"_Ok", //
            TAG_END);

    }




    app->app_port = CreateMsgPort();

    // projsettings_app_port = CreateMsgPort();

    y = 12;
    if(app->mainwindow.lockedscreen->Font) y = (app->mainwindow.lockedscreen->Font->ta_YSize) + 3 + 16;
    /* Create the window object. */
    app->window_obj = (Object *)NewObject( WINDOW_GetClass(), NULL,
        WA_Left, 40,
        WA_Top, (ULONG)y,
        WA_Width,320,
        WA_Height,240,
     //set by window or fullscreen   WA_CustomScreen, (ULONG) app->mainwindow.lockedscreen,
        WA_IDCMP, IDCMP_CLOSEWINDOW | IDCMP_MENUPICK | IDCMP_RAWKEY ,
        WA_Flags, WFLG_DRAGBAR | WFLG_DEPTHGADGET | WFLG_CLOSEGADGET | WFLG_SIZEGADGET | WFLG_ACTIVATE | WFLG_SMART_REFRESH,
        WA_Title,(ULONG) "Aukadicty",
        WINDOW_ParentGroup,(ULONG) app->mainvlayout,
        WINDOW_IconifyGadget, TRUE,
  //re      WINDOW_Icon,(ULONG) GetDiskObject("PROGDIR:ReAction"),
        WINDOW_IconTitle,(ULONG)  "Aukadicty",
        WINDOW_AppPort, (ULONG)app->app_port,
    TAG_END);
    if(!app->window_obj) cleanexit("can't create window");

    /* Initialize Project Settings window */
    if(!ProjectSettingsView_Init(&app->projectSettingsView,
                                  app->mainwindow.lockedscreen,
                                  LOC(MSG_SETTINGS_PROJECT)))
    {
        printf("Warning: Could not create Project Settings window\n");
    }

    /* Apply loaded temp dir to settings view */
    {
        const char *tempDir = AppSettings_GetTempDir(&app->appSettings);
        if (tempDir) {
            ProjectSettingsView_SetTempDir(&app->projectSettingsView, tempDir);
        }
    }

    /*  Open the window or screen. */
   // BMainWindow_SwitchToWB(&app->mainwindow,app->window_obj,&app->appSettings);
     BMainWindow_Show(&app->mainwindow,app->window_obj,&app->appSettings);

    if(!CurrentMainWindow) cleanexit("can't open window");

    {
        ULONG winsignal;        

        BOOL ok = TRUE;

        /* Obtain the window wait signal mask.*/
        GetAttr(WINDOW_SigMask, app->window_obj, &winsignal);

        /* Obtain the window wait signal mask also for this window */

//        { ULONG s; GetAttr(WINDOW_SigMask, app->window_prefs, &s);
//         printf("\n **** signal for prefswin:%08x\n",(int)s);
//         winsignal |= s; }

        /* Input Event Loop */
        while (ok)
        {
            ULONG result,currentSignals,waitedSignals;

            flushbdbprint();
            /* What to wait for ? */
            waitedSignals = winsignal |  /* window boopsi level wait port (different than Window->UserPort ?)*/
                        (1L << app->app_port->mp_SigBit) | /* apparently just for uniconify ? */
                        SIGBREAKF_CTRL_C |  /* will quit on Ctrl-C */
                        SIGBREAKF_CTRL_F    /* we use this available signal for additional messaging (asked by logs and messaging delayed). */
                        ;
            /* if subsidiary windows are open, we also listen them while they are open. */
            waitedSignals |= ProjectSettingsView_GetSignalMask(&app->projectSettingsView);

            /* AmigaOS magic, GUI process uses 0% cpu if it has nothing to do,
             * then wake up when anything needs it. */
            currentSignals = Wait(waitedSignals);

            /* exit app at any moment from Ctrl-C signal, atexit() magic does anything needed. */
            if(currentSignals & SIGBREAKF_CTRL_C) exit(0);

            /* CA_HandleInput() returns the gadget ID of a clicked
             * gadget, or one of several pre-defined values.  For
             * this demo, we're only actually interested in a
             * close window and a couple of gadget clicks.
             */
            while ((result = DoMethod(app->window_obj, WM_HANDLEINPUT, /*code*/NULL)) != WMHI_LASTMSG)
            {
            flushbdbprint();
                switch(result & WMHI_CLASSMASK)
                {
                   case WMHI_RAWKEY:
                        // quit on "esc down" key.
                        if((result & WMHI_KEYMASK) == 0x45) ok = FALSE;
                        break;
                    case WMHI_CLOSEWINDOW:
                        // quit on window close gadget
                        ok = FALSE;
                        break;

                    case WMHI_GADGETUP: /* the quick way to get button events at this level. */
                    {
                        /* releasing a button is only sent here
                          Pass it the same way gadget details are sent to
                          TargetInstance  OM_NOTIFY.*/

                        ULONG senderId = result & WMHI_GADGETMASK;
                        BoopsiDelay_BeginMessage(DelayQueue, senderId);
                        BoopsiDelay_AddTag(DelayQueue,WMHI_GADGETUP,1);
                        BoopsiDelay_EndMessage(DelayQueue);

                        break;
                    }
                    case WMHI_ICONIFY:
                        {
                            BMainWindow_Iconify(&app->mainwindow,app->window_obj);
                        }
                        break;
                    case WMHI_UNICONIFY:
                        {
                           BMainWindow_Show(&app->mainwindow,app->window_obj,&app->appSettings);
                            if (!CurrentMainWindow) cleanexit("can't re-open window");
                        }
                        break;
                    case WMHI_MENUPICK: // and not WMHI_POPUPMENU:
//                        {    
//                            AukAction *action = AukMenu_ToAction(&app->appMenu,result & WMHI_MENUMASK);
                        {
                            UWORD menuNum = result & WMHI_MENUMASK;
                            LONG actionID = AukMenu_ToActionID(&app->mainwindow.appMenu, menuNum);
                            AukAction *action = (actionID >= 0) ? AukAction_Get(actionID) : NULL;
                            if(action)
                            {
                                struct AukActionContext actionContext;
                                memset(&actionContext, 0, sizeof(actionContext));
                                actionContext.pproject = &app->_project;
                                actionContext.appWindow = CurrentMainWindow;
                                actionContext.appData = TargetInstance;
                                actionContext.trackListView = &app->tracksListView;
                               //old action->func(&actionContext);
                                actionContext.appSettings = &app->appSettings;

                                /* Set recent file index if applicable */
                                if (actionID >= ACTION_RECENT_FILE_0 && actionID <= ACTION_RECENT_FILE_7) {
                                    actionContext.recentFileIndex = actionID - ACTION_RECENT_FILE_0;
                                }

                                if (action->func(&actionContext)) {
                                    /* Rebuild menu if recent files may have changed */
                                    if (actionID == ACTION_PROJECT_OPEN ||
                                        actionID == ACTION_PROJECT_SAVE ||
                                        actionID == ACTION_PROJECT_SAVEAS ||
                                        (actionID >= ACTION_RECENT_FILE_0 && actionID <= ACTION_RECENT_FILE_7))
                                    {
                                        AukMenu_Rebuild(&app->mainwindow.appMenu, app->mainwindow.lockedscreen,
                                                        CurrentMainWindow, &app->appSettings);
                                    }
                                }
                            }
                        }
                        break;

                    default:
                        break;
                }


            } // end while messages

            /* other windows management : */
            /*  Project Settings window */
            ProjectSettingsView_HandleInput(&app->projectSettingsView);

            /* removing gadgets children that are currently in action may crash when disposed
                    ie: the trackheader exit button. In a general way it's better to do
                    the effective DisposeObject() on detached gadget, a round later.
              */
            if(ObjectLateDisposer->count>0) BoopsiDispose_Flush(ObjectLateDisposer);


            /* Process delayed BOOPSI notifications
                So now, we are in the main process where
                all buttons, sliders, and other UI action
                should be applied !
            */
            if (BoopsiDelay_HasMessages(DelayQueue))
            {
                struct TagItem *msg;
                while ((msg = BoopsiDelay_NextMessage(DelayQueue)) != NULL)
                {
                    struct opUpdate opUpd;
                    struct TagItem *ptag;
                    ULONG sender_ID = 0;

                    opUpd.MethodID = OM_UPDATE;
                    opUpd.opu_AttrList = msg;
                    opUpd.opu_GInfo = NULL;
                    opUpd.opu_Flags = 0;

                    if ((ptag = FindTagItem(GA_ID, msg)) != NULL)
                        sender_ID = ptag->ti_Data;

                    if (sender_ID >= GAD_HEADER_REWIND && sender_ID <= GAD_HEADER_FORWARD)
                    {
                        /* TODO: transport buttons */
                    }
                    else if (sender_ID >= GAD_HEADER_EDITMODE_FIRST && sender_ID <= GAD_HEADER_EDITMODE_LAST)
                    {
                        int modeChange =HeaderView_ListenMessage(&app->headerView,&opUpd,sender_ID);
                        /* -1 means no mode change */
                        if(modeChange>=0)
                        {
                            CurrentEditMode = modeChange;
                            TrackListView_SetEditMode(&app->tracksListView,modeChange);
                        }

                    }
                    else if (sender_ID == GAD_TRACKLIST)
                    {
                        TrackListView_ListenTrackListMessage(&app->tracksListView, &opUpd);
                    }
                    else if (sender_ID == GAD_SCROLLER_V)
                    {
                        TrackListView_ListenScrollVMessage(&app->tracksListView, &opUpd);
                    }
                    else if (sender_ID == GAD_SCROLLER_H)
                    {
                        TrackListView_ListenScrollHMessage(&app->tracksListView, &opUpd);
                    }
                    else if (sender_ID >= GAD_TRACKHEADER_BASE)
                    {
                        TrackListView_ListenTrackHeaderMessage(&app->tracksListView, &opUpd, sender_ID);
                    }
                }
            } // end if any delayed messages

            // delay some tracklayout messages to avoid big graphic update recursion
            if(app->tracksListView.updateBits)
            {
                TrackListView_CheckUpdates(&app->tracksListView);
            }

             /* debug purpose: init a project after all boopsi inits and starting messages proceceed once*/
             if(!testprojectinited)
             {
                initProject();
                TrackListView_UpdateTrackList(&app->tracksListView);
                TrackListView_UpdateTimeRule(&app->tracksListView);
                testprojectinited = 1;
             }

        } // end while app loop
    } // loop paragraph end

    // all close done in exitclose().
    return 0;
}

extern int AukObjectCount;

void exitclose(void)
{
    flushbdbprint();
    printf("exitclose()\n");
    if(app)
    {
        /* Save app settings (recent files, temp dir) before closing */
        {
            const char *tempDir = ProjectSettingsView_GetTempDir(&app->projectSettingsView);
            if (tempDir) {
                AppSettings_SetTempDir(&app->appSettings, tempDir);
            }
        }
        AppSettings_Save(&app->appSettings);
        AppSettings_Close(&app->appSettings);

        /* just release data listener and object retained */
        CloseHeaderView(&app->headerView);
        CloseTrackListView(&app->tracksListView);
        CloseFooterView(&app->footerView);



        /* Disposing of the window object will also close the
         * window if it is already opened and it will dispose of
         * all objects attached to it.
         */
        if(app->reportReq) DisposeObject( app->reportReq );
            printf("app->window_obj:%08x\n",(int)app->window_obj);

        // this should cascade all OM_DISPOSE:

        ProjectSettingsView_Dispose(&app->projectSettingsView);

        if(app->window_obj)
        {
           BMainWindow_Close(&app->mainwindow,app->window_obj);
            DisposeObject(app->window_obj);
        }
        CurrentMainWindow = NULL;

        if( ObjectLateDisposer)
        {
            BoopsiDispose_Flush(ObjectLateDisposer);
            FreeVec(ObjectLateDisposer);
         }
        ObjectLateDisposer = NULL;

        /* Shutdown sound file engine */

        if (soundFileEngine) {
            AukSoundFileEngine_Shutdown(soundFileEngine);
            soundFileEngine = NULL;
        }

        /* Release stylesheet object (will close fonts automatically) */
        if (app->styleSheet) {
            AukObjectPtr_Release((AukObjectPtr*)&app->styleSheet);
        }

        /* Delete message port */
        if (app->app_port) DeleteMsgPort(app->app_port);
        /*test if (projsettings_app_port) DeleteMsgPort(projsettings_app_port);
        projsettings_app_port = NULL;*/
        FreeVec(app);
        app = NULL;
    }

    closeMessageTargetModel();

    CloseTrackListView_StaticClasses();

    // debug mode, check private class gadgets instance areall closed.
    bdbprintf_report_leaks();
    bdbprintf_report_classes();

    /* Close localization system */
    AukLocale_Close();

    if(AukObjectCount !=0)
    {
        printf(" **** AukObject leaks: %d ****\n");
    }

    /* Close all libraries via table (in reverse order) */
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


void openAboutReq()
{
 static const char *p= "...";
    SetAttrs(app->reportReq,REQ_TitleText,(ULONG)"About...",TAG_END);
    // if ok, show a report requester

    SetAttrs(app->reportReq,REQ_BodyText,(ULONG)p,TAG_END);

    OpenRequester(app->reportReq,CurrentMainWindow);

}


int initProject()
{
    AukAProject* project;
    AukTrack* track1;
    AukTrack* track2;
    AukSoundFilePtr soundFile1 = NULL;
    AukSoundFilePtr soundFile2 = NULL;
    AukSound* sound1;
    AukSound* sound2;
    AukFixed duration;

    if(!app) return 1;

    /* Create a new project */
    AukAProject_New((AukObjectPtr *)&app->_project);
    project = app->_project;
    if (!project) {
        printf("Failed to create project\n");
        return 1;
    }
    // link document with UI
    TrackListView_setProject(&app->tracksListView,project);
    FooterView_SetProject(&app->footerView,project);


    // /* Set project properties */
    // project->base.SetName(&project->base, "My First Project");
    // project->base.SetPath(&project->base, "Work:");
    // AukAProject_SetPreferences(project, 44100, 16);



     /* Update footer with project frequency */
     FooterView_UpdateFrequency(&app->footerView, 44100);

//     /* Create tracks in the project */
//     track1 = project->CreateTrack(project);
//     track2 = project->CreateTrack(project);

//  project->CreateTrack(project);
// project->CreateTrack(project);


//     if (!track1 || !track2) {
//         printf("Failed to create tracks\n");
//         AukObjectPtr_Release((AukObjectPtr*)&app->_project);
//         return 1;
//     }
//     AukTrack_SetName(track1, "Vocals, like that");
//     AukTrack_SetName(track2, "Music");



//     /* Create a sound file reference */
//     AukSoundFile_New((AukObjectPtr*)&soundFile1);
//     if (!soundFile1) {
//         printf("Failed to create sound file\n");
//         AukObjectPtr_Release((AukObjectPtr*)&app->_project);
//         return 1;
//     }
//     AukSoundFile_SetFilename(soundFile1, "sounds/sample1.wav");
//     AukSoundFile_SetProperties(soundFile1, 44100, 2, 88200,2);


//     AukSoundFile_New((AukObjectPtr*)&soundFile2);
//     if (!soundFile2) {
//         printf("Failed to create sound file\n");
//         AukObjectPtr_Release((AukObjectPtr*)&app->_project);
//         return 1;
//     }
//     AukSoundFile_SetFilename(soundFile2, "sounds/sample2.wav");
//     AukSoundFile_SetProperties(soundFile2, 22050, 1, 88200,2);


//     /* Create sounds on tracks */
//     sound1 = track1->CreateSound(track1, soundFile2,
//                                  AukFixed_FromInt(0),    /* Start at 0 seconds */
//                                  AukFixed_FromInt(5));   /* End at 5 seconds */


//     sound2 = track2->CreateSound(track2, soundFile1,
//                                  AukFixed_FromInt(2),    /* Start at 2 seconds */
//                                  AukFixed_FromInt(8));   /* End at 8 seconds */


//     if (!sound1 || !sound2) {
//         printf("Failed to add sounds\n");
//         AukObjectPtr_Release((AukObjectPtr*)&soundFile1);
//         AukObjectPtr_Release((AukObjectPtr*)&app->_project);
//         return 1;
//     }


//     /* Release our reference to sound file (sounds now own it) */
//     AukObjectPtr_Release((AukObjectPtr*)&soundFile1);


//     /* Set sound properties */
//     sound1->SetLoopCount(sound1, 2);  /* Loop twice */


//     /* Add envelope points to track1 */
//     track1->AddEnvelopePoint(track1,
//                              AukFixed_FromDouble(-0.25),
//                              0x0100);  /* Full volume at start (0x0100 = 1.0) */
//     track1->AddEnvelopePoint(track1,
//                              AukFixed_FromInt(5),
//                              0x0080);  /* Half volume at 5 seconds (0x0080 = 0.5) */


//     /* Get project duration */
//     duration = project->GetDuration(project);
//     printf("Project duration: %d seconds\n",(int) AukFixed_ToInt(duration));

//     /* Save project to JSON file */
// //    if (project->base.Save(project, "my_project.auk")) {
// //        printf("Project saved successfully\n");
// //    } else {
// //        printf("Failed to save project\n");
// //    }

//     /* Display project info */
//     printf("Project: %s\n", project->base.GetName(project));
//     printf("Tracks: %d\n", (int)project->GetTrackCount(project));
//     printf("Track 1: %s, Sounds: %d\n",
//            AukTrack_GetName(track1),
//            (int)track1->GetSoundCount(track1));
//     printf("Track 2: %s, Sounds: %d\n",
//            AukTrack_GetName(track2),
//            (int)track2->GetSoundCount(track2));
    return 0;
}

void TrackListView_UpdateTrackList_Generic()
{
    if(!app) return;
   // bdbprintf("TrackListView_UpdateTrackList_Generic-> TLVB_UPDATE_REDRAW_TRACKLIST\n");
    //TrackListView_UpdateTrackList(&app->tracksListView);
        app->tracksListView.updateBits |= TLVB_UPDATE_REDRAW_TRACKLIST;
        if(myTask) Signal(myTask,SIGBREAKF_CTRL_F);
}

void TrackListView_UpdateTrackList_Headers()
{
    if(!app) return;
   // bdbprintf("TrackListView_UpdateTrackList_Headers-> TLVB_UPDATE_REDRAW_JUSTHEADERS\n");
    //TrackListView_UpdateTrackList(&app->tracksListView);
        app->tracksListView.updateBits |= TLVB_UPDATE_REDRAW_JUSTHEADERS;
        if(myTask) Signal(myTask,SIGBREAKF_CTRL_F);
}

/* GUI Error/Log system - maps error IDs to locale strings */

/* Map AukErrorID to MSG_GUI_xxx locale string ID */
static ULONG AukErrorToMsgID(AukErrorID errorID)
{
    /* The MSG_GUI_xxx enums start at MSG_GUI_TRACKLIST_CAPACITY_REACHED
     * and follow the same order as AukErrorID */
    return MSG_GUI_TRACKLIST_CAPACITY_REACHED + (ULONG)errorID;
}

static const char *AukLogLevelPrefix(AukLogLevel level)
{
    switch (level) {
        case AUKLOG_INFO:    return "[Info] ";
        case AUKLOG_WARNING: return "[Warn] ";
        case AUKLOG_ERROR:   return "[Error] ";
        default:             return "";
    }
}

/* Static buffer for status bar text */
static char statusBarBuffer[256];

/* Update the status bar gadget with the current buffer content */
static void UpdateStatusBar(void)
{
    if (app && app->statusBarLabel && CurrentMainWindow) {
        SetGadgetAttrs((struct Gadget *)app->statusBarLabel,
            CurrentMainWindow, NULL,
            GA_Text, (ULONG)statusBarBuffer,
            TAG_END);
    }
}

void AukLog_Message(AukLogLevel level, AukErrorID errorID)
{
    const char *msg;
    ULONG msgID;

    msgID = AukErrorToMsgID(errorID);
    msg = LOC(msgID);

    bdbprintf("%s%s\n", AukLogLevelPrefix(level), msg);

    /* Update status bar */
    snprintf(statusBarBuffer, sizeof(statusBarBuffer), "%s%s", AukLogLevelPrefix(level), msg);
    UpdateStatusBar();
}

void AukLog_MessageInt(AukLogLevel level, AukErrorID errorID, LONG param)
{
    const char *msg;
    char tempBuffer[200];
    ULONG msgID;

    msgID = AukErrorToMsgID(errorID);
    msg = LOC(msgID);

    bdbprintf("%s", AukLogLevelPrefix(level));
    bdbprintf(msg, param);
    bdbprintf("\n");

    /* Update status bar */
    snprintf(tempBuffer, sizeof(tempBuffer), msg, param);
    snprintf(statusBarBuffer, sizeof(statusBarBuffer), "%s%s", AukLogLevelPrefix(level), tempBuffer);
    UpdateStatusBar();
}

void AukLog_MessageStr(AukLogLevel level, AukErrorID errorID, const char *param)
{
    const char *msg;
    char tempBuffer[200];
    ULONG msgID;

    msgID = AukErrorToMsgID(errorID);
    msg = LOC(msgID);

    bdbprintf("%s", AukLogLevelPrefix(level));
    bdbprintf(msg, param);
    bdbprintf("\n");

    /* Update status bar */
    snprintf(tempBuffer, sizeof(tempBuffer), msg, param);
    snprintf(statusBarBuffer, sizeof(statusBarBuffer), "%s%s", AukLogLevelPrefix(level), tempBuffer);
    UpdateStatusBar();
}
