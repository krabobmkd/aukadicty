

#include <clib/alib_protos.h>
#include <proto/dos.h>
//#include <proto/utility.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/gadgetclass.h>

#include "class_tracklistarea.h"
#include "class_tracklistarea_private.h"

#ifdef USE_BEVEL_FRAME
    #include <proto/bevel.h>
    #include <images/bevel.h>
#endif

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/utility.h>

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

/** WATCH OUT ! boopsi docs says:
*  "the rkmmodelclass dispatcher must be able to run on Intuition's context,
*  which puts some limitations on what the dispatcher is permitted to do:
*  it can't use dos.library, it can't wait on application signals or message ports
* and it can't call any Intuition functions which might wait on Intuition."
*/
ULONG ASM SAVEDS TrackListArea_Dispatcher(
                    REG(a0,struct IClass *C),
                    REG(a2,struct Gadget *Gad),
                    REG(a1,union MsgUnion *M))
{
  TrackListArea *gdata;
  ULONG retval=0;


  switch(M->MethodID)
  {
    case OM_NEW:
      if((Gad=(struct Gadget *)DoSuperMethodA(C,(Object *)Gad,(Msg)M))!=NULL)
      {
        gdata=INST_DATA(C, Gad);

        gdata->_minimalWidth = 64;
        gdata->_minimalHeight = 64;

        /* Initialize gadget arrays */
        gdata->_tracks = NULL;
        gdata->_trackCount = 0;
        gdata->_headerWidth = 128;  /* default header width */
        gdata->_defaulTrackHeight = 40;   /* default track height */
        gdata->_scrollY = 0;
        gdata->_domainHeight = 0;
        gdata->_drawInfo = NULL;

        /* Initialize time projection:
         * _pixAtLeft: Time at left border, can be negative
         * _timePerPixelWidth: Amount of time per pixel in AukFixed 32.32 format.
         *   Lower value = more zoomed in (more detail).
         *   Default: 10 seconds / 640 pixels = 0.015625 seconds/pixel
         */
        gdata->_timeProjection._pixAtLeft = 0;
        gdata->_timeProjection._timePerPixelWidth = (10LL<<32)/640; /* 10 seconds for 640 pixel width.*/

        /* set gadget (super class) attributes for this instance like this: */
        /* (BOOL) Indicate whether gadget is part of TAB/SHIFT-TAB cycle. */
        /* default to false */
        //SetSuperAttrs(C,(Object *)Gad, GA_TabCycle,TRUE,TAG_DONE);

        gdata->_clipRegion = NewRegion();

        bdbprintf_new("TrackListArea", Gad);
        TrackListArea_SetAttrs(C,Gad,(struct opSet *)M);

        /* means new object OK so far: */
        retval=(ULONG)Gad;
      }
      break;

    case OM_UPDATE:
    case OM_SET:

       // Printf("OM_SET: GadgetID:%ld gad:%lx\n",(int)Gad->GadgetID,(int)Gad);
      retval=DoSuperMethodA(C,(Object *)Gad,(Msg)M);
      retval =TrackListArea_SetAttrs(C,Gad,(struct opSet *)M);
     break;

    case OM_GET:
      retval = TrackListArea_GetAttr(C,Gad,(struct opGet *)M);
     break;

    case OM_DISPOSE:
      gdata=INST_DATA(C, Gad);
        bdbprintf_dispose("TrackListArea", Gad);

        if(gdata->_clipRegion) DisposeRegion(gdata->_clipRegion);
        /* Dispose all track gadgets */
        TrackListArea_DisposeGadgets(gdata);

        AukObjectPtr_Release( &gdata->_project);

      retval=DoSuperMethodA(C,(Object *)Gad,(Msg)M);
      break;

    /* return GMR_GADGETHIT if you are clicked on (whether or not you
     * are disabled). */
//    case GM_HITTEST:
//      retval =TrackListArea_HandleHitTest(C,Gad,(struct gpHitTest *)M);
//      break;
//    /* you are now going to be fed input */
//    case GM_GOACTIVE:
//        retval=TrackListArea_HandleInput(C,Gad,(struct gpInput *)M, M->gpInput.gpi_Mouse.X,M->gpInput.gpi_Mouse.Y);
//      break;

//    case GM_GOINACTIVE:
//        TrackListArea_GoInactive(C,Gad,(struct gpRender *)M);
//      break;
//    case GM_HANDLEINPUT:
//      gdata=INST_DATA(C, Gad);
//      retval=TrackListArea_HandleInput(C,Gad,(struct gpInput *)M,M->gpInput.gpi_Mouse.X,M->gpInput.gpi_Mouse.Y);
//      break;


    case GM_LAYOUT:
      retval= TrackListArea_Layout(C,Gad,(struct gpLayout *)M,3);
      break;

    case GM_RENDER:
      retval=TrackListArea_Render(C,Gad,(struct gpRender *)M,3);
      break;

    case GM_DOMAIN:
      TrackListArea_Domain(C, Gad, (APTR)M);
      retval=1;

    break;

    default:
  //  Printf(" - default unmanaged method - %lx\n",);
      retval=DoSuperMethodA(C,(Object *)Gad,(Msg)M);
      break;
  }
  return(retval);
}

