

#include <clib/alib_protos.h>
#include <proto/dos.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/gadgetclass.h>
#include <intuition/icclass.h>
#include "class_volumerule.h"
#include "class_volumerule_private.h"

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/utility.h>

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


/**
 * VolumeRule Dispatcher
 *
 * Handles BOOPSI method calls for the VolumeRule gadget class.
 * VolumeRule is a simple vertical scale displaying volume values from -1 to +1.
 */
ULONG ASM SAVEDS VolumeRule_Dispatcher(
                    REG(a0,struct IClass *C),
                    REG(a2,struct Gadget *Gad),
                    REG(a1,union MsgUnion *M))
{
  VolumeRule *gdata;
  ULONG retval=0;

  switch(M->MethodID)
  {
    case OM_NEW:
      {
        if((Gad=(struct Gadget *)DoSuperMethodA(C,(Object *)Gad,(Msg)M))!=NULL)
        {
            gdata=INST_DATA(C, Gad);
            bdbprintf_new("VolumeRule", Gad);

            /* Initialize instance data */
            gdata->_style = NULL;

            /* Process initial attributes */
            VolumeRule_SetAttrs(C, Gad, (struct opSet *)M);

            retval=(ULONG)Gad;
        }
      }
      break;

    case OM_DISPOSE:
        bdbprintf_dispose("VolumeRule", Gad);
      retval=DoSuperMethodA(C,(Object *)Gad,(Msg)M);
      break;

    case OM_UPDATE:
    case OM_SET:
      retval = VolumeRule_SetAttrs(C, Gad, (struct opSet *)M);
      if(!retval) retval = DoSuperMethodA(C,(Object *)Gad,(Msg)M);
      break;

    case OM_GET:
      retval = VolumeRule_GetAttr(C, Gad, (struct opGet *)M);
      break;

    case GM_DOMAIN:
      retval = VolumeRule_Domain(C, Gad, (struct gpDomain *)M);
      break;

    case GM_RENDER:
      retval = VolumeRule_Render(C, Gad, (struct gpRender *)M);
      break;
    case GM_HITTEST:
        retval = 0; //GMR_GADGETHIT
        break;
    default:
      retval=DoSuperMethodA(C,(Object *)Gad,(Msg)M);
      break;
  }
  return(retval);
}
