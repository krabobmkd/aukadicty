
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/layers.h>

#include <string.h>  /* For strlen */

#ifdef __SASC
    #include <clib/alib_protos.h>
#else
    /* GCC */
    #include "minialib.h"
#endif

#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/gadgetclass.h>
#include <utility/tagitem.h>

#include "class_timerule.h"
#include "class_timerule_private.h"
#include "../aukstylesheet.h"

/* Include InfiniteScroll private for access to superclass data */
#include "../InfiniteScroll/class_infinitescroll_private.h"

#include "bdbprintf.h"
extern struct IClass   *TimeRuleClassPtr;
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* GM_DOMAIN */
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
       D->gpd_Domain.Height = 12; // gdata->_defaultHeight;
     }
     else
     {
       D->gpd_Domain.Width = 200;
       D->gpd_Domain.Height = 16;
     }
     break;

    case GDOMAIN_MAXIMUM:
      D->gpd_Domain.Width = 16000;
      D->gpd_Domain.Height =  16;
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
  bdbprintf("TimeRule_Layout\n");
  /* Lines are drawn from bottom - make them short */
  gdata->majorTickHeight = Gad->Height/3;
  gdata->minorTickHeight = Gad->Height/6;

   return DoSuperMethodA(C,Gad,(Msg)layout);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* Time Formatting Helper */
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/**
 * Format a time value (64-bit fixed point, integer part is seconds) as text.
 * Outputs format like "0:00", "1:23", "12:34" etc.
 * For sub-second graduations, adds milliseconds: "0:00.1", "0:00.5"
 *
 * @param timeHi   High 32 bits of time value
 * @param timeLo   Low 32 bits of time value (fractional part in upper bits)
 * @param buffer   Output buffer (must be at least 12 chars)
 * @param showMs   If TRUE, show one decimal for sub-second
 */
void TimeRule_FormatTime(LONG timeHi, LONG timeLo, char *buffer, BOOL showMs)
{
    LONG seconds;
    LONG minutes;
    LONG ms;
    char *p = buffer;

    /* Get integer seconds from fixed-point */
    /* timeHi contains the integer part for values >= 0 */
    /* For negative values, we would need special handling */
    seconds = timeHi;
    if(seconds < 0) seconds = 0;  /* Don't display negative times for now */

    minutes = seconds / 60;
    seconds = seconds % 60;

    /* Format minutes:seconds */
    if(minutes >= 10)
    {
        *p++ = '0' + (minutes / 10);
    }
    *p++ = '0' + (minutes % 10);
    *p++ = ':';
    *p++ = '0' + (seconds / 10);
    *p++ = '0' + (seconds % 10);

    if(showMs)
    {
        /* Get first decimal from fractional part */
        /* timeLo upper bits are fraction, scale to get 0-9 */
        ms = ((ULONG)timeLo >> 28) & 0xF;  /* Get top 4 bits */
        if(ms > 9) ms = 9;
        *p++ = '.';
        *p++ = '0' + ms;
    }

    *p = '\0';
}

void TimeRule_UpdateTimeInterval(TimeRule *gdata)
{

    /* computed for a tppw  */
  long long majorTickInterval;  /* Time between major ticks */
  long long minorTickInterval;  /* Time between minor ticks */
    int nbsubdiv=1;
    const long long minMajorPixels = 64;  /* Minimum pixels between major ticks */
    long long minMajorTime = gdata->_timePerPixelWidth * minMajorPixels;

    /* Round up to nice intervals: 0.1s, 0.5s, 1s, 2s, 5s, 10s, 30s, 1min, 5min... */
    /* Using fixed point: 1 second = 1LL << 32 */
    #define SEC_FP (1LL << 32)

    if(minMajorTime <= SEC_FP / 10)        /* <= 0.1s */
    {
        majorTickInterval = SEC_FP / 10;   /* 0.1 second */
        minorTickInterval = SEC_FP / 100;  /* 0.01 second (10 minor per major) */
    }
    else if(minMajorTime <= SEC_FP / 2)    /* <= 0.5s */
    {
        majorTickInterval = SEC_FP / 2;    /* 0.5 second */
        minorTickInterval = SEC_FP / 10;   /* 0.1 second */

    }
    else if(minMajorTime <= SEC_FP)        /* <= 1s */
    {
        majorTickInterval = SEC_FP;        /* 1 second */
        minorTickInterval = SEC_FP / 5;    /* 0.2 second */
    }
    else if(minMajorTime <= SEC_FP * 2)    /* <= 2s */
    {
        majorTickInterval = SEC_FP * 2;    /* 2 seconds */
        minorTickInterval = SEC_FP / 2;    /* 0.5 second */
    }
    else if(minMajorTime <= SEC_FP * 5)    /* <= 5s */
    {
        majorTickInterval = SEC_FP * 5;    /* 5 seconds */
        minorTickInterval = SEC_FP;        /* 1 second */
    }
    else if(minMajorTime <= SEC_FP * 10)   /* <= 10s */
    {
        majorTickInterval = SEC_FP * 10;   /* 10 seconds */
        minorTickInterval = SEC_FP * 2;    /* 2 seconds */
    }
    else if(minMajorTime <= SEC_FP * 30)   /* <= 30s */
    {
        majorTickInterval = SEC_FP * 30;   /* 30 seconds */
        minorTickInterval = SEC_FP * 5;    /* 5 seconds */
    }
    else if(minMajorTime <= SEC_FP * 60)   /* <= 1min */
    {
        majorTickInterval = SEC_FP * 60;   /* 1 minute */
        minorTickInterval = SEC_FP * 10;   /* 10 seconds */
    }
    else
    {
        majorTickInterval = SEC_FP * 300;  /* 5 minutes */
        minorTickInterval = SEC_FP * 60;   /* 1 minute */
    }

    gdata->majorTickInterval = majorTickInterval;
    gdata->minorTickInterval = minorTickInterval;
    gdata->tickSubDiv = majorTickInterval/minorTickInterval;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* GM_INFINITESCROLL_RENDERTILE - Override to draw time graduations */
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/**
 * Render a tile with time graduations.
 *
 * The TimeRule displays a time scale matching TrackListArea's horizontal scroll.
 * - Major ticks with numbers every N seconds (N depends on zoom)
 * - Minor ticks between major ticks
 *
 * Drawing coordinate system:
 * NOW
 * - Tile RastPort is at 0,0, size is TileWidth x TileHeight
 * - AbstractPos is the time value at the LEFT edge of this tile
 */
void TimeRule_RenderDelegate(InfiniteScrollRenderParams *p)
{
    int iloop=0;
    LONG px;
    TimeRule *gdata;
     struct TextFont *font=NULL;
    /* Time range for this TimeRule (from attributes) */
    long long timeLeft;
    /* Drawing parameters */
    long long timePerPixel;
    long long currentTime,currentTimeMin;
    char timeBuf[16];
//    return;

    struct Gadget *Gad = p->Gad;
    struct RastPort *rp = p->rp;
    bdbprintf("TimeRule_RenderDelegate %08x %08x\n",(int)Gad,(int)rp);
    if( !Gad || !rp) return ;

    gdata = INST_DATA(TimeRuleClassPtr, Gad); // note superclass can't send final class pointer implementation

//    bdbprintf("render gdata: %08x\n",(int)gdata);

     /* Get time range from TimeRule attributes */
     timePerPixel = gdata->_timePerPixelWidth;

bdbprintf("render timePerPixel: %08x.%08x\n",(int)(timePerPixel>>32),(int)timePerPixel);
bdbprintf("render minorTickInterval: %08x.%08x\n",(int)(gdata->minorTickInterval>>32),(int)gdata->minorTickInterval);
bdbprintf("render majorTickInterval: %08x.%08x\n",(int)(gdata->majorTickInterval>>32),(int)gdata->majorTickInterval);
 // 0.33
 // 1.00

    if( timePerPixel == 0 ||
       gdata->minorTickInterval ==0 || gdata->majorTickInterval==0 ) return;

    /* Clear tile to background (pen 0 = typically grey) */
    SetAPen(rp, 0 /*+ ((int)p->_start._scrollx)/128*/);
    SetBPen(rp, 0);
    RectFill(rp, p->destX, p->destY, p->destWidth - 1, p->destHeight - 1);

     timeLeft = timePerPixel * p->_start._scrollx;

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

     /* Use fontTiny if available from stylesheet */
     bdbprintf("render f stylesheet:%08x\n",(int)gdata->_styleSheet);
     if(gdata->_styleSheet && gdata->_styleSheet->fontTiny)
     {
     //re    font = gdata->_styleSheet->fontTiny;
      // bdbprintf("render f fontTiny:%08x\n",(int)font);
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
         /* IMPORTANT RP with no SetFont() would crash when Text() used.  */
        if(font) {
             LONG seconds = (LONG)(currentTime >> 32);
             LONG timeLo = (LONG)(currentTime & 0xFFFFFFFF);
             BOOL showMs = (gdata->majorTickInterval < SEC_FP);
             UWORD textY;

             TimeRule_FormatTime(seconds, timeLo, timeBuf, showMs);

            textY = p->destHeight - 2;

             /* Draw text - position adjusted for text width */
            Move(rp, px + 2, textY);
            Text(rp, timeBuf, strlen(timeBuf));

         }


         // loop per subtick
         currentTimeMin = currentTime + gdata->minorTickInterval;
        for(iminortick=1 ; iminortick < gdata->tickSubDiv ; iminortick++ )
        {
            px = (LONG)(((currentTimeMin) / timePerPixel) - p->_start._scrollx);
            if(px >= (p->destWidth)) break;
             /* Minor tick - draw shorter line */
             Move(rp, px, p->destHeight - gdata->minorTickHeight);
             Draw(rp, px, p->destHeight - 1);
             currentTimeMin += gdata->minorTickInterval;
        }

        currentTime += gdata->majorTickInterval;
        iloop++;
        if(iloop>8) break; // safety
        /* Convert time offset to pixel position within tile */
        px = (LONG)((currentTime / timePerPixel) - p->_start._scrollx);

       // break;
     }

}

