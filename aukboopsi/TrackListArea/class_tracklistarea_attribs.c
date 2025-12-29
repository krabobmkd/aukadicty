
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/dos.h>

#ifdef __SASC
//    #include "minialib.h"
    #include <clib/alib_protos.h>
#else
    // GCC
    #include "minialib.h"
#endif

#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/gadgetclass.h>

#include "class_tracklistarea.h"
#include "class_tracklistarea_private.h"

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
extern struct IClass   *TrackListClassPtr;

ULONG TrackListArea_NotifyAttribValue(struct Gadget *Gad, struct GadgetInfo *GInfo,ULONG attrib, ULONG value)
{
    struct opUpdate notifymsg;
    TrackListArea *gdata=INST_DATA(TrackListClassPtr, Gad);
    ULONG tags[]={
     GA_ID,0,
     0,0,
     TAG_DONE
    };
 bdbprintf(" **** TrackListArea_NotifyAttribValue: Gad->GadgetID:%d \n",Gad->GadgetID);
    tags[1] = Gad->GadgetID;
    tags[2] = attrib;
    tags[3] = value;
    notifymsg.MethodID = OM_NOTIFY;
    notifymsg.opu_AttrList = (struct TagItem *)&tags[0];
    notifymsg.opu_GInfo = GInfo; // "always there for gadget, in all messages"
    notifymsg.opu_Flags = 0;

    return DoSuperMethodA(TrackListClassPtr,(APTR)Gad,(Msg)&notifymsg );
}


ULONG TrackListArea_GetAttr(Class *C, struct Gadget *Gad, struct opGet *Get)
{
  ULONG retval=1;
  int   DoSuperCall=0;
  TrackListArea *gdata;
  ULONG *data;

  gdata=INST_DATA(C, Gad);

  data=Get->opg_Storage;

  switch(Get->opg_AttrID)
  {
    case TRACKLIST_ScrollY:
      *data = (ULONG)gdata->_scrollY;
      break;

    case TRACKLIST_DomainHeight:
      *data = gdata->_domainHeight;
      break;

    case TRACKLIST_TimeProjection:
    {
        ((TimeProjection *)data)->_timeAtLeft = gdata->_scrollX;
        ((TimeProjection *)data)->_timePerPixelWidth = gdata->_timePerPixelWidth;
      }
      break;

    case TRACKLIST_HeaderWidth:
      *data = (ULONG)gdata->_headerWidth;
      break;

    // super class gadget things. would manage attribs selected/hightlighted, ...
    default:
        DoSuperCall = 1;
      // everything we don't manage directly is managed by supercall.
  }
  if(DoSuperCall)  retval=DoSuperMethodA(C, (APTR)Gad, (APTR)Get);

  return(retval);
}



ULONG TrackListArea_SetAttrs(Class *C, struct Gadget *Gad, struct opSet *Set)
{
  struct TagItem *tag;
  ULONG data; // for SetAttribs, retval means if anything needed redraw.
  TrackListArea *gdata;
    ULONG used=0;
  gdata=INST_DATA(C, Gad);

 // set can use a list of attribs to change, so we manage this with a loop.
 // this also allows to have just one draw refresh for a set of change.
  for( tag = Set->ops_AttrList ;
        tag->ti_Tag != TAG_END ;
        tag++
   )
  {
    data=tag->ti_Data;

    switch(tag->ti_Tag)
    {
      case TRACKLIST_ScrollY:
        {
            used = 1;
          LONG newScrollY = (LONG)data;
          if(gdata->_scrollY != newScrollY)
          {
            gdata->_scrollY = newScrollY;
  //  bdbprintf(" **** TrackListArea_SetAttrs: newScrollY:%d  Gad->GadgetID:%d\n",newScrollY,Gad->GadgetID);
            TrackListArea_NotifyAttribValue(Gad,Set->ops_GInfo, TRACKLIST_ScrollY, newScrollY);
          }
        }
        break;

    case TRACKLIST_TimeProjection:
    {
       TimeProjection *pproj = (TimeProjection *)data;
        used = 1;
        if(pproj->_timeAtLeft != gdata->_scrollX ||
           pproj->_timePerPixelWidth != gdata->_timePerPixelWidth )
           {
                gdata->_scrollX = pproj->_timeAtLeft;
                gdata->_timePerPixelWidth = gdata->_timePerPixelWidth;
                TrackListArea_NotifyAttribValue(Gad, Set->ops_GInfo, TRACKLIST_TimeProjection, (ULONG)data);
           }
      }
      break;

      case TRACKLIST_StyleSheet:
      used = 1;
        gdata->_styleSheet = (struct AukStyleSheet *)data;
        break;

     // - - - actually we have to manage super class attribs:
     // with GA_XXX and struct Gadget members...
     // is there  a way to super call this ? DoSuperMethodA() deosn't seems to manage these attribs.
      case GA_Disabled:
        {
        used = 1;
            if(data) Gad->Flags |= GFLG_DISABLED; // set bit
            else Gad->Flags &= ~GFLG_DISABLED; // remove bit.
        }
        break;
      case GA_Highlight:
        {
        used = 1;
            if(data) Gad->Flags |= GFLG_GADGHBOX; // set bit
            else Gad->Flags &= ~GFLG_GADGHBOX; // remove bit.
        }
        break;
      case GA_Selected:
        {
        used = 1;
            if(data) Gad->Flags |= GFLG_SELECTED; // set bit
            else Gad->Flags &= ~GFLG_SELECTED; // remove bit.
        }
        break;
    case TRACKLIST_Refresh:
    {
        //bdbprintf("TrackListArea_SetAttrs TRACKLIST_Refresh:%08x\n",(int)Set->ops_GInfo);
        // goes layout...
        {
            struct gpLayout gpl;
            gpl.MethodID = GM_LAYOUT;
            gpl.gpl_GInfo = Set->ops_GInfo;
            gpl.gpl_Initial = 0;
            TrackListArea_Layout(C,Gad,&gpl);
        }
        // goes render...
        {
            struct gpRender gpr;
            gpr.MethodID = GM_RENDER;
            gpr.gpr_GInfo = Set->ops_GInfo;
            gpr.gpr_RPort = ObtainGIRPort(gpr.gpr_GInfo);
            if(gpr.gpr_RPort)
            {
                gpr.gpr_Redraw = 1;
                TrackListArea_Render(C,Gad,&gpr);
                ReleaseGIRPort(gpr.gpr_RPort);
            }
        }
    }
    break;

    default:
        //does not seems to do anything for gadgets.... DoSuperMethodA(C,(APTR)Gad,(Msg)Set);
        //note: apparently super call is not to be managed here (not sure !!!)
        break;

    } // end switch
  } // end for

  return(used);
}


