
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/layers.h>


#include <clib/alib_protos.h>

#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/gadgetclass.h>
#include <utility/tagitem.h>
#include <intuition/screens.h>
#include <graphics/text.h>

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

/*doesnt work
static int getHeight(Object *sub,struct gpDomain *D,int which)
 {
    ULONG tagend=0;
     struct gpDomain gpdmin;
    gpdmin.MethodID = GM_DOMAIN;
    gpdmin.gpd_GInfo = D->gpd_GInfo;
    gpdmin.gpd_RPort = D->gpd_RPort;
    gpdmin.gpd_Which = which;
    gpdmin.gpd_Attrs = &tagend;

    DoMethodA((Object*)sub,(Msg)&gpdmin);
    return (int) gpdmin.gpd_Domain.Height;
 }
*/

ULONG TrackHeader_Domain(Class *C, struct Gadget *Gad, struct gpDomain *D)
{
  struct Gadget *sub;
  TrackHeader *gdata=0;

  if(Gad) gdata=INST_DATA(C, Gad);
// Printf("TrackHeader_Domain data:%lx\n",(int)gdata);

  D->gpd_Domain.Left=0;
  D->gpd_Domain.Top=0;

    /* inquire min height of buttons */
    /* doesnt work
    sub = (struct Gadget *)gdata->subs[THS_NameButton];
    if(sub) gdata->row1height = getHeight(sub, D, GDOMAIN_NOMINAL);

    sub = (struct Gadget *)gdata->subs[THS_SilencerBt];
    if(sub) gdata->row2height = getHeight(sub, D, GDOMAIN_MINIMUM);

    sub = (struct Gadget *)gdata->subs[THS_VolumeSlider];
    if(sub) gdata->sliderheight = getHeight(sub,D, GDOMAIN_NOMINAL);
*/
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
  LONG leftPartWidth;
  // volumeRuleWidth
  LONG rowHeight;
  LONG curY;
  LONG closeW, labelW, sliderX, sliderW;
  struct Gadget *sub;
   int row1height,row2height, sliderheight;


    int clipTop = ((ULONG)Gad->UserData)>>16;
    int clipBottom = ((ULONG)Gad->UserData) & 0x0ffff;

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
   // volumeRuleWidth = 32;
    leftPartWidth = width ;
    //     leftPartWidth = width - volumeRuleWidth;
    rowHeight = height / 5;  /* 5 rows */
    //row1height = row2height = sliderheight = rowHeight;

  row1height = 22;
    row2height = 16;
    sliderheight = 22;
    closeW = 20;

    if(gdata->_gadgetInfo &&
        gdata->_gadgetInfo->gi_DrInfo &&
        gdata->_gadgetInfo->gi_DrInfo->dri_Font )
    {
        struct TextFont	*tfont = gdata->_gadgetInfo->gi_DrInfo->dri_Font;
        int btheight =  tfont->tf_YSize+8;
        if(row1height< btheight)
        {
         row1height = btheight;
         closeW = btheight+2;
        }
    }

  //  if(rowHeight<row1height) row1height = rowHeight;
   if(rowHeight<row2height) row2height = rowHeight;
    if(rowHeight<sliderheight) sliderheight = rowHeight;

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
        sub->Height = row1height;
        DoMethodA((Object*)sub, (Msg)layout);
    }

    sub = (struct Gadget *)gdata->subs[THS_NameButton];
    if(sub)
    {
        sub->LeftEdge = leftedge + closeW + 2;
        sub->TopEdge = curY;
        sub->Width = leftPartWidth - closeW - 3;
        sub->Height = row1height;
        DoMethodA((Object*)sub, (Msg)layout);
    }
    curY += row1height;

    /* Row 2: Silencer and Solo buttons */
    sub = (struct Gadget *)gdata->subs[THS_SilencerBt];
    if(sub)
    {
        sub->LeftEdge = leftedge + 1;
        sub->TopEdge = curY;
        sub->Width = (leftPartWidth - 3) / 2;
        sub->Height = row2height;
        DoMethodA((Object*)sub, (Msg)layout);
    }

    sub = (struct Gadget *)gdata->subs[THS_SoloBt];
    if(sub)
    {
        sub->LeftEdge = leftedge + 1 + (leftPartWidth - 3) / 2 + 1;
        sub->TopEdge = curY;
        sub->Width = (leftPartWidth - 3) / 2;
        sub->Height = row2height;
        DoMethodA((Object*)sub, (Msg)layout);
    }
    curY += row2height;

    /* Row 3: Vol label and slider */
    sub = (struct Gadget *)gdata->subs[THS_VolLabel];
    if(sub)
    {
        sub->LeftEdge = leftedge + 1;
        sub->TopEdge = curY;
        sub->Width = labelW;
        sub->Height = sliderheight;
        DoMethodA((Object*)sub, (Msg)layout);
    }

    sub = (struct Gadget *)gdata->subs[THS_VolumeSlider];
    if(sub)
    {
        sub->LeftEdge = sliderX;
        sub->TopEdge = curY;
        sub->Width = sliderW;
        sub->Height = sliderheight;
        DoMethodA((Object*)sub, (Msg)layout);
    }
    curY += sliderheight;

    /* Row 4: Pan label and slider */
    sub = (struct Gadget *)gdata->subs[THS_PanLabel];
    if(sub)
    {
        sub->LeftEdge = leftedge + 1;
        sub->TopEdge = curY;
        sub->Width = labelW;
        sub->Height = sliderheight;
        DoMethodA((Object*)sub, (Msg)layout);
    }

    sub = (struct Gadget *)gdata->subs[THS_PanSlider];
    if(sub)
    {
        sub->LeftEdge = sliderX;
        sub->TopEdge = curY;
        sub->Width = sliderW;
        sub->Height = sliderheight;
        DoMethodA((Object*)sub, (Msg)layout);
    }
    curY += sliderheight;

    /* - - - here consider spacer --- */
    sub = (struct Gadget *)gdata->subs[THS_Spacer];
    if(sub)
    {
        int h = height- ((curY-topedge) + row2height+row2height) ;
        if(h<=0) h=1;
        sub->LeftEdge = leftedge ;
        sub->TopEdge = curY;
        sub->Width = leftPartWidth;
        sub->Height = h;
        DoMethodA((Object*)sub, (Msg)layout);
    }


    /* Row 5: Info label (takes remaining height) */
    sub = (struct Gadget *)gdata->subs[THS_InfoLabel1];
    if(sub)
    {
        sub->LeftEdge = leftedge ;
        sub->TopEdge = topedge + height - (row2height*2); // curY;
        sub->Width = leftPartWidth ;
        sub->Height = row2height;
        DoMethodA((Object*)sub, (Msg)layout);
    }
    sub = (struct Gadget *)gdata->subs[THS_InfoLabel2];
    if(sub)
    {
        sub->LeftEdge = leftedge;
        sub->TopEdge = topedge + height - (row2height); // curY;
        sub->Width = leftPartWidth;
        sub->Height = row2height;
        DoMethodA((Object*)sub, (Msg)layout);
    }

  return(1);
}
/* need to fill gaps, supercall already done */
void TrackHeader_Render(Class *C, struct Gadget *Gad, struct gpRender *Render)
{
    TrackHeader *gdata;
    struct Gadget *sub;
    struct RastPort *rp;
    AukStyle *style;
    struct Hook *bfh=NULL;
    LONG topedge,leftedge,width,height;

    if(!Render->gpr_RPort) return;

    rp = Render->gpr_RPort;

    gdata=INST_DATA(C, Gad);
    style = gdata->_style;
    if(!style) return;

    topedge = Gad->TopEdge;
    leftedge = Gad->LeftEdge;
    width = Gad->Width;
    height = Gad->Height;

    GetAttr(GA_BackFill,Gad,&bfh);
//bdbprintf("gdata->_backfillHook %08x %08x\n",(int)gdata->_backfillHook,bfh);

    sub = (struct Gadget *)gdata->subs[THS_CloseButton];
    if(sub /*&& gdata->_backfillHook*/)
    {
        // LAYERS_NOBACKFILL
       // struct Hook *oldHook =  InstallLayerHook( rp->Layer , gdata->_backfillHook );

        SetAPen(rp,style->pens[AUK_COLOR_WHITE].pen);
        RectFill(rp,
                    leftedge,topedge,
                    leftedge,topedge+height
                    );
//        RectFill(rp,
//                    leftedge+sub->Width+1,topedge,
//                    leftedge+sub->Width+1,topedge+sub->Height
//                    );
//        EraseRect(rp,
//                    leftedge,topedge,
//                    leftedge,topedge+height
//                    );
        EraseRect(rp,
                    leftedge+sub->Width+1,topedge,
                    leftedge+sub->Width+1,topedge+sub->Height
                    );
      //  InstallLayerHook( rp->Layer , oldHook );
    }

}
