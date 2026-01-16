


#include <clib/alib_protos.h>
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
            gdata->subs[THS_SilencerBt] = NewObject( BUTTON_GetClass(),NULL,
                                        GA_Text, "Sil.",
                                        GA_TextAttr,(ULONG) &style->fontTiny_TA,
                                        GA_ID,GAD_TRACKHEADER_BASE|GAD_TRACKHEADER_SILENCER|(iTrack<<4),
                                        ICA_TARGET,target,
                                       // GA_DrawInfo,(ULONG)drawInfo,
                                        GA_RelVerify, TRUE,
                                        BUTTON_BevelStyle, BVS_THIN,
                                    TAG_END);

            gdata->subs[THS_SoloBt] = NewObject( BUTTON_GetClass(),NULL,
                                        GA_Text,"Solo",
                                        GA_TextAttr,(ULONG) &style->fontTiny_TA,
                                        GA_ID,GAD_TRACKHEADER_BASE|GAD_TRACKHEADER_SOLO|(iTrack<<4),
                                        ICA_TARGET,target,
                                       // GA_DrawInfo,(ULONG)drawInfo,
                                        GA_RelVerify, TRUE,
                                        BUTTON_BevelStyle, BVS_THIN,
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

