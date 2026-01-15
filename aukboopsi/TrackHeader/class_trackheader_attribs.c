
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/dos.h>
#include <stdio.h>
#include <string.h>

#include <clib/alib_protos.h>

#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/gadgetclass.h>

#include "class_trackheader.h"
#include "class_trackheader_private.h"

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

void HeaderButton_Notify(Class *C, struct Gadget *Gad, struct GadgetInfo *ginfo);

ULONG TrackHeader_GetAttr(Class *C, struct Gadget *Gad, struct opGet *Get)
{
  ULONG retval=1;
  int   DoSuperCall=0;
  TrackHeader *gdata;
  ULONG *data;

  gdata=INST_DATA(C, Gad);

  data=Get->opg_Storage;

  switch(Get->opg_AttrID)
  {
     case TRACKHEADER_TrackIndex:
         *data = (LONG)gdata->_trackIndex;
     break;
     case TRACKHEADER_Name:
         *data = (LONG)0;
     break;
     case TRACKHEADER_Pan:
         *data = (LONG)0;
     break;
     case TRACKHEADER_Volume:
         *data = (LONG)0;
     break;

    // super class gadget things. would manage attribs selected/hightlighted, ...
    default:
        DoSuperCall = 1;
      // everything we don't manage directly is managed by supercall.
  }
  if(DoSuperCall)  retval=DoSuperMethodA(C, (APTR)Gad, (APTR)Get);

  return(retval);
}

ULONG TrackHeader_SetAttrs(Class *C, struct Gadget *Gad, struct opSet *Set)
{
  struct TagItem *tag;
  ULONG data; // for SetAttribs, retval means if anything needed redraw.
  TrackHeader *gdata;
  ULONG actuallydone=0;

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
      case TRACKHEADER_StyleSheet:
        /* data points to AukStyle, extract the style member */
        gdata->_style = (struct AukStyle *)data;
        actuallydone = 1;
        break;
       case  TRACKHEADER_Name:
       {
          struct Gadget *btname =   gdata->subs[THS_NameButton];
          if(btname && data!=0)
          {
            char tname[32];
            char *name= (char *)data;
            if(strlen(name)>7)
            {
                snprintf(tname,7,"%s",name);
                strcat(tname,"..");
                name = &tname[0];
            }

            SetAttrs(btname,GA_Text,(ULONG)name,TAG_END);
            HeaderButton_Notify(OCLASS(btname),btname,Set->ops_GInfo);
          }
          actuallydone = 1;
       }
       break;

    default:
        break;

    } // end switch
  } // end for

  return(actuallydone);
}

