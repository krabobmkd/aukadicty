
#include <proto/exec.h>
#include <proto/intuition.h>

#include <clib/alib_protos.h>

#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/gadgetclass.h>

#include "class_trackarea.h"
#include "class_trackarea_private.h"

#include <utility/tagitem.h>
#include "../aukeditmode.h"

#include "gadgetid.h"
#include "auktrack.h"
#include "auksound.h"

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

//#define MRK_BUFFER_SIZE 3
// shared global state...
extern int CurrentEditMode;

// TRACKAREA_TimeSelectionChange
static ULONG TrackArea_NotifyAttribValue(struct Gadget *Gad, struct GadgetInfo *GInfo,ULONG attrib, ULONG value)
{
    TrackArea *gdata;
    struct opUpdate notifymsg;
    ULONG tags[]={
     GA_ID,0,
     0,0,
     TAG_DONE
    };

    gdata=INST_DATA(OCLASS(Gad), Gad);

    // use same GA_ID interval as button in trackheaders, to tag message sender
    tags[1] = GAD_TRACKHEADER_BASE | GAD_TRACKHEADER_TRACKAREA | ((gdata->_dataTrack->trackIndex) << 4) ; // Gad->GadgetID;
    tags[2] = attrib;
    tags[3] = value;
    notifymsg.MethodID = OM_NOTIFY;
    notifymsg.opu_AttrList = (struct TagItem *)&tags[0];
    notifymsg.opu_GInfo = GInfo; // "always there for gadget, in all messages"
    notifymsg.opu_Flags = 0;

    return DoSuperMethodA(OCLASS(Gad),(APTR)Gad,(Msg)&notifymsg );
}

/* Notify sound slide change */
static ULONG TrackArea_NotifySoundSlide(struct Gadget *Gad, struct GadgetInfo *GInfo,
                                        AukSoundSlideInfo *slideInfo)
{
    return TrackArea_NotifyAttribValue(Gad, GInfo, TRACKAREA_SoundSlideChange, (ULONG)slideInfo);
}




ULONG TrackArea_HandleInput(Class *C, struct Gadget *Gad, struct gpInput *Input, int isFirstActivate)
{
  ULONG retval=GMR_MEACTIVE; //default

  TrackArea *gdata;
  struct InputEvent *ie;

  gdata=INST_DATA(C, Gad);
  //retval = GMR_MEACTIVE;  
  ie = Input->gpi_IEvent;

  switch(ie->ie_Class)
  {    case IECLASS_RAWKEY:

      break;
    case IECLASS_RAWMOUSE:
      {
//        LONG x,y;
//        LONG r,c;

//        retval = GMR_MEACTIVE;

        // x=(Input->gpi_Mouse).X+Gad->LeftEdge;
        // y=(Input->gpi_Mouse).Y+Gad->TopEdge;

//          DKP("RawMouse %ld %ld\n", x, y);
/*        if(gdata->MouseMode)
        {
          for(r=0; r<gdata->Rows; r++)
          {
            if(y>=gdata->Row[r] && y<gdata->Row[r+1])
              break;
          }

          for(c=0; c<gdata->Cols; c++)
          {
            if(x>=gdata->Col[c] && x<gdata->Col[c+1])
              break;
          }
if((Gad->Flags & GFLG_DISABLED)==0) 
//          DKP("  c=%ld r=%ld\n", c, r);

          if(c<gdata->Cols && r<gdata->Rows)
          {
            gdata->ActivePen=r * gdata->Cols + c;
            if(gdata->ActivePen!=gdata->LastActivePen)
            {
              i_StoreUndoIfNeeded(C,Gad,Input);
              TrackArea_Notify(C,Gad,(APTR)Input, OPUF_INTERIM);
              gad_Render(C,Gad,(APTR)Input,GREDRAW_UPDATE);
            }
          }

        }
*/
   // bdbprintf("IECLASS_RAWMOUSE:\n");

    // still in IECLASS_RAWMOUSE
        switch(ie->ie_Code)
         {

          case SELECTUP:
    bdbprintf("TA SELECTUP: %d %d\n",(int)(Input->gpi_Mouse).X,(int)(Input->gpi_Mouse).Y);
            if( gdata->_MoveType == TRCKMOVE_Selection ||
               gdata->_MoveType == TRCKMOVE_PanZoom )
            {
                gdata->_inputselection._end =
                    (gdata->_pTimeProjection->_pixAtLeft
                    + Input->gpi_Mouse.X) * gdata->_pTimeProjection->_timePerPixelWidth;

                gdata->_MoveType = TRCKMOVE_NoMove;

                TrackArea_NotifyAttribValue(Gad,Input->gpi_GInfo,
                   (gdata->_MoveType == TRCKMOVE_Selection)?
                        TRACKAREA_TimeSelectionChange:TRACKAREA_TimeZoomChange,
                    (ULONG)&gdata->_inputselection);
            }
            else if(gdata->_MoveType == TRCKMOVE_Slide && gdata->_slidingSound)
            {
                /* End slide - send final notification */
                WORD deltaX = Input->gpi_Mouse.X - gdata->_slideStartMouseX;
                AukFixed deltaTime = (AukFixed)deltaX * gdata->_pTimeProjection->_timePerPixelWidth;
                AukFixed newStartTime = gdata->_slideOriginalStartTime + deltaTime;

                /* Clamp to allowed range */
                if(newStartTime < gdata->_slideMinTime) newStartTime = gdata->_slideMinTime;
                if(newStartTime > gdata->_slideMaxTime) newStartTime = gdata->_slideMaxTime;

                gdata->_slideInfo.itrack = gdata->_dataTrack->trackIndex;
                gdata->_slideInfo.sound = gdata->_slidingSound;
                gdata->_slideInfo.newStartTime = newStartTime;
                gdata->_slideInfo.isEnd = 1;

                TrackArea_NotifySoundSlide(Gad, Input->gpi_GInfo, &gdata->_slideInfo);

                gdata->_MoveType = TRCKMOVE_NoMove;
                gdata->_slidingSound = NULL;

                bdbprintf("TA Slide end: newStart=%lld\n", newStartTime);
            }
            retval = GMR_NOREUSE;
            break;
          case SELECTDOWN:
    bdbprintf("TA SELECTDOWN: %d %d\n",(int)(Input->gpi_Mouse).X,(int)(Input->gpi_Mouse).Y);
            // actually receive all clics on the whole WB !!
             if ( (((Input->gpi_Mouse).X < 0) ||
                 ((Input->gpi_Mouse).X >= Gad->Width) ||
                 ((Input->gpi_Mouse).Y < 0) ||
                 ((Input->gpi_Mouse).Y >= Gad->Height))
                 || ((Gad->Flags & GFLG_DISABLED)!=0)
                  )
            {// outside gadget or disabled.
                // click outside ?
                 retval = GMR_NOREUSE;
            }
            else // don't manage clicks if disabled.
            {
                // mouse click inside gadget !
                if((CurrentEditMode == EDITMODE_SELECT ||
                   CurrentEditMode == EDITMODE_VOLUME ) &&
                    gdata->_pTimeProjection )
                   {
                        gdata->_MoveType = (CurrentEditMode==EDITMODE_SELECT)
                            ? TRCKMOVE_Selection : TRCKMOVE_PanZoom ;


                        gdata->_inputselection._mode = 1;
                        gdata->_inputselection._itrack = gdata->_dataTrack->trackIndex ;
                        gdata->_inputselection._start =
                        gdata->_inputselection._end =
                            (gdata->_pTimeProjection->_pixAtLeft
                            + Input->gpi_Mouse.X) * gdata->_pTimeProjection->_timePerPixelWidth;
                    // sendmessage
                    TrackArea_NotifyAttribValue(Gad,Input->gpi_GInfo,
                       (gdata->_MoveType == TRCKMOVE_Selection)?
                            TRACKAREA_TimeSelectionChange:TRACKAREA_TimeZoomChange,
                        (ULONG)&gdata->_inputselection);

                        retval = GMR_MEACTIVE;
                   }
                else if(CurrentEditMode == EDITMODE_TIMESLIDE &&
                        gdata->_pTimeProjection &&
                        gdata->_dataTrack)
                   {
                        /* Slide mode: find sound under mouse */
                        AukFixed clickTime = (gdata->_pTimeProjection->_pixAtLeft
                            + Input->gpi_Mouse.X) * gdata->_pTimeProjection->_timePerPixelWidth;
                        unsigned int soundIndex = 0;
                        AukFixed minSlide = 0, maxSlide = 0;
                        AukSound *sound = AukTrack_FindSoundAtTime(gdata->_dataTrack, clickTime,
                                                                   &soundIndex, &minSlide, &maxSlide);

                        if(sound)
                        {
                            /* Found a sound - start sliding */
                            gdata->_MoveType = TRCKMOVE_Slide;
                            gdata->_slidingSound = sound;
                            gdata->_slideStartMouseX = Input->gpi_Mouse.X;
                            gdata->_slideOriginalStartTime = sound->startTime;
                            gdata->_slideMinTime = minSlide;
                            gdata->_slideMaxTime = maxSlide;

                            /* Release the reference - we keep a raw pointer during slide */
                            AukObjectPtr_Release((AukObjectPtr*)&sound);

                            bdbprintf("TA Slide start: sound at %lld, min=%lld, max=%lld\n",
                                     gdata->_slideOriginalStartTime, minSlide, maxSlide);

                            retval = GMR_MEACTIVE;
                        }
                        else
                        {
                            /* No sound under mouse - don't activate */
                            retval = GMR_NOREUSE;
                        }
                   }
                   else
                   {
                        retval = GMR_NOREUSE;
                   }
            }
            break;
            case IECODE_NOBUTTON:
            {
                /* if being moved */
                if( gdata->_MoveType == TRCKMOVE_Selection ||
                   gdata->_MoveType == TRCKMOVE_PanZoom )
                {
                    gdata->_inputselection._end =
                        (gdata->_pTimeProjection->_pixAtLeft
                        + Input->gpi_Mouse.X) * gdata->_pTimeProjection->_timePerPixelWidth;

                    // sendmessage
                    TrackArea_NotifyAttribValue(Gad,Input->gpi_GInfo,
                       (gdata->_MoveType == TRCKMOVE_Selection)?
                            TRACKAREA_TimeSelectionChange:TRACKAREA_TimeZoomChange,
                        (ULONG)&gdata->_inputselection);

                }
                else if(gdata->_MoveType == TRCKMOVE_Slide && gdata->_slidingSound)
                {
                    /* Live slide preview - compute new position and notify */
                    WORD deltaX = Input->gpi_Mouse.X - gdata->_slideStartMouseX;
                    AukFixed deltaTime = (AukFixed)deltaX * gdata->_pTimeProjection->_timePerPixelWidth;
                    AukFixed newStartTime = gdata->_slideOriginalStartTime + deltaTime;

                    /* Clamp to allowed range */
                    if(newStartTime < gdata->_slideMinTime) newStartTime = gdata->_slideMinTime;
                    if(newStartTime > gdata->_slideMaxTime) newStartTime = gdata->_slideMaxTime;

                    gdata->_slideInfo.itrack = gdata->_dataTrack->trackIndex;
                    gdata->_slideInfo.sound = gdata->_slidingSound;
                    gdata->_slideInfo.newStartTime = newStartTime;
                    gdata->_slideInfo.isEnd = 0;

                    TrackArea_NotifySoundSlide(Gad, Input->gpi_GInfo, &gdata->_slideInfo);
                }

            }
             break;
 /* The user hit the menu button. Go inactive and let      */
                                     /* Intuition reuse the menu button event so Intuition can */
                                     /* pop up the menu bar.                                   */

       /*   case MENUDOWN:
          if(gdata->EditMode)//                                                                      (44.3.1) (09/01/00)
            {//                                                                                        (44.3.1) (09/01/00)
              gdata->EditMode=0;//                                                                     (44.3.1) (09/01/00)
              gad_Render(C,Gad,(APTR)Input,GREDRAW_UPDATE);//                                          (44.3.1) (09/01/00)
              TrackArea_Notify(C,Gad,(APTR)Input, 0);//                                                        (44.3.1) (09/01/00)
            }//                                                                                        (44.3.1) (09/01/00)
            retval = GMR_REUSE;*/
                                          /* Since the gadget is going inactive, send a final   */
                                         /* notification to the ICA_TARGET.                    */
/*
            break;
            */
          default:
//    bdbprintf("TA ie->ie_Code: %d\n",ie->ie_Code);
            retval = GMR_MEACTIVE;
        } // end of
      } // end of IECLASS_RAWMOUSE
      break;
  } // end of ieclass switch

  if(retval!=GMR_MEACTIVE)
  {
    //TrackArea_Notify(C,Gad,(APTR)Input, 0);
  }

  return(retval);
}


ULONG TrackArea_GoInactive(Class *C, struct Gadget *Gad,struct gpGoInactive *M)
{
    return 0;

}

