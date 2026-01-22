


#include <clib/alib_protos.h>
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

#include <proto/slider.h>
#include <gadgets/slider.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/layers.h>
#include "gadgetid.h"

#include <stdio.h>

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

/* Forward declaration for layout function */
ULONG TrackHeader_Layout(Class *C, struct Gadget *Gad, struct gpLayout *layout);
//ULONG TrackHeader_Render(Class *C, struct Gadget *Gad, struct gpRender *Render);

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
        struct DrawInfo *drawInfo;
        AukStyle *style=NULL;
        char tname[32];
        int i;

        /* Parse OM_NEW tags */
        if((ptag = FindTagItem( TRACKHEADER_TrackIndex,M->opSet.ops_AttrList ))!=NULL)
        {
            iTrack = ptag->ti_Data;
        }
        if((ptag = FindTagItem( TRACKHEADER_Name,M->opSet.ops_AttrList ))!=NULL)
        {
            trackname = (char *)ptag->ti_Data;
        }
        if((ptag = FindTagItem( TRACKHEADER_StyleSheet,M->opSet.ops_AttrList ))!=NULL)
        {
            style = (AukStyle *)ptag->ti_Data;
        }
        if((ptag = FindTagItem( GA_DrawInfo, M->opSet.ops_AttrList ))!=NULL)
        {
            drawInfo = (struct DrawInfo *)ptag->ti_Data;
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
        }

        {
          if((Gad=(struct Gadget *)DoSuperMethodA(C,(Object *)Gad,(Msg)M))!=NULL)
          {
            int i;
            gdata=INST_DATA(C, Gad);
            bdbprintf_new("TrackHeader", Gad);
            Gad->GadgetID = GAD_TRACKHEADER_BASE+(iTrack<<4);

            // more than not sure: actually crash at boot.
           //not sure:  SetSuperAttrs(C,Gad,LAYOUT_DeferLayout,TRUE,TAG_END);

            /* Initialize all child pointers to NULL */
            for(i=0; i<THS_Total; i++) gdata->subs[i] = NULL;

            gdata->_style = style;
            gdata->_trackIndex = iTrack;

            /* Create all child gadgets directly - no nested layouts */

            /* Row 1: Close button and Name button */
            gdata->subs[THS_CloseButton] = NewObject( HEADERBUTTON_GetClass(),NULL,
                                        GA_Text, "X",
                                        GA_ID,GAD_TRACKHEADER_BASE|GAD_TRACKHEADER_CLOSE|(iTrack<<4),
                                        ICA_TARGET,target,
                                        GA_DrawInfo,(ULONG)drawInfo,
                                        GA_RelVerify, TRUE,
                                    TAG_END);

            gdata->subs[THS_NameButton] = NewObject( HEADERBUTTON_GetClass(),NULL,
                                        GA_Text,trackname,
                                        GA_ID,GAD_TRACKHEADER_BASE|GAD_TRACKHEADER_NAME|(iTrack<<4),
                                        ICA_TARGET,target,
                                        GA_DrawInfo,(ULONG)drawInfo,
                                        GA_RelVerify, TRUE,
                                    TAG_END);

            /* Row 2: Silencer and Solo buttons */
            gdata->subs[THS_SilencerBt] = NewObject( HEADERBUTTON_GetClass(),NULL,
                                        GA_Text, "Sil.",
                                        GA_TextAttr,(ULONG) &style->fontTiny_TA,
                                        GA_ID,GAD_TRACKHEADER_BASE|GAD_TRACKHEADER_SILENCER|(iTrack<<4),
                                        ICA_TARGET,target,
                                       // GA_DrawInfo,(ULONG)drawInfo,
                                        GA_RelVerify, TRUE,
                                        BUTTON_BevelStyle, BVS_THIN,
                                        BUTTON_PushButton,TRUE, // aka toggle button
                                    TAG_END);

            gdata->subs[THS_SoloBt] = NewObject( HEADERBUTTON_GetClass(),NULL,
                                        GA_Text,"Solo",
                                        GA_TextAttr,(ULONG) &style->fontTiny_TA,
                                        GA_ID,GAD_TRACKHEADER_BASE|GAD_TRACKHEADER_SOLO|(iTrack<<4),
                                        ICA_TARGET,target,
                                       // GA_DrawInfo,(ULONG)drawInfo,
                                        GA_RelVerify, TRUE,
                                        BUTTON_BevelStyle, BVS_THIN,
                                        BUTTON_PushButton,TRUE, // aka toggle button
                                    TAG_END);

            /* Row 3: Vol label and slider */
            gdata->subs[THS_VolLabel] = NewObject( BUTTON_GetClass(),NULL,
                                        GA_Text,(ULONG)"Vol.",
                                        GA_ReadOnly,TRUE,
                                      //  GA_DrawInfo,(ULONG)drawInfo,
                                        BUTTON_BevelStyle,BVS_NONE,
                                        BUTTON_Transparent, TRUE,
                                    TAG_END);

            gdata->subs[THS_VolumeSlider] = NewObject( SLIDER_GetClass(),NULL,
                                        SLIDER_Orientation, SLIDER_HORIZONTAL,
                                        SLIDER_Min, 0,
                                        SLIDER_Max, 128,
                                        SLIDER_Level, 128,
                                        ICA_TARGET,target,
                                       // GA_DrawInfo,(ULONG)drawInfo,
                                        GA_ID,GAD_TRACKHEADER_BASE|GAD_TRACKHEADER_VOL|(iTrack<<4),
                                        GA_RelVerify, TRUE,
                                    TAG_END);

            /* Row 4: Pan label and slider */
            gdata->subs[THS_PanLabel] = NewObject( BUTTON_GetClass(),NULL,
                                        GA_Text,(ULONG)"Pan",
                                        GA_ReadOnly,TRUE,
                                      //  GA_DrawInfo,(ULONG)drawInfo,
                                        BUTTON_BevelStyle,BVS_NONE,
                                        BUTTON_Transparent, TRUE,
                                    TAG_END);

            gdata->subs[THS_PanSlider] = NewObject( SLIDER_GetClass(),NULL,
                                        SLIDER_Orientation, SLIDER_HORIZONTAL,
                                        SLIDER_Min, 0,
                                        SLIDER_Max, 128,
                                        SLIDER_Level, 64,
                                        ICA_TARGET,target,
                                     //   GA_DrawInfo,(ULONG)drawInfo,
                                        GA_ID,GAD_TRACKHEADER_BASE|GAD_TRACKHEADER_PAN|(iTrack<<4),
                                        GA_RelVerify, TRUE,
                                    TAG_END);

            /* Row 5: Info label */
            gdata->subs[THS_InfoLabel] = NewObject( BUTTON_GetClass(),NULL,
                                        GA_Text,(ULONG)"Mono 22050Hz",
                                        GA_TextAttr,(ULONG) &style->fontTiny_TA,
                                        GA_ReadOnly,TRUE,
                                      //  GA_DrawInfo,(ULONG)drawInfo,
                                        BUTTON_BevelStyle,BVS_NONE,
                                        BUTTON_Transparent, TRUE,
                                    TAG_END);


            for(i=0;i<THS_Total;i++)
            {
                if(gdata->subs[i])
                {
                    //SetSuperAttrs(C,Gad,LAYOUT_AddChild,(ULONG)gdata->subs[i],TAG_END);
                    //SetAttrs(Gad,LAYOUT_AddChild,(ULONG)gdata->subs[i],TAG_END);
                    SetAttrs(Gad,LAYOUT_AddChild,(ULONG)gdata->subs[i],TAG_END);
                }

            }


            /* means new object OK so far: */
            retval=(ULONG)Gad;
          }
        }
      }
      break;
    case OM_DISPOSE:
      {
        int i;
        bdbprintf_dispose("TrackHeader", Gad);

        retval=DoSuperMethodA(C,(Object *)Gad,(Msg)M);
      }
      break;

    case GM_LAYOUT:
      {
        retval = TrackHeader_Layout(C, Gad, (struct gpLayout *)M);
      }
      break;
    case OM_UPDATE:
    case OM_SET:
      retval = TrackHeader_SetAttrs(C,Gad,(struct opSet *)M);
      if(!retval) retval = DoSuperMethodA(C,(Object *)Gad,(Msg)M);

     break;

    case OM_GET:
      retval= TrackHeader_GetAttr(C,Gad,(struct opGet *)M); // supercall done inside
     break;    default:
      // for anything, use default layout behaviour.
      retval=DoSuperMethodA(C,(Object *)Gad,(Msg)M);
      break;
  }
  return(retval);
}

// App Model instance as a Boopsi object.
extern Object *AppInstance;
void HeaderButton_Notify(Class *C, struct Gadget *Gad, struct GadgetInfo *ginfo)
{
   struct opUpdate notifymsg;
   ULONG tags[]={
    GA_ID,0,
    GA_Selected,0,
    GA_Disabled,0,
   // ICA_TARGET,0,
    TAG_DONE
   };

    //bdbprintf("HeaderButton_Notify:%08x\n", Gad->GadgetID);
    // official button Notify send exactly those states:
    tags[1] = Gad->GadgetID;

    tags[3] = ((Gad->Flags & GFLG_SELECTED)!=0);
//    GetAttr(GA_Selected,Gad,&tags[3]);
    GetAttr(GA_Disabled,Gad,&tags[5]);

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
    This is to ask a delayed redraw only on trackheaders.
    Solo button can affect maaaany buttons
*/
void TrackListView_UpdateTrackList_Headers();
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
          struct TagItem *ptag;
        gdata=INST_DATA(C, Gad);
        gdata->_isPushButton = 0;
        if((ptag = FindTagItem( BUTTON_PushButton,M->opSet.ops_AttrList ))!=NULL)
        {
            gdata->_isPushButton = (char *)ptag->ti_Data;
        }

     //   bdbprintf_new("HeaderButton", Gad);
        /* means new object OK so far: */
        retval=(ULONG)Gad;
      }
      break;
    case OM_DISPOSE:
     //   bdbprintf_dispose("HeaderButton", Gad);
      retval=DoSuperMethodA(C,(Object *)Gad,(Msg)M);
      break;


    case GM_GOACTIVE:
    {
        struct gpInput *gpi = (struct gpInput *)M;

        gdata=INST_DATA(C, Gad);

      //bdbprintf("hbt GOA isPushButton:%d\n",gdata->_isPushButton);
        /* Only become active if the GM_GOACTIVE   */
        /* was triggered by direct user input.     */
        if (((struct gpInput *)M)->gpi_IEvent)
        {
            int changed = 0;
            ULONG prevselected;
            GetAttr(GA_Selected, Gad,&prevselected );
            // = Gad->Flags & GFLG_SELECTED;
      //bdbprintf("hbt GOA isPushButton:2 prevselected %08x \n",(int)prevselected);
            /* This gadget is now active, change    */
            /* visual state to selected and render. */
            if(gdata->_isPushButton)
            {            
                changed = 1;
                SetAttrs(Gad,GA_Selected,prevselected ^1,TAG_END );
               // Gad->Flags ^= GFLG_SELECTED;
            } else
            {
                if(!prevselected)
                {
                    changed = 1;
                    SetAttrs(Gad,GA_Selected,1,TAG_END );
                }
                //Gad->Flags |= GFLG_SELECTED;
            }
 // bdbprintf("hbt GOA isPushButton:3 now %08x \n",(int)(Gad->Flags & GFLG_SELECTED));
            if(changed)
            {
              //  HeaderButton_Notify(C ,Gad,gpi->gpi_GInfo);
                /* delay drawing, button change color ! */
                TrackListView_UpdateTrackList_Headers();
            }

            retval = GMR_MEACTIVE;
        }
        else            /* The GM_GOACTIVE was not         */
            /* triggered by direct user input. */
            retval = GMR_NOREUSE;

        }
        break;
    case GM_HANDLEINPUT:
    {    
        struct gpInput *gpi = (struct gpInput *)M;
        struct InputEvent *ie = gpi->gpi_IEvent;
        gdata=INST_DATA(C, Gad);

        retval = GMR_MEACTIVE;

        if (ie->ie_Class == IECLASS_RAWMOUSE)
        {

            switch (ie->ie_Code)
            {
            case SELECTUP:
            {
                ULONG prevselected; // = Gad->Flags & GFLG_SELECTED;
                GetAttr(GA_Selected, Gad,&prevselected );
            /* The user let go of the gadget so return GMR_NOREUSE    */
                /* to deactivate and to tell Intuition not to reuse       */
                /* this Input Event as we have already processed it.      */

      //bdbprintf("hbt selup isPushButton:%d state %d\n",gdata->_isPushButton,(int)((Gad->Flags &GFLG_SELECTED)!=0) );
                /*If the user let go of the gadget while the mouse was    */
                /*over it, mask GMR_VERIFY into the return value so       */
                /*Intuition will send a Release Verify (GADGETUP).        */
                if ( ((gpi->gpi_Mouse).X < Gad->LeftEdge) ||
                    ((gpi->gpi_Mouse).X > Gad->LeftEdge + Gad->Width) ||
                    ((gpi->gpi_Mouse).Y < Gad->TopEdge) ||
                    ((gpi->gpi_Mouse).Y > Gad->TopEdge + Gad->Height) )
                {
                    retval = GMR_NOREUSE | GMR_VERIFY;
                }
                else
                {
                    retval = GMR_NOREUSE;
                }

                if(gdata->_isPushButton)
                {
                    // keep state
                } else
                {
                    SetAttrs(Gad,GA_Selected,0,TAG_END );
                     //Gad->Flags &= ~GFLG_SELECTED;
                }
                if(prevselected != (Gad->Flags & GFLG_SELECTED))
                {
                    //HeaderButton_Notify(C ,Gad,gpi->gpi_GInfo);
                    /* modified to delay drawing, button turns back unselected color ! */
                    TrackListView_UpdateTrackList_Headers();
                }
            }
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

//        gdata=INST_DATA(C, Gad);

//      bdbprintf("hbt GOI isPushButton:%d\n",gdata->_isPushButton);
//        if(gdata->_isPushButton)
//        {

//        } else
//        {
//             Gad->Flags &= ~GFLG_SELECTED;
//        }
//        /* modified to delay drawing, button turns back unselected color ! */
//        TrackListView_UpdateTrackList_Headers();
        retval = 1;
        break;
    case GM_RENDER:
        retval=DoSuperMethodA(C,(Object *)Gad,(Msg)M);
       // bdbprintf("bt GM_RENDER:%d\n",retval);
    break;

    default:
    {
      // for anything, use default  behaviour.
      retval=DoSuperMethodA(C,(Object *)Gad,(Msg)M);
      }
      break;
  }
  return(retval);


}

/*
 * Extend Slider class to prevent it from drawing from input events
 * like GM_GOACTIVE/GM_HANDLEINPUT/GM_GOINACTIVE.
 * In our TrackListArea layout logic, rendering should only be done during GM_RENDER,
 * and GM_RENDER sent from TrackListArea, so the correct Clipping region
 * is always applied.
 * Same pattern as HeaderButton_Dispatcher.
 */
ULONG ASM SAVEDS HeaderSlider_Dispatcher(
                    REG(a0,struct IClass *C),
                    REG(a2,struct Gadget *Gad),
                    REG(a1,union MsgUnion *M))
{
  TrackHeaderSlider *gdata;
  ULONG retval=0;
  gdata=INST_DATA(C, Gad);

  switch(M->MethodID)
  {
      case OM_NEW:
      if(Gad=(struct Gadget *)DoSuperMethodA(C,(Object *)Gad,(Msg)M))
      {
        gdata=INST_DATA(C, Gad);
        bdbprintf_new("HeaderSlider", Gad);
        /* means new object OK so far: */
        retval=(ULONG)Gad;
      }
      break;
    case OM_DISPOSE:
        bdbprintf_dispose("HeaderSlider", Gad);
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
            /* modified to delay drawing */
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
            default:
                retval = GMR_MEACTIVE;
            }

        }
        else if (ie->ie_Class == IECLASS_TIMER)
        {
            /* If the gadget gets a timer event, it sends an interim OM_NOTIFY */
            /* For sliders, we might want to handle continuous updates here */
        }
    }
    break;
    case GM_GOINACTIVE:           /* Intuition said to go inactive.  Clear the GFLG_SELECTED */
        Gad->Flags &= ~GFLG_SELECTED;
        /* modified to delay drawing */
        TrackListView_UpdateTrackList_Generic();
        retval = 1;
        break;
    case GM_RENDER:
        retval=DoSuperMethodA(C,(Object *)Gad,(Msg)M);
    break;

    default:
    {
      // for anything, use default slider behaviour.
      retval=DoSuperMethodA(C,(Object *)Gad,(Msg)M);
      }
      break;
  }
  return(retval);
}
