

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
#include "../VolumeRule/class_volumerule.h"

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
ULONG TrackHeader_Render(Class *C, struct Gadget *Gad, struct gpRender *Render);

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

       // bdbprintf("OM_NEW trackheader style %08x\n",(int)style);
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
          if(Gad=(struct Gadget *)DoSuperMethodA(C,(Object *)Gad,(Msg)M))
          {
            int i;
            gdata=INST_DATA(C, Gad);
            bdbprintf_new("TrackHeader", Gad);
            Gad->GadgetID = GAD_TRACKHEADER_BASE+(iTrack<<4);

           //not sure SetSuperAttrs(C,Gad,LAYOUT_DeferLayout,TRUE,TAG_END);
            /* Initialize all child pointers to NULL */
            for(i=0; i<THS_Total; i++) gdata->subs[i] = NULL;

            gdata->_style = style;
            gdata->_trackIndex = iTrack;

            /* Create all child gadgets directly - no nested layouts */

            /* Row 1: Close button and Name button */
            gdata->subs[THS_CloseButton] = NewObject( BUTTON_GetClass(),NULL,
                                        GA_Text, "X",
                                        GA_ID,GAD_TRACKHEADER_BASE|GAD_TRACKHEADER_CLOSE|(iTrack<<4),
                                      //  ICA_TARGET,target,
                                       // GA_DrawInfo,(ULONG)drawInfo,
                                      //  GA_RelVerify, TRUE,
                                    TAG_END);

            gdata->subs[THS_NameButton] = NewObject( BUTTON_GetClass(),NULL,
                                        GA_Text,trackname,
                                        GA_ID,GAD_TRACKHEADER_BASE|GAD_TRACKHEADER_NAME|(iTrack<<4),
                                      //  ICA_TARGET,target,
                                       // GA_DrawInfo,(ULONG)drawInfo,
                                      //  GA_RelVerify, TRUE,
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
                                    TAG_END);

            gdata->subs[THS_SoloBt] = NewObject( HEADERBUTTON_GetClass(),NULL,
                                        GA_Text,"Solo",
                                        GA_TextAttr,(ULONG) &style->fontTiny_TA,
                                        GA_ID,GAD_TRACKHEADER_BASE|GAD_TRACKHEADER_SOLO|(iTrack<<4),
                                        ICA_TARGET,target,
                                       // GA_DrawInfo,(ULONG)drawInfo,
                                        GA_RelVerify, TRUE,
                                        BUTTON_BevelStyle, BVS_THIN,
                                    TAG_END);

            /* Row 3: Vol label and slider */
            gdata->subs[THS_VolLabel] = NewObject( HEADERBUTTON_GetClass(),NULL,
                                        GA_Text,(ULONG)"Vol.",
                                        GA_ReadOnly,TRUE,
                                      //  GA_DrawInfo,(ULONG)drawInfo,
                                        BUTTON_BevelStyle,BVS_NONE,
                                        BUTTON_Transparent, TRUE,
                                    TAG_END);

            gdata->subs[THS_VolumeSlider] = NewObject( HEADERSLIDER_GetClass(),NULL,
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
            gdata->subs[THS_PanLabel] = NewObject( HEADERBUTTON_GetClass(),NULL,
                                        GA_Text,(ULONG)"Pan",
                                        GA_ReadOnly,TRUE,
                                      //  GA_DrawInfo,(ULONG)drawInfo,
                                        BUTTON_BevelStyle,BVS_NONE,
                                        BUTTON_Transparent, TRUE,
                                    TAG_END);

            gdata->subs[THS_PanSlider] = NewObject( HEADERSLIDER_GetClass(),NULL,
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
            gdata->subs[THS_InfoLabel] = NewObject( HEADERBUTTON_GetClass(),NULL,
                                        GA_Text,(ULONG)"Mono 22050Hz",
                                        GA_TextAttr,(ULONG) &style->fontTiny_TA,
                                        GA_ReadOnly,TRUE,
                                      //  GA_DrawInfo,(ULONG)drawInfo,
                                        BUTTON_BevelStyle,BVS_NONE,
                                        BUTTON_Transparent, TRUE,
                                    TAG_END);

            /* Right side: VolumeRule */
            gdata->subs[THS_VolumeRule] = NewObject( VOLUMERULE_GetClass(),NULL,
                                        VOLUMERULE_StyleSheet,(ULONG)style,
                                    TAG_END );
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


        /* Dispose all child gadgets manually since we don't use LAYOUT_AddChild */
        // now use LAYOUT_AddChild
//        gdata=INST_DATA(C, Gad);
//        for(i=0; i<THS_Total; i++)
//        {
//            if(gdata->subs[i])
//            {
//                DisposeObject(gdata->subs[i]);
//                gdata->subs[i] = NULL;
//            }
//        }
        retval=DoSuperMethodA(C,(Object *)Gad,(Msg)M);
      }
      break;

    case GM_LAYOUT:
      {
       // gdata=INST_DATA(C, Gad);
        retval = TrackHeader_Layout(C, Gad, (struct gpLayout *)M);
      }
      break;
//    case GM_RENDER:
//      {
//       // gdata=INST_DATA(C, Gad);
//        retval = TrackHeader_Render(C, Gad, (struct gpLayout *)M);
//      }
//      break;

    case OM_UPDATE:
    case OM_SET:
      retval = TrackHeader_SetAttrs(C,Gad,(struct opSet *)M);
      if(!retval) retval = DoSuperMethodA(C,(Object *)Gad,(Msg)M);

     break;

    case OM_GET:
      retval= TrackHeader_GetAttr(C,Gad,(struct opGet *)M); // supercall done inside
     break;

//    case GM_HITTEST:
//      {
//        /* Forward hit test to child gadgets */
//        struct gpHitTest *ht = (struct gpHitTest *)M;
//        WORD mx = ht->gpht_Mouse.X;
//        WORD my = ht->gpht_Mouse.Y;
//        int i;

//        gdata=INST_DATA(C, Gad);
//        retval = 0; /* default: not hit */

//        /* Check all child gadgets for hit */
//        for(i=0; i<THS_Total; i++)
//        {
//            struct Gadget *sub = (struct Gadget *)gdata->subs[i];
//            if(sub)
//            {
//                /* Convert mouse coords relative to child gadget */
//                WORD childX = mx + Gad->LeftEdge - sub->LeftEdge;
//                WORD childY = my + Gad->TopEdge - sub->TopEdge;

//                /* Check if inside child bounds */
//                if(childX >= 0 && childX < sub->Width &&
//                   childY >= 0 && childY < sub->Height)
//                {
//                    /* Forward to child's GM_HITTEST */
//                    struct gpHitTest childHt;
//                    childHt.MethodID = GM_HITTEST;
//                    childHt.gpht_GInfo = ht->gpht_GInfo;
//                    childHt.gpht_Mouse.X = childX;
//                    childHt.gpht_Mouse.Y = childY;

//                    if(DoMethodA((Object*)sub, (Msg)&childHt))
//                    {
//                        retval = GMR_GADGETHIT;
//                        break;
//                    }
//                }
//            }
//        }
//        /* If no child hit, check if we are hit */
//        if(!retval)
//        {
//            if(mx >= 0 && mx < Gad->Width && my >= 0 && my < Gad->Height)
//            {
//                retval = GMR_GADGETHIT;
//            }
//        }
//      }
//      break;

//    case GM_GOACTIVE:
//    case GM_HANDLEINPUT:
//      {
//        /* Forward to child gadget that was hit */
//        struct gpInput *gpi = (struct gpInput *)M;
//        WORD mx = gpi->gpi_Mouse.X;
//        WORD my = gpi->gpi_Mouse.Y;
//        int i;

//        gdata=INST_DATA(C, Gad);
//        retval = GMR_NOREUSE;

//        /* Find which child gadget was clicked */
//        for(i=0; i<THS_Total; i++)
//        {
//            struct Gadget *sub = (struct Gadget *)gdata->subs[i];
//            if(sub)
//            {
//                /* Convert mouse coords relative to child gadget */
//                WORD childX = mx + Gad->LeftEdge - sub->LeftEdge;
//                WORD childY = my + Gad->TopEdge - sub->TopEdge;

//                /* Check if inside child bounds */
//                if(childX >= 0 && childX < sub->Width &&
//                   childY >= 0 && childY < sub->Height)
//                {
//                    /* Forward to child */
//                    struct gpInput childGpi;
//                    childGpi.MethodID = M->MethodID;
//                    childGpi.gpi_GInfo = gpi->gpi_GInfo;
//                    childGpi.gpi_IEvent = gpi->gpi_IEvent;
//                    childGpi.gpi_Termination = gpi->gpi_Termination;
//                    childGpi.gpi_Mouse.X = childX;
//                    childGpi.gpi_Mouse.Y = childY;
//                    childGpi.gpi_TabletData = gpi->gpi_TabletData;

//                    retval = DoMethodA((Object*)sub, (Msg)&childGpi);
//                    break;
//                }
//            }
//        }
//      }
//      break;

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
    //bdbprintf("HeaderButton_Notify:%08x\n", Gad->GadgetID);
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
