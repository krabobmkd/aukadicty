
#ifdef __SASC
    #include <clib/alib_protos.h>
#else
    /* GCC, vbcc */
    #include "../minialib.h"
#endif
#include <proto/dos.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/gadgetclass.h>

#include "class_infinitescroll.h"
#include "class_infinitescroll_private.h"

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/utility.h>

/* Most of the calls to BOOPSI methods are not done from the App's context,
 * but from a specific intuition context, and because of that we can't use DOS calls
 * like dos/Printf() , and also stdlib printf().
 * So we may print debug informations with a special buffer, and function bdbprintf(),
 * then flushbdbprint() in main process will print for real to standard output.
 * remove word USE_DEBUG_BDBPRINT to desactivate all bdbprintf()/flushbdbprint() calls.
 * Template projects that links BOOPSI classes statically use USE_DEBUG_BDBPRINT by default.
 * Template projects that uses BOOPSI classes with LoadLibrary() do not.
 */
#include "../bdbprintf.h"


/** WATCH OUT ! BOOPSI docs says:
*  "the model class dispatcher must be able to run on Intuition's context,
*  which puts some limitations on what the dispatcher is permitted to do:
*  it can't use dos.library, it can't wait on application signals or message ports
* and it can't call any Intuition functions which might wait on Intuition."
*/
ULONG ASM SAVEDS InfiniteScroll_Dispatcher(
                    REG(a0,struct IClass *C),
                    REG(a2,struct Gadget *Gad),
                    REG(a1,union MsgUnion *M))
{
  InfiniteScroll *gdata;
  ULONG retval=0;
  gdata=INST_DATA(C, Gad);

  switch(M->MethodID)
  {
    case OM_NEW:
      if(Gad=(struct Gadget *)DoSuperMethodA(C,(Object *)Gad,(Msg)M))
      {
        gdata=INST_DATA(C, Gad);

        gdata->_minimalWidth = 128;
        gdata->_minimalHeight = 32;

        /* Initialize tile arrays */
        gdata->_tiles = NULL;
        gdata->_tileCount = 0;
        gdata->_tileWidth = 128;  /* default tile width */
        gdata->_tileHeight = 0;   /* will be set at layout */

        /* Initialize domain to 0..0 */
        gdata->_domainMinHi = 0;
        gdata->_domainMinLo = 0;
        gdata->_domainMaxHi = 0;
        gdata->_domainMaxLo = 0;

        /* Initialize view to 0 pos, zoom 1 */
        gdata->_viewPosHi = 0;
        gdata->_viewPosLo = 0;
        gdata->_viewZoomHi = 0;
        gdata->_viewZoomLo = 1;  /* zoom = 1 (no zoom) */

        gdata->_friendBitmap = NULL;

#ifdef USE_REGION_CLIPPING
        gdata->_clipRegion = NULL;
#endif

        /* set gadget (super class) attributes for this instance like this: */
        /* (BOOL) Indicate whether gadget is part of TAB/SHIFT-TAB cycle. */
        /* default to false */
        SetSuperAttrs(C,(Object *)Gad, GA_TabCycle,FALSE,TAG_DONE);

        /* Process initial attributes */
        InfiniteScroll_SetAttrs(C,Gad,(struct opSet *)M);

        bdbprintf_new("InfiniteScroll", Gad);

        /* means new object OK so far: */
        retval=(ULONG)Gad;
      }
      break;

    case OM_UPDATE:
    case OM_SET:
      retval=DoSuperMethodA(C,(Object *)Gad,(Msg)M);
      InfiniteScroll_SetAttrs(C,Gad,(struct opSet *)M);
     break;

    case OM_GET:
      InfiniteScroll_GetAttr(C,Gad,(struct opGet *)M);
     break;

    case OM_DISPOSE:
        bdbprintf_dispose("InfiniteScroll", Gad);

        /* Dispose all tiles */
        InfiniteScroll_DisposeTiles(gdata);

#ifdef USE_REGION_CLIPPING
        if(gdata->_clipRegion)
        {
            DisposeRegion(gdata->_clipRegion);
            gdata->_clipRegion = NULL;
        }
#endif

      retval=DoSuperMethodA(C,(Object *)Gad,(Msg)M);
      break;

    case GM_HITTEST:
      retval = GMR_GADGETHIT;
      break;

    case GM_GOACTIVE:
      Gad->Flags |= GFLG_SELECTED;
      retval=InfiniteScroll_HandleInput(C,Gad,(struct gpInput *)M);
      break;

    case GM_GOINACTIVE:
      Gad->Flags &= ~GFLG_SELECTED;
     // InfiniteScroll_Render(C,Gad,(APTR)M,GREDRAW_UPDATE);
      break;

    case GM_LAYOUT:
      retval= InfiniteScroll_Layout(C,Gad,(struct gpLayout *)M);
      break;

    case GM_RENDER:
      retval=InfiniteScroll_Render(C,Gad,(struct gpRender *)M,0);
      break;

    case GM_HANDLEINPUT:
      retval=InfiniteScroll_HandleInput(C,Gad,(struct gpInput *)M);
      break;

    case GM_DOMAIN:
      InfiniteScroll_Domain(C, Gad, (APTR)M);
      retval=1;
    break;

    case GM_INFINITESCROLL_INVALIDATETILES:
      InfiniteScroll_InvalidateTiles(C, Gad, (struct gpInvalidateTiles *)M);
      retval=1;
    break;

    default:
      retval=DoSuperMethodA(C,(Object *)Gad,(Msg)M);
      break;
  }
  return(retval);
}

