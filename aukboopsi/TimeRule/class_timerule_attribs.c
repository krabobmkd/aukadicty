
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/dos.h>


#include <clib/alib_protos.h>

#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/gadgetclass.h>

#include "class_timerule.h"
#include "class_timerule_private.h"
#include "../aukstyle.h"

#include <utility/tagitem.h>

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


//static ULONG TimeRule_NotifyAttribValue(Class *C,struct Gadget *Gad, struct GadgetInfo *GInfo,ULONG attrib, ULONG value)
//{
//    struct opUpdate notifymsg;
//    // InfiniteScroll *gdata;
//    // gdata = INST_DATA(C, Gad);
//    ULONG tags[]={
//     GA_ID,0,
//     0,0,
//     TAG_DONE
//    };
//    tags[1] = Gad->GadgetID;
//    tags[2] = attrib;
//    tags[3] = value;
//    notifymsg.MethodID = OM_NOTIFY;
//    notifymsg.opu_AttrList = (struct TagItem *)&tags[0];
//    notifymsg.opu_GInfo = GInfo; // "always there for gadget, in all messages"
//    notifymsg.opu_Flags = 0;

//    return DoSuperMethodA(C,(APTR)Gad,(Msg)&notifymsg );
//}


ULONG TimeRule_GetAttr(Class *C, struct Gadget *Gad, struct opGet *Get)
{
  ULONG retval=1;
  int   DoSuperCall=0;
  TimeRule *gdata;
  ULONG *data;

  gdata=INST_DATA(C, Gad);

  data=Get->opg_Storage;

  switch(Get->opg_AttrID)
  {
    case TIMERULE_TimePerPixelWidth:
    {
        long long *pv = (long long *)data;
        *pv = gdata->_timePerPixelWidth;
    }
    break;
    case TIMERULE_StyleSheet:
        *data = (ULONG)gdata->_style;
        break;
    case TIMERULE_TimeSelection:
        *data = (ULONG)gdata->_timeSelection;
        break;

    /* super class gadget things. would manage attribs selected/highlighted, ... */
    /* InfiniteScroll attribs are also handled by supercall */
    default:
        DoSuperCall = 1;
        /* everything we don't manage directly is managed by supercall. */
  }
  if(DoSuperCall)  retval=DoSuperMethodA(C, (APTR)Gad, (APTR)Get);

  return(retval);
}


ULONG TimeRule_SetAttrs(Class *C, struct Gadget *Gad, struct opSet *Set)
{
  struct TagItem *tag;
  ULONG data;
  TimeRule *gdata;
  ULONG fullRedraw=0, justScroll=0,used=0;

  gdata=INST_DATA(C, Gad);

  /* set can use a list of attribs to change, so we manage this with a loop. */
  /* this also allows to have just one draw refresh for a set of change. */
  for( tag = Set->ops_AttrList ;
        tag->ti_Tag != TAG_END ;
        tag++
   )
  {
    data=tag->ti_Data;

    switch(tag->ti_Tag)
    {
        case TIMERULE_TimePerPixelWidth:
        {
            long long *pv = (long long *)data;
            if( gdata->_timePerPixelWidth != *pv)
            {
                fullRedraw=1;
                gdata->_timePerPixelWidth = *pv;
                TimeRule_UpdateTimeInterval(gdata);
            }

       //  bdbprintf(" TIMERULE_TimePerPixelWidth set %08x.%08x fullRedraw:%d\n",
        //  (int)(gdata->_timePerPixelWidth>>32),(int)gdata->_timePerPixelWidth,fullRedraw);
           // fullRedraw = 1;
            used=1;
        }
        break;
      case TIMERULE_StyleSheet:
      {
        /* data points to AukStyleSheet, extract the style member */
        AukStyle *newStyle = (struct AukStyle *)data;
        if(newStyle != gdata->_style)
        {
            gdata->_style = newStyle;
            bdbprintf("");

            fullRedraw=1;
            used=1;
        }

       }break;
      case TIMERULE_TimeSelection:
      {
        AukSelection *newSelection = (AukSelection *)data;
        if(newSelection != gdata->_timeSelection)
        {
            gdata->_timeSelection = newSelection;
            fullRedraw = 1;
            used = 1;
        }
      }break;
       case TIMERULE_Refresh:
       {
            // layout should be ok
            // goes render...
            {
                struct gpRender gpr;
                gpr.MethodID = GM_RENDER;
                gpr.gpr_GInfo = Set->ops_GInfo;
                gpr.gpr_RPort = ObtainGIRPort(gpr.gpr_GInfo);
                if(gpr.gpr_RPort)
                {
                    gpr.gpr_Redraw = 1;
                   // supercall
                   DoSuperMethodA(C,(APTR)Gad,(Msg)&gpr );
                    ReleaseGIRPort(gpr.gpr_RPort);
                }
            }
       } break;
      /* GA_XXX attribs with struct Gadget members... */
      case GA_Disabled:
        {
            if(data) Gad->Flags |= GFLG_DISABLED;
            else Gad->Flags &= ~GFLG_DISABLED;
            fullRedraw=1;
            used=1;
        }
        break;
      case GA_Highlight:
        {
            if(data) Gad->Flags |= GFLG_GADGHBOX;
            else Gad->Flags &= ~GFLG_GADGHBOX;
            fullRedraw=1;
            used=1;
        }
        break;
      case GA_Selected:
        {
            if(data) Gad->Flags |= GFLG_SELECTED;
            else Gad->Flags &= ~GFLG_SELECTED;
            fullRedraw=1;
            used=1;
        }
        break;
      default:
        /* InfiniteScroll attribs are handled by supercall in dispatcher */
        break;

    } /* end switch */
  } /* end for */

 if(fullRedraw)
 {
    ULONG atr[]={INFINITESCROLL_FullTilesRefresh,TRUE,TAG_END};
    struct opSet sSet;
    sSet.MethodID = OM_SET;
    sSet.ops_AttrList = &atr[0];
    sSet.ops_GInfo = Set->ops_GInfo;
    DoSuperMethodA(C,(Object *)Gad,(Msg)&sSet);
 }

    if((justScroll|fullRedraw)!=0)
    {
       // goes render...
        {
            struct gpRender gpr;
            gpr.MethodID = GM_RENDER;
            gpr.gpr_GInfo = Set->ops_GInfo;
            gpr.gpr_RPort = ObtainGIRPort(gpr.gpr_GInfo);
            if(gpr.gpr_RPort)
            {
                gpr.gpr_Redraw = 1;
                DoSuperMethodA(C,(APTR)Gad,(Msg)&gpr );
                ReleaseGIRPort(gpr.gpr_RPort);
            }
        }
    }

  return(used);
}


