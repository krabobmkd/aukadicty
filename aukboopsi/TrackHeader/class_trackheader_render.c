
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/layers.h>


#include <clib/alib_protos.h>

#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/gadgetclass.h>
#include <utility/tagitem.h>

#include "class_trackheader.h"
#include "class_trackheader_private.h"

#ifdef USE_BEVEL_FRAME
    #include <proto/bevel.h>
    #include <images/bevel.h>
#endif

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

ULONG TrackHeader_Domain(Class *C, struct Gadget *Gad, struct gpDomain *D)
{
  TrackHeader *gdata=0;

  if(Gad) gdata=INST_DATA(C, Gad);
// Printf("TrackHeader_Domain data:%lx\n",(int)gdata);

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
      D->gpd_Domain.Width=16000;
      D->gpd_Domain.Height=16000;
      break;

    case GDOMAIN_MINIMUM:
    default:

        D->gpd_Domain.Width=  128;
        D->gpd_Domain.Height= 70;

      break;

  }
  return(1);
}

/**
 * method GM_LAYOUT
 * Manually position all child gadgets by setting TopEdge, LeftEdge, Width, Height.
 * This replaces the nested layout approach that caused bugs.
 *
 * Layout structure:
 * +-------------------------------------------+--------+
 * | [X] [Track Name                         ] |        |
 * +-------------------------------------------+ Volume |
 * | [Sil.]  [Solo]                            |  Rule  |
 * +-------------------------------------------+        |
 * | Vol. [==========slider==============]     |        |
 * +-------------------------------------------+        |
 * | Pan  [==========slider==============]     |        |
 * +-------------------------------------------+        |
 * | Mono 22050Hz                              |        |
 * +-------------------------------------------+--------+
 */
ULONG TrackHeader_Layout(Class *C, struct Gadget *Gad, struct gpLayout *layout)
{
  TrackHeader *gdata;
  LONG topedge,leftedge,width,height;
  LONG leftPartWidth, volumeRuleWidth;
  LONG rowHeight;
  LONG curY;
  LONG closeW, labelW, sliderX, sliderW;
  struct Gadget *sub;

    gdata=INST_DATA(C, Gad);

    //bdbprintf(" $$$ TrackHeader_Layout\n");

    topedge = Gad->TopEdge;
    leftedge = Gad->LeftEdge;
    width = Gad->Width;
    height = Gad->Height;

    /* Store frame rectangle */
    gdata->_framerec.MinX = leftedge+1;
    gdata->_framerec.MinY = topedge+1;
    gdata->_framerec.MaxX = leftedge + width  -2;
    gdata->_framerec.MaxY = topedge  + height -2;

    /* Layout constants */
    volumeRuleWidth = 32;
    leftPartWidth = width - volumeRuleWidth;
    rowHeight = height / 5;  /* 5 rows */
    closeW = 18;
    labelW = 28;  /* Width for "Vol." and "Pan" labels */
    sliderX = leftedge + labelW;
    sliderW = leftPartWidth - labelW - 2;

    curY = topedge;

    /* Row 1: Close button and Name button */
    sub = (struct Gadget *)gdata->subs[THS_CloseButton];
    if(sub)
    {
        sub->LeftEdge = leftedge + 1;
        sub->TopEdge = curY;
        sub->Width = closeW;
        sub->Height = rowHeight;
        DoMethodA((Object*)sub, (Msg)layout);
    }

    sub = (struct Gadget *)gdata->subs[THS_NameButton];
    if(sub)
    {
        sub->LeftEdge = leftedge + closeW + 2;
        sub->TopEdge = curY;
        sub->Width = leftPartWidth - closeW - 3;
        sub->Height = rowHeight;
        DoMethodA((Object*)sub, (Msg)layout);
    }
    curY += rowHeight;

    /* Row 2: Silencer and Solo buttons */
    sub = (struct Gadget *)gdata->subs[THS_SilencerBt];
    if(sub)
    {
        sub->LeftEdge = leftedge + 1;
        sub->TopEdge = curY;
        sub->Width = (leftPartWidth - 3) / 2;
        sub->Height = rowHeight;
        DoMethodA((Object*)sub, (Msg)layout);
    }

    sub = (struct Gadget *)gdata->subs[THS_SoloBt];
    if(sub)
    {
        sub->LeftEdge = leftedge + 1 + (leftPartWidth - 3) / 2 + 1;
        sub->TopEdge = curY;
        sub->Width = (leftPartWidth - 3) / 2;
        sub->Height = rowHeight;
        DoMethodA((Object*)sub, (Msg)layout);
    }
    curY += rowHeight;

    /* Row 3: Vol label and slider */
    sub = (struct Gadget *)gdata->subs[THS_VolLabel];
    if(sub)
    {
        sub->LeftEdge = leftedge + 1;
        sub->TopEdge = curY;
        sub->Width = labelW;
        sub->Height = rowHeight;
        DoMethodA((Object*)sub, (Msg)layout);
    }

    sub = (struct Gadget *)gdata->subs[THS_VolumeSlider];
    if(sub)
    {
        sub->LeftEdge = sliderX;
        sub->TopEdge = curY;
        sub->Width = sliderW;
        sub->Height = rowHeight;
        DoMethodA((Object*)sub, (Msg)layout);
    }
    curY += rowHeight;

    /* Row 4: Pan label and slider */
    sub = (struct Gadget *)gdata->subs[THS_PanLabel];
    if(sub)
    {
        sub->LeftEdge = leftedge + 1;
        sub->TopEdge = curY;
        sub->Width = labelW;
        sub->Height = rowHeight;
        DoMethodA((Object*)sub, (Msg)layout);
    }

    sub = (struct Gadget *)gdata->subs[THS_PanSlider];
    if(sub)
    {
        sub->LeftEdge = sliderX;
        sub->TopEdge = curY;
        sub->Width = sliderW;
        sub->Height = rowHeight;
        DoMethodA((Object*)sub, (Msg)layout);
    }
    curY += rowHeight;

    /* Row 5: Info label (takes remaining height) */
    sub = (struct Gadget *)gdata->subs[THS_InfoLabel];
    if(sub)
    {
        sub->LeftEdge = leftedge + 1;
        sub->TopEdge = curY;
        sub->Width = leftPartWidth - 2;
        sub->Height = topedge + height - curY;
        DoMethodA((Object*)sub, (Msg)layout);
    }

    /* Right side: VolumeRule (full height) */
    sub = (struct Gadget *)gdata->subs[THS_VolumeRule];
    if(sub)
    {
        sub->LeftEdge = leftedge + leftPartWidth;
        sub->TopEdge = topedge;
        sub->Width = volumeRuleWidth;
        sub->Height = height;
        DoMethodA((Object*)sub, (Msg)layout);
    }

  return(1);
}

/* draw yourself, in the appropriate state */
ULONG TrackHeader_Render(Class *C, struct Gadget *Gad, struct gpRender *Render)
{
    LONG topedge,leftedge,width,height;
    TrackHeader *gdata;
    struct RastPort *rp;
    int i;
    ULONG retval=1;

    gdata=INST_DATA(C, Gad);

//    // also sent from GM_GOINACTIVE (4).
//    if(Render->MethodID!=GM_RENDER || !Render->gpr_RPort) return 1;

//    rp = Render->gpr_RPort;

//    topedge = Gad->TopEdge;
//    leftedge = Gad->LeftEdge;
//    width = Gad->Width;
//    height = Gad->Height;

//    gdata->_framerec.MinX = leftedge+1;
//    gdata->_framerec.MinY = topedge+1;
//    gdata->_framerec.MaxX = leftedge + width  -2;
//    gdata->_framerec.MaxY = topedge  + height -2;

//      SetDrMd(rp,JAM1);
//      SetAPen(rp,penbg);
//      RectFill(rp,gdata->_framerec.MinX,
//                  gdata->_framerec.MinY,
//                  gdata->_framerec.MaxX,
//                  gdata->_framerec.MaxY) ;

//    /* Forward GM_RENDER to all child gadgets */
//    for(i=0; i<THS_Total; i++)
//    {
//        if(gdata->subs[i])
//        {
//            DoMethodA((Object*)gdata->subs[i], (Msg)Render);
//        }
//    }



  return(retval);
}
