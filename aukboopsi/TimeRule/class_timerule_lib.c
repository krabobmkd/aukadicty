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


#include <stdlib.h>
#include <string.h>


/* this is the only global writable we should see in the whole class binary ! */
struct IClass   *TimeRuleClassPtr=NULL;
/* this 2 strings are also linked to the asm startup header ( in .gadget mode) */
/* note: (const char *str="") would make str be a (char **) to the linker. so char str[] is linkable to asm startup */

/* TimeRule uses InfiniteScroll as superclass (class pointer, not string) */
/* const char TimeRuleSuperClassID[]=TimeRule_SUPERCLASS_ID; -- not used, we use class pointer */



BOOL TimeRule_OpenLibs_Dependencies(void)
{

    return TRUE;
}

void TimeRule_CloseLibs_Dependencies(void)
{

}
//==========================================================================================
// does not need to be exact, we just want the function pointer:
ULONG ASM SAVEDS TimeRule_Dispatcher(
                    REG(a0,struct IClass *C),
                    REG(a2,struct Gadget *Gad),
                    REG(a1,union MsgUnion *M));


// static version:
struct IClass   *TIMERULE_GetClass()
{
    return TimeRuleClassPtr;
}

//====================================================================================

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
    if((TimeRuleClassPtr = MakeClass(NULL, NULL, superClass, sizeof(TimeRule), 0))!=NULL)
    {
        bdbprintf_makeclass("TimeRule", TimeRuleClassPtr);
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
      bdbprintf_freeclass("TimeRule", TimeRuleClassPtr);
      FreeClass(TimeRuleClassPtr);
      TimeRuleClassPtr = NULL;
    }

}


