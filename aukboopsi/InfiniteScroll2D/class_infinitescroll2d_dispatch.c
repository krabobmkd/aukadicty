

#include <clib/alib_protos.h>

#include <proto/dos.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/gadgetclass.h>

#include "class_infinitescroll2d.h"
#include "class_infinitescroll2d_private.h"

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

/** for dispatcher, very wise use of union.
 *  each  struct also starts with MethodID.
 * and they are the very parameters for each methods.
 */
typedef union MsgUnion2D
{
  ULONG  MethodID;
  /* from classusr.h or gadgetclass.h, all starts with MethodID. */
  struct opSet        opSet;
  struct opUpdate     opUpdate;
  struct opGet        opGet;
  struct gpHitTest    gpHitTest;
  struct gpRender     gpRender;
  struct gpInput      gpInput;
  struct gpGoInactive gpGoInactive;
  struct gpLayout     gpLayout;
  struct gpDomain     gpDomain;
} *Msgs2D;

/** WATCH OUT ! BOOPSI docs says:
*  "the model class dispatcher must be able to run on Intuition's context,
*  which puts some limitations on what the dispatcher is permitted to do:
*  it can't use dos.library, it can't wait on application signals or message ports
* and it can't call any Intuition functions which might wait on Intuition."
*/
ULONG ASM SAVEDS InfiniteScroll2D_Dispatcher(
                    REG(a0,struct IClass *C),
                    REG(a2,struct Gadget *Gad),
                    REG(a1,union MsgUnion2D *M))
{
  InfiniteScroll2D *gdata;
  ULONG retval=0;
  gdata=INST_DATA(C, Gad);

  switch(M->MethodID)
  {
    case OM_NEW:
      if((Gad=(struct Gadget *)DoSuperMethodA(C,(Object *)Gad,(Msg)M))!=NULL)
      {
        gdata=INST_DATA(C, Gad);

        /* Initialize 2D tile grid */
        gdata->_tiles = NULL;
        gdata->_tilesX = 0;
        gdata->_tilesY = 0;
        gdata->_tileSize = TILE2D_SIZE;  /* fixed 128x128 */

        gdata->_position._scrollx = 0;
        gdata->_position._scrolly = 0;
        gdata->_pposition = &gdata->_position;

        gdata->_torusOffsetX = 0;
        gdata->_torusOffsetY = 0;
        gdata->_renderedTilesX = 0;
        gdata->_renderedTilesY = 0;

        gdata->_layoutedForWidth = 0;
        gdata->_layoutedForHeight = 0;

        gdata->_renderFunction = NULL;
        gdata->_friendBitmap = NULL;

        /* Initialize margins */
        gdata->_marginLeft = 0;
        gdata->_marginRight = 0;
        gdata->_marginTop = 0;
        gdata->_marginBottom = 0;
        gdata->_marginPen = 1;  /* default pen */

        /* Process initial attributes */
        InfiniteScroll2D_SetAttrs(C,Gad,(struct opSet *)M);

        bdbprintf_new("InfiniteScroll2D", Gad);

        /* means new object OK so far: */
        retval=(ULONG)Gad;
      }
      break;

    case OM_UPDATE:
    case OM_SET:
      retval=DoSuperMethodA(C,(Object *)Gad,(Msg)M);
      InfiniteScroll2D_SetAttrs(C,Gad,(struct opSet *)M);
     break;

    case OM_GET:
      InfiniteScroll2D_GetAttr(C,Gad,(struct opGet *)M);
     break;

    case OM_DISPOSE:
        bdbprintf_dispose("InfiniteScroll2D", Gad);

        /* Dispose all tiles */
        InfiniteScroll2D_DisposeTiles(gdata);

      retval=DoSuperMethodA(C,(Object *)Gad,(Msg)M);
      break;

    case GM_HITTEST:
      retval = 0; /* no interaction by default */
      break;
    case GM_GOACTIVE:
      return 0;
      break;
    case GM_HANDLEINPUT:
     return 0;
      break;
    case GM_GOINACTIVE:
     return 0;
      break;

    case GM_LAYOUT:
      retval= InfiniteScroll2D_Layout(C,Gad,(struct gpLayout *)M);
      break;

    case GM_RENDER:
      retval=InfiniteScroll2D_Render(C,Gad,(struct gpRender *)M);
      break;

    case GM_DOMAIN:
      InfiniteScroll2D_Domain(C, Gad, (APTR)M);
      retval=1;
    break;

    default:
      retval=DoSuperMethodA(C,(Object *)Gad,(Msg)M);
      break;
  }
  return(retval);
}

