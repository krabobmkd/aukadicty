
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

#include "class_track.h"

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

/* Gadget action IDs, just to demonstrate some interactions
 */
#define GAD_BUTTON_GENERATE 1
#define GAD_BUTTON_ABOUT 2
#define GAD_CB_SASC 3
#define GAD_CB_MAKEFILE 4
#define GAD_CB_CMAKELIST 5

#define GAD_START_SELECT_TEMPLATE 16



// all app related variables are here:
struct App
{
    Object *window_obj; // window as boopsi object
    struct Window *win; // current re-opened windows, as a classic intuition Window.

    struct MsgPort *app_port;

    struct Screen *lockedscreen;
    struct DrawInfo *drawInfo; // informations on how to draw on the screen, passed to gagdets.

    ULONG   fontHeight; // some stat to size according to current font.

    Object *mainvlayout;
        Object *horizontallayoutA;
         //   Object *titlelabel;
            Object* btAbout;
        Object *horizontallayoutB;
            Object *HeaderZone;
            Object *TrackVertLZone;
            Object *TrackVirtZone;

            // status bar
        Object *horizontallayoutC;
            Object *bottombarlayout;
            Object* statusbarlabel;

     Object *reportReq;

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
            // here receive events from gadgets as target.
            ULONG sender_ID=0;
            if((ptag = FindTagItem( GA_ID,M->opUpdate.opu_AttrList ))!=NULL) sender_ID = ptag->ti_Data;
            // our gadget is notifying new clicked coordinates!
            // note any button action is either managed here or in more generic main loop

            // if( sender_ID >= GAD_START_SELECT_TEMPLATE)
            // {
            // } else
            // if( sender_ID == GAD_BUTTON_GENERATE )
            // {
            //     retval = 1;
            // } else
            {
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


typedef struct ButINST
{
    LONG midX, midY; /* Coordinates of middle of gadget */
} ButINST;

ULONG RenderRKMBut(Class *cl, struct Gadget *g, struct gpRender *msg)
{
    struct ButINST *inst = (ButINST *)INST_DATA(cl, (Object *)g);
    struct RastPort *rp;
    ULONG retval = TRUE;
    UWORD *pens = msg->gpr_GInfo->gi_DrInfo->dri_Pens;

    if (msg->MethodID == GM_RENDER)   /* If msg is truly a GM_RENDER message (not a gpInput that */
        /* looks like a gpRender), use the rastport within it...   */
        rp = msg->gpr_RPort;
    else                              /* ...Otherwise, get a rastport using ObtainGIRPort().     */
        rp = ObtainGIRPort(msg->gpr_GInfo);

    if (rp)
    {
        UWORD back, shine, shadow, w, h, x, y;

        if (g->Flags & GFLG_SELECTED) /* If the gadget is selected, reverse the meanings of the  */
        {                             /* pens.                                                   */
            back   = pens[FILLPEN];
            shine  = pens[SHADOWPEN];
            shadow = pens[SHINEPEN];
        }
        else
        {
            back   = pens[BACKGROUNDPEN];
            shine  = pens[SHINEPEN];
            shadow = pens[SHADOWPEN];
        }
        SetDrMd(rp, JAM1);

        SetAPen(rp, back);          /* Erase the old gadget.       */
        RectFill(rp, g->LeftEdge,
            g->TopEdge,
            g->LeftEdge + g->Width,
            g->TopEdge + g->Height);

        SetAPen(rp, shadow);     /* Draw shadow edge.            */
        Move(rp, g->LeftEdge + 1, g->TopEdge + g->Height);
        Draw(rp, g->LeftEdge + g->Width, g->TopEdge + g->Height);
        Draw(rp, g->LeftEdge + g->Width, g->TopEdge + 1);

        w = g->Width / 4;       /* Draw Arrows - Sorry, no frills imagery */
        h = g->Height / 2;
        x = g->LeftEdge + (w/2);
        y = g->TopEdge + (h/2);

        Move(rp, x, inst->midY);
        Draw(rp, x + w, y);
        Draw(rp, x + w, y + (g->Height) - h);
        Draw(rp, x, inst->midY);

        x = g->LeftEdge + (w/2) + g->Width / 2;

        Move(rp, x + w, inst->midY);
        Draw(rp, x, y);
        Draw(rp, x, y  + (g->Height) - h);
        Draw(rp, x + w, inst->midY);

        SetAPen(rp, shine);    /* Draw shine edge.           */
        Move(rp, g->LeftEdge, g->TopEdge + g->Height - 1);
        Draw(rp, g->LeftEdge, g->TopEdge);
        Draw(rp, g->LeftEdge + g->Width - 1, g->TopEdge);

        if (msg->MethodID != GM_RENDER) /* If we allocated a rastport, give it back.             */
            ReleaseGIRPort(rp);
    }
    else retval = FALSE;
    return(retval);
}

ULONG RKMBut_Domain(Class *C, struct Gadget *Gad, struct gpDomain *D)
{
//  Track *gdata=0;

 // if(Gad) gdata=INST_DATA(C, Gad);
// Printf("Track_Domain data:%lx\n",(int)gdata);

  D->gpd_Domain.Left=0;
  D->gpd_Domain.Top=0;

  switch(D->gpd_Which)
  {
    case GDOMAIN_NOMINAL:
     // if(gdata)
     // {
     //   D->gpd_Domain.Width =gdata->_minimalWidth;
     //   D->gpd_Domain.Height=gdata->_minimalHeight;
     // }
     // else
      {
        D->gpd_Domain.Width=100;
        D->gpd_Domain.Height=50;
      }
      break;

    case GDOMAIN_MAXIMUM:
      D->gpd_Domain.Width=4000;
      D->gpd_Domain.Height=4000;
      break;

    case GDOMAIN_MINIMUM:
    default:
//     if(gdata)
//     {
//       D->gpd_Domain.Width =gdata->_minimalWidth; // sqrt(gdata->Pens) * 8 + 8;
//       D->gpd_Domain.Height=gdata->_minimalHeight; // sqrt(gdata->Pens) * 8 + 8;
//     }
//     else
      {
        D->gpd_Domain.Width=  50;
        D->gpd_Domain.Height= 50;
      }
      break;

  }
  return(1);
}
ULONG ASM SAVEDS dispatchRKMButGad(
                    REG(a0,struct IClass *cl),
                    REG(a2,struct Gadget *o),
                    REG(a1,union MsgUnion *msg))
//ULONG dispatchRKMButGad(Class *cl, Object *o, Msg msg)
{
    struct ButINST *inst;
    ULONG retval = FALSE;
    Object *object;

    switch (msg->MethodID)
    {
    case OM_NEW:       /* First, pass up to superclass */
        if (object = (Object *)DoSuperMethod(cl, o, msg))
        {
            struct Gadget *g = (struct Gadget *)object;

            /* Initial local instance data */
            inst = (ButINST *)INST_DATA(cl, object);
            inst->midX   = g->LeftEdge + ( (g->Width) / 2);
            inst->midY   = g->TopEdge + ( (g->Height) / 2);

            retval = (ULONG)object;
        }
        break;
    case GM_HITTEST:
        /* Since this is a rectangular gadget this  */
        /* method always returns GMR_GADGETHIT.     */
        retval = GMR_GADGETHIT;
        break;
    case GM_GOACTIVE:
        inst = (ButINST*)INST_DATA(cl, o);

        /* Only become active if the GM_GOACTIVE   */
        /* was triggered by direct user input.     */
        if (((struct gpInput *)msg)->gpi_IEvent)
        {
            /* This gadget is now active, change    */
            /* visual state to selected and render. */
            ((struct Gadget *)o)->Flags |= GFLG_SELECTED;
            RenderRKMBut(cl, (struct Gadget *)o, (struct gpRender *)msg);
            retval = GMR_MEACTIVE;
        }
        else            /* The GM_GOACTIVE was not         */
            /* triggered by direct user input. */
            retval = GMR_NOREUSE;
        break;
    case GM_RENDER:
        retval = RenderRKMBut(cl, (struct Gadget *)o, (struct gpRender *)msg);
        break;
    case GM_HANDLEINPUT:   /* While it is active, this gadget sends its superclass an        */
        /* OM_NOTIFY pulse for every IECLASS_TIMER event that goes by     */
        /* (about one every 10th of a second).  Any object that is        */
        /* connected to this gadget will get A LOT of OM_UPDATE messages. */
    {
        struct Gadget *g = (struct Gadget *)o;
        struct gpInput *gpi = (struct gpInput *)msg;
        struct InputEvent *ie = gpi->gpi_IEvent;

        inst = (ButINST*)INST_DATA(cl, o);

        retval = GMR_MEACTIVE;

        if (ie->ie_Class == IECLASS_RAWMOUSE)
        {
            switch (ie->ie_Code)
            {
            case SELECTUP: /* The user let go of the gadget so return GMR_NOREUSE    */
                /* to deactivate and to tell Intuition not to reuse       */
                /* this Input Event as we have already processed it.      */

                /*If the user let go of the gadget while the mouse was    */
                /*over it, mask GMR_VERIFY into the return value so       */
                /*Intuition will send a Release Verify (GADGETUP).        */
                if ( ((gpi->gpi_Mouse).X < g->LeftEdge) ||
                    ((gpi->gpi_Mouse).X > g->LeftEdge + g->Width) ||
                    ((gpi->gpi_Mouse).Y < g->TopEdge) ||
                    ((gpi->gpi_Mouse).Y > g->TopEdge + g->Height) )
                    retval = GMR_NOREUSE | GMR_VERIFY;
                else
                    retval = GMR_NOREUSE;

                /* Since the gadget is going inactive, send a final   */
                /* notification to the ICA_TARGET.                    */
               // NotifyPulse(cl , o, 0L, inst->midX, (struct gpInput *)msg);
               bdbprintf("RKMBut up !\n");


                break;
            case MENUDOWN: /* The user hit the menu button. Go inactive and let      */
                /* Intuition reuse the menu button event so Intuition can */
                /* pop up the menu bar.                                   */
                retval = GMR_REUSE;

                /* Since the gadget is going inactive, send a final   */
                /* notification to the ICA_TARGET.                    */
               // NotifyPulse(cl , o, 0L, inst->midX, (struct gpInput *)msg);
                break;
            default:
                retval = GMR_MEACTIVE;
            }

        }
        else if (ie->ie_Class == IECLASS_TIMER)
        {
            /* If the gadget gets a timer event, it sends an interim OM_NOTIFY */
            //NotifyPulse(cl, o, OPUF_INTERIM, inst->midX, gpi); /*     to its superclass. */
        }
    }
        break;

    case GM_GOINACTIVE:           /* Intuition said to go inactive.  Clear the GFLG_SELECTED */
        /* bit and render using unselected imagery.                */
        ((struct Gadget *)o)->Flags &= ~GFLG_SELECTED;
        RenderRKMBut(cl, (struct Gadget *)o, (struct gpRender *)msg);
        break;
    case OM_SET:/* Although this class doesn't have settable attributes, this gadget class   */
        /* does have scaleable imagery, so it needs to find out when its size and/or */
        /* position has changed so it can erase itself, THEN scale, and rerender.    */
//        if ( FindTagItem(GA_Width,  ((struct opSet *)msg)->ops_AttrList) ||
//            FindTagItem(GA_Height, ((struct opSet *)msg)->ops_AttrList) ||
//            FindTagItem(GA_Top,    ((struct opSet *)msg)->ops_AttrList) ||
//            FindTagItem(GA_Left,   ((struct opSet *)msg)->ops_AttrList) )
//        {
//            struct RastPort *rp;
//            struct Gadget *g = (struct Gadget *)o;

//            WORD x,y,w,h;

//            x = g->LeftEdge;
//            y = g->TopEdge;
//            w = g->Width;
//            h = g->Height;

//            inst = (ButINST *)INST_DATA(cl, o);

//            retval = DoSuperMethod(cl, o, msg);

//            /* Get pointer to RastPort for gadget. */
//            if (rp = ObtainGIRPort( ((struct opSet *)msg)->ops_GInfo) )
//            {
//                UWORD *pens = ((struct opSet *)msg)->ops_GInfo->gi_DrInfo->dri_Pens;

//                SetAPen(rp, pens[BACKGROUNDPEN]);
//                SetDrMd(rp, JAM1);                            /* Erase the old gadget.       */
//                RectFill(rp, x, y, x+w, y+h);

//                inst->midX = g->LeftEdge + ( (g->Width) / 2); /* Recalculate where the       */
//                inst->midY = g->TopEdge + ( (g->Height) / 2); /* center of the gadget is.    */

//                /* Rerender the gadget.        */
//                IDoMethod(o, GM_RENDER, ((struct opSet *)msg)->ops_GInfo, rp, GREDRAW_REDRAW);
//                ReleaseGIRPort(rp);
//            }
//        }
//        else
            retval = DoSuperMethod(cl, o, msg);
        break;
       case GM_DOMAIN:
           retval = RKMBut_Domain(cl, o, msg);
       break;
    default:          /* rkmmodelclass does not recognize the methodID, let the superclass's */
        /* dispatcher take a look at it.                                       */
        retval = DoSuperMethod(cl, o, msg);
        break;
    }
    return(retval);
}

Class *initRKMButGadClass(void)
{
    Class *cl = NULL;
   // extern ULONG HookEntry();     /* defined in amiga.lib */

    if ( cl =  MakeClass( NULL,
        "gadgetclass", NULL,
        sizeof ( struct ButINST ),
        0 ))
    {
        /* initialize the cl_Dispatcher Hook    */
        cl->cl_Dispatcher.h_Entry = (HOOKFUNC)dispatchRKMButGad;
    }
    return ( cl );
}
Class *RKMButClass=NULL;
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


#ifdef TRACK_STATICLINK
    if(TrackStaticInit())  cleanexit("Can't init private Track gadget");
#endif

    if(!initAppModel())  cleanexit("Can't create app");

    RKMButClass = initRKMButGadClass();
    if(!RKMButClass)  cleanexit("Can't create RKMButClass");

    // = = = = = now that needed classes are loaded
    // = = = = = creates the instances...

    app->lockedscreen = LockPubScreen(NULL);
    if (!app->lockedscreen) cleanexit("Can't lock screen");

    app->drawInfo = GetScreenDrawInfo(app->lockedscreen);
    // let's size according to font height.
    app->fontHeight = 8+4; // default;
    if(app->drawInfo && app->drawInfo->dri_Font) app->fontHeight =app->drawInfo->dri_Font->tf_YSize + 4;

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

//    {
//        app->layoutBList = populateTemplateList();
//    }
    {
        app->HeaderZone = NewObject( BUTTON_GetClass(),NULL,
                                    GA_Text, "test bt1",
                                    GA_RelVerify, TRUE,
                                    CHILD_MaxWidth,120,
                                   CHILD_MaxHeight,6000,
                                TAG_END);


        Object *gadtrack1 = NewObject(RKMButClass,NULL,
              //   GA_RelVerify, TRUE,
                TAG_END);
// TRACK_GetClass()
// RKMButClass

//        Object *trackHlayout = (Object *)NewObject( LAYOUT_GetClass(), NULL,
//            //GA_DrawInfo, app->drawInfo,
//            //LAYOUT_DeferLayout, TRUE, // Layout refreshes done on task's context (by thewindow class)
//            // LAYOUT_SpaceOuter, FALSE,
//            // LAYOUT_SpaceInner, FALSE,
//            // LAYOUT_BottomSpacing, 0,
//            // LAYOUT_TopSpacing,0,
//            // LAYOUT_LeftSpacing,0,
//            // LAYOUT_RightSpacing,0,
//            // LAYOUT_InnerSpacing,0,
//             LAYOUT_Orientation, LAYOUT_HORIZONTAL,
//            // LAYOUT_BevelStyle, BVS_NONE,
//            LAYOUT_AddChild, gadtrack1,
//                CHILD_MinWidth,800,
//                CHILD_MinHeight,120,
//                CHILD_WeightedHeight,1,
//            TAG_END);


           /* NewObject( BUTTON_GetClass(),NULL,
                                    GA_Text, "test track1",
                                    GA_RelVerify, TRUE,
                                 // CHILD_MinWidth,6000,
                                 //   CHILD_MinHeight,6000,
                                 //    CHILD_MaxWidth,6000,
                                 //   CHILD_MaxHeight,6000,
                                TAG_END);*/
        Object *gadtrack2 = NewObject( BUTTON_GetClass(),NULL,
                                    GA_Text, "test track2",
                                    GA_RelVerify, TRUE,
                                 // CHILD_MinWidth,6000,
                                 //   CHILD_MinHeight,6000,
                                 //    CHILD_MaxWidth,6000,
                                 //   CHILD_MaxHeight,6000,
                                TAG_END);
        Object *gadtrack3 = NewObject( BUTTON_GetClass(),NULL,
                                    GA_Text, "test track3",
                                    GA_RelVerify, TRUE,
                                 // CHILD_MinWidth,6000,
                                 //   CHILD_MinHeight,6000,
                                 //    CHILD_MaxWidth,6000,
                                 //   CHILD_MaxHeight,6000,
                                TAG_END);
        app->TrackVertLZone = (Object *)NewObject( LAYOUT_GetClass(), NULL,
            //GA_DrawInfo, app->drawInfo,
            //LAYOUT_DeferLayout, TRUE, // Layout refreshes done on task's context (by thewindow class)
            // LAYOUT_SpaceOuter, FALSE,
            // LAYOUT_SpaceInner, FALSE,
            // LAYOUT_BottomSpacing, 0,
            // LAYOUT_TopSpacing,0,
            // LAYOUT_LeftSpacing,0,
            // LAYOUT_RightSpacing,0,
            // LAYOUT_InnerSpacing,0,
             LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
            // LAYOUT_BevelStyle, BVS_NONE,
            LAYOUT_AddChild, gadtrack2,
                CHILD_MinWidth,800,
                CHILD_MinHeight,120,
                CHILD_WeightedHeight,1,
            LAYOUT_AddChild, gadtrack1,
                CHILD_MinWidth,800,
                CHILD_MinHeight,120,
                CHILD_WeightedHeight,1,

            LAYOUT_AddChild, gadtrack3,
                CHILD_MinHeight,120,
                CHILD_WeightedHeight,1,
            TAG_END);


        app->TrackVirtZone = NewObject( VIRTUAL_GetClass(),NULL,
                                   //  GA_Text, "test bt2",
                                   //  GA_RelVerify, TRUE,
                                   //  CHILD_MaxWidth,6000,
                                   // CHILD_MaxHeight,6000,
                                   VIRTUALA_Contents,app->TrackVertLZone,
                                   // VIRTUALA_TotalX,4000,
                                   // VIRTUALA_TotalY,800,
                                TAG_END);

        app->horizontallayoutB =
             (Object *)NewObject( LAYOUT_GetClass(), NULL,
                    LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
                    LAYOUT_EvenSize, TRUE,
                    LAYOUT_HorizAlignment, LALIGN_RIGHT,
                    LAYOUT_InnerSpacing,0,

                   LAYOUT_AddChild,  app->HeaderZone,
                CHILD_WeightedWidth,0,
                    LAYOUT_AddChild, app->TrackVirtZone,
                CHILD_MinHeight,60,
                CHILD_WeightedWidth,1,
                  //  GA_Height,app->fontHeight,
                    TAG_DONE);
    }

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
            LAYOUT_AddChild, app->horizontallayoutB,
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
        WA_Left, 0,
        WA_Top, (ULONG)(app->lockedscreen->Font->ta_YSize) + 3,
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


    updateUIToStates();
//    // gui inited here.
//    {
//        char temp[64];
//        snprintf(temp,63,"Found %d templates", getNbTemplates());
//        guiNotifier(0,temp);
//    }


    {
        ULONG signal;
        BOOL ok = TRUE;

        /* Obtain the window wait signal mask.*/
        GetAttr(WINDOW_SigMask, app->window_obj, &signal);

        /* Input Event Loop */
        while (ok)
        {
            ULONG result;

            Wait(signal | (1L << app->app_port->mp_SigBit) | SIGBREAKF_CTRL_F);
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
                        ULONG gid = result &0xffff;
                       // printf("up gid:%d\n",gid);
                        if(gid>=GAD_START_SELECT_TEMPLATE)
                        {   // toggle button: which state ?

//                            gid -= GAD_START_SELECT_TEMPLATE;
//                            if(app->TemplateButtonsList[gid])
//                            {
//                                 int selected = 0;
//                                GetAttr(GA_Selected, app->TemplateButtonsList[gid], &selected);
//                                if(selected)  selectTemplate(gid);
//                            }


                        } else if(gid == GAD_BUTTON_GENERATE)
                        {
                            //generate();
                        } else if(gid == GAD_BUTTON_ABOUT)
                        {
                            openAboutReq();
                        }
                        break;
                    }
                    case WMHI_ICONIFY:
                        //if (RA_Iconify(window_obj)) win = NULL;
                        if(DoMethod(app->window_obj, WM_ICONIFY, NULL)) app->win = NULL;
                        break;

                    case WMHI_UNICONIFY:
                        app->win = boopsi_OpenWindow(app->window_obj);
                        if (!app->win) cleanexit("can't open window");

                        break;

                    default:
                        break;
                }


            } // end while messages
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


    if(app)
    {

        /* Disposing of the window object will also close the
         * window if it is already opened and it will dispose of
         * all objects attached to it.
         */
        if(app->reportReq) DisposeObject( app->reportReq );
        if(app->window_obj) DisposeObject(app->window_obj);
        else {
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
//        if(dtbmLogo.bm) {
//            closeDataTypeBm(&dtbmLogo);
//        }

        if(app->drawInfo) FreeScreenDrawInfo(app->lockedscreen, app->drawInfo);
        if(app->lockedscreen) UnlockPubScreen(0, app->lockedscreen);

    }

    closeAppModel(); // thi is meant to close app implicitely, If i'm correct...


#ifdef TRACK_STATICLINK
    TrackStaticClose();
#endif

    if(RKMButClass)
        FreeClass(RKMButClass);

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
    "***Be warned***:\nThis wizard is not an official AmigaOS NDK project\nand will most likely stick to Beta stage forever.\n\n"
    "What it is, is: an OpenSource effort of individual developpers, open to participation\n"
    " at: https://github.com/krabobmkd/boopsiwizard\n"
    " The fact is, a lot of aspect of Amiga OS development are difficult to set up,\n"
    "  and setting a simple project for a library, class, gadget, datatype, commodity\n"
    "  project, for a given C compiler, is cryptic, and takes days if not more.\n"
    "  So you *may* gain some times with this.\n\n"
    "The templates code proposed here will try to be the more compliant possible with\n"
    " official Amiga guidelines, but may not be 100% compliant. You are loudly welcome\n"
    " to make any suggestion on the code at:\n"
    " https://github.com/krabobmkd/boopsiwizard/issues\n"
    " Your resources for coding Amiga OS3 should be:\n"
    " The Amiga Developer CD v2.1, forum https://developer.amigaos3.net/forum\n"
    " https://developer.amigaos3.net/article/13-recommended-reading-amiga-developer\n\n"
    "How does it work and How can I do a template ?\n"
    " Templates are just a json file with a corresponding zip file in templates dir.\n"
    " Each json describes what should be renamed. At generation, if your project name\n"
    " is \"MyProject\",In target files, BaseName will be MyProject,BASENAME MYPROJECT\n"
    " and basename myproject. File names and text contents are replaced.\n"
    " Adress file names case-wise, we allow linux cross-compilation.\n\n"
    "License of wizard itself is LGPL, which means you can fork it or embedd it in\n"
    " commercial projects. It uses cJson and zlib.\n"
    " Some templates have code parts from official Amiga examples, some not.\n"
    " If your compiler is GCC, from now on you should also install phxass, needed to\n"
    " assemble the C startups."
    "\n\n - krb, Nov.2025."
    ;
    SetAttrs(app->reportReq,REQ_BodyText,(ULONG)p,TAG_END);

    OpenRequester(app->reportReq,app->win);

}


