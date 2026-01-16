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

#include <stdlib.h>
#include <string.h>


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


/* this is the only global writable we should see in the whole class binary ! */
struct IClass   *VolumeRuleClassPtr=NULL;

/* Local char array for superclass ID - safer than using macro directly */
const char VolumeRuleSuperClassID[] = VolumeRule_SUPERCLASS_ID;

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


/* Static version */
struct IClass *VOLUMERULE_GetClass()
{
    return VolumeRuleClassPtr;
}

/* Static link initialization */
int VolumeRuleStaticInit()
{
    if(!VolumeRule_OpenLibs_Dependencies()) return 0;

    /* MakeClass with superclass string (gadgetclass) */
    if((VolumeRuleClassPtr = MakeClass(NULL, VolumeRuleSuperClassID, 0, sizeof(VolumeRule), 0))!=NULL)
    {
        bdbprintf_makeclass("VolumeRule", VolumeRuleClassPtr);
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
      bdbprintf_freeclass("VolumeRule", VolumeRuleClassPtr);
      FreeClass(VolumeRuleClassPtr);
      VolumeRuleClassPtr = NULL;
    }
}
