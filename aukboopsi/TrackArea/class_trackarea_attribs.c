
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/dos.h>


#include <clib/alib_protos.h>

#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/gadgetclass.h>

#include "class_trackarea.h"
#include "class_trackarea_private.h"

#include <utility/tagitem.h>

#include "bdbprintf.h"

ULONG TrackArea_GetAttr(Class *C, struct Gadget *Gad, struct opGet *Get)
{
  ULONG retval=1;
  int   DoSuperCall=0;
  TrackArea *gdata;
  ULONG *data;

  gdata=INST_DATA(C, Gad);

  data=Get->opg_Storage;

  switch(Get->opg_AttrID)
  {
    case TRACKAREA_PTimeProjection:
        *data = (ULONG)gdata->_pTimeProjection;
        break;

    case TRACKAREA_StyleSheet:
        *data = (ULONG)gdata->_style;
        break;

    case TRACKAREA_DataTrack:
        *data = (ULONG)gdata->_dataTrack;
        break;

    case TRACKAREA_TimeSelection:
        *data = (ULONG)gdata->_dataSelection;
        break;

    /* super class gadget things. would manage attribs selected/highlighted, ... */
    default:
        DoSuperCall = 1;
      /* everything we don't manage directly is managed by supercall. */
  }
  if(DoSuperCall)  retval=DoSuperMethodA(C, (APTR)Gad, (APTR)Get);

  return(retval);
}


ULONG TrackArea_SetAttrs(Class *C, struct Gadget *Gad, struct opSet *Set)
{
  struct TagItem *tag;
  ULONG data; /* for SetAttribs, retval means if anything needed redraw. */
  TrackArea *gdata;
  ULONG redraw=0, update=0;

  gdata=INST_DATA(C, Gad);

 //bdbprintf(" **** TrackArea_SetAttrs\n");

  /* set can use a list of attribs to change, so we manage this with a loop.
   * this also allows to have just one draw refresh for a set of change.
   */
  for( tag = Set->ops_AttrList ;
        tag->ti_Tag != TAG_END ;
        tag++
   )
  {
    data=tag->ti_Data;

    switch(tag->ti_Tag)
    {
      case TRACKAREA_StyleSheet:
        /* data points to AukStyle */
         //bdbprintf(" **** setattr TRACKAREA_StyleSheet %08x\n",data);
        gdata->_style = (struct AukStyle *)data;
        redraw = 1;
        break;

      case TRACKAREA_PTimeProjection:
        /* data is a pointer to TimeProjection in TrackListArea */
       //  bdbprintf(" **** TimeProjection %08x\n",data);
        gdata->_pTimeProjection = (TimeProjection *)data;
        redraw = 1;
        break;

      case TRACKAREA_DataTrack:
        /* data is a pointer to AukTrack - use AukObjectPtr_Set for reference counting */
         bdbprintf(" **** TRACKAREA_DataTrack %08x\n",data);
        AukObjectPtr_Set((AukObjectPtr*)&gdata->_dataTrack, (AukObject*)data);
        redraw = 1;
        break;

      case TRACKAREA_TimeSelection:
        /* data is a pointer to AukSelection (selection start/end) */
        gdata->_dataSelection = (AukSelection *)data;
        redraw = 1;
        break;

     default:
        /* other attribs handled by InfiniteScroll superclass */
        break;

    } /* end switch */
  } /* end for */


  return(redraw | update);
}


