
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/layers.h>

#include <string.h>  /* For strlen */
#include <stdio.h>   /* For sprintf */


#include <clib/alib_protos.h>

#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/gadgetclass.h>
#include <utility/tagitem.h>

#include "class_timerule.h"
#include "class_timerule_private.h"
#include "../aukstyle.h"
#include "aukselection.h"

/* Include InfiniteScroll private for access to superclass data */
#include "../InfiniteScroll/class_infinitescroll_private.h"

#include "bdbprintf.h"
extern struct IClass   *TimeRuleClassPtr;
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* GM_DOMAIN - Where we tell Intuition how big we'd LIKE to be.           */
/* Spoiler: Intuition doesn't care about our feelings.                    */
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

ULONG TimeRule_Domain(Class *C, struct Gadget *Gad, struct gpDomain *D)
{
  TimeRule *gdata=0;

  if(Gad) gdata=INST_DATA(C, Gad);

  D->gpd_Domain.Left=0;
  D->gpd_Domain.Top=0;

  switch(D->gpd_Which)
  {
    case GDOMAIN_NOMINAL:
     if(gdata)
     {
       D->gpd_Domain.Width = 200;
       D->gpd_Domain.Height = 14; // gdata->_defaultHeight;
     }
     else
     {
       D->gpd_Domain.Width = 200;
       D->gpd_Domain.Height = 16;
     }
     break;

    case GDOMAIN_MAXIMUM:
      D->gpd_Domain.Width = 16000;  /* Dream big, little gadget */
      D->gpd_Domain.Height =  24;   /* But not TOO big, we're not barbarians */
      break;

    case GDOMAIN_MINIMUM:
    default:

       D->gpd_Domain.Width = 50;
       D->gpd_Domain.Height = 12;

     break;
  }
  return(1);
}

ULONG TimeRule_Layout(Class *C, struct Gadget *Gad, struct gpLayout *layout)
{
  TimeRule *gdata=0;
  gdata=INST_DATA(C, Gad);
//  bdbprintf("TimeRule_Layout\n");  /* Commented out like my social life */
  /* Lines are drawn from bottom - make them short.
   * The math below was derived through the ancient art of
   * "tweak until it looks right on my monitor" */
  gdata->majorTickHeight = (Gad->Height/3)+2;  /* The boss tick */
  gdata->minorTickHeight = (Gad->Height/6)+1;  /* The intern tick */

   return DoSuperMethodA(C,Gad,(Msg)layout);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* Time Formatting Helper */
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* Fixed-point time constants - because floats are for people with FPUs,
 * and we're coding like it's 1992 (because it basically is on Amiga) */
#define SEC_FP      (1LL << 32)                 /* 1 second - feels like an eternity on 7MHz */
#define MS_FP       (SEC_FP / 1000)             /* 1 millisecond - blink and you'll miss 14 of them */
#define US_FP       (SEC_FP / 1000000)          /* 1 microsecond - the 68000 just felt that */

/* Time scale enumeration for formatting
 * From "blink of an electron's eye" to "time to make coffee" */
typedef enum {
    TIMESCALE_USEC,     /* Microseconds: for when you REALLY zoomed in */
    TIMESCALE_MSEC,     /* Milliseconds: the sweet spot of audio editing */
    TIMESCALE_SEC,      /* Seconds: patience, young padawan */
    TIMESCALE_MIN       /* Minutes: go touch grass, you've zoomed out too far */
} TimeScale;

/**
 * Format a time value with appropriate unit suffix.
 * Outputs formats like "100µs", "50ms", "1.5s", "2m30s"
 *
 * @param stime      Time value in 32.32 fixed-point (integer part = seconds)
 * @param buffer     Output buffer (must be at least 16 chars)
 * @param scale      The scale to use for formatting
 */
void TimeRule_FormatTime(long long stime, char *buffer, int scale)
{
    char *p = buffer;
    int negative = 0;
    ULONG seconds, minutes, hours;
    (void)p;  /* Suppress unused warning if buffer manipulation changes */

    if(stime < 0)
    {
        negative = 1;
        stime = -stime;
    }

    /* Extract integer seconds */
    seconds = (ULONG)(stime >> 32);

    minutes = seconds / 60;
    seconds = seconds % 60;
    hours = minutes / 60;
    minutes = minutes % 60;

    if(negative) *p++ = '-';

    switch(scale)
    {
        case TIMESCALE_USEC:
        {
            /* Format as microseconds - timePerPixel determined we need µs precision,
             * so that's what we show. Always. No second-guessing the zoom level. */
            /* Add 0x80000000 for rounding before shift */
            ULONG totalUs = (ULONG)(((stime * 1000000ULL) + 0x80000000ULL) >> 32);

            p += sprintf(p, "%lu", (unsigned long)totalUs);
            /* µ is 0xB5 in ISO-8859-1 / Amiga charset */
            *p++ = (char)0xB5;
            *p++ = 's';
            break;
        }

        case TIMESCALE_MSEC:
        {
            /* Format as milliseconds - ALWAYS show ms at this zoom level
             * because that's what the user zoomed in to see! */
            /* Add 0x80000000 for rounding before shift */
            ULONG totalMs = (ULONG)(((stime * 1000ULL) + 0x80000000ULL) >> 32);

            /* Always show full milliseconds: "7500ms" not "7.5s" */
            p += sprintf(p, "%lu", (unsigned long)totalMs);
            *p++ = 'm';
            *p++ = 's';
            break;
        }

        case TIMESCALE_SEC:
        {
            /* Format as seconds with s suffix */
            if(minutes > 0 || hours > 0)
            {
                /* Show minutes:seconds format */
                if(hours > 0)
                {
                    p += sprintf(p, "%lu", (unsigned long)hours);
                    *p++ = 'h';
                }
                if(minutes > 0 || hours > 0)
                {
                    p += sprintf(p, "%lu", (unsigned long)minutes);
                    *p++ = 'm';
                }
                if(seconds > 0)
                {
                    p += sprintf(p, "%lu", (unsigned long)seconds);
                    *p++ = 's';
                }
            }
            else
            {
                /* Just seconds */
                ULONG totalSec = (ULONG)(stime >> 32);
                /* Add 0x80000000 for rounding before shift */
                ULONG fracTenth = (ULONG)((((stime & 0xFFFFFFFFULL) * 10) + 0x80000000ULL) >> 32);
                if(fracTenth > 0 && totalSec < 10)
                {
                    p += sprintf(p, "%lu.%lu", (unsigned long)totalSec, (unsigned long)fracTenth);
                }
                else
                {
                    p += sprintf(p, "%lu", (unsigned long)totalSec);
                }
                *p++ = 's';
            }
            break;
        }

        case TIMESCALE_MIN:
        {
            /* Format as minutes with m suffix */
            ULONG totalMin = (ULONG)(stime >> 32) / 60;
            ULONG remainSec = (ULONG)(stime >> 32) % 60;

            if(hours > 0)
            {
                p += sprintf(p, "%lu", (unsigned long)hours);
                *p++ = 'h';
                if(minutes > 0)
                {
                    p += sprintf(p, "%lu", (unsigned long)minutes);
                    *p++ = 'm';
                }
            }
            else
            {
                p += sprintf(p, "%lu", (unsigned long)totalMin);
                *p++ = 'm';
                if(remainSec > 0 && totalMin < 10)
                {
                    p += sprintf(p, "%lu", (unsigned long)remainSec);
                    *p++ = 's';
                }
            }
            break;
        }
    }

    *p = '\0';
}

/**
 * Update tick intervals based on current zoom level (_timePerPixelWidth).
 * Extended to support microsecond through hour scales.
 *
 * For a 44100Hz sample to be 8 pixels wide:
 * sampleDuration = 1/44100 s ≈ 22.68 µs
 * timePerPixel = 22.68µs / 8 ≈ 2.83 µs
 *
 * Fun fact: this function has more if-else branches than a
 * choose-your-own-adventure book from the 80s.
 */
void TimeRule_UpdateTimeInterval(TimeRule *gdata)
{
    long long majorTickInterval;
    long long minorTickInterval;
    TimeScale scale;
    const long long minMajorPixels = 64;  /* Minimum pixels between major ticks */
    long long minMajorTime = gdata->_timePerPixelWidth * minMajorPixels;

    /* The great ladder of time scales - from quantum to coffee break.
     * Warning: the following code was written by someone who clearly
     * enjoys typing the same pattern 15 times. */
    /* Microsecond scales (for when you want to see individual electrons party) */
    if(minMajorTime <= US_FP * 10)              /* <= 10µs */
    {
        majorTickInterval = US_FP * 10;         /* 10 microseconds */
        minorTickInterval = US_FP * 2;          /* 2 microseconds */
        scale = TIMESCALE_USEC;
    }
    else if(minMajorTime <= US_FP * 50)         /* <= 50µs */
    {
        majorTickInterval = US_FP * 50;         /* 50 microseconds */
        minorTickInterval = US_FP * 10;         /* 10 microseconds */
        scale = TIMESCALE_USEC;
    }
    else if(minMajorTime <= US_FP * 100)        /* <= 100µs */
    {
        majorTickInterval = US_FP * 100;        /* 100 microseconds */
        minorTickInterval = US_FP * 20;         /* 20 microseconds */
        scale = TIMESCALE_USEC;
    }
    else if(minMajorTime <= US_FP * 500)        /* <= 500µs */
    {
        majorTickInterval = US_FP * 500;        /* 500 microseconds */
        minorTickInterval = US_FP * 100;        /* 100 microseconds */
        scale = TIMESCALE_USEC;
    }
    /* Millisecond scales */
    else if(minMajorTime <= MS_FP * 1)          /* <= 1ms */
    {
        majorTickInterval = MS_FP * 1;          /* 1 millisecond */
        minorTickInterval = US_FP * 200;        /* 200 microseconds */
        scale = TIMESCALE_MSEC;
    }
    else if(minMajorTime <= MS_FP * 5)          /* <= 5ms */
    {
        majorTickInterval = MS_FP * 5;          /* 5 milliseconds */
        minorTickInterval = MS_FP * 1;          /* 1 millisecond */
        scale = TIMESCALE_MSEC;
    }
    else if(minMajorTime <= MS_FP * 10)         /* <= 10ms */
    {
        majorTickInterval = MS_FP * 10;         /* 10 milliseconds */
        minorTickInterval = MS_FP * 2;          /* 2 milliseconds */
        scale = TIMESCALE_MSEC;
    }
    else if(minMajorTime <= MS_FP * 50)         /* <= 50ms */
    {
        majorTickInterval = MS_FP * 50;         /* 50 milliseconds */
        minorTickInterval = MS_FP * 10;         /* 10 milliseconds */
        scale = TIMESCALE_MSEC;
    }
    else if(minMajorTime <= MS_FP * 100)        /* <= 100ms */
    {
        majorTickInterval = MS_FP * 100;        /* 100 milliseconds */
        minorTickInterval = MS_FP * 20;         /* 20 milliseconds */
        scale = TIMESCALE_MSEC;
    }
    else if(minMajorTime <= MS_FP * 500)        /* <= 500ms */
    {
        majorTickInterval = MS_FP * 500;        /* 500 milliseconds */
        minorTickInterval = MS_FP * 100;        /* 100 milliseconds */
        scale = TIMESCALE_MSEC;
    }
    /* Second scales */
    else if(minMajorTime <= SEC_FP)             /* <= 1s */
    {
        majorTickInterval = SEC_FP;             /* 1 second */
        minorTickInterval = MS_FP * 200;        /* 200 milliseconds */
        scale = TIMESCALE_SEC;
    }
    else if(minMajorTime <= SEC_FP * 2)         /* <= 2s */
    {
        majorTickInterval = SEC_FP * 2;         /* 2 seconds */
        minorTickInterval = MS_FP * 500;        /* 500 milliseconds */
        scale = TIMESCALE_SEC;
    }
    else if(minMajorTime <= SEC_FP * 5)         /* <= 5s */
    {
        majorTickInterval = SEC_FP * 5;         /* 5 seconds */
        minorTickInterval = SEC_FP;             /* 1 second */
        scale = TIMESCALE_SEC;
    }
    else if(minMajorTime <= SEC_FP * 10)        /* <= 10s */
    {
        majorTickInterval = SEC_FP * 10;        /* 10 seconds */
        minorTickInterval = SEC_FP * 2;         /* 2 seconds */
        scale = TIMESCALE_SEC;
    }
    else if(minMajorTime <= SEC_FP * 30)        /* <= 30s */
    {
        majorTickInterval = SEC_FP * 30;        /* 30 seconds */
        minorTickInterval = SEC_FP * 5;         /* 5 seconds */
        scale = TIMESCALE_SEC;
    }
    /* Minute scales */
    else if(minMajorTime <= SEC_FP * 60)        /* <= 1min */
    {
        majorTickInterval = SEC_FP * 60;        /* 1 minute */
        minorTickInterval = SEC_FP * 10;        /* 10 seconds */
        scale = TIMESCALE_MIN;
    }
    else if(minMajorTime <= SEC_FP * 120)       /* <= 2min */
    {
        majorTickInterval = SEC_FP * 120;       /* 2 minutes */
        minorTickInterval = SEC_FP * 30;        /* 30 seconds */
        scale = TIMESCALE_MIN;
    }
    else if(minMajorTime <= SEC_FP * 300)       /* <= 5min */
    {
        majorTickInterval = SEC_FP * 300;       /* 5 minutes */
        minorTickInterval = SEC_FP * 60;        /* 1 minute */
        scale = TIMESCALE_MIN;
    }
    else if(minMajorTime <= SEC_FP * 600)       /* <= 10min */
    {
        majorTickInterval = SEC_FP * 600;       /* 10 minutes */
        minorTickInterval = SEC_FP * 120;       /* 2 minutes */
        scale = TIMESCALE_MIN;
    }
    else
    {
        /* If you've zoomed out THIS far, you're either editing a symphony
         * or you accidentally scrolled with your elbow */
        majorTickInterval = SEC_FP * 1800;      /* 30 minutes */
        minorTickInterval = SEC_FP * 300;       /* 5 minutes */
        scale = TIMESCALE_MIN;
    }

    gdata->majorTickInterval = majorTickInterval;
    gdata->minorTickInterval = minorTickInterval;
    gdata->tickSubDiv = (int)(majorTickInterval / minorTickInterval);
    gdata->timeScale = scale;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* GM_INFINITESCROLL_RENDERTILE - Override to draw time graduations       */
/* Also known as "the function that draws those tiny lines you never      */
/* consciously notice but would definitely miss if they weren't there"    */
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/**
 * Render a tile with time graduations.
 *
 * The TimeRule displays a time scale matching TrackListArea's horizontal scroll.
 * - Major ticks with numbers every N seconds (N depends on zoom)
 * - Minor ticks between major ticks (the little guys nobody thanks)
 *
 * Drawing coordinate system:
 * NOW - Because "later" is for people who plan ahead
 * - Tile RastPort is at 0,0, size is TileWidth x TileHeight
 * - AbstractPos is the time value at the LEFT edge of this tile
 *
 * Selection rendering:
 * - Mode 1: time span selection draws selected background color
 *   (making your selection feel special and validated)
 */
void TimeRule_RenderDelegate(InfiniteScrollRenderParams *p)
{
    int iloop=0;
    LONG px;
    TimeRule *gdata;
    struct TextFont *font=NULL; /* No font? No problem! We'll just... not draw text. Modern problems. */
    /* Time range for this TimeRule (from attributes) */
    long long timeLeft;
    /* Drawing parameters */
    long long timePerPixel;
    long long currentTime,currentTimeMin;
    char timeBuf[32];
    /* Selection variables */
    AukSelection *selection;
    LONG selPixLeft, selPixRight;
    int selectionActive;
    WORD penBackground, penBackgroundSelected;

    struct Gadget *Gad = p->Gad;
    struct RastPort *rp = p->rp;
    if( !Gad || !rp) return ;

    gdata = INST_DATA(TimeRuleClassPtr, Gad);

    /* Get time range from TimeRule attributes */
    timePerPixel = gdata->_timePerPixelWidth;

    if( timePerPixel == 0 ||
       gdata->minorTickInterval ==0 || gdata->majorTickInterval==0 )
    {
        return;
    }

    /* Get background pens from style */
    penBackground = 0;
    penBackgroundSelected = 0;
    if(gdata->_style)
    {
        if(gdata->_style->selectedBackground.pen != -1)
            penBackgroundSelected = gdata->_style->selectedBackground.pen;
    }

    /* Determine selection state */
    selectionActive = 0;
    selPixLeft = -1;
    selPixRight = -1;
    selection = gdata->_timeSelection;

    if(selection && selection->_mode == 1)
    {
        /* Mode 1: time span selection (for any _itrack value) */
        selPixLeft = (LONG)((selection->_start / timePerPixel) - p->_start._scrollx);
        selPixRight = (LONG)((selection->_end / timePerPixel) - p->_start._scrollx);

        /* If start==end, means cursor - need 1 pixel */
        if(selPixLeft == selPixRight) selPixRight++;

        /* Clamp to tile bounds */
        if(selPixLeft < p->destX) selPixLeft = p->destX;
        if(selPixRight > (LONG)(p->destWidth - 1)) selPixRight = p->destWidth - 1;
        if(selPixLeft <= selPixRight) selectionActive = 1;
    }

    /* Clear tile to background with selection handling */
    SetBPen(rp, penBackground);

    if(selectionActive)
    {
        LONG tileLeft = p->destX;
        LONG tileRight = p->destWidth - 1;

        /* Part before selection */
        if(tileLeft < selPixLeft)
        {
            SetAPen(rp, penBackground);
            RectFill(rp, tileLeft, p->destY, selPixLeft - 1, p->destHeight - 1);
        }
        /* Part within selection */
        {
            LONG selStart = (tileLeft > selPixLeft) ? tileLeft : selPixLeft;
            LONG selEnd = (tileRight < selPixRight) ? tileRight : selPixRight;
            if(selStart <= selEnd)
            {
                SetAPen(rp, penBackgroundSelected);
                RectFill(rp, selStart, p->destY, selEnd, p->destHeight - 1);
            }
        }
        /* Part after selection */
        if(tileRight > selPixRight)
        {
            SetAPen(rp, penBackground);
            RectFill(rp, selPixRight + 1, p->destY, tileRight, p->destHeight - 1);
        }
    }
    else
    {
        /* No selection - fill entire tile with normal background */
        SetAPen(rp, penBackground);
        RectFill(rp, p->destX, p->destY, p->destWidth - 1, p->destHeight - 1);
    }

    timeLeft = timePerPixel * p->_start._scrollx;
//bdbprintf("render timeLeft: %08x.%08x\n",(int)(timeLeft>>32),(int)timeLeft);
     /* Now, Abstract InfiniteScroll only give a pixel offset position for the left border of this tile.

     */
    // tileAbstractPos = p->_start._scrollx;


     /* Draw ticks within this tile */
     /* Find first minor tick at or after tileAbstractPos */
//     currentTime = (timeLeft / gdata->minorTickInterval) * gdata->minorTickInterval;
//     currentTime -= gdata->minorTickInterval; // because may draw text of last tick, it may overdrop

     currentTime = (timeLeft / gdata->majorTickInterval) * gdata->majorTickInterval;
     if(timeLeft<0) currentTime -=  gdata->majorTickInterval;
     currentTime -= gdata->majorTickInterval; // because may draw text of last tick, it may overdrop

     /* Set pen for tick marks (pen 1 = typically dark) */
     SetAPen(rp, 1);

     /* Use fontTiny if available from style */

     if(gdata->_style && gdata->_style->fontTiny)
     {
       font = gdata->_style->fontTiny;
     }
     if(font) SetFont(rp, font);

    /* Convert time offset to pixel position within tile */
    px = (LONG)((currentTime / timePerPixel) -p->_start._scrollx);

     while(1)
     {
         int iminortick;
         if(px >= (p->destWidth)) break;  /* Past end of tile  */

         if(px >= 0)
         {
             /* Major tick - draw full height line */
             Move(rp, px, p->destHeight - gdata->majorTickHeight);
             Draw(rp, px, p->destHeight - 1);
         }

         /* Draw time text near bottom of tile */
         /* IMPORTANT: Calling Text() without SetFont() is like asking
          * a mime to read Shakespeare - it will NOT end well. Guru Meditation awaits. */
        if(font) {
             UWORD textY;

             TimeRule_FormatTime(currentTime, timeBuf, gdata->timeScale);

            textY = p->destHeight - 4;

             /* Draw text - position adjusted for text width */
            Move(rp, px + 2, textY);
            Text(rp, timeBuf, strlen(timeBuf));

         }


         /* Now draw the minor ticks - the unsung heroes of time visualization.
          * They don't get numbers, they don't get glory, but without them
          * the timeline would look like a sad picket fence. */
         currentTimeMin = currentTime + gdata->minorTickInterval;
        for(iminortick=1 ; iminortick < gdata->tickSubDiv ; iminortick++ )
        {
            px = (LONG)(((currentTimeMin) / timePerPixel) - p->_start._scrollx);
            if(px >= (p->destWidth)) break;
             /* Minor tick - shorter than major, like a little sibling */
             Move(rp, px, p->destHeight - gdata->minorTickHeight);
             Draw(rp, px, p->destHeight - 1);
             currentTimeMin += gdata->minorTickInterval;
        }

        currentTime += gdata->majorTickInterval;
        iloop++;
        if(iloop>8) break; /* Safety valve: if we're drawing more than 8 major ticks
                            * per tile, something has gone horribly wrong, or the user
                            * has a 4K monitor and we need to have a serious talk. */
        /* Convert time offset to pixel position within tile */
        px = (LONG)((currentTime / timePerPixel) - p->_start._scrollx);

       // break;
     }

}

