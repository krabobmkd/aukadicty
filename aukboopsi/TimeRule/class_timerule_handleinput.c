
#include <proto/exec.h>
#include <proto/intuition.h>

#ifdef __SASC
//    #include "minialib.h"
    #include <clib/alib_protos.h>
#else
    // GCC
    #include "minialib.h"
#endif

#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/gadgetclass.h>

#include "class_timerule.h"
#include "class_timerule_private.h"

#include <utility/tagitem.h>

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


// ULONG TimeRule_DoNotify(struct IClass *C, struct Gadget *Gad, Msg M, ULONG Flags, Tag Tags, ...);
// ULONG TimeRule_NotifyCoords(Class *C, struct Gadget *Gad, struct GadgetInfo	*GInfo)
// {
//     struct opUpdate notifymsg;
//     TimeRule *gdata=INST_DATA(C, Gad);
//     ULONG tags[]={
//         GA_ID,0,
//         TIMERULE_CenterX,0,
//         TIMERULE_CenterY,0,
//         TAG_DONE
//     };

//     tags[1] = Gad->GadgetID;
//     tags[3] = (LONG)gdata->_circleCenterX;
//     tags[5] = (LONG)gdata->_circleCenterY;
//     notifymsg.MethodID = OM_NOTIFY;
//     notifymsg.opu_AttrList = (struct TagItem *)&tags[0];
//     notifymsg.opu_GInfo = GInfo; // "always there for gadget, in all messages"
//     notifymsg.opu_Flags = 0;
//     return DoSuperMethodA(C,(APTR)Gad,(Msg)&notifymsg );

// }

ULONG TimeRule_HandleInput(Class *C, struct Gadget *Gad, struct gpInput *Input)
{
  ULONG retval=GMR_MEACTIVE; //default

  TimeRule *gdata;
  struct InputEvent *ie;

  gdata=INST_DATA(C, Gad);
  retval = GMR_MEACTIVE;
  ie = Input->gpi_IEvent;

  switch(ie->ie_Class)
  {    case IECLASS_RAWKEY:

      break;
    case IECLASS_RAWMOUSE:
      {

    // still in IECLASS_RAWMOUSE
        switch(ie->ie_Code)
         {

          case SELECTUP:
             gdata->_MouseMode=0;

           retval = GMR_MEACTIVE;
            break;

          case SELECTDOWN:
            // actually receive all clics on the whole WB !!
             if ( (((Input->gpi_Mouse).X < 0) ||
                 ((Input->gpi_Mouse).X >= Gad->Width) ||
                 ((Input->gpi_Mouse).Y < 0) ||
                 ((Input->gpi_Mouse).Y >= Gad->Height))
                  )
            {// outside gadget or disabled.

              if(gdata->_EditMode)
              {
                gdata->_EditMode=0;
                TimeRule_Render(C,Gad,(APTR)Input,GREDRAW_UPDATE);
              }
//              retval = GMR_NOREUSE | GMR_VERIFY;
              retval = GMR_REUSE;
            }
            else if((Gad->Flags & GFLG_DISABLED)==0) // don't manage clicks if disabled.
            {

              retval = GMR_MEACTIVE;
            }
            break;

          default:
            retval = GMR_MEACTIVE;
        } // end of
      } // end of IECLASS_RAWMOUSE
      break;
  } // end of ieclass switch

  if(retval!=GMR_MEACTIVE)
  {
    //TimeRule_Notify(C,Gad,(APTR)Input, 0);
  }

  return(retval);
}
