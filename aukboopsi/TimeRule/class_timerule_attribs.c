
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

#include "class_timerule.h"
#include "class_timerule_private.h"
#include "../aukstylesheet.h"

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
    case TIMERULE_DefHeight:
        *data = (ULONG)gdata->_defaultHeight;
        break;

    case TIMERULE_TimeLeftHi:
        *data = (ULONG)gdata->_timeLeftHi;
        break;
    case TIMERULE_TimeLeftLo:
        *data = (ULONG)gdata->_timeLeftLo;
        break;

    case TIMERULE_TimeRightHi:
        *data = (ULONG)gdata->_timeRightHi;
        break;
    case TIMERULE_TimeRightLo:
        *data = (ULONG)gdata->_timeRightLo;
        break;

    case TIMERULE_TrackAreaOffsetX:
        *data = (ULONG)gdata->_trackAreaOffsetX;
        break;

    case TIMERULE_StyleSheet:
        *data = (ULONG)gdata->_styleSheet;
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
  ULONG redraw=0, invalidateTiles=0;

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
      case TIMERULE_DefHeight:
        if((UWORD)data != gdata->_defaultHeight)
        {
            gdata->_defaultHeight = (UWORD)data;
            redraw=1;
        }
        break;

      case TIMERULE_TimeLeftHi:
        if((LONG)data != gdata->_timeLeftHi)
        {
            gdata->_timeLeftHi = (LONG)data;
            invalidateTiles=1;
        }
        break;
      case TIMERULE_TimeLeftLo:
        if((LONG)data != gdata->_timeLeftLo)
        {
            gdata->_timeLeftLo = (LONG)data;
            invalidateTiles=1;
        }
        break;

      case TIMERULE_TimeRightHi:
        if((LONG)data != gdata->_timeRightHi)
        {
            gdata->_timeRightHi = (LONG)data;
            invalidateTiles=1;
        }
        break;
      case TIMERULE_TimeRightLo:
        if((LONG)data != gdata->_timeRightLo)
        {
            gdata->_timeRightLo = (LONG)data;
            invalidateTiles=1;
        }
        break;

      case TIMERULE_TrackAreaOffsetX:
        if((UWORD)data != gdata->_trackAreaOffsetX)
        {
            gdata->_trackAreaOffsetX = (UWORD)data;
            invalidateTiles=1;
        }
        break;

      case TIMERULE_StyleSheet:
        if((struct AukStyleSheet *)data != gdata->_styleSheet)
        {
            gdata->_styleSheet = (struct AukStyleSheet *)data;
            invalidateTiles=1;
        }
        break;

      /* GA_XXX attribs with struct Gadget members... */
      case GA_Disabled:
        {
            if(data) Gad->Flags |= GFLG_DISABLED;
            else Gad->Flags &= ~GFLG_DISABLED;
            redraw=1;
        }
        break;
      case GA_Highlight:
        {
            if(data) Gad->Flags |= GFLG_GADGHBOX;
            else Gad->Flags &= ~GFLG_GADGHBOX;
            redraw=1;
        }
        break;
      case GA_Selected:
        {
            if(data) Gad->Flags |= GFLG_SELECTED;
            else Gad->Flags &= ~GFLG_SELECTED;
            redraw=1;
        }
        break;
      default:
        /* InfiniteScroll attribs are handled by supercall in dispatcher */
        break;

    } /* end switch */
  } /* end for */

  if(invalidateTiles)
  {
      /* When time borders change, all tiles need to be redrawn */
      /* The render function will handle this */
      redraw = 1;
  }

  return(redraw);
}


