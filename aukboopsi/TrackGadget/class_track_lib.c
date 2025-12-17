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

#include "class_track_private.h"

#ifdef USE_BEVEL_FRAME
    #include <proto/bevel.h>
#endif

typedef ULONG (*REHOOKFUNC)();

#ifdef TRACK_STATICLINK
#include <stdlib.h>
#include <string.h>
#endif

#ifndef TRACK_STATICLINK
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
struct IClass   *TrackClassPtr=NULL;
// this 2 strings are also linked to the asm startup header ( in .gadget mode)
// note: (const char *str="") would make str be a (char **) to the linker. so char str[] is linkable to asm startup
#ifndef TRACK_STATICLINK
const char Class_ID[]= Track_CLASS_ID;
const char *VersionString = "track.gadget 1.0 "; // add date
#endif
const char TrackSuperClassID[]=Track_SUPERCLASS_ID;




// note: if other boopsi classes are dependences, they need to be opened here.
#ifndef TRACK_STATICLINK
    BOOL Track_OpenLibs(void)
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

    void Track_CloseLibs(void)
    {
        if(LayersBase) CloseLibrary(LayersBase);
        if(DOSBase) CloseLibrary((struct Library *)DOSBase);
        if(UtilityBase) CloseLibrary(UtilityBase);
        if(GfxBase) CloseLibrary((struct Library *)GfxBase);
        if(IntuitionBase) CloseLibrary((struct Library *)IntuitionBase);
    }

#endif
BOOL Track_OpenLibs_Dependencies(void)
{
#ifdef USE_BEVEL_FRAME
    if(!BevelBase) BevelBase = OpenLibrary("images/bevel.image",44);
    if(!BevelBase) return FALSE;
#endif
    return TRUE;
}

void Track_CloseLibs_Dependencies(void)
{
#ifdef USE_BEVEL_FRAME
    if(BevelBase) CloseLibrary(BevelBase);
    BevelBase = NULL;
#endif
}
//==========================================================================================
// does not need to be exact, we just want the function pointer:
ULONG ASM SAVEDS Track_Dispatcher(
                    REG(a0,struct IClass *C),
                    REG(a2,struct Gadget *Gad),
                    REG(a1,union MsgUnion *M));

#ifndef TRACK_STATICLINK
// called by shared Lib init to create class.

int ASM CreateClass(REG(a6,struct ExtClassLib *LibBase))
{
  if(LibBase) SysBase = LibBase->cb_SysBase;
  if(Track_OpenLibs() && Track_OpenLibs_Dependencies())
  {
    if(TrackClassPtr=MakeClass(Track_CLASS_ID,TrackSuperClassID,0,sizeof(Track),0))
    {
     if(LibBase) LibBase->cb_ClassLibrary.cl_Class = TrackClassPtr;
      TrackClassPtr->cl_Dispatcher.h_Data=LibBase;
      TrackClassPtr->cl_Dispatcher.h_Entry=(REHOOKFUNC)Track_Dispatcher;

      AddClass(TrackClassPtr);
      /* Success */
      return(0);
    }
    Track_CloseLibs_Dependencies();
    Track_CloseLibs();
  }
  /* Fail */
  return(-1);
}
// called by shared Lib expunge to dispose class.
void ASM DestroyClass(REG(a6,struct ExtClassLib *LibBase))
{
    // note LibBase and TrackClassPtr should be the same
    if(TrackClassPtr)
    {
      RemoveClass(TrackClassPtr);
      FreeClass(TrackClassPtr);
      TrackClassPtr = NULL;
    }
  Track_CloseLibs_Dependencies();
  Track_CloseLibs();
}
// first public lib function for boopsi classes
Class * ASM GetClass(void)
{
    return TrackClassPtr;
}
// end if shared class
#else
// static version:
Class *TRACK_GetClass()
{
    return TrackClassPtr;
}
#endif

//====================================================================================

#ifdef TRACK_STATICLINK

// just use this one once when static link
int TrackStaticInit()
{ 
   if(!Track_OpenLibs_Dependencies()) return 0;
    if(TrackClassPtr=MakeClass(NULL,TrackSuperClassID,0,sizeof(Track),0))
    {
      TrackClassPtr->cl_Dispatcher.h_Entry=(REHOOKFUNC)Track_Dispatcher;
     // do not AddClass() when static, no need to publish, TrackClassPtr will be enough.
      /* Success */
      return(1);
    }
    return 0;
}

void TrackStaticClose()
{
    Track_CloseLibs_Dependencies();
    if(TrackClassPtr)
    {
      FreeClass(TrackClassPtr);
      TrackClassPtr = NULL;
    }

}

#endif


