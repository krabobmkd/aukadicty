
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/layers.h>


#include <clib/alib_protos.h>

#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/gadgetclass.h>
#include <utility/tagitem.h>

#include "class_trackarea.h"
#include "class_trackarea_private.h"
#include "../aukstyle.h"
#include "auktrack.h"
#include "aukarray.h"
#include "auksound.h"

/* Include InfiniteScroll private for access to superclass data */
#include "../InfiniteScroll/class_infinitescroll_private.h"

#ifdef USE_BEVEL_FRAME
    #include <proto/bevel.h>
    #include <images/bevel.h>
#endif

/* Most of the calls to boopsi methods are not done from the App's context,
 * but from a specific intuition context, and because of that we can't use DOS calls
 * like dos/Printf() , and also stdlib printf().
 * So we may print debug informations with a special buffer,and function bdbprintf(),
 * then flushbdbprint() in main process will print for real to standard output.
 * remove word USE_DEBUG_BDBPRINT to desactivate all bdbprintf()/flushbdbprint() calls.
 * Template projects that links boopsi classes statically use USE_DEBUG_BDBPRINT by default.
 * Template projects that uses boopsi classes with LoadLibrary() do not.
 */
#include "bdbprintf.h"
extern struct IClass   *TrackAreaClassPtr;

/* The GM_DOMAIN method is used to obtain the sizing requirements of an
 * object for a class before ever creating an object. */

/* GM_DOMAIN */
//struct gpDomain
//{
//    ULONG		 MethodID;
//    struct GadgetInfo	*gpd_GInfo;
//    struct RastPort	*gpd_RPort;	/* RastPort to layout for */
//    LONG		 gpd_Which;
//    struct IBox		 gpd_Domain;	/* Resulting domain */
//    struct TagItem	*gpd_Attrs;	/* Additional attributes */
//};




ULONG TrackArea_Domain(Class *C, struct Gadget *Gad, struct gpDomain *D)
{
  TrackArea *gdata=0;

  if(Gad) gdata=INST_DATA(C, Gad);
// Printf("TrackArea_Domain data:%lx\n",(int)gdata);

  D->gpd_Domain.Left=0;
  D->gpd_Domain.Top=0;

  switch(D->gpd_Which)
  {
    case GDOMAIN_NOMINAL:
     // if(gdata)
     // {
     //   D->gpd_Domain.Width =gdata->_minimalWidth;
     //   D->gpd_Domain.Height=gdata->_minimalHeight;
     // }
     // else
      {
        D->gpd_Domain.Width=100;
        D->gpd_Domain.Height=50;
      }
      break;

    case GDOMAIN_MAXIMUM:
      D->gpd_Domain.Width=4000;
      D->gpd_Domain.Height=4000;
      break;

    case GDOMAIN_MINIMUM:
    default:

    D->gpd_Domain.Width=  50;
    D->gpd_Domain.Height= 50;

      break;

  }
  return(1);
}


/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* GM_LAYOUT - TrackArea layout handling */
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

// ULONG TrackArea_Layout(Class *C, struct Gadget *Gad, struct gpLayout *layout)
// {
//   TrackArea *gdata=0;
//   gdata=INST_DATA(C, Gad);

//   /* Store frame rectangle for this gadget */
//   // gdata->_framerec.MinX = Gad->LeftEdge;
//   // gdata->_framerec.MinY = Gad->TopEdge;
//   // gdata->_framerec.MaxX = Gad->LeftEdge + Gad->Width - 1;
//   // gdata->_framerec.MaxY = Gad->TopEdge + Gad->Height - 1;

//   /* Let InfiniteScroll handle tile setup */
//   return DoSuperMethodA(C, Gad, (Msg)layout);
// }

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* InfiniteScroll RenderDelegate - Override to draw track content */
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/**
 * Render a tile with track content.
 *
 * The TrackArea displays sound clips and track content matching TrackListArea's
 * horizontal scroll position and zoom level via the TimeProjection pointer.
 *
 * Drawing coordinate system:
 * - Tile RastPort is at 0,0, size is destWidth x destHeight
 * - _start._scrollx is the pixel offset at the LEFT edge of this tile
 *
 * Time projection:
 * - Left edge time = _pixAtLeft * _timePerPixelWidth
 * - Right edge time = (_pixAtLeft + gadgetWidth) * _timePerPixelWidth
 * - For this tile: tileLeftTime = (p->_start._scrollx) * _timePerPixelWidth
 *
 * Selection rendering:
 * - Mode 0: no selection, use normal colors
 * - Mode 1: time span selection for track selection->_itrack
 * - Mode 2: whole track selected if track->selectionFlags & AukTrackSelFlag_Selected
 */
void TrackArea_RenderDelegate(InfiniteScrollRenderParams *p)
{
    TrackArea *gdata;
    struct Gadget *Gad = p->Gad;
    struct RastPort *rp = p->rp;
    TimeProjection *proj;
    AukSelection *selection = NULL;
    AukTrack *track;
    AukArray *sounds;
    unsigned int soundCount, i;
    long long timePerPixel;
    long long tileLeftTime, tileRightTime;
    AukStyle *style;
    WORD penBackground, penBgSound;
    WORD penBackgroundSelected, penBgSoundSelected;
    /* Selection pixel span (-1 means no selection active for this track) */
    LONG selPixLeft, selPixRight;
    int selectionActive;

    if(!Gad || !rp) return;

    gdata = INST_DATA(TrackAreaClassPtr, Gad);

    selection = gdata->_dataSelection;

    /* Style concern... */
    style = gdata->_style;
    penBackground = (style->background.pen != -1) ? style->background.pen : 1;
    penBgSound = (style->soundBackground.pen != -1) ? style->soundBackground.pen : 2;

    penBackgroundSelected = (style->selectedBackground.pen != -1) ? style->selectedBackground.pen : 1;
    penBgSoundSelected = (style->selectedSoundBackground.pen != -1) ? style->selectedSoundBackground.pen : 2;

    /* Check we have required data */
    proj = gdata->_pTimeProjection;
    track = gdata->_dataTrack;
    if(!proj || !track || !track->sounds)
    {
        /* No data: clear entire tile to background */
        SetAPen(rp, penBackground);
        RectFill(rp, p->destX, p->destY, p->destWidth - 1, p->destHeight - 1);
        SetAPen(rp, 2);
        Move(rp, p->destX, p->destHeight - 1);
        Draw(rp, p->destWidth - 1, p->destHeight - 1);
        return;
    }

    timePerPixel = proj->_timePerPixelWidth;
    if(timePerPixel == 0)
    {
        SetAPen(rp, penBackground);
        RectFill(rp, p->destX, p->destY, p->destWidth - 1, p->destHeight - 1);
        SetAPen(rp, 2);
        Move(rp, p->destX, p->destHeight - 1);
        Draw(rp, p->destWidth - 1, p->destHeight - 1);
        return;
    }

    sounds = track->sounds;
    soundCount = AukArray_GetCount(sounds);

    /* Calculate time range visible in this tile */
    tileLeftTime = p->_start._scrollx * timePerPixel;
    tileRightTime = (p->_start._scrollx + p->destWidth) * timePerPixel;

    /* Determine selection state for this track */
    selectionActive = 0;
    selPixLeft = -1;
    selPixRight = -1;

    if(selection)
    {
        if(selection->_mode == 1 && track->trackIndex == selection->_itrack)
        {
            /* Mode 1: time span selection for this specific track */
            selPixLeft = (LONG)((selection->_start / timePerPixel) - p->_start._scrollx);
            selPixRight = (LONG)((selection->_end / timePerPixel) - p->_start._scrollx) /*- 1*/;

            /* specifc: of start==end, means cursor. need 1 pixel */
            if(selPixLeft == selPixRight) selPixRight++;

            /* Clamp to tile bounds */
            if(selPixLeft < p->destX) selPixLeft = p->destX;
            if(selPixRight > (LONG)(p->destWidth - 1)) selPixRight = p->destWidth - 1;
            if(selPixLeft <= selPixRight) selectionActive = 1;
        }
        else if(selection->_mode == 2 && (track->selectionFlags & AukTrackSelFlag_Selected))
        {
            /* Mode 2: whole track is selected */
            selPixLeft = p->destX;
            selPixRight = p->destWidth - 1;
            selectionActive = 2;
        }
        /* Mode 0: no selection, selectionActive stays 0 */
    }

    /* Track current X position for gap filling - sounds are sorted and non-overlapping */
    {
        LONG currentX = p->destX;
        LONG tileRight = p->destWidth - 1;

        /* Iterate through sounds and draw gaps + sounds without overdraw */
        for(i = 0; i < soundCount; i++)
        {
            AukSound *sound = (AukSound *)sounds->items[i];
            long long soundStart, soundEnd;
            LONG pixLeft, pixRight;

            if(!sound) continue;

            soundStart = sound->startTime;
            soundEnd = sound->endTime;

            /* Since sounds are sorted, break early if sound starts after tile ends */
            if(soundStart >= tileRightTime) break;
            if(soundEnd <= tileLeftTime) continue;

            /* Convert sound times to pixel positions relative to tile left edge */
            pixLeft = (LONG)((soundStart / timePerPixel) - p->_start._scrollx);
            pixRight = (LONG)((soundEnd / timePerPixel) - p->_start._scrollx) - 1;

            /* Clamp to tile bounds */
            if(pixLeft < p->destX) pixLeft = p->destX;
            if(pixRight > tileRight) pixRight = tileRight;

            /* Skip if completely outside tile after clamping */
            if(pixLeft > pixRight) continue;

            /* Fill background gap before this sound */
            if(currentX < pixLeft)
            {
                LONG gapLeft = currentX;
                LONG gapRight = pixLeft - 1;

                if(selectionActive && gapRight >= selPixLeft && gapLeft <= selPixRight)
                {
                    /* Gap intersects selection - split into up to 3 parts */
                    /* Part before selection */
                    if(gapLeft < selPixLeft)
                    {
                        SetAPen(rp, penBackground);
                        RectFill(rp, gapLeft, p->destY, selPixLeft - 1, p->destHeight - 1);
                    }
                    /* Part within selection */
                    {
                        LONG selStart = (gapLeft > selPixLeft) ? gapLeft : selPixLeft;
                        LONG selEnd = (gapRight < selPixRight) ? gapRight : selPixRight;
                        if(selStart <= selEnd)
                        {
                            SetAPen(rp, penBackgroundSelected);
                            RectFill(rp, selStart, p->destY, selEnd, p->destHeight - 1);
                        }
                    }
                    /* Part after selection */
                    if(gapRight > selPixRight)
                    {
                        SetAPen(rp, penBackground);
                        RectFill(rp, selPixRight + 1, p->destY, gapRight, p->destHeight - 1);
                    }
                }
                else
                {
                    /* No selection intersection - normal background */
                    SetAPen(rp, penBackground);
                    RectFill(rp, gapLeft, p->destY, gapRight, p->destHeight - 1);
                }
            }

            /* Draw sound rectangle (with 2px margin top/bottom for visibility) */
            if(selectionActive && pixRight >= selPixLeft && pixLeft <= selPixRight)
            {
                /* Sound intersects selection - split into up to 3 parts */
                /* Part before selection */
                if(pixLeft < selPixLeft)
                {
                    SetAPen(rp, penBackground);
                    RectFill(rp, pixLeft, p->destY, selPixLeft - 1, p->destY + 1);
                    SetAPen(rp, penBgSound);
                    RectFill(rp, pixLeft, p->destY + 2, selPixLeft - 1, p->destHeight - 3);
                    SetAPen(rp, penBackground);
                    RectFill(rp, pixLeft, p->destHeight - 2, selPixLeft - 1, p->destHeight - 1);
                }
                /* Part within selection */
                {
                    LONG selStart = (pixLeft > selPixLeft) ? pixLeft : selPixLeft;
                    LONG selEnd = (pixRight < selPixRight) ? pixRight : selPixRight;
                    if(selStart <= selEnd)
                    {
                        SetAPen(rp, penBackgroundSelected);
                        RectFill(rp, selStart, p->destY, selEnd, p->destY + 1);
                        SetAPen(rp, penBgSoundSelected);
                        RectFill(rp, selStart, p->destY + 2, selEnd, p->destHeight - 3);
                        SetAPen(rp, penBackgroundSelected);
                        RectFill(rp, selStart, p->destHeight - 2, selEnd, p->destHeight - 1);
                    }
                }
                /* Part after selection */
                if(pixRight > selPixRight)
                {
                    SetAPen(rp, penBackground);
                    RectFill(rp, selPixRight + 1, p->destY, pixRight, p->destY + 1);
                    SetAPen(rp, penBgSound);
                    RectFill(rp, selPixRight + 1, p->destY + 2, pixRight, p->destHeight - 3);
                    SetAPen(rp, penBackground);
                    RectFill(rp, selPixRight + 1, p->destHeight - 2, pixRight, p->destHeight - 1);
                }
            }
            else
            {
                /* No selection intersection - normal colors */
                SetAPen(rp, penBackground);
                RectFill(rp, pixLeft, p->destY, pixRight, p->destY + 1);
                SetAPen(rp, penBgSound);
                RectFill(rp, pixLeft, p->destY + 2, pixRight, p->destHeight - 3);
                SetAPen(rp, penBackground);
                RectFill(rp, pixLeft, p->destHeight - 2, pixRight, p->destHeight - 1);
            }

            currentX = pixRight + 1;
        }

        /* Fill remaining background after last sound */
        if(currentX <= tileRight)
        {
            LONG gapLeft = currentX;
            LONG gapRight = tileRight;

            if(selectionActive && gapRight >= selPixLeft && gapLeft <= selPixRight)
            {
                /* Gap intersects selection - split into up to 3 parts */
                /* Part before selection */
                if(gapLeft < selPixLeft)
                {
                    SetAPen(rp, penBackground);
                    RectFill(rp, gapLeft, p->destY, selPixLeft - 1, p->destHeight - 1);
                }
                /* Part within selection */
                {
                    LONG selStart = (gapLeft > selPixLeft) ? gapLeft : selPixLeft;
                    LONG selEnd = (gapRight < selPixRight) ? gapRight : selPixRight;
                    if(selStart <= selEnd)
                    {
                        SetAPen(rp, penBackgroundSelected);
                        RectFill(rp, selStart, p->destY, selEnd, p->destHeight - 1);
                    }
                }
                /* Part after selection */
                if(gapRight > selPixRight)
                {
                    SetAPen(rp, penBackground);
                    RectFill(rp, selPixRight + 1, p->destY, gapRight, p->destHeight - 1);
                }
            }
            else
            {
                /* No selection intersection - normal background */
                SetAPen(rp, penBackground);
                RectFill(rp, gapLeft, p->destY, gapRight, p->destHeight - 1);
            }
        }
    }

    /* Draw bottom separator line */
    SetAPen(rp, 2);
    Move(rp, p->destX, p->destHeight - 1);
    Draw(rp, p->destWidth - 1, p->destHeight - 1);
}

