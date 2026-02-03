
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/layers.h>


#include <clib/alib_protos.h>

#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/gadgetclass.h>
#include <intuition/icclass.h>
#include <utility/tagitem.h>

#include "class_trackarea_private.h"

#include "class_tracklistarea.h"
#include "class_tracklistarea_private.h"

/* Include child gadget classes */
#include "../TrackArea/class_trackarea.h"
#include "../TrackHeader/class_trackheader.h"
#include "../VolumeRule/class_volumerule.h"

#include <proto/layout.h>
#include <gadgets/layout.h>

#include <proto/button.h>
#include <gadgets/button.h>

#include <aukarray.h>
#include <auktrack.h>

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


extern struct IClass   *TrackListClassPtr;
extern struct IClass   *TrackHeaderClassPtr;
extern struct IClass   *TrackAreaClassPtr;


static ULONG TrackListArea_NotifyChangeHeight(struct Gadget *Gad, struct GadgetInfo	*GInfo)
{
    struct opUpdate notifymsg;
    TrackListArea *gdata=INST_DATA(TrackListClassPtr, Gad);
    ULONG tags[]={
     GA_ID,0,
     TRACKLIST_DomainHeight,0,
     TAG_DONE
    };

    tags[1] = Gad->GadgetID;
    tags[3] = (LONG)gdata->_domainHeight;
    notifymsg.MethodID = OM_NOTIFY;
    notifymsg.opu_AttrList = (struct TagItem *)&tags[0];
    notifymsg.opu_GInfo = GInfo; // "always there for gadget, in all messages"
    notifymsg.opu_Flags = 0;

    return DoSuperMethodA(TrackListClassPtr,(APTR)Gad,(Msg)&notifymsg );
}


ULONG TrackListArea_Domain(Class *C, struct Gadget *Gad, struct gpDomain *D)
{
  TrackListArea *gdata=0;

  if(Gad) gdata=INST_DATA(C, Gad);
// Printf("TrackListArea_Domain data:%lx\n",(int)gdata);

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
        D->gpd_Domain.Width=256;
        D->gpd_Domain.Height=128;
      }
      break;

    case GDOMAIN_MAXIMUM:
      D->gpd_Domain.Width=16000;
      D->gpd_Domain.Height=16000;
      break;

    case GDOMAIN_MINIMUM:
    default:
     if(gdata)
     {
       D->gpd_Domain.Width =gdata->_minimalWidth; // sqrt(gdata->Pens) * 8 + 8;
       D->gpd_Domain.Height=gdata->_minimalHeight; // sqrt(gdata->Pens) * 8 + 8;
     }
     else
      {
        D->gpd_Domain.Width=  50;
        D->gpd_Domain.Height= 50;
      }
      break;

  }
  return(1);
}

/**
 * method GM_LAYOUT
 * The gadget knows its final coordinates,
 * So we may have to resize what's inside our gadget.
 */
ULONG TrackListArea_Layout(Class *C, struct Gadget *Gad, struct gpLayout *layout,int filter)
{
  TrackListArea *gdata;
  LONG topedge,leftedge,width,height;
  LONG trackTop;
  ULONG itrack,iChannel;
 ULONG prevDomainHeight;
 int selectionBorderWidth=2;

// ULONG prevHeight;
    gdata=INST_DATA(C, Gad);

    if(gdata->_styleSheet)
        selectionBorderWidth =  gdata->_styleSheet->borderSelectionWidth;

    topedge = Gad->TopEdge;
    leftedge = Gad->LeftEdge;
    width = Gad->Width;
    height = Gad->Height;

    /* this is for the cliprect */
    gdata->_framerec.MinX = leftedge;
    gdata->_framerec.MinY = topedge;
    gdata->_framerec.MaxX = leftedge + width  -1;
    gdata->_framerec.MaxY = topedge  + height -1;

    leftedge += selectionBorderWidth;
    width -= selectionBorderWidth;

    prevDomainHeight = gdata->_domainHeight;

    /* Layout child gadgets (TrackHeaders and TrackGadgets) */
    if(gdata->_tracks && gdata->_trackCount > 0)
    {
        LONG totalDomainHeight = selectionBorderWidth;

        /* Default track height if not set */
        if(gdata->_defaulTrackHeight == 0) gdata->_defaulTrackHeight = 40;
        /* Default header width if not set */
        if(gdata->_headerWidth == 0) gdata->_headerWidth = 96;


        /* count total height first */
        for(itrack = 0; itrack < gdata->_trackCount; itrack++)
        {            
            TrackChild *strack;
            strack = &gdata->_tracks[itrack];

            for(iChannel = 0; iChannel < strack->_nbChannels; iChannel++)
            {
                TrackChannelChild *chan = &strack->_channels[iChannel];
                UWORD trackHeight=0;

                trackHeight = chan->_prefHeight;
                if(trackHeight==0) trackHeight = gdata->_defaulTrackHeight;
                totalDomainHeight += trackHeight;
            }
            totalDomainHeight += selectionBorderWidth;
        }

        if((totalDomainHeight-gdata->_scrollY) < height)
        {
            gdata->_scrollY = totalDomainHeight-height;
        }
        if(gdata->_scrollY<0) gdata->_scrollY=0;
        /* Start from the top, accounting for vertical scroll */
        trackTop = topedge - gdata->_scrollY;


        /* Layout each track row */
        trackTop += selectionBorderWidth;
        for(itrack = 0; itrack < gdata->_trackCount; itrack++)
        {
            LONG headerTop = trackTop;
            TrackChild *strack;
            struct Gadget *headerGad = NULL;
            strack = &gdata->_tracks[itrack];
            strack->_layouted = 0; // if any chan layouted...
            for(iChannel = 0; iChannel < strack->_nbChannels; iChannel++)
            {
                struct Gadget *trackGad,*volumeRule;
                //TrackArea *trackArea;
                TrackChannelChild *chan = &strack->_channels[iChannel];
                UWORD trackHeight=0;
                if(iChannel == 0)
                {   /* the header is always only on tyhe first chan
                    This is layouted after the loop
                    */
                    headerGad = (struct Gadget *) chan->_trackHeader;
                }
                trackGad = (struct Gadget *) chan->_trackArea;
                volumeRule = (struct Gadget *) chan->_volumeRule;
                chan->_layouted = 0;

                trackHeight = chan->_prefHeight;
                if(trackHeight==0) trackHeight = gdata->_defaulTrackHeight;

                /* Skip tracks that are scrolled out of view (above visible area)
                disable also if is below visible area
                */
                if((trackTop + trackHeight < topedge ) ||
                    (trackTop > topedge + height)
                     )
                {
                    chan->_top = trackTop;
                    chan->_bottom = trackTop+trackHeight;
                    trackTop += trackHeight ;
                    continue;
                }
                // now header is layout after all chans trackareas
                if(volumeRule)
                {
                    volumeRule->LeftEdge = leftedge + gdata->_headerWidth;
                    volumeRule->TopEdge = trackTop;
                    volumeRule->Width = gdata->_volruleWidth;
                    volumeRule->Height = trackHeight;

                    /* Call child's GM_LAYOUT */
                   DoMethodA((Object*)volumeRule, (Msg)layout);
                }
                if(trackGad)
                {

                    /* Position TrackArea on the right, after header */
                    trackGad->LeftEdge = leftedge + gdata->_headerWidth+gdata->_volruleWidth;
                    trackGad->TopEdge = trackTop;
                    trackGad->Width = width - (gdata->_headerWidth+gdata->_volruleWidth);
                    trackGad->Height = trackHeight;

                    /* Call child's GM_LAYOUT */
                   DoMethodA((Object*)trackGad, (Msg)layout);
                }
                chan->_layouted = 1;
                strack->_layouted = 1;
                chan->_top = trackTop;
                chan->_bottom = trackTop+trackHeight;
                chan->_xmid = leftedge+ gdata->_headerWidth+gdata->_volruleWidth;

                trackTop += trackHeight;
            } // end loop per chan

            // headerTop
            if(headerGad )
            {
                /* Position TrackHeader on the left */
                headerGad->LeftEdge = leftedge;
                headerGad->TopEdge = headerTop;
                headerGad->Width = gdata->_headerWidth;
                headerGad->Height = trackTop-headerTop;

                /* Call child's GM_LAYOUT */
                DoMethodA((Object*)headerGad, (Msg)layout);
            }
            trackTop += selectionBorderWidth;
        } // end loop per track

        /* Store the calculated domain height */
        gdata->_domainHeight = totalDomainHeight;
    }
    else
    {
        /* No tracks, domain height is zero */
        gdata->_domainHeight = 0;
    }


//bdbprintf("TrackListArea_Layout:%d %d\n",prevDomainHeight,gdata->_domainHeight);
    if((prevDomainHeight != gdata->_domainHeight) ||
        (gdata->_prevHeight != (ULONG) Gad->Height))
    {
        TrackListArea_NotifyChangeHeight(Gad,layout->gpl_GInfo);
    }
    gdata->_prevHeight = (ULONG) Gad->Height;


    if(gdata->_clipRegion)
    {
        ClearRegion(gdata->_clipRegion);
        OrRectRegion(gdata->_clipRegion, &gdata->_framerec);
    }

  return(1);
}

/* draw yourself, in the appropriate state */
ULONG TrackListArea_Render(Class *C, struct Gadget *Gad, struct gpRender *Render,int filter)
{
    struct Region *oldClipRegion;
 //   struct TextFont *oldfont=NULL;
    TrackListArea *gdata;
    struct RastPort *rp;
    int bLayerUpdating=FALSE;
    LONG itrack,iChannel;
    AukSelection *selection=NULL;
    LONG topedge; //,leftedge,width,height;
     int selectionBorderWidth=2;
     int prevSelected=0;
    if(Render->MethodID==GM_RENDER &&  Render->gpr_RPort )
    {
        rp=Render->gpr_RPort;
    }
    else
    {
        return 1;
    }

 //   oldfont = rp->Font;

    gdata=INST_DATA(C, Gad);

    if(gdata->_styleSheet)
        selectionBorderWidth =  gdata->_styleSheet->borderSelectionWidth;

    topedge = Gad->TopEdge;
//    leftedge = Gad->LeftEdge;
//    width = Gad->Width;
//    height = Gad->Height;
    if( gdata->_project )
    {
        selection = &gdata->_project->selection ;
    }

	// if( ( rp->Layer->Flags & LAYERUPDATING ) != 0L )
	// {
	// 	bLayerUpdating = TRUE;
	// 	EndUpdate(rp->Layer, FALSE);
	// 	bdbprintf(" ****Render->MethodID:%08lx LAYERUPDATING\n",(int)Render->MethodID);
	// }

    oldClipRegion = InstallClipRegion( rp->Layer, gdata->_clipRegion);

    for(itrack = 0; itrack < (int)gdata->_trackCount; itrack++)
    {
        int trtop,trbot,isSelected;
        //int yscrol = Gad->TopEdge - gdata->_scrollY;
        TrackChild *strack;
        strack = &gdata->_tracks[itrack];
        // shouldn't happen
        if(strack->_nbChannels==0 || !strack->_dataTrack) continue;

        trtop = strack->_channels[0]._top;
        trbot = strack->_channels[strack->_nbChannels-1]._bottom;
        isSelected = strack->_dataTrack->selectionFlags & AukTrackSelFlag_Selected;
        if(selection && selection->_mode == 1 &&
              selection->_itrack ==  strack->_dataTrack->trackIndex ) isSelected = 1;
        //if()
        // draw left border trackBackground
        //bdbprintf("_styleSheet %08x top %d bot %d\n",gdata->_styleSheet,trtop,trbot);

        /* draw marges that tells selection */

        /* intermarge top/bottom */
        {
            int y1 = trtop-selectionBorderWidth;
            int y2 = trtop-1;

            if(y1<=gdata->_framerec.MaxY &&
                y2>=gdata->_framerec.MinY )
            if(isSelected||prevSelected)
            {
                SetAPen(rp,
                    (prevSelected)? gdata->_styleSheet->pens[AUK_COLOR_TRACK_HIGHLIGHT].pen:
                        gdata->_styleSheet->pens[AUK_COLOR_TRACK_BACKGROUND].pen);

                    RectFill(rp,gdata->_framerec.MinX, y1,
                    gdata->_framerec.MaxX, y1 );
                if(y2-y1>2)
                {
                    SetAPen(rp, gdata->_styleSheet->pens[AUK_COLOR_TRACK_HIGHLIGHT2].pen);
                            RectFill(rp,gdata->_framerec.MinX, y1+1,
                        gdata->_framerec.MaxX, y2-1 );
                }

                SetAPen(rp,
                    (isSelected)? gdata->_styleSheet->pens[AUK_COLOR_TRACK_HIGHLIGHT].pen:
                        gdata->_styleSheet->pens[AUK_COLOR_TRACK_BACKGROUND].pen);

                    RectFill(rp,gdata->_framerec.MinX, y2,
                    gdata->_framerec.MaxX, y2 );
// trackHighlight2
            } else
            {
                // not selected
                SetAPen(rp,gdata->_styleSheet->pens[AUK_COLOR_TRACK_BACKGROUND].pen );
                RectFill(rp,gdata->_framerec.MinX, y1,
                            gdata->_framerec.MaxX, y2 );

            }
        }

        /*left*/
        {
            int y1 = trtop;
            int y2 = trbot-1;
            if(isSelected)
            {
                /* note: amiga bitmap wise, this should, ...must be a bitmap copy. */
                if(selectionBorderWidth>2)
                {
                    SetAPen(rp,gdata->_styleSheet->pens[AUK_COLOR_TRACK_BACKGROUND].pen);
                    RectFill(rp,gdata->_framerec.MinX, y1,
                                gdata->_framerec.MinX+selectionBorderWidth-3, y2 );
                }
                SetAPen(rp,gdata->_styleSheet->pens[AUK_COLOR_TRACK_HIGHLIGHT2].pen);
                RectFill(rp,gdata->_framerec.MinX+selectionBorderWidth-2, y1,
                            gdata->_framerec.MinX+selectionBorderWidth-2, y2 );

                SetAPen(rp,gdata->_styleSheet->pens[AUK_COLOR_TRACK_HIGHLIGHT].pen);
                RectFill(rp,gdata->_framerec.MinX+selectionBorderWidth-1, y1,
                            gdata->_framerec.MinX+selectionBorderWidth-1, y2 );
            } else
            {
                SetAPen(rp, gdata->_styleSheet->pens[AUK_COLOR_TRACK_BACKGROUND].pen );
                RectFill(rp,gdata->_framerec.MinX, y1,
                            gdata->_framerec.MinX+selectionBorderWidth-1, y2 );
            }
        }

        if(strack->_layouted)
        for(iChannel = 0; iChannel < (int)strack->_nbChannels; iChannel++)
        {
            struct Gadget *headerGad,*volumeRule;
            struct Gadget *trackGad;
          //  TrackArea *trackArea;
            TrackChannelChild *chan = &strack->_channels[iChannel];

            headerGad = (struct Gadget *) chan->_trackHeader;
            volumeRule = (struct Gadget *) chan->_volumeRule;
            trackGad = (struct Gadget *) chan->_trackArea;

            // if layouted

            /* Call child's GM_RENDER */

            if(strack->_layouted &&
                 headerGad && ((filter & 2)!=0) && bLayerUpdating == 0) // if layouted && selected for refresh
            {
                //            SetAPen(rp, gdata->_styleSheet->trackHeaderBG.pen);
                //            RectFill(rp,headerGad->LeftEdge,
                //                        headerGad->TopEdge,
                //                        headerGad->LeftEdge + headerGad->Width -1,
                //                        headerGad->TopEdge + headerGad->Height -1);
              //  struct TextFont *prevfont = rp->Font;
                    DoMethodA((Object*)headerGad, (Msg)Render); // not DoGadgetMethodA in that case
              //  if(prevfont) SetFont(rp,prevfont);
            }
            if(!chan->_layouted) continue;
            if(volumeRule && ((filter & 2)!=0)) // if layouted && selected for refresh
            {
                DoMethodA((Object*)volumeRule, (Msg)Render); // not DoGadgetMethodA in that case
            }
            if(trackGad && ((filter & 1)!=0)) // if layouted && selected for refresh
            {
                DoMethodA((Object*)trackGad, (Msg)Render); // not DoGadgetMethodA in that case
            }
         } // end loop per chan

        prevSelected = isSelected;
        /* draw last marge down*/
        if(itrack == (int)gdata->_trackCount-1)
        {
            int y1 = trbot;
            int y2 = trbot+selectionBorderWidth-1;

            if(y1<=gdata->_framerec.MaxY &&
                y2>=gdata->_framerec.MinY )
            if(prevSelected)
            {
                SetAPen(rp,
                    (prevSelected)? gdata->_styleSheet->pens[AUK_COLOR_TRACK_HIGHLIGHT].pen:
                        gdata->_styleSheet->pens[AUK_COLOR_TRACK_BACKGROUND].pen);

                    RectFill(rp,gdata->_framerec.MinX, y1,
                    gdata->_framerec.MaxX, y1 );
                if(y2-y1>2)
                {
                    SetAPen(rp, gdata->_styleSheet->pens[AUK_COLOR_TRACK_HIGHLIGHT2].pen);
                            RectFill(rp,gdata->_framerec.MinX, y1+1,
                        gdata->_framerec.MaxX, y2-1 );
                }

                SetAPen(rp,gdata->_styleSheet->pens[AUK_COLOR_TRACK_BACKGROUND].pen);
                    RectFill(rp,gdata->_framerec.MinX, y2,
                    gdata->_framerec.MaxX, y2 );

            } else
            {
                // not selected
                SetAPen(rp,gdata->_styleSheet->pens[AUK_COLOR_TRACK_BACKGROUND].pen );
                RectFill(rp,gdata->_framerec.MinX, y1,
                            gdata->_framerec.MaxX, y2 );

            }

        }

    } // end loop per track

    /* may render empty space down there */
    {
        int lasttrackY = topedge - gdata->_scrollY + gdata->_domainHeight;
        if(lasttrackY<=gdata->_framerec.MaxY)
        {
           SetAPen(rp, gdata->_styleSheet->pens[AUK_COLOR_TRACK_BACKGROUND].pen);
           RectFill(rp,gdata->_framerec.MinX,
                       lasttrackY,
                       gdata->_framerec.MaxX,
                       gdata->_framerec.MaxY);
        }
    }


    InstallClipRegion( rp->Layer,oldClipRegion); // important to pass NULL if oldClipRegion is NULL.

    /* may render zoom span when running */
    if(gdata->_zoomSpan._mode == 1)
    {
        AukStyle *style;
        TimeProjection *tproj;
        WORD penline;
        LONG xstart,xend;
        LONG trackareax1,trackareax2;

        tproj = &gdata->_timeProjection;
        style = gdata->_styleSheet;

        trackareax1 = Gad->LeftEdge + gdata->_headerWidth + gdata->_volruleWidth;
        trackareax2 = Gad->LeftEdge + Gad->Width;

        penline = style->pens[AUK_COLOR_BLACK].pen;

        xstart = (gdata->_zoomSpan._start / tproj->_timePerPixelWidth) - tproj->_pixAtLeft
                        + trackareax1;
        xend = (gdata->_zoomSpan._end / tproj->_timePerPixelWidth) - tproj->_pixAtLeft
                        + trackareax1;

        SetAPen(rp, penline);
        if(xstart>=trackareax1 && xstart<trackareax2)
        {
            Move(rp, Gad->LeftEdge + xstart,Gad->TopEdge );
            Draw(rp, Gad->LeftEdge + xstart,Gad->TopEdge+Gad->Height );
        }
        if(xend>=trackareax1 && xend<trackareax2)
        {
            Move(rp, Gad->LeftEdge + xend,Gad->TopEdge );
            Draw(rp, Gad->LeftEdge + xend,Gad->TopEdge+Gad->Height );
        }

    }


 //   if(oldfont) SetFont(rp,oldfont);

    // if(bLayerUpdating)
    // {
    //     BeginUpdate(rp->Layer);
    // }

    // if (Render->MethodID != GM_RENDER)
    //   ReleaseGIRPort(rp);

    // vertical drawing management:
    // todo: recursively draw tracks on their projected rectangle

    // then draw eventually clear a rectangle of the empty scrool Area.

  return(1);
}



