

#include <clib/alib_protos.h>
#include <proto/dos.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/gadgetclass.h>

#include "class_trackarea.h"
#include "class_trackarea_private.h"

/* Include InfiniteScroll for access to superclass */
#include "../InfiniteScroll/class_infinitescroll.h"
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/utility.h>

#ifdef USE_BEVEL_FRAME
    #include <proto/bevel.h>
    #include <images/bevel.h>
#endif

/* Most of the calls to boopsi methods are not done from the App's context,
 * but from a specific intuition context, and because of that we can't use DOS calls
 * like dos/Printf() , and also stdlib printf().
 * So we may print debug informations with a special buffer,and function bdbprintf(),
 * then flushbdbprint() in main process will print for real to standard output.
 * remove word USE_DEBUG_BDBPRINT to desactivate all bdbprintf()/flushbdbprint() calls.
 * Template projects that links boopsi classes statically use USE_DEBUG_BDBPRINT by default.
 * Template projects that uses boopsi classes with LoadLibrary() do not.
 */
#include "bdbprintf.h"

/* Message union for dispatcher */
typedef union MsgUnion
{
  ULONG  MethodID;
  struct opSet        opSet;
  struct opUpdate     opUpdate;
  struct opGet        opGet;
  struct gpHitTest    gpHitTest;
  struct gpRender     gpRender;
  struct gpInput      gpInput;
  struct gpGoInactive gpGoInactive;
  struct gpLayout     gpLayout;
  struct gpDomain     gpDomain;
} *Msgs;


/** WATCH OUT ! BOOPSI docs says:
 *  "the model class dispatcher must be able to run on Intuition's context,
 *  which puts some limitations on what the dispatcher is permitted to do:
 *  it can't use dos.library, it can't wait on application signals or message ports
 *  and it can't call any Intuition functions which might wait on Intuition."
 *
 * TrackArea inherits from InfiniteScroll.
 * InfiniteScroll handles: tiles, GM_LAYOUT, GM_RENDER
 * TrackArea adds: TimeProjection pointer, custom tile rendering for track content
 */
ULONG ASM SAVEDS TrackArea_Dispatcher(
                    REG(a0,struct IClass *C),
                    REG(a2,struct Gadget *Gad),
                    REG(a1,union MsgUnion *M))
{
  TrackArea *gdata;
  ULONG retval=0;
  gdata=INST_DATA(C, Gad);

  switch(M->MethodID)
  {
    case OM_NEW:
      /* Let InfiniteScroll handle creation first (it sets up tiles, etc) */
      if((Gad=(struct Gadget *)DoSuperMethodA(C,(Object *)Gad,(Msg)M))!=NULL)
      {
            struct opSet superops;
            ULONG supertags[] = {
                INFINITESCROLL_RenderFunction,NULL,
                TAG_END
            };
            superops.MethodID     = OM_SET;
            superops.ops_AttrList = ( struct TagItem *)&supertags[0];
            superops.ops_GInfo    = M->opSet.ops_GInfo;
            supertags[1] = (ULONG)&TrackArea_RenderDelegate;

            DoSuperMethodA(C,(Object*) Gad, (Msg)&superops);

            bdbprintf_new("TrackArea", Gad);

        gdata=INST_DATA(C, Gad);

        gdata->_pTimeProjection = NULL;
        gdata->_style = NULL;
        gdata->_dataTrack = NULL;
        gdata->_justScroll = gdata->_fullRedraw = 0;

#ifdef USE_BEVEL_FRAME
          gdata->Bevel= NewObject(BEVEL_GetClass(),NULL,
            BEVEL_Style, BVS_BUTTON,
            BEVEL_FillPen, -1,
            TAG_END);
#endif

        /* Process TrackArea-specific attributes from creation tags */
        if(M->opSet.ops_AttrList)
        {
            TrackArea_SetAttrs(C,Gad,&M->opSet);
        }
        /* means new object OK so far: */
        retval=(ULONG)Gad;
      }
      break;

    case OM_UPDATE:
    case OM_SET:
      /* Let InfiniteScroll handle its attribs first */
      retval=DoSuperMethodA(C,(Object *)Gad,(Msg)M);
      /* Then handle TrackArea-specific attribs */
      retval |= TrackArea_SetAttrs(C,Gad,(struct opSet *)M);
      break;

    case OM_GET:
      /* Try TrackArea attribs first, then delegate to InfiniteScroll */
      retval = TrackArea_GetAttr(C,Gad,(struct opGet *)M);
      break;

    case OM_DISPOSE:
      bdbprintf_dispose("TrackArea", Gad);
      /* Release the data track reference */
      AukObjectPtr_Release((AukObjectPtr*)&gdata->_dataTrack);
    #ifdef USE_BEVEL_FRAME
        if(gdata->Bevel) DisposeObject(gdata->Bevel);
    #endif
      /* Let InfiniteScroll clean up tiles, etc */
      retval=DoSuperMethodA(C,(Object *)Gad,(Msg)M);
      break;

    case GM_DOMAIN:
      /* Override domain for TrackArea height */
      TrackArea_Domain(C, Gad, (APTR)M);
      retval=1;
      break;


// - - -  - -
   case GM_HITTEST:
     retval = GMR_GADGETHIT;
     break;
   /* you are now going to be fed input */
   case GM_GOACTIVE:
      retval=TrackArea_HandleInput(C,Gad,(struct gpInput *)M,TRUE);
      break;
   case GM_HANDLEINPUT:
      retval=TrackArea_HandleInput(C,Gad,(struct gpInput *)M,FALSE);
     break;
   case GM_GOINACTIVE:
      retval = TrackArea_GoInactive(C,Gad,(struct gpGoInactive *)M);
     break;

    /* Let InfiniteScroll handle these: GM_RENDER, etc */
    default:
      retval=DoSuperMethodA(C,(Object *)Gad,(Msg)M);
      break;
  }
  return(retval);
}

