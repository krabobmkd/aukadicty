
#ifdef __SASC
    #include <clib/alib_protos.h>
#else
    /* GCC, vbcc */
    #include "minialib.h"
#endif
#include <proto/dos.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/gadgetclass.h>

#include "class_timerule.h"
#include "class_timerule_private.h"

/* Include InfiniteScroll private for access to superclass data */
//#include "../InfiniteScroll/class_infinitescroll_private.h"
#include "../InfiniteScroll/class_infinitescroll.h"
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


/** WATCH OUT ! BOOPSI docs says:
 *  "the model class dispatcher must be able to run on Intuition's context,
 *  which puts some limitations on what the dispatcher is permitted to do:
 *  it can't use dos.library, it can't wait on application signals or message ports
 *  and it can't call any Intuition functions which might wait on Intuition."
 *
 * TimeRule inherits from InfiniteScroll.
 * InfiniteScroll handles: tiles, _framerec, _clipRegion, GM_LAYOUT, GM_RENDER
 * TimeRule adds: time border attributes, custom tile rendering for graduations
 */

ULONG ASM SAVEDS TimeRule_Dispatcher(
                    REG(a0,struct IClass *C),
                    REG(a2,struct Gadget *Gad),
                    REG(a1,union MsgUnion *M))
{
  TimeRule *gdata;
  ULONG retval=0;
  gdata=INST_DATA(C, Gad);

  switch(M->MethodID)
  {
    case OM_NEW:
      /* Let InfiniteScroll handle creation first (it sets up tiles, etc) */
      if(Gad=(struct Gadget *)DoSuperMethodA(C,(Object *)Gad,(Msg)M))
      {
            struct opSet superops;
            ULONG supertags[] = {
                INFINITESCROLL_RenderFunction,NULL,
                TAG_END
            };
            superops.MethodID     = OM_SET;
            superops.ops_AttrList = ( struct TagItem *)&supertags[0];
            superops.ops_GInfo    = M->opSet.ops_GInfo;
            supertags[1] = (ULONG)&TimeRule_RenderDelegate;

            DoSuperMethodA(C, Gad, (Msg)&superops);
           // SHOULD WORK BUT DO NOT, INLINE COMPILER ISSUE.
           //SetSuperAttrs(C,Gad, INFINITESCROLL_RenderFunction, renderFunction,TAG_DONE);
            bdbprintf_new("TimeRule", Gad);

        gdata=INST_DATA(C, Gad);

        gdata->_timePerPixelWidth = 0; // 0 means not inited, important.
        gdata->majorTickInterval = 0;
        gdata->minorTickInterval = 0;
        gdata->_style = NULL;
        gdata->_justScroll = gdata->_fullRedraw = 0;

        /* Process TimeRule-specific attributes from creation tags */
        if(M->opSet.ops_AttrList)
        {
            TimeRule_SetAttrs(C,Gad,&M->opSet);
        }


        /* means new object OK so far: */
        retval=(ULONG)Gad;
      }
      break;

    case OM_UPDATE:
    case OM_SET:
      /* Let InfiniteScroll handle its attribs first */
      retval=DoSuperMethodA(C,(Object *)Gad,(Msg)M);
      /* Then handle TimeRule-specific attribs */
      retval |= TimeRule_SetAttrs(C,Gad,(struct opSet *)M);
      break;

    case OM_GET:
      /* Try TimeRule attribs first, then delegate to InfiniteScroll */
      retval = TimeRule_GetAttr(C,Gad,(struct opGet *)M);
      break;

    case OM_DISPOSE:
      bdbprintf_dispose("TimeRule", Gad);
      /* Let InfiniteScroll clean up tiles, clipRegion, etc */
      retval=DoSuperMethodA(C,(Object *)Gad,(Msg)M);
      break;

    case GM_DOMAIN:
      /* Override domain for TimeRule height */
      TimeRule_Domain(C, Gad, (APTR)M);
      retval=1;
      break;
   case GM_LAYOUT:
      retval = TimeRule_Layout(C,(Object *)Gad,(struct gpLayout *)M);
        break;

    /* Let InfiniteScroll handle these: GM_LAYOUT, GM_RENDER, GM_HITTEST, etc */
    default:
      retval=DoSuperMethodA(C,(Object *)Gad,(Msg)M);
      break;
  }
  return(retval);
}

