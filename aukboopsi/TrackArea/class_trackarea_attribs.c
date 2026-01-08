
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

#include "class_trackarea.h"
#include "class_trackarea_private.h"

#include <utility/tagitem.h>


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
    // case TRACKAREA_CenterX:
    //     *data = (LONG)gdata->_circleCenterX;
    // break;
    // case TRACKAREA_CenterY:
    //     *data = (LONG)gdata->_circleCenterY;
    // break;
    // super class gadget things. would manage attribs selected/hightlighted, ...
    default:
        DoSuperCall = 1;
      // everything we don't manage directly is managed by supercall.
  }
  if(DoSuperCall)  retval=DoSuperMethodA(C, (APTR)Gad, (APTR)Get);

  return(retval);
}


ULONG TrackArea_SetAttrs(Class *C, struct Gadget *Gad, struct opSet *Set)
{
  struct TagItem *tag;
  ULONG data; // for SetAttribs, retval means if anything needed redraw.
  TrackArea *gdata;
  ULONG redraw=0, update=0, notifCoords=0;

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
      case TRACKAREA_StyleSheet:
        /* data points to AukStyle, extract the style member */
        gdata->_style = (struct AukStyle *)data;
        break;

     // case TRACKAREA_CenterX:
     //    if((UWORD)data != gdata->_circleCenterX )
     //    {
     //        gdata->_circleCenterX = (UWORD)data ;
     //        redraw=1;
     //        notifCoords = 1;
     //    }
     //    break;
     // case TRACKAREA_CenterY:
     //    if((UWORD)data != gdata->_circleCenterY )
     //    {
     //        gdata->_circleCenterY = (UWORD)data ;

     //        redraw=1;
     //        notifCoords = 1;
     //    }
     //    break;
     // - - - actually we have to manage super class attribs:
     // with GA_XXX and struct Gadget members...
     // is there  a way to super call this ? DoSuperMethodA() deosn't seems to manage these attribs.
      case GA_Disabled:
        {
            if(data) Gad->Flags |= GFLG_DISABLED; // set bit
            else Gad->Flags &= ~GFLG_DISABLED; // remove bit.
            redraw=1;
        }
        break;
      case GA_Highlight:
        {
            if(data) Gad->Flags |= GFLG_GADGHBOX; // set bit
            else Gad->Flags &= ~GFLG_GADGHBOX; // remove bit.
            redraw=1;
        }
        break;
      case GA_Selected:
        {
            if(data) Gad->Flags |= GFLG_SELECTED; // set bit
            else Gad->Flags &= ~GFLG_SELECTED; // remove bit.
            redraw=1;
        }
        break;
    default:
        //does not seems to do anything for gadgets.... DoSuperMethodA(C,(APTR)Gad,(Msg)Set);
        //note: apparently super call is not to be managed here (not sure !!!)
        break;

    } // end switch
  } // end for


  return(redraw| update);
}


