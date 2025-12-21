
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

#include "class_tracklistareaui.h"
#include "class_tracklistareaui_private.h"

#include <utility/tagitem.h>

#define MRK_BUFFER_SIZE 3

ULONG TrackListAreaUi_HandleInput(Class *C, struct Gadget *Gad, struct gpInput *Input)
{
  ULONG retval=GMR_MEACTIVE; //default

  TrackListAreaUi *gdata;
  struct InputEvent *ie;

  gdata=INST_DATA(C, Gad);
  retval = GMR_MEACTIVE;
  ie = Input->gpi_IEvent;

  switch(ie->ie_Class)
  {    case IECLASS_RAWKEY:
        // todo: recursively send event to tracks.
      break;
    case IECLASS_RAWMOUSE:
      {

    // still in IECLASS_RAWMOUSE
        switch(ie->ie_Code)
         {

          case SELECTUP:

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
              retval = GMR_REUSE;
            }
            else
            {
              // todo: recursively send event to tracks.
              retval = GMR_MEACTIVE;
            }
            break;

          default:
            retval = GMR_MEACTIVE;
        } // end of
      } // end of IECLASS_RAWMOUSE
      break;
  } // end of ieclass switch

  return(retval);
}

