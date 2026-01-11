
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

#include "class_volumerule.h"
#include "class_volumerule_private.h"
#include "../aukstyle.h"

#include "bdbprintf.h"

/**
 * Handle GM_DOMAIN - report size constraints
 */
ULONG VolumeRule_Domain(Class *C, struct Gadget *Gad, struct gpDomain *D)
{
    D->gpd_Domain.Left = 0;
    D->gpd_Domain.Top = 0;

    switch(D->gpd_Which)
    {
        case GDOMAIN_NOMINAL:
            D->gpd_Domain.Width = 32;
            D->gpd_Domain.Height = 100;
            break;

        case GDOMAIN_MAXIMUM:
            D->gpd_Domain.Width = 48;
            D->gpd_Domain.Height = 16000;
            break;

        case GDOMAIN_MINIMUM:
        default:
            D->gpd_Domain.Width = 24;
            D->gpd_Domain.Height = 40;
            break;
    }
    return 1;
}

/**
 * Handle GM_RENDER - draw the volume scale
 *
 * Draws a vertical volume scale with marks at -1, -0.5, 0, 0.5, 1.0
 * The gadget height represents the volume range [-1, 1].
 * Value 0 is at center, +1 at top, -1 at bottom.
 */
ULONG VolumeRule_Render(Class *C, struct Gadget *Gad, struct gpRender *R)
{
    VolumeRule *gdata;
    struct RastPort *rp;
    struct TextFont *font = NULL;
    WORD left, top, width, height;
    WORD centerY, topY, bottomY;
    WORD textX, lineEndX;
    WORD tickLen = 4;

    /* Volume mark definitions: value, label */
    static const struct {
        int valueTimes2;  /* -2 = -1.0, -1 = -0.5, 0 = 0, 1 = 0.5, 2 = 1.0 */
        const char *label;
    } marks[] = {
        {  2, " 1.0" },
        {  1, " 0.5" },
        {  0, "  0" },
        { -1, "-0.5" },
        { -2, "-1.0" }
    };
    int numMarks = 5;
    int i;

    gdata = INST_DATA(C, Gad);
    rp = R->gpr_RPort;

    if(!rp) return 0;

    /* Get gadget bounds */
    left = Gad->LeftEdge;
    top = Gad->TopEdge;
    width = Gad->Width;
    height = Gad->Height;

    /* Calculate key Y positions */
    centerY = top + height / 2;
    topY = top;
    bottomY = top + height - 1;

    /* X positions: text on left, line on right edge */
    textX = left + 2;
    lineEndX = left + width - 1;

    /* Clear background */
    SetAPen(rp, 0);
    SetBPen(rp, 0);
    RectFill(rp, left, top, left + width - 1, top + height - 1);

    /* Set pen for drawing lines and text */
    SetAPen(rp, 1);

    /* Use tiny font from style if available */
    if(gdata->_style && gdata->_style->fontTiny)
    {
        font = gdata->_style->fontTiny;
        SetFont(rp, font);
    }

    /* Draw each volume mark */
    for(i = 0; i < numMarks; i++)
    {
        WORD markY;
        WORD lineStartX;

        /* Calculate Y position for this value
         * valueTimes2: -2 to +2 maps to bottom to top
         * +2 (1.0) -> top, -2 (-1.0) -> bottom, 0 -> center
         */
        markY = centerY - ((marks[i].valueTimes2 * (height / 2)) / 2);

        /* Clamp to gadget bounds */
        if(markY < top) markY = top;
        if(markY > bottomY) markY = bottomY;

        /* Draw horizontal tick line */
        if(marks[i].valueTimes2 == 0)
        {
            /* Center line (0) is longer */
            lineStartX = lineEndX - (tickLen*2);
        }
        else
        {
            lineStartX = lineEndX - tickLen;
        }

        Move(rp, lineStartX, markY);
        Draw(rp, lineEndX, markY);

        /* Draw label text */
        if(font)
        {
            WORD textY = markY + (gdata->_style->fontHeight / 2) - 1;
            WORD textLen = strlen(marks[i].label);

            /* Adjust text position so it doesn't go outside bounds */
            if(textY < top + gdata->_style->fontHeight + 1)
                textY = top + gdata->_style->fontHeight + 1;
            if(textY > bottomY-3)
                textY = bottomY-3;

            Move(rp, textX, textY);
            Text(rp, marks[i].label, textLen);
        }
    }

    /* Draw vertical line at right edge */
    Move(rp, lineEndX, topY);
    Draw(rp, lineEndX, bottomY);

    /* Also need bottom */
    Move(rp, left, bottomY);
    Draw(rp, left+width-1, bottomY);

    return 1;
}
