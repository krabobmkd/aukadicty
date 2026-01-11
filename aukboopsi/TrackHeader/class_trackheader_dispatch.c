

#ifdef __SASC
    #include <clib/alib_protos.h>
#else
    // GCC, vbcc
    #include "minialib.h"
#endif
#include <proto/dos.h>
//#include <proto/utility.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/gadgetclass.h>
#include <intuition/icclass.h>
#include "class_trackheader.h"
#include "class_trackheader_private.h"

#include <proto/layout.h>
#include <gadgets/layout.h>

#include <proto/button.h>
#include <gadgets/button.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/utility.h>

#include "gadgetid.h"

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

/* Most of the calls to boopsi methods are not done from the App's context,
 * but from a specific intuition context, and because of that we can't use DOS calls
 * like dos/Printf() , and also stdlib printf().
 * So we may print debug informations with a special buffer,and function bdbprintf(),
 * hen flushbdbprint() in main process will print for real to standard output.
 * remove word USE_DEBUG_BDBPRINT to desactivate all bdbprintf()/flushbdbprint() calls.
 * Template projects that links boopsi classes statically use USE_DEBUG_BDBPRINT by default.
 * Template projects that uses boopsi classes with LoadLibrary() do not.
 */
#include "bdbprintf.h"

/** WATCH OUT ! boopsi docs says:
*  "the rkmmodelclass dispatcher must be able to run on Intuition's context,
*  which puts some limitations on what the dispatcher is permitted to do:
*  it can't use dos.library, it can't wait on application signals or message ports
* and it can't call any Intuition functions which might wait on Intuition."
*/
ULONG ASM SAVEDS TrackHeader_Dispatcher(
                    REG(a0,struct IClass *C),
                    REG(a2,struct Gadget *Gad),
                    REG(a1,union MsgUnion *M))
{
  TrackHeader *gdata;
  ULONG retval=0;


  switch(M->MethodID)
  {
    case OM_NEW:
      {
        struct TagItem *ptag;
        ULONG iTrack=0;
        ULONG target=0;
        char *trackname=NULL;
        char tname[32];
        Object *VolumeRule,*CloseButton,*NameButton,*VolumeSlider,*PanSlider,
                *LeftVertlayout,*CloseAndNameHl;
        //
        if((ptag = FindTagItem( TRACKHEADER_TrackIndex,M->opSet.ops_AttrList ))!=NULL)
        {
            iTrack = ptag->ti_Data;
        }
        if((ptag = FindTagItem( TRACKHEADER_Name,M->opSet.ops_AttrList ))!=NULL)
        {
            trackname = (char *)ptag->ti_Data;
        }
        if(!trackname)
        {
            snprintf(tname,31,"Track %d",iTrack);
            trackname = &tname[0];
        } else if(strlen(trackname)>9)
        {
            snprintf(tname,31,"%9s...",trackname);
            trackname = &tname[0];
        }
        if((ptag = FindTagItem( ICA_TARGET,M->opSet.ops_AttrList ))!=NULL)
        {
            target = ptag->ti_Data;
          //  bdbprintf("header target:%08x\n",target);
        }

        CloseButton = NewObject( /*BUTTON_GetClass()*/HEADERBUTTON_GetClass(),NULL,
                                    GA_Text, "X",
                                    GA_ID,GAD_TRACKHEADER_BASE|GAD_TRACKHEADER_CLOSE|(iTrack<<4),
                                    ICA_TARGET,target,
                                    GA_RelVerify, TRUE,
                         //           GA_Disabled,TRUE,
                        // BUTTON_BevelStyle,BVS_NONE,
                        // BUTTON_Transparent, TRUE,
                                TAG_END);
        NameButton = NewObject( HEADERBUTTON_GetClass(),NULL,
                                    GA_Text,trackname,
                                    GA_ID,GAD_TRACKHEADER_BASE|GAD_TRACKHEADER_NAME|(iTrack<<4),
                                    ICA_TARGET,target,
                                    GA_RelVerify, TRUE,
                         //           GA_Disabled,TRUE,
                        // BUTTON_BevelStyle,BVS_NONE,
                        // BUTTON_Transparent, TRUE,
                                TAG_END);


        CloseAndNameHl  = (Object *)NewObject( LAYOUT_GetClass(), NULL,
                    LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
             LAYOUT_BottomSpacing, 1,
            LAYOUT_TopSpacing,1,
            LAYOUT_LeftSpacing,1,
            LAYOUT_RightSpacing,1,
            LAYOUT_InnerSpacing,1,
                    LAYOUT_AddChild, CloseButton,
                    CHILD_MaxWidth,18,CHILD_MinWidth,18,
                   // CHILD_WeightedWidth,0,
                    LAYOUT_AddChild, NameButton,
                     CHILD_MaxWidth,96-22,CHILD_MinWidth,96-22,
                   // CHILD_WeightedWidth,1,
                 //  CHILD_MaxWidth,64,
                    TAG_DONE);

        VolumeSlider = NewObject( HEADERBUTTON_GetClass(),NULL,
                                    GA_Text, "VolSlider",
                                    ICA_TARGET,target,
                                    GA_ID,GAD_TRACKHEADER_BASE|GAD_TRACKHEADER_VOL|(iTrack<<4),
                                  //  GA_ID,GAD_BUTTON_ABOUT,
                                    GA_RelVerify, TRUE,
                         //           GA_Disabled,TRUE,
                        // BUTTON_BevelStyle,BVS_NONE,
                        // BUTTON_Transparent, TRUE,
                                TAG_END);

        // in this paragraph we create the layout hierarchy
        LeftVertlayout  = (Object *)NewObject( LAYOUT_GetClass(), NULL,
                    LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
            LAYOUT_BevelStyle,BVS_NONE,
            LAYOUT_BottomSpacing, 0,
            LAYOUT_TopSpacing,0,
            LAYOUT_LeftSpacing,0,
            LAYOUT_RightSpacing,0,
            LAYOUT_InnerSpacing,1,

//                    LAYOUT_BevelStyle, /*BVS_GROUP*/BVS_NONE,
                    LAYOUT_AddChild, CloseAndNameHl,
                     CHILD_WeightedHeight,0,

                    LAYOUT_AddChild, VolumeSlider,
                     CHILD_WeightedHeight,1,

                    TAG_DONE);

        VolumeRule = NewObject( HEADERBUTTON_GetClass(),NULL,
                                    GA_Text, "VR",
                                  //  GA_ID,GAD_BUTTON_ABOUT,
                                    GA_RelVerify, TRUE,
                         //           GA_Disabled,TRUE,
                        // BUTTON_BevelStyle,BVS_NONE,
                        // BUTTON_Transparent, TRUE,
                                TAG_END);

/*
    Object *CloseButton;
    Object *NameLabel;

    Object  *VolumeSlider;
    Object  *PanSlider;

    // ------------- H
    Object *VolumeRule;

*/
        {

        // we are a layout that forces its parameter...
        ULONG tags[]={
            LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
           // / LAYOUT_DeferLayout, TRUE, // because not added to main layout directly ?
            LAYOUT_BottomSpacing, 0,
            LAYOUT_TopSpacing,0,
            LAYOUT_LeftSpacing,0,
            LAYOUT_RightSpacing,0,
            LAYOUT_InnerSpacing,0,

                    LAYOUT_BevelStyle, /*BVS_GROUP*/BVS_NONE,
                    LAYOUT_AddChild, LeftVertlayout,
                    CHILD_MaxWidth,128-32,CHILD_MinWidth,128-32,
                    // CHILD_WeightedWidth,7,
                    LAYOUT_AddChild, VolumeRule,
                   CHILD_MaxWidth,32,CHILD_MinWidth,32,
                    // CHILD_WeightedWidth,1,
                    TAG_DONE
        };
        struct opSet opset;
        opset.MethodID = OM_NEW;
        opset.ops_GInfo = M->opSet.ops_GInfo;
        opset.ops_AttrList = &tags[0];

          if(Gad=(struct Gadget *)DoSuperMethodA(C,(Object *)Gad,&opset))
          {
            gdata=INST_DATA(C, Gad);
            bdbprintf_new("TrackHeader", Gad);
            Gad->GadgetID = GAD_TRACKHEADER_BASE+(iTrack<<4);

            gdata->subs[THS_CloseButton] = CloseButton;
            gdata->subs[THS_NameButton] = NameButton;
            gdata->subs[THS_VolumeSlider] = VolumeSlider;
            gdata->subs[THS_PanSlider] = NULL ; //TODO
            gdata->subs[THS_VolumeRule] = VolumeRule;
            gdata->_trackIndex = iTrack;

            /* means new object OK so far: */
            retval=(ULONG)Gad;
          }
        }
      }
      break;
    case OM_DISPOSE:
        bdbprintf_dispose("TrackHeader", Gad);
      retval=DoSuperMethodA(C,(Object *)Gad,(Msg)M);
      break;

    case OM_UPDATE:
    case OM_SET:
      retval = TrackHeader_SetAttrs(C,Gad,(struct opSet *)M);
      if(!retval) retval = DoSuperMethodA(C,(Object *)Gad,(Msg)M);

     break;

    case OM_GET:
      retval= TrackHeader_GetAttr(C,Gad,(struct opGet *)M); // supercall done inside
     break;


    default:
      // for anything, use default layout behaviour.
      retval=DoSuperMethodA(C,(Object *)Gad,(Msg)M);
      break;
  }
  return(retval);
}
/* redraw later */
extern void TrackListView_UpdateTrackList_Generic();
/* the App object that can receive events */
extern Object *AppInstance;
/*
    Note it should be DoSuperMethodA(button) with OM_NOTIFY,
    that shoudl then turn to the AppInstance ICA_TARGET as OM_UPDATE,
    but it doesnt work (because of overridings?), so
    it becomes a direct call to OM_UPDATE AppInstance.
*/
void HeaderButton_Notify(Class *C, struct Gadget *Gad, struct GadgetInfo *ginfo)
{
   struct opUpdate notifymsg;
   ULONG tags[]={
    GA_ID,0,
   // ICA_TARGET,0,
    TAG_DONE
   };
    bdbprintf("HeaderButton_Notify:%08x\n", Gad->GadgetID);
    tags[1] = Gad->GadgetID;
    notifymsg.MethodID = OM_UPDATE;
    notifymsg.opu_AttrList = (struct TagItem *)&tags[0];
    notifymsg.opu_GInfo = ginfo; // "always there for gadget, in all messages"
    notifymsg.opu_Flags = 0;
    return DoMethodA((APTR)AppInstance,(Msg)&notifymsg );

  //  tags[3] = (ULONG)AppInstance;
   // notifymsg.MethodID = OM_NOTIFY;
   // notifymsg.opu_AttrList = (struct TagItem *)&tags[0];
   // notifymsg.opu_GInfo = ginfo; // "always there for gadget, in all messages"
   // notifymsg.opu_Flags = 0;
   // return DoSuperMethodA(C,(APTR)Gad,(Msg)&notifymsg );
}

/*
 Extend Button class to prevent it from drawing from inputs events
  like GM_GOACTIVE/GM_HANDLEINPUT/GM_GOINACTIVE.
  In our TrackListArea layout logic, rendering should only be done during GM_RENDER,
  and GM_RENDER sent from TrackListArea, so the correct Clipping region
  is always applied.
  Rewriting GM_GOACTIVE/GM_HANDLEINPUT/GM_GOINACTIVE has been took from dev example.

*/
ULONG ASM SAVEDS HeaderButton_Dispatcher(
                    REG(a0,struct IClass *C),
                    REG(a2,struct Gadget *Gad),
                    REG(a1,union MsgUnion *M))
{
  TrackHeaderButton *gdata;
  ULONG retval=0;
  gdata=INST_DATA(C, Gad);

  switch(M->MethodID)
  {
      case OM_NEW:
      if(Gad=(struct Gadget *)DoSuperMethodA(C,(Object *)Gad,(Msg)M))
      {
        gdata=INST_DATA(C, Gad);
        bdbprintf_new("HeaderButton", Gad);
        /* means new object OK so far: */
        retval=(ULONG)Gad;
      }
      break;
    case OM_DISPOSE:
        bdbprintf_dispose("HeaderButton", Gad);
      retval=DoSuperMethodA(C,(Object *)Gad,(Msg)M);
      break;
    case GM_GOACTIVE:

        /* Only become active if the GM_GOACTIVE   */
        /* was triggered by direct user input.     */
        if (((struct gpInput *)M)->gpi_IEvent)
        {
            /* This gadget is now active, change    */
            /* visual state to selected and render. */
            Gad->Flags |= GFLG_SELECTED;
            /* modified to delay drawing, button turns blue ! */
           TrackListView_UpdateTrackList_Generic();
            retval = GMR_MEACTIVE;
        }
        else            /* The GM_GOACTIVE was not         */
            /* triggered by direct user input. */
            retval = GMR_NOREUSE;
        break;
    case GM_HANDLEINPUT:
    {
        struct gpInput *gpi = (struct gpInput *)M;
        struct InputEvent *ie = gpi->gpi_IEvent;

        //inst = (ButINST*)INST_DATA(cl, o);

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
                if ( ((gpi->gpi_Mouse).X < Gad->LeftEdge) ||
                    ((gpi->gpi_Mouse).X > Gad->LeftEdge + Gad->Width) ||
                    ((gpi->gpi_Mouse).Y < Gad->TopEdge) ||
                    ((gpi->gpi_Mouse).Y > Gad->TopEdge + Gad->Height) )
                    retval = GMR_NOREUSE | GMR_VERIFY;
                else
                    retval = GMR_NOREUSE;

                /* Since the gadget is going inactive, send a final   */
                /* notification to the ICA_TARGET.                    */
               HeaderButton_Notify(C ,Gad,gpi->gpi_GInfo);
                break;
            // case MENUDOWN: /* The user hit the menu button. Go inactive and let      */
            //     /* Intuition reuse the menu button event so Intuition can */
            //     /* pop up the menu bar.                                   */
            //     retval = GMR_REUSE;

            //     /* Since the gadget is going inactive, send a final   */
            //     /* notification to the ICA_TARGET.                    */
            //     NotifyPulse(cl , o, 0L, inst->midX, (struct gpInput *)msg);
            //     break;
            default:
                retval = GMR_MEACTIVE;
            }

        }
        else if (ie->ie_Class == IECLASS_TIMER)
        {
            /* If the gadget gets a timer event, it sends an interim OM_NOTIFY */
            //todo ... NotifyPulse(cl, o, OPUF_INTERIM, inst->midX, gpi); /*     to its superclass. */
        }
    }
    break;
    case GM_GOINACTIVE:           /* Intuition said to go inactive.  Clear the GFLG_SELECTED */
        Gad->Flags &= ~GFLG_SELECTED;
        /* modified to delay drawing, button turns back unselcected color ! */
        TrackListView_UpdateTrackList_Generic();
        retval = 1;
        break;
    case GM_RENDER:
        retval=DoSuperMethodA(C,(Object *)Gad,(Msg)M);
       // bdbprintf("bt GM_RENDER:%d\n",retval);
    break;

    default:
    {
      // for anything, use default layout behaviour.
      retval=DoSuperMethodA(C,(Object *)Gad,(Msg)M);
      }
      break;
  }
  return(retval);


}
