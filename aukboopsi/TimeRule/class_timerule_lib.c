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

#include "class_timerule_private.h"

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

#ifdef TIMERULE_STATICLINK
#include <stdlib.h>
#include <string.h>
#endif

#ifndef TIMERULE_STATICLINK
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
struct IClass   *TimeRuleClassPtr=NULL;
/* this 2 strings are also linked to the asm startup header ( in .gadget mode) */
/* note: (const char *str="") would make str be a (char **) to the linker. so char str[] is linkable to asm startup */
#ifndef TIMERULE_STATICLINK
const char Class_ID[]= TimeRule_CLASS_ID;
const char *VersionString = "timerule.gadget 1.0 "; /* add date */
#endif
/* TimeRule uses InfiniteScroll as superclass (class pointer, not string) */
/* const char TimeRuleSuperClassID[]=TimeRule_SUPERCLASS_ID; -- not used, we use class pointer */




// note: if other boopsi classes are dependences, they need to be opened here.
#ifndef TIMERULE_STATICLINK
    BOOL TimeRule_OpenLibs(void)
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

    void TimeRule_CloseLibs(void)
    {
        if(LayersBase) CloseLibrary(LayersBase);
        if(DOSBase) CloseLibrary((struct Library *)DOSBase);
        if(UtilityBase) CloseLibrary(UtilityBase);
        if(GfxBase) CloseLibrary((struct Library *)GfxBase);
        if(IntuitionBase) CloseLibrary((struct Library *)IntuitionBase);
    }

#endif
BOOL TimeRule_OpenLibs_Dependencies(void)
{
#ifdef USE_BEVEL_FRAME
    if(!BevelBase) BevelBase = OpenLibrary("images/bevel.image",44);
    if(!BevelBase) return FALSE;
#endif
    return TRUE;
}

void TimeRule_CloseLibs_Dependencies(void)
{
#ifdef USE_BEVEL_FRAME
    if(BevelBase) CloseLibrary(BevelBase);
    BevelBase = NULL;
#endif
}
//==========================================================================================
// does not need to be exact, we just want the function pointer:
ULONG ASM SAVEDS TimeRule_Dispatcher(
                    REG(a0,struct IClass *C),
                    REG(a2,struct Gadget *Gad),
                    REG(a1,union MsgUnion *M));

#ifndef TIMERULE_STATICLINK
// called by shared Lib init to create class.

int ASM CreateClass(REG(a6,struct ExtClassLib *LibBase))
{
  if(LibBase) SysBase = LibBase->cb_SysBase;
  if(TimeRule_OpenLibs() && TimeRule_OpenLibs_Dependencies())
  {
    if(TimeRuleClassPtr=MakeClass(TimeRule_CLASS_ID,TimeRuleSuperClassID,0,sizeof(TimeRule),0))
    {
     if(LibBase) LibBase->cb_ClassLibrary.cl_Class = TimeRuleClassPtr;
      TimeRuleClassPtr->cl_Dispatcher.h_Data=LibBase;
      TimeRuleClassPtr->cl_Dispatcher.h_Entry=(REHOOKFUNC)TimeRule_Dispatcher;

      AddClass(TimeRuleClassPtr);
      /* Success */
      return(0);
    }
    TimeRule_CloseLibs_Dependencies();
    TimeRule_CloseLibs();
  }
  /* Fail */
  return(-1);
}
// called by shared Lib expunge to dispose class.
void ASM DestroyClass(REG(a6,struct ExtClassLib *LibBase))
{
    // note LibBase and TimeRuleClassPtr should be the same
    if(TimeRuleClassPtr)
    {
      RemoveClass(TimeRuleClassPtr);
      FreeClass(TimeRuleClassPtr);
      TimeRuleClassPtr = NULL;
    }
  TimeRule_CloseLibs_Dependencies();
  TimeRule_CloseLibs();
}
// first public lib function for boopsi classes
Class * ASM GetClass(void)
{
    return (Class *)TimeRuleClassPtr;
}
// end if shared class
#else
// static version:
struct IClass   *TIMERULE_GetClass()
{
    return TimeRuleClassPtr;
}
#endif

//====================================================================================

#ifdef TIMERULE_STATICLINK

/* just use this one once when static link */
/* TimeRule inherits from InfiniteScroll - must be initialized first */
int TimeRuleStaticInit()
{
    struct IClass *superClass;

    if(!TimeRule_OpenLibs_Dependencies()) return 0;

    /* Get InfiniteScroll class - it must be initialized before TimeRule */
    superClass = INFINITESCROLL_GetClass();
    if(!superClass) return 0;

    /* MakeClass with class pointer (not string) as superclass */
    if(TimeRuleClassPtr = MakeClass(NULL, NULL, superClass, sizeof(TimeRule), 0))
    {
        TimeRuleClassPtr->cl_Dispatcher.h_Entry = (REHOOKFUNC)TimeRule_Dispatcher;
        /* do not AddClass() when static, no need to publish, TimeRuleClassPtr will be enough. */
        /* Success */
        return(1);
    }
    return 0;
}

void TimeRuleStaticClose()
{
    TimeRule_CloseLibs_Dependencies();
    if(TimeRuleClassPtr)
    {
      FreeClass(TimeRuleClassPtr);
      TimeRuleClassPtr = NULL;
    }

}

#endif


