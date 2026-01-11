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

#include "class_volumerule_private.h"

#include "bdbprintf.h"

typedef ULONG (*REHOOKFUNC)();

#ifdef VOLUMERULE_STATICLINK
#include <stdlib.h>
#include <string.h>
#endif

#ifndef VOLUMERULE_STATICLINK
struct ExecBase       *SysBase=NULL;
struct GfxBase        *GfxBase=NULL;
struct IntuitionBase  *IntuitionBase=NULL;
struct Library        *LayersBase=NULL;
struct DosLibrary     *DOSBase=NULL;
struct Library        *UtilityBase=NULL;
    #if defined(__GNUC__) && (__GNUC__ < 3)
        struct Library        *__UtilityBase=NULL;
    #endif
#endif

/* this is the only global writable we should see in the whole class binary ! */
struct IClass   *VolumeRuleClassPtr=NULL;

#ifndef VOLUMERULE_STATICLINK
const char Class_ID[]= VolumeRule_CLASS_ID;
const char *VersionString = "volumerule.gadget 1.0 ";
#endif

/* Local char array for superclass ID - safer than using macro directly */
const char VolumeRuleSuperClassID[] = VolumeRule_SUPERCLASS_ID;

#ifndef VOLUMERULE_STATICLINK
    BOOL VolumeRule_OpenLibs(void)
    {
       if(!DOSBase)  DOSBase = (struct DosLibrary *)OpenLibrary("dos.library",1);
       if(!IntuitionBase)  IntuitionBase = (struct IntuitionBase *) OpenLibrary("intuition.library",39);
       if(!GfxBase) GfxBase = (struct GfxBase *) OpenLibrary("graphics.library",39);
       if(!UtilityBase) UtilityBase = OpenLibrary("utility.library",39);
       if(!LayersBase) LayersBase = OpenLibrary("layers.library",39);
    #if defined(__GNUC__) && (__GNUC__ < 3)
        __UtilityBase = UtilityBase;
    #endif
        return TRUE;
    }

    void VolumeRule_CloseLibs(void)
    {
        if(LayersBase) CloseLibrary(LayersBase);
        if(DOSBase) CloseLibrary((struct Library *)DOSBase);
        if(UtilityBase) CloseLibrary(UtilityBase);
        if(GfxBase) CloseLibrary((struct Library *)GfxBase);
        if(IntuitionBase) CloseLibrary((struct Library *)IntuitionBase);
    }

#endif

BOOL VolumeRule_OpenLibs_Dependencies(void)
{
    return TRUE;
}

void VolumeRule_CloseLibs_Dependencies(void)
{
}

/* Dispatcher forward declaration */
ULONG ASM SAVEDS VolumeRule_Dispatcher(
                    REG(a0,struct IClass *C),
                    REG(a2,struct Gadget *Gad),
                    REG(a1,union MsgUnion *M));

#ifndef VOLUMERULE_STATICLINK
/* Called by shared Lib init to create class */
int ASM CreateClass(REG(a6,struct ExtClassLib *LibBase))
{
  if(LibBase) SysBase = LibBase->cb_SysBase;
  if(VolumeRule_OpenLibs() && VolumeRule_OpenLibs_Dependencies())
  {
    if(VolumeRuleClassPtr=MakeClass(VolumeRule_CLASS_ID, VolumeRuleSuperClassID, 0, sizeof(VolumeRule), 0))
    {
     if(LibBase) LibBase->cb_ClassLibrary.cl_Class = VolumeRuleClassPtr;
      VolumeRuleClassPtr->cl_Dispatcher.h_Data=LibBase;
      VolumeRuleClassPtr->cl_Dispatcher.h_Entry=(REHOOKFUNC)VolumeRule_Dispatcher;

      AddClass(VolumeRuleClassPtr);
      return(0);
    }
    VolumeRule_CloseLibs_Dependencies();
    VolumeRule_CloseLibs();
  }
  return(-1);
}

/* Called by shared Lib expunge to dispose class */
void ASM DestroyClass(REG(a6,struct ExtClassLib *LibBase))
{
    if(VolumeRuleClassPtr)
    {
      RemoveClass(VolumeRuleClassPtr);
      FreeClass(VolumeRuleClassPtr);
      VolumeRuleClassPtr = NULL;
    }
  VolumeRule_CloseLibs_Dependencies();
  VolumeRule_CloseLibs();
}

/* First public lib function for boopsi classes */
Class * ASM GetClass(void)
{
    return (Class *)VolumeRuleClassPtr;
}

#else
/* Static version */
struct IClass *VOLUMERULE_GetClass()
{
    return VolumeRuleClassPtr;
}
#endif

#ifdef VOLUMERULE_STATICLINK

/* Static link initialization */
int VolumeRuleStaticInit()
{
    if(!VolumeRule_OpenLibs_Dependencies()) return 0;

    /* MakeClass with superclass string (gadgetclass) */
    if(VolumeRuleClassPtr = MakeClass(NULL, VolumeRuleSuperClassID, 0, sizeof(VolumeRule), 0))
    {
        VolumeRuleClassPtr->cl_Dispatcher.h_Entry = (REHOOKFUNC)VolumeRule_Dispatcher;
        return(1);
    }
    return 0;
}

void VolumeRuleStaticClose()
{
    VolumeRule_CloseLibs_Dependencies();
    if(VolumeRuleClassPtr)
    {
      FreeClass(VolumeRuleClassPtr);
      VolumeRuleClassPtr = NULL;
    }
}

#endif
