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

#include "class_trackheaderlist_private.h"

#ifdef USE_BEVEL_FRAME
    #include <proto/bevel.h>
#endif

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


typedef ULONG (*REHOOKFUNC)();

#ifdef TRACKHEADERLIST_STATICLINK
#include <stdlib.h>
#include <string.h>
#endif

#ifndef TRACKHEADERLIST_STATICLINK
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
// this is the only global writtable we should see in the whole class binary !
struct IClass   *TrackHeaderListClassPtr=NULL;
// this 2 strings are also linked to the asm startup header ( in .gadget mode)
// note: (const char *str="") would make str be a (char **) to the linker. so char str[] is linkable to asm startup
#ifndef TRACKHEADERLIST_STATICLINK
const char Class_ID[]= TrackHeaderList_CLASS_ID;
const char *VersionString = "trackheaderlist.gadget 1.0 "; // add date
#endif
const char TrackHeaderListSuperClassID[]=TrackHeaderList_SUPERCLASS_ID;




// note: if other boopsi classes are dependences, they need to be opened here.
#ifndef TRACKHEADERLIST_STATICLINK
    BOOL TrackHeaderList_OpenLibs(void)
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

    void TrackHeaderList_CloseLibs(void)
    {
        if(LayersBase) CloseLibrary(LayersBase);
        if(DOSBase) CloseLibrary((struct Library *)DOSBase);
        if(UtilityBase) CloseLibrary(UtilityBase);
        if(GfxBase) CloseLibrary((struct Library *)GfxBase);
        if(IntuitionBase) CloseLibrary((struct Library *)IntuitionBase);
    }

#endif
BOOL TrackHeaderList_OpenLibs_Dependencies(void)
{
#ifdef USE_BEVEL_FRAME
    if(!BevelBase) BevelBase = OpenLibrary("images/bevel.image",44);
    if(!BevelBase) return FALSE;
#endif
    return TRUE;
}

void TrackHeaderList_CloseLibs_Dependencies(void)
{
#ifdef USE_BEVEL_FRAME
    if(BevelBase) CloseLibrary(BevelBase);
    BevelBase = NULL;
#endif
}
//==========================================================================================
// does not need to be exact, we just want the function pointer:
ULONG ASM SAVEDS TrackHeaderList_Dispatcher(
                    REG(a0,struct IClass *C),
                    REG(a2,struct Gadget *Gad),
                    REG(a1,union MsgUnion *M));

#ifndef TRACKHEADERLIST_STATICLINK
// called by shared Lib init to create class.

int ASM CreateClass(REG(a6,struct ExtClassLib *LibBase))
{
  if(LibBase) SysBase = LibBase->cb_SysBase;
  if(TrackHeaderList_OpenLibs() && TrackHeaderList_OpenLibs_Dependencies())
  {
    if(TrackHeaderListClassPtr=MakeClass(TrackHeaderList_CLASS_ID,TrackHeaderListSuperClassID,0,sizeof(TrackHeaderList),0))
    {
     if(LibBase) LibBase->cb_ClassLibrary.cl_Class = TrackHeaderListClassPtr;
      TrackHeaderListClassPtr->cl_Dispatcher.h_Data=LibBase;
      TrackHeaderListClassPtr->cl_Dispatcher.h_Entry=(REHOOKFUNC)TrackHeaderList_Dispatcher;

      AddClass(TrackHeaderListClassPtr);
      /* Success */
      return(0);
    }
    TrackHeaderList_CloseLibs_Dependencies();
    TrackHeaderList_CloseLibs();
  }
  /* Fail */
  return(-1);
}
// called by shared Lib expunge to dispose class.
void ASM DestroyClass(REG(a6,struct ExtClassLib *LibBase))
{
    // note LibBase and TrackHeaderListClassPtr should be the same
    if(TrackHeaderListClassPtr)
    {
      RemoveClass(TrackHeaderListClassPtr);
      FreeClass(TrackHeaderListClassPtr);
      TrackHeaderListClassPtr = NULL;
    }
  TrackHeaderList_CloseLibs_Dependencies();
  TrackHeaderList_CloseLibs();
}
// first public lib function for boopsi classes
Class * ASM GetClass(void)
{
    return (Class *)TrackHeaderListClassPtr;
}
// end if shared class
#else
// static version:
struct IClass   *TRACKHEADERLIST_GetClass()
{
    return TrackHeaderListClassPtr;
}
#endif

//====================================================================================

#ifdef TRACKHEADERLIST_STATICLINK

// just use this one once when static link
int TrackHeaderListStaticInit()
{ 
   if(!TrackHeaderList_OpenLibs_Dependencies()) return 1;
    if(TrackHeaderListClassPtr=MakeClass(NULL,TrackHeaderListSuperClassID,0,sizeof(TrackHeaderList),0))
    {
      TrackHeaderListClassPtr->cl_Dispatcher.h_Entry=(REHOOKFUNC)TrackHeaderList_Dispatcher;
     // do not AddClass() when static, no need to publish, TrackHeaderListClassPtr will be enough.
      /* Success */
      return(0);
    }
    return 1;
}

void TrackHeaderListStaticClose()
{
    TrackHeaderList_CloseLibs_Dependencies();
    if(TrackHeaderListClassPtr)
    {
      FreeClass(TrackHeaderListClassPtr);
      TrackHeaderListClassPtr = NULL;
    }

}

#endif


