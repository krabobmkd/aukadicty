/**
 * This file contains lib inits that create the class
 * and would OpenLibrary() for other dependencies.
 * word XXX_STATICLINK decides if this is used as the header for a shared .class file
 * or if it is statically linked.
 */

#include <exec/alerts.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/layers.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>

#include "class_trackarea_private.h"

#include "bdbprintf.h"

#ifdef USE_BEVEL_FRAME
    #include <proto/bevel.h>
#endif

typedef ULONG (*REHOOKFUNC)();

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


#include <stdlib.h>
#include <string.h>

/* this is the only global writable we should see in the whole class binary ! */
struct IClass   *TrackAreaClassPtr=NULL;


// note: if other boopsi classes are dependences, they need to be opened here.

BOOL TrackArea_OpenLibs_Dependencies(void)
{
    return TRUE;
}

void TrackArea_CloseLibs_Dependencies(void)
{

}
//==========================================================================================
// does not need to be exact, we just want the function pointer:
ULONG ASM SAVEDS TrackArea_Dispatcher(
                    REG(a0,struct IClass *C),
                    REG(a2,struct Gadget *Gad),
                    REG(a1,union MsgUnion *M));


// static version:
Class *TRACKAREA_GetClass()
{
    return TrackAreaClassPtr;
}

//====================================================================================


/* just use this one once when static link */
/* TrackArea inherits from InfiniteScroll - must be initialized first */
int TrackAreaStaticInit()
{
    struct IClass *superClass;

    if(!TrackArea_OpenLibs_Dependencies()) return 0;

    /* Get InfiniteScroll class - it must be initialized before TrackArea */
    superClass = INFINITESCROLL_GetClass();
    if(!superClass) return 0;

    /* MakeClass with class pointer (not string) as superclass */
    if((TrackAreaClassPtr = MakeClass(NULL, NULL, superClass, sizeof(TrackArea), 0))!=NULL)
    {
        bdbprintf_makeclass("TrackArea", TrackAreaClassPtr);
        TrackAreaClassPtr->cl_Dispatcher.h_Entry = (REHOOKFUNC)TrackArea_Dispatcher;
        /* do not AddClass() when static, no need to publish, TrackAreaClassPtr will be enough. */
        /* Success */
        return(1);
    }
    return 0;
}

void TrackAreaStaticClose()
{
    TrackArea_CloseLibs_Dependencies();
    if(TrackAreaClassPtr)
    {
      bdbprintf_freeclass("TrackArea", TrackAreaClassPtr);
      FreeClass(TrackAreaClassPtr);
      TrackAreaClassPtr = NULL;
    }

}


