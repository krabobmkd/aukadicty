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

#include "class_infinitescroll.h"
#include "class_infinitescroll_private.h"

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


typedef ULONG (*REHOOKFUNC)();

#ifdef INFINITESCROLL_STATICLINK
#include <stdlib.h>
#include <string.h>
#endif

#ifndef INFINITESCROLL_STATICLINK
struct ExecBase       *SysBase=NULL;
struct GfxBase        *GfxBase=NULL;
struct IntuitionBase  *IntuitionBase=NULL;
struct Library        *LayersBase=NULL;
struct DosLibrary     *DOSBase=NULL;
struct Library        *UtilityBase=NULL;
    #if defined(__GNUC__) && (__GNUC__ < 3)
        struct Library        *__UtilityBase=NULL; /* amiga gcc2.95 with noixemul and 68000, and our gadget startup needs that. */
    #endif
#endif

/* this is the only global writable we should see in the whole class binary ! */
struct IClass   *InfiniteScrollClassPtr=NULL;
/* this 2 strings are also linked to the asm startup header ( in .gadget mode) */
/* note: (const char *str="") would make str be a (char **) to the linker. so char str[] is linkable to asm startup */
#ifndef INFINITESCROLL_STATICLINK
const char Class_ID[]= InfiniteScroll_CLASS_ID;
const char *VersionString = "infinitescroll.gadget 1.0 "; /* add date */
#endif
const char InfiniteScrollSuperClassID[]=InfiniteScroll_SUPERCLASS_ID;




/* note: if other BOOPSI classes are dependences, they need to be opened here. */
#ifndef INFINITESCROLL_STATICLINK
    BOOL InfiniteScroll_OpenLibs(void)
    {
      /* if here, sysbase is already acquired from LibInit. */
      /*NO: if(!SysBase) SysBase = *(( struct ExecBase **)4); */
       if(!DOSBase)  DOSBase = (struct DosLibrary *)OpenLibrary("dos.library",1);
       if(!IntuitionBase)  IntuitionBase = (struct IntuitionBase *) OpenLibrary("intuition.library",39);
       if(!GfxBase) GfxBase = (struct GfxBase *) OpenLibrary("graphics.library",39);
       if(!UtilityBase) UtilityBase = OpenLibrary("utility.library",39);
       if(!LayersBase) LayersBase = OpenLibrary("layers.library",39);
    #if defined(__GNUC__) && (__GNUC__ < 3)
        __UtilityBase = UtilityBase; /* amiga gcc2.95 with noixemul and 68000, and our gadget startup needs that. */
    #endif
        return TRUE;
    }

    void InfiniteScroll_CloseLibs(void)
    {
        if(LayersBase) CloseLibrary(LayersBase);
        if(DOSBase) CloseLibrary((struct Library *)DOSBase);
        if(UtilityBase) CloseLibrary(UtilityBase);
        if(GfxBase) CloseLibrary((struct Library *)GfxBase);
        if(IntuitionBase) CloseLibrary((struct Library *)IntuitionBase);
    }

#endif

BOOL InfiniteScroll_OpenLibs_Dependencies(void)
{
    /* No additional dependencies for now */
    return TRUE;
}

void InfiniteScroll_CloseLibs_Dependencies(void)
{
    /* No additional dependencies to close */
}

/*==========================================================================================*/
/* does not need to be exact, we just want the function pointer: */
ULONG ASM SAVEDS InfiniteScroll_Dispatcher(
                    REG(a0,struct IClass *C),
                    REG(a2,struct Gadget *Gad),
                    REG(a1,union MsgUnion *M));

#ifndef INFINITESCROLL_STATICLINK
/* called by shared Lib init to create class. */

int ASM CreateClass(REG(a6,struct ExtClassLib *LibBase))
{
  if(LibBase) SysBase = LibBase->cb_SysBase;
  if(InfiniteScroll_OpenLibs() && InfiniteScroll_OpenLibs_Dependencies())
  {
    if(InfiniteScrollClassPtr=MakeClass(InfiniteScroll_CLASS_ID,InfiniteScrollSuperClassID,0,sizeof(InfiniteScroll),0))
    {
     if(LibBase) LibBase->cb_ClassLibrary.cl_Class = InfiniteScrollClassPtr;
      InfiniteScrollClassPtr->cl_Dispatcher.h_Data=LibBase;
      InfiniteScrollClassPtr->cl_Dispatcher.h_Entry=(REHOOKFUNC)InfiniteScroll_Dispatcher;

      AddClass(InfiniteScrollClassPtr);
      /* Success */
      return(0);
    }
    InfiniteScroll_CloseLibs_Dependencies();
    InfiniteScroll_CloseLibs();
  }
  /* Fail */
  return(-1);
}
/* called by shared Lib expunge to dispose class. */
void ASM DestroyClass(REG(a6,struct ExtClassLib *LibBase))
{
    /* note LibBase and InfiniteScrollClassPtr should be the same */
    if(InfiniteScrollClassPtr)
    {
      RemoveClass(InfiniteScrollClassPtr);
      FreeClass(InfiniteScrollClassPtr);
      InfiniteScrollClassPtr = NULL;
    }
  InfiniteScroll_CloseLibs_Dependencies();
  InfiniteScroll_CloseLibs();
}
/* first public lib function for BOOPSI classes */
Class * ASM GetClass(void)
{
    return (Class *)InfiniteScrollClassPtr;
}
/* end if shared class */
#else
/* static version: */
struct IClass   *INFINITESCROLL_GetClass()
{
    return InfiniteScrollClassPtr;
}
#endif

/*====================================================================================*/

#ifdef INFINITESCROLL_STATICLINK

/* just use this one once when static link */
int InfiniteScrollStaticInit()
{
   if(!InfiniteScroll_OpenLibs_Dependencies()) return 0;
    if(InfiniteScrollClassPtr=MakeClass(NULL,InfiniteScrollSuperClassID,0,sizeof(InfiniteScroll),0))
    {
      InfiniteScrollClassPtr->cl_Dispatcher.h_Entry=(REHOOKFUNC)InfiniteScroll_Dispatcher;
     /* do not AddClass() when static, no need to publish, InfiniteScrollClassPtr will be enough. */
      /* Success */
      return(1);
    }
    return 0;
}

void InfiniteScrollStaticClose()
{
    InfiniteScroll_CloseLibs_Dependencies();
    if(InfiniteScrollClassPtr)
    {
      FreeClass(InfiniteScrollClassPtr);
      InfiniteScrollClassPtr = NULL;
    }

}

#endif


