
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
 */
void TrackArea_RenderDelegate(InfiniteScrollRenderParams *p)
{
    TrackArea *gdata;
    int penbg = 1;
    struct Gadget *Gad = p->Gad;
    struct RastPort *rp = p->rp;
    TimeProjection *proj;
    AukTrack *track;
    AukArray *sounds;
    unsigned int soundCount, i;
    long long timePerPixel;
    long long tileLeftTime, tileRightTime;
    AukStyle *style = gdata->_style;
    WORD penBackground,penBgSound;
    if(!Gad || !rp) return;

    gdata = INST_DATA(TrackAreaClassPtr, Gad);

    /* Style concern... */
    style = gdata->_style;
    penBackground = (style->background.pen!=-1)?style->background.pen:1;
    penBgSound = (style->soundBackground.pen!=-1)?style->soundBackground.pen:2;

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
            if(pixLeft >= pixRight) continue;

            /* Fill background gap before this sound */
            if(currentX < pixLeft)
            {
                SetAPen(rp, penBackground);
                RectFill(rp, currentX, p->destY, pixLeft - 1, p->destHeight - 1);
            }

            /* Draw sound rectangle (with 2px margin top/bottom for visibility) */
            SetAPen(rp, penBackground);
            RectFill(rp, pixLeft, p->destY, pixRight, p->destY + 1);
            SetAPen(rp, penBgSound);
            RectFill(rp, pixLeft, p->destY + 2, pixRight, p->destHeight - 3);
            SetAPen(rp, penBackground);
            RectFill(rp, pixLeft, p->destHeight - 2, pixRight, p->destHeight - 1);

            currentX = pixRight + 1;
        }

        /* Fill remaining background after last sound */
        if(currentX <= tileRight)
        {
            SetAPen(rp, penBackground);
            RectFill(rp, currentX, p->destY, tileRight, p->destHeight - 1);
        }
    }

    /* Draw bottom separator line */
    SetAPen(rp, 2);
    Move(rp, p->destX, p->destHeight - 1);
    Draw(rp, p->destWidth - 1, p->destHeight - 1);
}

