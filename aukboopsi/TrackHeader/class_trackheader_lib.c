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

#include "class_trackheader_private.h"

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

#ifdef TRACKHEADER_STATICLINK
#include <stdlib.h>
#include <string.h>
#endif

#ifndef TRACKHEADER_STATICLINK
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
struct IClass   *TrackHeaderClassPtr=NULL;
// this 2 strings are also linked to the asm startup header ( in .gadget mode)
// note: (const char *str="") would make str be a (char **) to the linker. so char str[] is linkable to asm startup
#ifndef TRACKHEADER_STATICLINK
const char Class_ID[]= TrackHeader_CLASS_ID;
const char *VersionString = "trackheader.gadget 1.0 "; // add date
#endif
const char TrackHeaderSuperClassID[]=TrackHeader_SUPERCLASS_ID;




// note: if other boopsi classes are dependences, they need to be opened here.
#ifndef TRACKHEADER_STATICLINK
    BOOL TrackHeader_OpenLibs(void)
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

    void TrackHeader_CloseLibs(void)
    {
        if(LayersBase) CloseLibrary(LayersBase);
        if(DOSBase) CloseLibrary((struct Library *)DOSBase);
        if(UtilityBase) CloseLibrary(UtilityBase);
        if(GfxBase) CloseLibrary((struct Library *)GfxBase);
        if(IntuitionBase) CloseLibrary((struct Library *)IntuitionBase);
    }

#endif
BOOL TrackHeader_OpenLibs_Dependencies(void)
{
#ifdef USE_BEVEL_FRAME
    if(!BevelBase) BevelBase = OpenLibrary("images/bevel.image",44);
    if(!BevelBase) return FALSE;
#endif
    return TRUE;
}

void TrackHeader_CloseLibs_Dependencies(void)
{
#ifdef USE_BEVEL_FRAME
    if(BevelBase) CloseLibrary(BevelBase);
    BevelBase = NULL;
#endif
}
//==========================================================================================
// does not need to be exact, we just want the function pointer:
ULONG ASM SAVEDS TrackHeader_Dispatcher(
                    REG(a0,struct IClass *C),
                    REG(a2,struct Gadget *Gad),
                    REG(a1,union MsgUnion *M));

#ifndef TRACKHEADER_STATICLINK
// called by shared Lib init to create class.

int ASM CreateClass(REG(a6,struct ExtClassLib *LibBase))
{
  if(LibBase) SysBase = LibBase->cb_SysBase;
  if(TrackHeader_OpenLibs() && TrackHeader_OpenLibs_Dependencies())
  {
    if(TrackHeaderClassPtr=MakeClass(TrackHeader_CLASS_ID,TrackHeaderSuperClassID,0,sizeof(TrackHeader),0))
    {
     if(LibBase) LibBase->cb_ClassLibrary.cl_Class = TrackHeaderClassPtr;
      TrackHeaderClassPtr->cl_Dispatcher.h_Data=LibBase;
      TrackHeaderClassPtr->cl_Dispatcher.h_Entry=(REHOOKFUNC)TrackHeader_Dispatcher;

      AddClass(TrackHeaderClassPtr);
      /* Success */
      return(0);
    }
    TrackHeader_CloseLibs_Dependencies();
    TrackHeader_CloseLibs();
  }
  /* Fail */
  return(-1);
}
// called by shared Lib expunge to dispose class.
void ASM DestroyClass(REG(a6,struct ExtClassLib *LibBase))
{
    // note LibBase and TrackHeaderClassPtr should be the same
    if(TrackHeaderClassPtr)
    {
      RemoveClass(TrackHeaderClassPtr);
      FreeClass(TrackHeaderClassPtr);
      TrackHeaderClassPtr = NULL;
    }
  TrackHeader_CloseLibs_Dependencies();
  TrackHeader_CloseLibs();
}
// first public lib function for boopsi classes
Class * ASM GetClass(void)
{
    return (Class *)TrackHeaderClassPtr;
}
// end if shared class
#else
// static version:
struct IClass   *TRACKHEADER_GetClass()
{
    return TrackHeaderClassPtr;
}
#endif

//====================================================================================

#ifdef TRACKHEADER_STATICLINK

// just use this one once when static link
int TrackHeaderStaticInit()
{ 
   if(!TrackHeader_OpenLibs_Dependencies()) return 1;
    if(TrackHeaderClassPtr=MakeClass(NULL,TrackHeaderSuperClassID,0,sizeof(TrackHeader),0))
    {
      TrackHeaderClassPtr->cl_Dispatcher.h_Entry=(REHOOKFUNC)TrackHeader_Dispatcher;
     // do not AddClass() when static, no need to publish, TrackHeaderClassPtr will be enough.
      /* Success */
      return(0);
    }
    return 1;
}

void TrackHeaderStaticClose()
{
    TrackHeader_CloseLibs_Dependencies();
    if(TrackHeaderClassPtr)
    {
      FreeClass(TrackHeaderClassPtr);
      TrackHeaderClassPtr = NULL;
    }

}

#endif


