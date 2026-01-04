
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


#include <proto/asl.h>
#include <libraries/asl.h>

#include "gadgetid.h"
#include "TrackListView.h"

//#include "aukaproject.h"
#include <aukadicty.h>

#include "compilers.h"
#include "bdbprintf.h"
INLINE struct Window *boopsi_OpenWindow(Object *owin) {
    return  (struct Window *)DoMethod(owin, WM_OPEN, NULL);
}


typedef ULONG (*REHOOKFUNC)();

struct Task	*myTask=NULL;

static const char *pVersion="$VER: 0.1";

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

// boopsi classes bases:
struct Library *WindowBase=NULL;
struct Library *LayoutBase=NULL;
struct Library *BitMapBase=NULL;
struct Library *ButtonBase=NULL;
struct Library *LabelBase=NULL;
struct Library *VirtualBase=NULL;

struct Library *CheckBoxBase=NULL;
struct Library *StringBase=NULL;
struct Library *TextFieldBase=NULL;
struct Library *RequesterBase=NULL;
struct Library *ScrollerBase=NULL;

void cleanexit(const char *pmessage)
{
    if(pmessage) printf("%s\n",pmessage);
    // will execute functions registered with atexit().
    // this way if C startup manages it, Ctrl-C will also close nicely.
    exit(0);
}
void exitclose(void);

void guiNotifier(int loglevel, const char *log);
// synchronize greying buttons...
void updateUIToStates();

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
    struct Window *win; // current re-opened windows, as a classic intuition Window.

    struct MsgPort *app_port;

    struct Screen *lockedscreen;
    struct DrawInfo *drawInfo; // informations on how to draw on the screen, passed to gagdets.

    AukStyleSheet styleSheet; /* shared stylesheet instance is now here */

    Object *mainvlayout;
        Object *horizontallayoutA;
         //   Object *titlelabel;
            Object* btAbout;

        TrackListView tracksListView;

            // status bar
        Object *horizontallayoutC;
            Object *bottombarlayout;
            Object* statusbarlabel;

     Object *reportReq;

     // - - - retain document object
     AukAProjectPtr _project;

};
// - - - note having a private "boopsi object class and instance"
// - - - makes it fancy to connect values and receive events.
// Boopsi class pointer to manage our private modelclass.
Class *AppModelClass = NULL;
// App Model instance as a Boopsi object.
Object *AppInstance = NULL;
// App Modelinstance as our private struct.
struct App *app=NULL;

ULONG ASM SAVEDS AppModelDispatch(
                    REG(a0,struct IClass *C),
                    REG(a2,Object *obj),
                    REG(a1,union MsgUnion *M))
{
    // Warning: this is executed on "intuition's context", can't use dos nor print, like for interupts.
  ULONG retval=0;
  switch(M->MethodID)
  {
    case OM_NEW:
        if(obj=(Object *)DoSuperMethodA(C,(Object *)obj,(Msg)M))
        {
            app=(struct App *)INST_DATA(C, obj);
            memset(app,0,sizeof(struct App)); // absolutely *NOT* sure about this being cleaned, more secure.
            retval = (ULONG)obj;
        }
    break;
    case OM_DISPOSE:
        retval=DoSuperMethodA(C,(Object *)obj,(Msg)M);
      break;
    case OM_UPDATE:
        {
            struct TagItem *ptag;
            // here receive events from gadgets which ICA_TARGET is appModel.
            ULONG sender_ID=0;
            if((ptag = FindTagItem( GA_ID,M->opUpdate.opu_AttrList ))!=NULL) sender_ID = ptag->ti_Data;
            // our gadget is notifying new clicked coordinates!
            // note any button action is either managed here or in more generic main loop

            if( sender_ID == GAD_TRACKLIST )
            {
                //   bdbprintf(" ( sender_ID == GAD_TRACKLIST )\n");
                TrackListView_ListenTrackListMessage( &app->tracksListView, &M->opUpdate );
                retval = 1;
            } else
            if( sender_ID == GAD_SCROLLER_V )
            {
            // bdbprintf(" ( sender_ID == GAD_SCROLLER_V )\n");
                TrackListView_ListenScrollVMessage( &app->tracksListView, &M->opUpdate );
                retval=1;
            } else
            if( sender_ID == GAD_SCROLLER_H )
            {
            // bdbprintf(" ( sender_ID == GAD_SCROLLER_H )\n");
                TrackListView_ListenScrollHMessage( &app->tracksListView, &M->opUpdate );
                retval=1;
            } else
            if( sender_ID >= GAD_TRACKHEADER_BASE)
            {
                TrackListView_ListenTrackHeaderMessage( &app->tracksListView, &M->opUpdate,sender_ID);
                retval=1;
            }
            //else{...}
            else
            {
                if(sender_ID != 0)
                {
                    bdbprintf(" ( sender_ID == %d )\n",sender_ID);
                }

                retval=DoSuperMethodA(C,(Object *)obj,(Msg)M);
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
    if(AppModelClass) FreeClass(AppModelClass);
    AppModelClass = NULL;
}

int initProject();
//  - - - -- - - - -  end of App modelclass management.

int main(int argc, char **argv)
{
    myTask = FindTask(NULL);
    atexit(&exitclose);

    // - - - - open libraries...

    if ( ! (IntuitionBase = (struct IntuitionBase*)OpenLibrary("intuition.library",39)))
        cleanexit("Can't open intuition.library");

    if ( ! (GfxBase = (struct GfxBase *)OpenLibrary("graphics.library",39)))
        cleanexit("Can't open graphics.library");

    if ( ! (UtilityBase = OpenLibrary("utility.library",39)))
        cleanexit("Can't open utility.library");

    if ( ! (LayersBase = OpenLibrary("layers.library",39)))
        cleanexit("Can't open layers.library");

    if ( ! (IconBase = OpenLibrary("icon.library",39)))
        cleanexit("Can't open icon.library");

    if ( ! (AslBase = OpenLibrary("asl.library",39)))
        cleanexit("Can't open asl.library");

    if ( ! (DiskfontBase = OpenLibrary("diskfont.library",39)))
        cleanexit("Can't open diskfont.library");
    // note: DOSBase is opened by C startup.

    // - - - - open boopsi classes...
    int mingadgetversion=45; // OS3.9, needed for virtual.
    if ( ! (WindowBase = OpenLibrary("window.class",mingadgetversion)))
        cleanexit("Can't open window.class");

    if ( ! (LayoutBase = OpenLibrary("gadgets/layout.gadget",mingadgetversion)))
        cleanexit("Can't open layout.gadget");

    if ( ! (BitMapBase = OpenLibrary("images/bitmap.image",mingadgetversion)))
        cleanexit("Can't open bitmap.image");

    if ( ! (ButtonBase = OpenLibrary("gadgets/button.gadget",mingadgetversion)))
        cleanexit("Can't open button.gadget");

    if ( ! (LabelBase = OpenLibrary("images/label.image",mingadgetversion)))
        cleanexit("Can't open label.image");

    if ( ! (VirtualBase = OpenLibrary("gadgets/virtual.gadget",mingadgetversion)))
        cleanexit("Can't open virtual.gadget");

   if ( ! (CheckBoxBase = OpenLibrary("gadgets/checkbox.gadget",mingadgetversion)))
       cleanexit("Can't open checkbox.gadget");

    if ( ! (StringBase = OpenLibrary("gadgets/string.gadget",mingadgetversion)))
        cleanexit("Can't open string.gadget");

    if ( ! (TextFieldBase = OpenLibrary("gadgets/texteditor.gadget",mingadgetversion)))
        cleanexit("Can't open texteditor.gadget");

    if ( ! (RequesterBase = OpenLibrary("requester.class",mingadgetversion)))
        cleanexit("Can't open requester.class");

   if ( ! (ScrollerBase = OpenLibrary("gadgets/scroller.gadget",mingadgetversion)))
       cleanexit("Can't open scroller.gadget");

    if(!initAppModel())  cleanexit("Can't create app");

    // = = = = = now that needed classes are loaded
    // = = = = = creates the instances...

    app->lockedscreen = LockPubScreen(NULL);
    if (!app->lockedscreen) cleanexit("Can't lock screen");

    app->drawInfo = GetScreenDrawInfo(app->lockedscreen);
    // let's size according to font height.
    app->styleSheet.fontHeight = 8+6; // default;
    if(app->drawInfo && app->drawInfo->dri_Font)
            app->styleSheet.fontHeight =app->drawInfo->dri_Font->tf_YSize + 4;

    /* Initialize stylesheet fonts using OpenDiskFont */
    {
        static struct TextAttr tinyFontAttr = {
            "topaz.font",   /* Font name */
            8,              /* YSize - small font */
            FS_NORMAL,      /* Style */
            /*FPF_ROMFONT*/FPF_DISKFONT
        };
        app->styleSheet.fontTiny = OpenDiskFont(&tinyFontAttr);
        printf(" * * * fontTiny:%08x\n",(int)app->styleSheet.fontTiny);
        /* fontNormal and fontBig can use screen font or be opened similarly */
//        app->tracksListView.styleSheet.fontNormal = app->drawInfo ? app->drawInfo->dri_Font : NULL;
//        app->tracksListView.styleSheet.fontBig = NULL; /* TODO: open larger font if needed */
//        app->tracksListView.styleSheet.fontHeight = app->fontHeight;
    }

    {
        extern unsigned char bpwizard_png[];
        extern unsigned int bpwizard_png_size;

        // BitMap class can load from file datatype, but not from memory. We just do this:
//        int isok = LoadDataTypeToBm(&bpwizard_png[0],bpwizard_png_size,
//                        &dtbmLogo,&dtbmLogo_mask, app->lockedscreen);
/*
        int isok = LoadDataTypeToBm("bpwizard.png",0,
                        &dtbmLogo,&dtbmLogo_mask, app->lockedscreen);
*/
  

//        Object* filler =  NewObject( BUTTON_GetClass(),NULL,
//                                    GA_Text, " ",BUTTON_BevelStyle,BVS_NONE,BUTTON_Transparent, TRUE,TAG_END);


        Object* label1 = (Object *)NewObject( LABEL_GetClass(), NULL,
                        LABEL_DrawInfo, app->drawInfo,
                        //IA_Font, &helvetica15bu,
                        //LABEL_SoftStyle, FSF_BOLD | FSF_ITALIC,
                        LABEL_Justification, LABEL_CENTRE,
                        LABEL_Text,(ULONG)"0.01",
                    TAG_END);

        // app->btAbout = NewObject( BUTTON_GetClass(),NULL,
        //                             GA_Text, "About...",
        //                             GA_ID,GAD_BUTTON_ABOUT,
        //                             GA_RelVerify, TRUE,
        //                  //           GA_Disabled,TRUE,
        //                 // BUTTON_BevelStyle,BVS_NONE,
        //                 // BUTTON_Transparent, TRUE,
        //                         TAG_END);

        app->horizontallayoutA =
             (Object *)NewObject( LAYOUT_GetClass(), NULL,
                    LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
                    LAYOUT_EvenSize, TRUE,
                    LAYOUT_HorizAlignment, LALIGN_CENTER,
                    LAYOUT_BevelStyle, /*BVS_GROUP*/BVS_NONE,
                    //CHILD_WeightedWidth,0,
                     //CHILD_MaxWidth,dtbmLogo.width+2,
                    LAYOUT_AddImage, label1,
                     //CHILD_WeightedWidth,1,
                    //LAYOUT_AddImage, filler,
                    // CHILD_WeightedWidth,1,
                    CHILD_MaxWidth,2560,
                   // LAYOUT_AddChild, app->btAbout,
                    // CHILD_WeightedWidth,0,
                   //  CHILD_MinWidth,32,
                   //  CHILD_MaxWidth,32,
                    TAG_DONE);
    }


    CreateTrackListView(&app->tracksListView,app->drawInfo, AppInstance,&app->styleSheet);

    {
        app->statusbarlabel = (Object *)NewObject( BUTTON_GetClass(),NULL,
                        GA_DrawInfo,(ULONG) app->drawInfo,
                        BUTTON_BevelStyle,BVS_NONE,
                        BUTTON_Transparent, TRUE,
						GA_ReadOnly, TRUE,
                        BUTTON_Justification, BCJ_CENTER,
                        GA_Text,(ULONG)"...",
                    TAG_END);


        app->bottombarlayout =
             (Object *)NewObject( LAYOUT_GetClass(), NULL,
                    LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
                    LAYOUT_EvenSize, TRUE,
                    LAYOUT_HorizAlignment, LALIGN_RIGHT,
                  //  CHILD_ScaleHeight,1, //%
                   // CHILD_MaxHeight,app->fontHeight,
                   // LAYOUT_SpaceInner, FALSE,
                    LAYOUT_AddChild, app->statusbarlabel,
                  //  LAYOUT_AddChild, app->labelValues,
                   // LAYOUT_AddChild, app->disablecheckbox,
                  //  GA_Height,app->fontHeight,
                    TAG_DONE);
        if(!app->bottombarlayout) cleanexit("Can't layout 2");
    }



    {
     //   struct DrawInfo *drinfo = GetScreenDrawInfo(screen);
        app->mainvlayout = (Object *)NewObject( LAYOUT_GetClass(), NULL,
            GA_DrawInfo, app->drawInfo,
            LAYOUT_DeferLayout, TRUE, // Layout refreshes done on task's context (by thewindow class)
            LAYOUT_SpaceOuter, TRUE,
            LAYOUT_BottomSpacing, 2,
            LAYOUT_TopSpacing,0,
            LAYOUT_LeftSpacing,0,
            LAYOUT_RightSpacing,0,
            LAYOUT_InnerSpacing,0,
         //   LAYOUT_HorizAlignment, LALIGN_RIGHT,
            LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
            LAYOUT_AddChild, app->horizontallayoutA,
                CHILD_WeightedHeight,0,
            LAYOUT_AddChild, app->tracksListView.mainVl,
                CHILD_WeightedHeight,4,
            LAYOUT_AddChild, app->bottombarlayout,
                CHILD_WeightedHeight,0,
            TAG_END);
        if (!app->mainvlayout) cleanexit("layout error 3");

        app->reportReq = NewObject(REQUESTER_GetClass(), NULL,
			// REQ_TitleText, "Project Generated",
			REQ_Image,REQIMAGE_INFO,
			REQ_BodyText,"....",
			REQ_GadgetText,(ULONG)"_Ok", //
            TAG_END);

    } //end if screen





    app->app_port = CreateMsgPort();

    /* Create the window object. */
    app->window_obj = (Object *)NewObject( WINDOW_GetClass(), NULL,
        WA_Left, 40,
        WA_Top, (ULONG)(app->lockedscreen->Font->ta_YSize) + 3 + 16,
        WA_Width,320,
        WA_Height,240,
        WA_CustomScreen, (ULONG) app->lockedscreen,
        WA_IDCMP, IDCMP_CLOSEWINDOW | IDCMP_RAWKEY ,
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
    app->win = boopsi_OpenWindow(app->window_obj);
    if(!app->win) cleanexit("can't open window");
    app->tracksListView.window = app->win;

    updateUIToStates();

    initProject();
    TrackListView_UpdateTrackList(&app->tracksListView);
//    // gui inited here.
//    {
//        char temp[64];
//        snprintf(temp,63,"Found %d templates", getNbTemplates());
//        guiNotifier(0,temp);
//    }


    {
        ULONG winsignal;
        BOOL ok = TRUE;

        /* Obtain the window wait signal mask.*/
        GetAttr(WINDOW_SigMask, app->window_obj, &winsignal);

        /* Input Event Loop */
        while (ok)
        {
            ULONG result,currentSignal;

            currentSignal = Wait(winsignal | (1L << app->app_port->mp_SigBit) | SIGBREAKF_CTRL_F);

           flushbdbprint();
            /* CA_HandleInput() returns the gadget ID of a clicked
             * gadget, or one of several pre-defined values.  For
             * this demo, we're only actually interested in a
             * close window and a couple of gadget clicks.
             */
            while ((result = DoMethod(app->window_obj, WM_HANDLEINPUT, /*code*/NULL)) != WMHI_LASTMSG)
            {
            flushbdbprint();
            // printf("result:%08x\n",(int)result);
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
//                        if(gid == GAD_BUTTON_ABOUT)
//                        {
//                            openAboutReq();
//                        }
                        break;
                    }
                    case WMHI_ICONIFY:
                        //if (RA_Iconify(window_obj)) win = NULL;
                        if(DoMethod(app->window_obj, WM_ICONIFY, NULL)) app->win = NULL;
                        break;

                    case WMHI_UNICONIFY:
                        app->win = boopsi_OpenWindow(app->window_obj);
                        app->tracksListView.window = app->win;
                        if (!app->win) cleanexit("can't open window");

                        break;

                    default:
                        break;
                }


            } // end while messages

            // delay some messages to avoid big graphic update recursion
            if(app->tracksListView.updateBits)
            {
                TrackListView_CheckUpdates(&app->tracksListView);
            }
        } // end while app loop
    } // loop paragraph end

           flushbdbprint();

    // all close done in exitclose().
    return 0;
}

void guiNotifier(int loglevel, const char *log)
{
    if(!app || !app->statusbarlabel) return;

// todo errors in red/ warning in orange
//re    int textpen = -1; // default text pen
    SetGadgetAttrs((struct Gadget *)app->statusbarlabel,app->win,NULL,
    //    BUTTON_TextPen,(ULONG)textpen,
        GA_Text,(ULONG)log,
        TAG_END);



}

void exitclose(void)
{
           flushbdbprint();
    printf("exitclose()\n");
    if(app)
    {
        /* just release data listener and object retained */
        CloseTrackListView(&app->tracksListView);

        /* Disposing of the window object will also close the
         * window if it is already opened and it will dispose of
         * all objects attached to it.
         */
        if(app->reportReq) DisposeObject( app->reportReq );
            printf("app->window_obj:%08x\n",(int)app->window_obj);

        // this should cascade all OM_DISPOSE:
        if(app->window_obj) DisposeObject(app->window_obj);
        else {
        // not sure about mid-failure boopsies
//            // but if not attached because mid-init fail, has to be manual.
//            if(app->mainvlayout)  DisposeObject(app->mainvlayout);
//            else {
//                if(app->horizontallayout) DisposeObject(app->horizontallayout);
//                else {
//                    if(app->testbt) DisposeObject(app->testbt);
//                    if(app->kbdview) DisposeObject(app->kbdview);
//                }
//                if(app->bottombarlayout) DisposeObject(app->bottombarlayout);
//                else {
//                    if(app->label1) DisposeObject(app->label1);
//                    if(app->labelValues) DisposeObject(app->labelValues);
//                    if(app->disablecheckbox) DisposeObject(app->disablecheckbox);
//                }
//            }
        }
        // debug mode, check private class gadgets instance areall closed.
        bdbprintf_report_leaks();

//        if(dtbmLogo.bm) {
//            closeDataTypeBm(&dtbmLogo);
//        }

        /* Close fonts opened with OpenDiskFont before closing library */
        if(app->styleSheet.fontTiny) {
            CloseFont(app->styleSheet.fontTiny);
            app->styleSheet.fontTiny = NULL;
        }
        /* Note: fontNormal points to drawInfo->dri_Font, don't close it separately */

        if(app->drawInfo) FreeScreenDrawInfo(app->lockedscreen, app->drawInfo);
        if(app->lockedscreen) UnlockPubScreen(0, app->lockedscreen);

    }

    closeAppModel(); // thi is meant to close app implicitely, If i'm correct...

    CloseTrackListView_StaticClasses();

    if(ScrollerBase) CloseLibrary(ScrollerBase);
    if(RequesterBase) CloseLibrary(RequesterBase);
    if(TextFieldBase) CloseLibrary(TextFieldBase);
    if(StringBase) CloseLibrary(StringBase);
    if(CheckBoxBase) CloseLibrary(CheckBoxBase);
    if(VirtualBase) CloseLibrary(VirtualBase);
    if(LabelBase) CloseLibrary(LabelBase);
    if(ButtonBase) CloseLibrary(ButtonBase);
    if(BitMapBase) CloseLibrary(BitMapBase);
    if(LayoutBase) CloseLibrary(LayoutBase);
    if(WindowBase) CloseLibrary(WindowBase);


    if(GfxBase) CloseLibrary((struct Library*)GfxBase);
    if(IntuitionBase) CloseLibrary((struct Library*)IntuitionBase);
    if(LayersBase) CloseLibrary(LayersBase);
    if (UtilityBase) CloseLibrary(UtilityBase);

    //if(DOSBase) CloseLibrary((struct Library*)DOSBase);
    if(IconBase) CloseLibrary(IconBase);
    if(AslBase) CloseLibrary(AslBase);
    if(DiskfontBase) CloseLibrary(DiskfontBase);

}
// synchronize greying buttons...
int CurrentTemplate = -1;
void updateUIToStates()
{
    if(!app) return;
// GA_Disabled
//    ULONG generateBtDisabled = (CurrentTemplate<0);

   // SetGadgetAttrs((struct Gadget *) app->btGenerate,app->win,NULL,
   //     GA_DISABLED,generateBtDisabled,
   //     TAG_END);
}

void openAboutReq()
{

    SetAttrs(app->reportReq,REQ_TitleText,(ULONG)"About...",TAG_END);
    // if ok, show a report requester
 static const char *p=
    "..."
    ;
    SetAttrs(app->reportReq,REQ_BodyText,(ULONG)p,TAG_END);

    OpenRequester(app->reportReq,app->win);

}


int initProject()
{
    if(!app) return;

    AukAProject* project;
    AukTrack* track1;
    AukTrack* track2;
    AukSoundFilePtr soundFile1 = NULL;
    AukSound* sound1;
    AukSound* sound2;
    AukFixed duration;

    /* Create a new project */
    AukAProject_New(&app->_project);
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
    AukTrack_SetName(track1, "Vocals");
    AukTrack_SetName(track2, "Music");


    /* Create a sound file reference */
    AukSoundFile_New(&soundFile1);
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
    printf("Project duration: %ld seconds\n", AukFixed_ToInt(duration));

    /* Save project to JSON file */
//    if (project->base.Save(project, "my_project.auk")) {
//        printf("Project saved successfully\n");
//    } else {
//        printf("Failed to save project\n");
//    }

    /* Display project info */
    printf("Project: %s\n", project->base.GetName(project));
    printf("Tracks: %lu\n", project->GetTrackCount(project));
    printf("Track 1: %s, Sounds: %lu\n",
           AukTrack_GetName(track1),
           track1->GetSoundCount(track1));
    printf("Track 2: %s, Sounds: %lu\n",
           AukTrack_GetName(track2),
           track2->GetSoundCount(track2));
    return 0;
}

void TrackListView_UpdateTrackList_Generic()
{
    if(!app) return;
    //TrackListView_UpdateTrackList(&app->tracksListView);
        app->tracksListView.updateBits |= TLVB_UPDATE_FULLREDRAW;
        if(myTask) Signal(myTask,SIGBREAKF_CTRL_F);
}
