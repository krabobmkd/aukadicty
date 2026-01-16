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

#include <proto/layout.h>
#include <gadgets/layout.h>

#include "class_tracklistarea.h"
#include "class_tracklistarea_private.h"

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


typedef ULONG (*REHOOKFUNC)();

#ifdef TRACKLIST_STATICLINK
#include <stdlib.h>
#include <string.h>
#endif

#ifndef TRACKLIST_STATICLINK
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

// this is the only global writtable we should see in the whole class binary !
struct IClass   *TrackListClassPtr=NULL;
// this 2 strings are also linked to the asm startup header ( in .gadget mode)
// note: (const char *str="") would make str be a (char **) to the linker. so char str[] is linkable to asm startup
#ifndef TRACKLIST_STATICLINK
const char Class_ID[]= TrackList_CLASS_ID;
const char *VersionString = "tracklist.gadget 1.0 "; // add date
#endif
const char TrackListSuperClassID[]=TrackList_SUPERCLASS_ID;




// note: if other boopsi classes are dependences, they need to be opened here.
#ifndef TRACKLIST_STATICLINK
    BOOL TrackList_OpenLibs(void)
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

    void TrackList_CloseLibs(void)
    {
        if(LayersBase) CloseLibrary(LayersBase);
        if(DOSBase) CloseLibrary((struct Library *)DOSBase);
        if(UtilityBase) CloseLibrary(UtilityBase);
        if(GfxBase) CloseLibrary((struct Library *)GfxBase);
        if(IntuitionBase) CloseLibrary((struct Library *)IntuitionBase);
    }

#endif
BOOL TrackList_OpenLibs_Dependencies(void)
{

    return TRUE;
}

void TrackList_CloseLibs_Dependencies(void)
{

}
//==========================================================================================
// does not need to be exact, we just want the function pointer:
ULONG ASM SAVEDS TrackListArea_Dispatcher(
                    REG(a0,struct IClass *C),
                    REG(a2,struct Gadget *Gad),
                    REG(a1,union MsgUnion *M));


// static version:
struct IClass   *TRACKLIST_GetClass()
{
    return TrackListClassPtr;
}

//====================================================================================

// just use this one once when static link
int TrackListStaticInit()
{
   if(!TrackList_OpenLibs_Dependencies()) return 0;
//    if(TrackListClassPtr=MakeClass(NULL,TrackListSuperClassID,0,sizeof(TrackListArea),0))
    if((TrackListClassPtr=MakeClass(NULL,NULL,LAYOUT_GetClass(),sizeof(TrackListArea),0))!=NULL)
    {
      bdbprintf_makeclass("TrackList", TrackListClassPtr);
      TrackListClassPtr->cl_Dispatcher.h_Entry=(REHOOKFUNC)TrackListArea_Dispatcher;
     // do not AddClass() when static, no need to publish, TrackListClassPtr will be enough.
      /* Success */
      return(1);
    }
    return 0;
}

void TrackListStaticClose()
{
    TrackList_CloseLibs_Dependencies();
    if(TrackListClassPtr)
    {
      bdbprintf_freeclass("TrackList", TrackListClassPtr);
      FreeClass(TrackListClassPtr);
      TrackListClassPtr = NULL;
    }

}

