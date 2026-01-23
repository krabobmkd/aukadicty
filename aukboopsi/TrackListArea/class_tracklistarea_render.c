
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

#include "boopsidispose.h"

// memcpy
#include <string.h>

/* Default capacity for track array allocation */
#define TRACKLIST_DEFAULT_CAPACITY 64

#include "aukerrors.h"
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


extern struct IClass   *TrackListClassPtr;
extern struct IClass   *TrackHeaderClassPtr;
extern struct IClass   *TrackAreaClassPtr;

/* This can be reallocated, so this is shared like this */
extern struct Window *CurrentMainWindow;


extern BoopsiDisposeQueue *ObjectLateDisposer;

struct Region *TrackListArea_clipRegion=NULL;
//static ULONG TrackListArea_NotifyChangeWidth(struct Gadget *Gad, struct GadgetInfo	*GInfo)
//{
//    struct opUpdate notifymsg;
//    TrackListArea *gdata=INST_DATA(TrackListClassPtr, Gad);
//    ULONG tags[]={
//     GA_ID,0,
//     TRACKLIST_DomainWidth,0,
//     TRACKLIST_DomainWidthHigh,0,
//     TAG_DONE
//    };

//    tags[1] = Gad->GadgetID;
//    tags[3] = (ULONG)gdata->_domainWidth;
//    tags[5] = (ULONG)(gdata->_domainWidth>>32);
//    notifymsg.MethodID = OM_NOTIFY;
//    notifymsg.opu_AttrList = (struct TagItem *)&tags[0];
//    notifymsg.opu_GInfo = GInfo; // "always there for gadget, in all messages"
//    notifymsg.opu_Flags = 0;

//    return DoSuperMethodA(TrackListClassPtr,(APTR)Gad,(Msg)&notifymsg );
//}


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
    leftedge = Gad->LeftEdge + selectionBorderWidth;
    width = Gad->Width - selectionBorderWidth;
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
        TrackListArea_clipRegion = gdata->_clipRegion;
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
        int yscrol = Gad->TopEdge - gdata->_scrollY;
        TrackChild *strack;
        strack = &gdata->_tracks[itrack];
        if(strack->_nbChannels==0 || !strack->_dataTrack) continue;
        if(!strack->_layouted) continue;
        trtop = strack->_channels[0]._top;
        trbot = strack->_channels[strack->_nbChannels-1]._bottom;
        isSelected = strack->_dataTrack->selectionFlags & AukTrackSelFlag_Selected;
        //if()
        // draw left border trackBackground
        bdbprintf("_styleSheet %08x top %d bot %d\n",gdata->_styleSheet,trtop,trbot);
        SetAPen(rp,
        //gdata->_styleSheet->trackBackground.pen);
            isSelected?gdata->_styleSheet->trackHighlight.pen:
                               gdata->_styleSheet->trackBackground.pen );

       RectFill(rp,gdata->_framerec.MinX,
                   trtop-selectionBorderWidth+yscrol,
                   gdata->_framerec.MinX+selectionBorderWidth-1,
                   trtop+selectionBorderWidth-1+yscrol);

       RectFill(rp,gdata->_framerec.MinX,
                   trtop+yscrol,
                   gdata->_framerec.MaxX,
                   trbot+yscrol);

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
            if(volumeRule && ((filter & 1)!=0)) // if layouted && selected for refresh
            {
                DoMethodA((Object*)volumeRule, (Msg)Render); // not DoGadgetMethodA in that case
            }
            if(trackGad && ((filter & 1)!=0)) // if layouted && selected for refresh
            {
                DoMethodA((Object*)trackGad, (Msg)Render); // not DoGadgetMethodA in that case
            }
         } // end loop per chan

        prevSelected = isSelected;
    } // end loop per track


    /* may render empty space */
    {
        int lasttrackY = topedge - gdata->_scrollY + gdata->_domainHeight;
        if(lasttrackY<gdata->_framerec.MaxY)
        {
           SetAPen(rp, gdata->_styleSheet->trackBackground.pen);
           RectFill(rp,gdata->_framerec.MinX,
                       lasttrackY,
                       gdata->_framerec.MaxX,
                       gdata->_framerec.MaxY);
        }
    }


    InstallClipRegion( rp->Layer,oldClipRegion); // important to pass NULL if oldClipRegion is NULL.

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



/** Helper - dispose all allocated gadgets and free array */
void TrackListArea_DisposeGadgets(struct Gadget *Gad,TrackListArea *gdata)
{
    ULONG itrack,iChannel;
    if(!gdata) return;
    bdbprintf("TrackListArea_DisposeGadgets() ->all\n");
    /* Dispose all active TrackHeader/TrackArea gadgets */
    if(gdata->_tracks)
    {
//        for(i = 0; i < gdata->_trackCount; i++)
//        {
//            Object *trackHeader = gdata->_tracks[i]._trackHeader;
//            Object *trackArea = gdata->_tracks[i]._trackArea;
        for(itrack = 0; itrack < gdata->_trackCount; itrack++)
        {
            TrackChild *strack;
            strack = &gdata->_tracks[itrack];

            for(iChannel = 0; iChannel < strack->_nbChannels; iChannel++)
            {
                struct Gadget *trackHeader;
                struct Gadget *trackArea;
                struct Gadget *volumeRule;
                TrackChannelChild *chan = &strack->_channels[iChannel];

                trackHeader = (struct Gadget *) chan->_trackHeader;
                trackArea = (struct Gadget *) chan->_trackArea;
                volumeRule = (struct Gadget *) chan->_volumeRule;
                if(trackHeader)
                {
                    /* LAYOUT_RemoveChild: This will destroy the object as well. */
                    SetGadgetAttrs(Gad,CurrentMainWindow,NULL,
                                LAYOUT_RemoveChild,(ULONG)trackHeader,TAG_END);
                    BoopsiDispose_Later( ObjectLateDisposer, (Object *)trackHeader);
                }
                if(volumeRule)
                {
                    SetGadgetAttrs(Gad,CurrentMainWindow,NULL,
                                LAYOUT_RemoveChild,(ULONG)volumeRule,TAG_END);
                    BoopsiDispose_Later( ObjectLateDisposer,(Object *)volumeRule);
                }

                if(trackArea)
                {
                    SetGadgetAttrs(Gad,CurrentMainWindow,NULL,
                                LAYOUT_RemoveChild,(ULONG)trackArea,TAG_END);
                    BoopsiDispose_Later( ObjectLateDisposer,(Object *)trackArea);
                }


                /* Clear the slot */
                chan->_trackHeader = NULL;
                chan->_volumeRule = NULL;
                chan->_trackArea = NULL;

            }
             /* release data we sync: */
            AukObjectPtr_Release((AukObjectPtr *)&strack->_dataTrack);
            /* free channels base */
            if(strack->_channels) FreeVec(strack->_channels);
            strack->_channels = NULL;
            strack->_nbChannels = 0;
        }

        FreeVec(gdata->_tracks);
        gdata->_tracks = NULL;
    }
    gdata->_trackCount = 0;
    gdata->_trackCapacity = 0;
}

/** Helper - ensure track array is allocated with default capacity */
static int TrackListArea_EnsureTrackArray(TrackListArea *gdata)
{
    if(gdata->_tracks != NULL) return 1; /* Already allocated */

    gdata->_tracks = (TrackChild*)AllocVec(TRACKLIST_DEFAULT_CAPACITY * sizeof(TrackChild), MEMF_CLEAR);
    if(!gdata->_tracks) return 0; /* Allocation failed */

    gdata->_trackCapacity = TRACKLIST_DEFAULT_CAPACITY;
    gdata->_trackCount = 0;
    return 1;
}

extern Object *AppInstance;
//static int TrackListArea_CreateTrackLine(
//            struct Gadget *Gad,
//            TrackListArea *gdata,
//            TrackChild *strack, AukTrack *dataTrack, int iTrack)
static int TrackListArea_CreateTrackChannelLine(
            struct Gadget *Gad,
            TrackListArea *gdata,
            TrackChild *strack,
            TrackChannelChild *schan, AukTrack *dataTrack, int iTrack, int iChannel)
{
    char *trackname=NULL;
    ULONG TRACKHEADER_Nametag =TAG_END;

    struct AukStyle *styleSheet = gdata->_styleSheet;

    if(dataTrack && dataTrack->name) trackname = dataTrack->name;
    if(trackname) TRACKHEADER_Nametag = TRACKHEADER_Name;
    bdbprintf("TrackListArea_CreateTrackLine trackname:%s\n",trackname);
    /* Create TrackHeader gadget */
    if(iChannel ==0)
    {
        schan->_trackHeader =
            NewObject(TRACKHEADER_GetClass(), NULL,
                         LAYOUT_DeferLayout,TRUE,
                          // CHILD_NoDispose,TRUE,
                            TRACKHEADER_StyleSheet, (ULONG)styleSheet,
                           //test LAYOUT_FillPen, gdata->_styleSheet->trackHeaderBG.pen,
                            TRACKHEADER_TrackIndex,iTrack,
                            ICA_TARGET,AppInstance,
                            GA_DrawInfo, (ULONG)gdata->_drawInfo,
                            TRACKHEADER_Nametag,trackname, // optional, must be last
                            TAG_END);

        if(schan->_trackHeader)
        {
            /* CHILD_NoDispose superimportant, to manage smooth detach  */
            SetAttrs(Gad,LAYOUT_AddChild,(ULONG)schan->_trackHeader,
                        CHILD_NoDispose,TRUE,
                        TAG_END);


        }
    }
    //TODO TRACKHEADER_TrackIndex TRACKHEADER_Name should be later setAttribs()

      /* Right header side: VolumeRule */
     schan->_volumeRule = NewObject( VOLUMERULE_GetClass(),NULL,
                                    VOLUMERULE_StyleSheet,(ULONG)styleSheet,
                                    TAG_END );
    if(schan->_volumeRule)
    {
        SetAttrs(Gad,LAYOUT_AddChild,(ULONG)schan->_volumeRule,
                            CHILD_NoDispose,TRUE,
                            TAG_END);
    }


    /* Create TrackArea - pass data track for reference counted retention */
    schan->_trackArea = NewObject(TRACKAREA_GetClass(), NULL,
                                   INFINITESCROLL_PPosition,(ULONG) &gdata->_timeProjection._pixAtLeft,
                                   TRACKAREA_StyleSheet, (ULONG)styleSheet,
                                   TRACKAREA_PTimeProjection,(ULONG)&gdata->_timeProjection,
                                   TRACKAREA_DataTrack,(ULONG)dataTrack,
                                   TAG_END);
    if(schan->_trackArea)
    {
        SetAttrs(Gad,LAYOUT_AddChild,(ULONG)schan->_trackArea,
                            CHILD_NoDispose,TRUE,
                            TAG_END);
    }

    /* retain data we sync: */
    AukObjectPtr_Set((AukObjectPtr *)&strack->_dataTrack,dataTrack);
    // default value
    schan->_prefHeight = 96;

    return 1;
}

/** private,
* Full sync of track UI to data.
* Called on project set or when incremental updates can't handle changes.
*/
//static void TrackListArea_updateTrackListUiToData(struct Gadget *Gad)
//{
//    TrackListArea *gdata;
//    AukAProject *project;
//    ULONG dataTrackCount;
//    ULONG i;

//    if(!TrackListClassPtr || !Gad) return;
//    gdata = INST_DATA(TrackListClassPtr, Gad);

//    project = gdata->_project;
//    if(!project)
//    {
//        /* No project, clean up everything */
//        TrackListArea_DisposeGadgets(Gad,gdata);
//        return;
//    }

//    /* Get the track count from project */
//    dataTrackCount = AukArray_GetCount(project->tracks);

//    /* Check capacity - if data exceeds our capacity, we need a full rebuild */
//    if(dataTrackCount > TRACKLIST_DEFAULT_CAPACITY)
//    {
//        AukLog_MessageInt(AUKLOG_WARNING, AUKERR_TRACKLIST_CAPACITY_REACHED, TRACKLIST_DEFAULT_CAPACITY);
//        /* For now, just handle up to capacity */
//        dataTrackCount = TRACKLIST_DEFAULT_CAPACITY;
//    }

//    /* If count changed, rebuild UI */
//    if(dataTrackCount != gdata->_trackCount)
//    {
//        /* Dispose old gadgets first (keeps array if allocated) */
//        TrackListArea_DisposeGadgets(Gad, gdata);

//        if(dataTrackCount > 0)
//        {
//            /* Ensure array is allocated */
//            if(!TrackListArea_EnsureTrackArray(gdata))
//            {
//                return;
//            }

//            /* Create gadgets for each track */
//            for(i = 0; i < dataTrackCount; i++)
//            {

//                        struct Gadget *Gad,
//            TrackListArea *gdata,
//            TrackChannelChild *strack, AukTrack *dataTrack, int iTrack, int iChannel)

//                if(!TrackListArea_CreateTrackChannelLine(Gad, gdata, &gdata->_tracks[i], project->tracks->items[i], i))
//                {
//                    /* Failed to create gadgets, cleanup and abort */
//                    TrackListArea_DisposeGadgets(Gad, gdata);
//                    return;
//                }
//            }
//            gdata->_trackCount = dataTrackCount;
//        }
//    }
//}


// set main project - TrackListArea NULL means clean everything, back to empty state.
void TrackListArea_setTrackList(struct Gadget *Gad,AukAProject *tracklist)
{
    TrackListArea *gdata;

    if(!TrackListClassPtr || !Gad) return;

    gdata=INST_DATA(TrackListClassPtr, Gad);

    AukObjectPtr_Set(&gdata->_project,tracklist);

   // TrackListArea_updateTrackListUiToData(Gad);
}

/* Insert track at specified index, shifting existing tracks up */
void TrackListArea_insertTrack(struct Gadget *Gad, AukTrack *track, int indexToInsert)
{
    TrackListArea *gdata;

    if(!TrackListClassPtr || !Gad || !track || track->channelCount<=0) return;
    gdata = INST_DATA(TrackListClassPtr, Gad);

 printf("TrackListArea_insertTrack track->channelCount:%d\n",track->channelCount);

    /* Ensure array is allocated */
    if(!TrackListArea_EnsureTrackArray(gdata))
    {
        return;
    }

    /* Check capacity */
    if((gdata->_trackCount+1) >= gdata->_trackCapacity)
    {
        AukLog_MessageInt(AUKLOG_WARNING, AUKERR_TRACKLIST_CAPACITY_REACHED, gdata->_trackCapacity);
        return;
    }

    /* Validate index */
    if(indexToInsert < 0) indexToInsert = 0;
    if((ULONG)indexToInsert > gdata->_trackCount) indexToInsert = gdata->_trackCount;

    bdbprintf("TrackListArea_insertTrack index:%d count:%ld\n", indexToInsert, gdata->_trackCount);

    /* Shift existing tracks up to make room */
    if((ULONG)indexToInsert < gdata->_trackCount)
    {
        ULONG i;
        /* Shift from end to insert position */
        for(i = gdata->_trackCount; i > (ULONG)indexToInsert; i--)
        {
            gdata->_tracks[i] = gdata->_tracks[i-1];
        }
    }

    /* Clear the slot for new track */
    memset(&gdata->_tracks[indexToInsert], 0, sizeof(TrackChild));

    /* Create TrackChannelChild per channel */
    {
        ULONG ic;
        TrackChild *pchild = &gdata->_tracks[indexToInsert];
        pchild->_channels = (TrackChannelChild *)AllocVec(sizeof(TrackChannelChild)*track->channelCount, MEMF_CLEAR );
        if(!pchild->_channels)
        {
            /* Failed - shift back down and return */
            for(ic = indexToInsert; ic < gdata->_trackCount; ic++)
            {
                gdata->_tracks[ic] = gdata->_tracks[ic+1];
            }
            memset(&gdata->_tracks[gdata->_trackCount], 0, sizeof(TrackChild));
            return;
        }
        for(ic=0;ic<track->channelCount;ic++)
        {
            /* Create gadgets for the new track at the insert position */
            if(!TrackListArea_CreateTrackChannelLine(Gad, gdata,
                                    pchild,
                                    &pchild->_channels[ic], track,indexToInsert ,ic))
            {
                /* Failed - shift back down and return */
                //TODO well should delete create lines
                FreeVec(pchild->_channels);
                pchild->_channels = 0;

                for(ic = indexToInsert; ic < gdata->_trackCount; ic++)
                {
                    gdata->_tracks[ic] = gdata->_tracks[ic+1];
                }

                memset(&gdata->_tracks[gdata->_trackCount], 0, sizeof(TrackChild));
                return;
            }

        } // end loop per chan
  printf("///// pchild->_nbChannels %d\n",track->channelCount);
        pchild->_nbChannels = track->channelCount;
    }

    gdata->_trackCount++;

    /* Update track indices for shifted TrackHeaders */
    {
        ULONG i;
        for(i = indexToInsert + 1; i < gdata->_trackCount; i++)
        {
            TrackChannelChild *pc = &gdata->_tracks[i]._channels[0];
            if(gdata->_tracks[i]._nbChannels==0) continue;
            if(pc->_trackHeader)
            {
                SetAttrs(pc->_trackHeader, TRACKHEADER_TrackIndex, i, TAG_END);
            }
        }
    }

}

/* Remove track at specified index, shifting remaining tracks down */
void TrackListArea_removeTrack(struct Gadget *Gad, int indexToRemove)
{
    TrackListArea *gdata;
    Object *trackheader,*trackarea,*volumeRule;
    TrackChild *trackChild;
    TrackChannelChild *chan;
    ULONG i;

    if(!TrackListClassPtr || !Gad) return;
    gdata = INST_DATA(TrackListClassPtr, Gad);

    /* Nothing to remove */
    if(!gdata->_tracks || gdata->_trackCount == 0) return;

    /* Validate index */
    if(indexToRemove < 0 || (ULONG)indexToRemove >= gdata->_trackCount)
    {
        AukLog_MessageInt(AUKLOG_WARNING, AUKERR_TRACKLIST_INVALID_INDEX, indexToRemove);
        return;
    }

    bdbprintf("TrackListArea_removeTrack index:%d count:%ld\n", indexToRemove, gdata->_trackCount);

    trackChild = &gdata->_tracks[indexToRemove];
    /* remove per chan */
    for(i=0 ; i<trackChild->_nbChannels ; i++)
    {
        chan = &trackChild->_channels[i];

        /* Dispose gadgets at this index using LAYOUT_RemoveChild */
        trackheader = chan->_trackHeader;
        if(trackheader)
        {
            /* do that first, may help messaging */
            chan->_trackHeader = NULL;
            SetAttrs(Gad, LAYOUT_RemoveChild, (ULONG)trackheader, TAG_END);
            BoopsiDispose_Later( ObjectLateDisposer, trackheader);
        }

        trackarea =chan->_trackArea;
        if(trackarea)
        {
             /* do that first, may help messaging */
            chan->_trackArea = NULL;
            SetAttrs(Gad, LAYOUT_RemoveChild, (ULONG)trackarea, TAG_END);
            BoopsiDispose_Later( ObjectLateDisposer, trackarea);
        }

        volumeRule =chan->_volumeRule;
        if(volumeRule)
        {
             /* do that first, may help messaging */
            chan->_volumeRule = NULL;
            SetAttrs(Gad, LAYOUT_RemoveChild, (ULONG)volumeRule, TAG_END);
            BoopsiDispose_Later( ObjectLateDisposer, volumeRule);
        }


    } // end loop per chan


    AukObjectPtr_Release((AukObjectPtr *)&trackChild->_dataTrack);

    /* Shift remaining tracks down */
    for(i = indexToRemove; i < gdata->_trackCount - 1; i++)
    {
        gdata->_tracks[i] = gdata->_tracks[i+1];
    }

    /* Clear the last slot (now unused) */
    memset(&gdata->_tracks[gdata->_trackCount - 1], 0, sizeof(TrackChild));

    gdata->_trackCount--;

    /* Update track indices for shifted TrackHeaders */
    for(i = indexToRemove; i < gdata->_trackCount; i++)
    {    
        trackChild = &gdata->_tracks[i];

        if( trackChild->_dataTrack &&
            trackChild->_nbChannels>0 && trackChild->_channels[0]._trackHeader)
        {
            SetAttrs( trackChild->_channels[0]._trackHeader, TRACKHEADER_TrackIndex,
                trackChild->_dataTrack->trackIndex, TAG_END);
        }
    }
    /* Will need big refesh with layout and render */
    SetGadgetAttrs(Gad,CurrentMainWindow, NULL,TRACKLIST_Refresh,TRUE,TAG_END);

}

/* Swap two tracks by their indices */
void TrackListArea_swapTracks(struct Gadget *Gad, int indexA, int indexB)
{
    TrackListArea *gdata;
    TrackChild temp;

    if(!TrackListClassPtr || !Gad) return;
    gdata = INST_DATA(TrackListClassPtr, Gad);

    /* Nothing to swap */
    if(!gdata->_tracks || gdata->_trackCount == 0) return;

    /* Same index, nothing to do */
    if(indexA == indexB) return;

    /* Validate indices */
    if(indexA < 0 || (ULONG)indexA >= gdata->_trackCount ||
       indexB < 0 || (ULONG)indexB >= gdata->_trackCount)
    {
        AukLog_Message(AUKLOG_WARNING, AUKERR_TRACKLIST_INVALID_INDEX);
        return;
    }

    bdbprintf("TrackListArea_swapTracks %d <-> %d\n", indexA, indexB);

    /* Swap the TrackChild entries */
    temp = gdata->_tracks[indexA];
    gdata->_tracks[indexA] = gdata->_tracks[indexB];
    gdata->_tracks[indexB] = temp;

    {
        Object *trackHeaderA = (gdata->_tracks[indexA]._nbChannels>0)?
            gdata->_tracks[indexA]._channels[0]._trackHeader:NULL;
        Object *trackHeaderB = (gdata->_tracks[indexB]._nbChannels>0)?
            gdata->_tracks[indexB]._channels[0]._trackHeader:NULL;

        /* Update track indices for swapped TrackHeaders */
        if(trackHeaderA) SetAttrs(trackHeaderA, TRACKHEADER_TrackIndex, indexA, TAG_END);
        if(trackHeaderB) SetAttrs(trackHeaderB, TRACKHEADER_TrackIndex, indexB, TAG_END);

    }
}

void TrackListArea_trackModified(struct Gadget *Gad,AukTrack *track)
{
    /* Track content modified - for now we don't need to do anything
     * as the TrackGadgets will handle their own rendering based on data */
}

void TrackListArea_CheckTrackChannels( struct Gadget *Gad,int itrack)
{
   TrackListArea *gdata;
    TrackChild *strack;
    AukTrackPtr aukTrack;
    ULONG nnbc;
    //TrackChannelChild *schan;
    if(!Gad) return;
    gdata=INST_DATA(OCLASS(Gad), Gad);

    if(itrack>= (int)gdata->_trackCount) return;
    strack = &gdata->_tracks[itrack];
    nnbc = strack->_dataTrack->channelCount;
    if(strack->_nbChannels < nnbc)
    {
        TrackChannelChild *channels = (TrackChannelChild *)
                AllocVec(sizeof(TrackChannelChild)*nnbc, MEMF_CLEAR );

        memcpy(channels,strack->_channels,sizeof(TrackChannelChild) * strack->_nbChannels);
        while(strack->_nbChannels<nnbc)
        {
            /* Create gadgets for the new track at the insert position */
            int r = TrackListArea_CreateTrackChannelLine(Gad, gdata,
                                strack,
                                &channels[strack->_nbChannels], strack->_dataTrack,itrack , strack->_nbChannels);

            strack->_nbChannels ++;
        }
        FreeVec(strack->_channels);
        strack->_channels = channels;

        // need relayout...s
    }
//    else
//    if(strack->_nbChannels > strack->_dataTrack->channelCount)
//    {

//    }


}

