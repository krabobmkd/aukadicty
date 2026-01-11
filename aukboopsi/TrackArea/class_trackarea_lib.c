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

#ifdef USE_BEVEL_FRAME
    #include <proto/bevel.h>
#endif

typedef ULONG (*REHOOKFUNC)();

#ifdef TRACKAREA_STATICLINK
#include <stdlib.h>
#include <string.h>
#endif

#ifndef TRACKAREA_STATICLINK
struct ExecBase       *SysBase=NULL;
struct GfxBase        *GfxBase=NULL;
struct IntuitionBase  *IntuitionBase=NULL;
struct Library        *LayersBase=NULL;
struct DosLibrary     *DOSBase=NULL;
struct Library        *UtilityBase=NULL;
    #if defined(__GNUC__) && (__GNUC__ < 3)
        struct Library        *__UtilityBase=NULL; // amiga gcc2.95 with noixemul and 68000, and our gadget startup needs that.
    #endif
#endif

#ifdef USE_BEVEL_FRAME
struct Library        *BevelBase=NULL;
#endif
/* this is the only global writable we should see in the whole class binary ! */
struct IClass   *TrackAreaClassPtr=NULL;
/* this 2 strings are also linked to the asm startup header ( in .gadget mode) */
/* note: (const char *str="") would make str be a (char **) to the linker. so char str[] is linkable to asm startup */
#ifndef TRACKAREA_STATICLINK
const char Class_ID[]= TrackArea_CLASS_ID;
const char *VersionString = "track.gadget 1.0 "; /* add date */
#endif
/* TrackArea uses InfiniteScroll as superclass (class pointer, not string) */
/* const char TrackAreaSuperClassID[]=TrackArea_SUPERCLASS_ID; -- not used, we use class pointer */




// note: if other boopsi classes are dependences, they need to be opened here.
#ifndef TRACKAREA_STATICLINK
    BOOL TrackArea_OpenLibs(void)
    {
      // if here, sysbase is already acquired from LibInit.
      //NO: if(!SysBase) SysBase = *(( struct ExecBase **)4);
       if(!DOSBase)  DOSBase = (struct DosLibrary *)OpenLibrary("dos.library",1);
       if(!IntuitionBase)  IntuitionBase = (struct IntuitionBase *) OpenLibrary("intuition.library",39);
       if(!GfxBase) GfxBase = (struct GfxBase *) OpenLibrary("graphics.library",39);
       if(!UtilityBase) UtilityBase = OpenLibrary("utility.library",39);
       if(!LayersBase) LayersBase = OpenLibrary("layers.library",39);
    #if defined(__GNUC__) && (__GNUC__ < 3)
        __UtilityBase = UtilityBase; // amiga gcc2.95 with noixemul and 68000, and our gadget startup needs that.
    #endif
        return TRUE;
    }

    void TrackArea_CloseLibs(void)
    {
        if(LayersBase) CloseLibrary(LayersBase);
        if(DOSBase) CloseLibrary((struct Library *)DOSBase);
        if(UtilityBase) CloseLibrary(UtilityBase);
        if(GfxBase) CloseLibrary((struct Library *)GfxBase);
        if(IntuitionBase) CloseLibrary((struct Library *)IntuitionBase);
    }

#endif
BOOL TrackArea_OpenLibs_Dependencies(void)
{
#ifdef USE_BEVEL_FRAME
    if(!BevelBase) BevelBase = OpenLibrary("images/bevel.image",44);
    if(!BevelBase) return FALSE;
#endif
    return TRUE;
}

void TrackArea_CloseLibs_Dependencies(void)
{
#ifdef USE_BEVEL_FRAME
    if(BevelBase) CloseLibrary(BevelBase);
    BevelBase = NULL;
#endif
}
//==========================================================================================
// does not need to be exact, we just want the function pointer:
ULONG ASM SAVEDS TrackArea_Dispatcher(
                    REG(a0,struct IClass *C),
                    REG(a2,struct Gadget *Gad),
                    REG(a1,union MsgUnion *M));

#ifndef TRACKAREA_STATICLINK
// called by shared Lib init to create class.

int ASM CreateClass(REG(a6,struct ExtClassLib *LibBase))
{
  if(LibBase) SysBase = LibBase->cb_SysBase;
  if(TrackArea_OpenLibs() && TrackArea_OpenLibs_Dependencies())
  {
    if(TrackAreaClassPtr=MakeClass(TrackArea_CLASS_ID,TrackAreaSuperClassID,0,sizeof(TrackArea),0))
    {
     if(LibBase) LibBase->cb_ClassLibrary.cl_Class = TrackAreaClassPtr;
      TrackAreaClassPtr->cl_Dispatcher.h_Data=LibBase;
      TrackAreaClassPtr->cl_Dispatcher.h_Entry=(REHOOKFUNC)TrackArea_Dispatcher;

      AddClass(TrackAreaClassPtr);
      /* Success */
      return(0);
    }
    TrackArea_CloseLibs_Dependencies();
    TrackArea_CloseLibs();
  }
  /* Fail */
  return(-1);
}
// called by shared Lib expunge to dispose class.
void ASM DestroyClass(REG(a6,struct ExtClassLib *LibBase))
{
    // note LibBase and TrackAreaClassPtr should be the same
    if(TrackAreaClassPtr)
    {
      RemoveClass(TrackAreaClassPtr);
      FreeClass(TrackAreaClassPtr);
      TrackAreaClassPtr = NULL;
    }
  TrackArea_CloseLibs_Dependencies();
  TrackArea_CloseLibs();
}
// first public lib function for boopsi classes
Class * ASM GetClass(void)
{
    return TrackAreaClassPtr;
}
// end if shared class
#else
// static version:
Class *TRACKAREA_GetClass()
{
    return TrackAreaClassPtr;
}
#endif

//====================================================================================

#ifdef TRACKAREA_STATICLINK

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
    if(TrackAreaClassPtr = MakeClass(NULL, NULL, superClass, sizeof(TrackArea), 0))
    {
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
      FreeClass(TrackAreaClassPtr);
      TrackAreaClassPtr = NULL;
    }

}

#endif


