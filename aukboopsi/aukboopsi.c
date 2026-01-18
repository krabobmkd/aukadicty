
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

#include <proto/virtual.h>
#include <gadgets/virtual.h>

#include <proto/requester.h>
#include <classes/requester.h>

#include <proto/scroller.h>
#include <gadgets/scroller.h>

#include <proto/slider.h>
#include <gadgets/slider.h>

#include <proto/asl.h>
#include <libraries/asl.h>

#include "gadgetid.h"
#include "TrackListView.h"
#include "HeaderView.h"
#include "FooterView.h"
#include "auklocale.h"
#include "aukerrors.h"
#include "aukaction.h"
#include "aukstylesheet.h"
#include "aukmenu.h"
#include "boopsidelay.h"
#include "TrackListArea/class_tracklistarea.h"
//#include "aukaproject.h"
#include <aukadicty.h>

#include "compilers.h"
#include "bdbprintf.h"
INLINE struct Window *boopsi_OpenWindow(Object *owin) {
    return  (struct Window *)DoMethod(owin, WM_OPEN, NULL);
}


typedef ULONG (*REHOOKFUNC)();

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
// usefull union for dispatchers. Each structs also starts with MethodID.
typedef union MsgUnion
{
  ULONG  MethodID;
  // from classusr.h or gadgetclass.h, all starts with MethodID.
  struct opSet        opSet;
  struct opUpdate     opUpdate;
  struct opGet        opGet;
  struct gpHitTest    gpHitTest;
  struct gpRender     gpRender;
  struct gpInput      gpInput;
  struct gpGoInactive gpGoInactive;
  struct gpLayout     gpLayout;
} *Msgs;

// all app related variables are here:
struct App
{
    Object *window_obj; // window as boopsi object
    // now it's CurrentMainWindow struct Window *win; // current re-opened windows, as a classic intuition Window.

    struct MsgPort *app_port;

    struct Screen *lockedscreen;
    struct DrawInfo *drawInfo; // informations on how to draw on the screen, passed to gagdets.

    AukStyleSheetPtr styleSheet; /* shared stylesheet instance pointer */

    AukMenu appMenu; /* GadTools menu */

    Object *mainvlayout;

            Object* btAbout;

        HeaderView headerView;
        TrackListView tracksListView;
        FooterView footerView;

     Object *reportReq;

     // - - - retain document object
     AukAProjectPtr _project;

     // - - - delayed BOOPSI notification queue
     BoopsiDelayQueue delayQueue;
};
// - - - note having a private "boopsi object class and instance"
// - - - makes it fancy to connect values and receive events.
// Boopsi class pointer to manage our private modelclass.
Class *AppModelClass = NULL;
// App Model instance as a Boopsi object.
Object *AppInstance = NULL;
// App Modelinstance as our private struct.
struct App *app=NULL;
// This is the intuition level Window, on OS3 it's recreated when iconizing/reopening !
// when  iconizing/reopening BOOPSI objects are kept, but Intuition level instances and buffers are wiped out.
// Yet, it's needed for most Gadget method calls, and this is not retained by boopsi objects.
// note there vould be many windows.
struct Window *CurrentMainWindow=NULL;

/* The attribs we actually delay
*/
static ULONG delayedAttribs[]={
    GA_Selected,SLIDER_Level,SCROLLER_Top,
    TRACKLIST_ScrollY,TRACKLIST_TimeProjection,TRACKLIST_DomainHeight
};
#define nbDelayedAttribs (sizeof(delayedAttribs)/sizeof(ULONG))

ULONG ASM SAVEDS AppModelDispatch(
                    REG(a0,struct IClass *C),
                    REG(a2,Object *obj),
                    REG(a1,union MsgUnion *M))
{
  ULONG retval=0;

  switch(M->MethodID)
  {
    case OM_NEW:
        if((obj=(Object *)DoSuperMethodA(C,(Object *)obj,(Msg)M))!= NULL)
        {
            app=(struct App *)INST_DATA(C, obj);
            memset(app,0,sizeof(struct App)); // absolutely *NOT* sure about this being cleaned, more secure.
            BoopsiDelay_Init(&app->delayQueue);
            retval = (ULONG)obj;
        }
    break;
    case OM_DISPOSE:
        retval=DoSuperMethodA(C,(Object *)obj,(Msg)M);
      break;
    case OM_NOTIFY:
    case OM_UPDATE:
        {
            struct TagItem *ptag;
            ULONG sender_ID=0;

            if((ptag = FindTagItem( GA_ID,M->opUpdate.opu_AttrList ))!=NULL) sender_ID = ptag->ti_Data;

            /* Queue message if sender_ID != 0 */
            if (sender_ID != 0)
            {
                int i;
                BoopsiDelay_BeginMessage(&app->delayQueue, sender_ID);
                for(i=0;i<nbDelayedAttribs;i++)
                {
                    if ((ptag = FindTagItem(delayedAttribs[i], M->opUpdate.opu_AttrList)) != NULL)
                        BoopsiDelay_AddTag(&app->delayQueue, delayedAttribs[i], ptag->ti_Data);
                }

                BoopsiDelay_EndMessage(&app->delayQueue);

                /* Signal main loop to process queue */
                if (myTask) Signal(myTask, SIGBREAKF_CTRL_F);

                retval = 1;
            }
        }
        break;
    default:
        retval=DoSuperMethodA(C,(Object *)obj,(Msg)M);
    break;
  }
  return retval;
}

int initAppModel(void)
{
    // this is how you create a private transient class:
    // -First param: no name needed for itself.
    // - "modelclass" is super class name, which is the base for all boopsi class.
    // a super class name or pointer must always be provided.
    AppModelClass = MakeClass(NULL,"modelclass",NULL,sizeof(struct App),0);
    if(!AppModelClass) return 0;
    bdbprintf_makeclass("AppModel", AppModelClass);

    AppModelClass->cl_Dispatcher.h_Entry = (REHOOKFUNC) &AppModelDispatch;

    AppInstance = (Object *)NewObject( AppModelClass, NULL, TAG_DONE);
    if(!AppInstance) return 0;

    return 1;
}
void closeAppModel(void)
{
    if(AppInstance) DisposeObject(AppInstance);
    AppInstance = NULL;
    app=NULL;
    if(AppModelClass)
    {
        bdbprintf_freeclass("AppModel", AppModelClass);
        FreeClass(AppModelClass);
    }
    AppModelClass = NULL;
}
static int testprojectinited=0;
int initProject();
//  - - - -- - - - -  end of App modelclass management.

int main(int argc, char **argv)
{
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

    /* Initialize localization system */
    AukLocale_Init("aukadicty.catalog", 1);

    /* Initialize action system (after locale init) */
    AukAction_Init();

    if(!initAppModel())  cleanexit("Can't create app");
printf("AppInstance %08x\n",AppInstance);
    /* BOOPSI needs */
    app->lockedscreen = LockPubScreen(NULL);
    if (!app->lockedscreen) cleanexit("Can't lock screen");

    app->drawInfo = GetScreenDrawInfo(app->lockedscreen);

    /* Create AukStyleSheet object */
    AukStyleSheet_New(&app->styleSheet);
    if (!app->styleSheet) cleanexit("Can't create stylesheet");

    /* Set stylesheet font specifications */
    app->styleSheet->SetFontTiny(app->styleSheet, "SevenAlone.font", 7);

    /* Open fonts from specifications */
    app->styleSheet->ApplyStyle( app->styleSheet,app->lockedscreen );
   bdbprintf(" **** main init style:%08x fontTiny:%08x \n",(int)&app->styleSheet->style,(int)app->styleSheet->style.fontTiny);

    CreateHeaderView(&app->headerView, app->drawInfo, AppInstance, &app->styleSheet->style);

    CreateTrackListView(&app->tracksListView,app->drawInfo, AppInstance,&app->styleSheet->style);

    CreateFooterView(&app->footerView, app->drawInfo, AppInstance, &app->styleSheet->style);

    /* create final layout */
    {
        app->mainvlayout = (Object *)NewObject( LAYOUT_GetClass(), NULL,
            GA_DrawInfo, app->drawInfo,
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
            TAG_END);
        if (!app->mainvlayout) cleanexit("layout error 3");

        app->reportReq = NewObject(REQUESTER_GetClass(), NULL,
			// REQ_TitleText, "Project Generated",
			REQ_Image,REQIMAGE_INFO,
			REQ_BodyText,"....",
			REQ_GadgetText,(ULONG)"_Ok", //
            TAG_END);

    }

    app->app_port = CreateMsgPort();

    /* Create the window object. */
    app->window_obj = (Object *)NewObject( WINDOW_GetClass(), NULL,
        WA_Left, 40,
        WA_Top, (ULONG)(app->lockedscreen->Font->ta_YSize) + 3 + 16,
        WA_Width,320,
        WA_Height,240,
        WA_CustomScreen, (ULONG) app->lockedscreen,
        WA_IDCMP, IDCMP_CLOSEWINDOW | IDCMP_MENUPICK | IDCMP_RAWKEY | IDCMP_IDCMPUPDATE ,
        WA_Flags, WFLG_DRAGBAR | WFLG_DEPTHGADGET | WFLG_CLOSEGADGET | WFLG_SIZEGADGET | WFLG_ACTIVATE | WFLG_SMART_REFRESH,
        WA_Title,(ULONG) "Aukadicty",
        WINDOW_ParentGroup,(ULONG) app->mainvlayout,
        WINDOW_IconifyGadget, TRUE,
  //re      WINDOW_Icon,(ULONG) GetDiskObject("PROGDIR:ReAction"),
        WINDOW_IconTitle,(ULONG)  "Aukadicty",
        WINDOW_AppPort, (ULONG)app->app_port,
    TAG_END);
    if(!app->window_obj) cleanexit("can't create window");

    /*  Open the window. */
    CurrentMainWindow = boopsi_OpenWindow(app->window_obj);
    if(!CurrentMainWindow) cleanexit("can't open window");

    /* Create and attach menus */
    if (!AukMenu_Create(&app->appMenu, app->lockedscreen, CurrentMainWindow)) {
        printf("Warning: Could not create menus\n");
    }



    {
        ULONG winsignal;
        BOOL ok = TRUE;

        /* Obtain the window wait signal mask.*/
        GetAttr(WINDOW_SigMask, app->window_obj, &winsignal);

        /* Input Event Loop */
        while (ok)
        {
            ULONG result,currentSignals,waitedSignals;

            flushbdbprint();
            /* What to wait for ? */
            waitedSignals = winsignal |  // window boopsi level wait port (different than intuition level ?)
                        (1L << app->app_port->mp_SigBit) |
                        SIGBREAKF_CTRL_C |  // quit on Ctrl-C
                        SIGBREAKF_CTRL_F    // we use that as special refresh if something happen.
                        ;

            currentSignals = Wait(waitedSignals);

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

                    case WMHI_GADGETUP:
                    {
                        if((result>>16) >=GAD_TRACKHEADER_BASE)
                        {
                        printf("WMHI_GADGETUP:%08x\n",result>>16);
                        }
//                        if(gid == GAD_BUTTON_ABOUT)
//                        {
//                            openAboutReq();
//                        }
                        break;
                    }
                    case WMHI_ICONIFY:
                        {
                            AukMenu_Close(&app->appMenu, CurrentMainWindow);
                            if(DoMethod(app->window_obj, WM_ICONIFY, NULL)) CurrentMainWindow = NULL;
                        }
                        break;

                    case WMHI_UNICONIFY:
                        {
                            CurrentMainWindow = boopsi_OpenWindow(app->window_obj);
                            if (!CurrentMainWindow) cleanexit("can't re-open window");
                            /* re-Create and attach menus */
                            if (!AukMenu_Create(&app->appMenu, app->lockedscreen,CurrentMainWindow)) {
                                cleanexit("Warning: Could not re-create menus\n");
                            }
                        }
                        break;
                    case WMHI_MENUPICK: // and not WMHI_POPUPMENU:
                        {    
                            AukAction *action = AukMenu_ToAction(&app->appMenu,result & WMHI_MENUMASK);
                            if(action)
                            {
                                struct AukActionContext actionContext;
                                actionContext.project = app->_project;
                                actionContext.appWindow = CurrentMainWindow;
                                actionContext.appData = AppInstance;
                                action->func(&actionContext);
                            }
                        }
                        break;

                    default:
                        break;
                }


            } // end while messages

            /* Process delayed BOOPSI notifications
                So now, we are in the main process where
                all buttons, sliders, and other UI action
                should be applied !
            */
            if (BoopsiDelay_HasMessages(&app->delayQueue))
            {
                struct TagItem *msg;
                while ((msg = BoopsiDelay_NextMessage(&app->delayQueue)) != NULL)
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
                    else if (sender_ID >= GAD_HEADER_EDITMODE1 && sender_ID <= GAD_HEADER_EDITMODE6)
                    {
                        bdbprintf("Edit mode button: %08x\n", sender_ID);
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

void exitclose(void)
{
           flushbdbprint();
    printf("exitclose()\n");
    if(app)
    {
        /* just release data listener and object retained */
        CloseHeaderView(&app->headerView);
        CloseTrackListView(&app->tracksListView);
        CloseFooterView(&app->footerView);

        /* Close menus before closing window */
        AukMenu_Close(&app->appMenu, CurrentMainWindow);

        /* Disposing of the window object will also close the
         * window if it is already opened and it will dispose of
         * all objects attached to it.
         */
        if(app->reportReq) DisposeObject( app->reportReq );
            printf("app->window_obj:%08x\n",(int)app->window_obj);

        // this should cascade all OM_DISPOSE:
        if(app->window_obj) DisposeObject(app->window_obj);
        CurrentMainWindow = NULL;


        /* Release stylesheet object (will close fonts automatically) */
        if (app->styleSheet) {
            AukObjectPtr_Release((AukObjectPtr*)&app->styleSheet);
        }

        if(app->drawInfo) FreeScreenDrawInfo(app->lockedscreen, app->drawInfo);
        if(app->lockedscreen) UnlockPubScreen(0, app->lockedscreen);

    }
    /* Delete message port */
    if (app->app_port) DeleteMsgPort(app->app_port);

    closeAppModel();

    CloseTrackListView_StaticClasses();

    // debug mode, check private class gadgets instance areall closed.
    bdbprintf_report_leaks();
    bdbprintf_report_classes();

    /* Close localization system */
    AukLocale_Close();

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

    /* Set project properties */
    project->base.SetName(&project->base, "My First Project");
    project->base.SetPath(&project->base, "Work:");
    AukAProject_SetPreferences(project, 44100, 16);

    /* Update footer with project frequency */
    FooterView_UpdateFrequency(&app->footerView, 44100);

    /* Create tracks in the project */
    track1 = project->CreateTrack(project);
    track2 = project->CreateTrack(project);

 project->CreateTrack(project);
project->CreateTrack(project);
//project->CreateTrack(project);
//project->CreateTrack(project);
//project->CreateTrack(project);
//project->CreateTrack(project);

    if (!track1 || !track2) {
        printf("Failed to create tracks\n");
        AukObjectPtr_Release((AukObjectPtr*)&app->_project);
        return 1;
    }
    AukTrack_SetName(track1, "Vocals, like that");
    AukTrack_SetName(track2, "Music");


    /* Create a sound file reference */
    AukSoundFile_New((AukObjectPtr*)&soundFile1);
    if (!soundFile1) {
        printf("Failed to create sound file\n");
        AukObjectPtr_Release((AukObjectPtr*)&app->_project);
        return 1;
    }
    AukSoundFile_SetFilename(soundFile1, "sounds/sample1.wav");
    AukSoundFile_SetProperties(soundFile1, 44100, 2, 88200);

    /* Create sounds on tracks */
    sound1 = track1->CreateSound(track1, soundFile1,
                                 AukFixed_FromInt(0),    /* Start at 0 seconds */
                                 AukFixed_FromInt(5));   /* End at 5 seconds */

    sound2 = track2->CreateSound(track2, soundFile1,
                                 AukFixed_FromInt(2),    /* Start at 2 seconds */
                                 AukFixed_FromInt(8));   /* End at 8 seconds */

    if (!sound1 || !sound2) {
        printf("Failed to add sounds\n");
        AukObjectPtr_Release((AukObjectPtr*)&soundFile1);
        AukObjectPtr_Release((AukObjectPtr*)&app->_project);
        return 1;
    }

    /* Release our reference to sound file (sounds now own it) */
    AukObjectPtr_Release((AukObjectPtr*)&soundFile1);

    /* Set sound properties */
    sound1->SetLoopCount(sound1, 2);  /* Loop twice */

    /* Add envelope points to track1 */
    track1->AddEnvelopePoint(track1,
                             AukFixed_FromDouble(-0.25),
                             0x0100);  /* Full volume at start (0x0100 = 1.0) */
    track1->AddEnvelopePoint(track1,
                             AukFixed_FromInt(5),
                             0x0080);  /* Half volume at 5 seconds (0x0080 = 0.5) */

    /* Get project duration */
    duration = project->GetDuration(project);
    printf("Project duration: %d seconds\n",(int) AukFixed_ToInt(duration));

    /* Save project to JSON file */
//    if (project->base.Save(project, "my_project.auk")) {
//        printf("Project saved successfully\n");
//    } else {
//        printf("Failed to save project\n");
//    }

    /* Display project info */
    printf("Project: %s\n", project->base.GetName(project));
    printf("Tracks: %d\n", (int)project->GetTrackCount(project));
    printf("Track 1: %s, Sounds: %d\n",
           AukTrack_GetName(track1),
           (int)track1->GetSoundCount(track1));
    printf("Track 2: %s, Sounds: %d\n",
           AukTrack_GetName(track2),
           (int)track2->GetSoundCount(track2));
    return 0;
}

void TrackListView_UpdateTrackList_Generic()
{
    if(!app) return;
    //TrackListView_UpdateTrackList(&app->tracksListView);
        app->tracksListView.updateBits |= TLVB_UPDATE_REDRAW_TRACKLIST;
        if(myTask) Signal(myTask,SIGBREAKF_CTRL_F);
}

/* GUI Error/Log system - stub implementations for now */
void AukLog_Message(AukLogLevel level, AukErrorID errorID)
{
    /* TODO: Implement actual logging/display */
    (void)level;
    (void)errorID;
}

void AukLog_MessageInt(AukLogLevel level, AukErrorID errorID, LONG param)
{
    /* TODO: Implement actual logging/display with parameter */
    (void)level;
    (void)errorID;
    (void)param;
}
