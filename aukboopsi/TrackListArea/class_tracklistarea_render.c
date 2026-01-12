
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/layers.h>

#ifdef __SASC
//    #include "minialib.h"
    #include <clib/alib_protos.h>
#else
    // GCC
    #include "minialib.h"
#endif

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
  ULONG i;
 ULONG prevDomainHeight;
// ULONG prevHeight;
    gdata=INST_DATA(C, Gad);


    topedge = Gad->TopEdge;
    leftedge = Gad->LeftEdge;
    width = Gad->Width;
    height = Gad->Height;

    gdata->_framerec.MinX = leftedge;
    gdata->_framerec.MinY = topedge;
    gdata->_framerec.MaxX = leftedge + width  -1;
    gdata->_framerec.MaxY = topedge  + height -1;

    prevDomainHeight = gdata->_domainHeight;

    /* Layout child gadgets (TrackHeaders and TrackGadgets) */
    if(gdata->_tracks && gdata->_trackCount > 0)
    {
        ULONG totalDomainHeight = 0;

        /* Default track height if not set */
        if(gdata->_defaulTrackHeight == 0) gdata->_defaulTrackHeight = 40;
        /* Default header width if not set */
        if(gdata->_headerWidth == 0) gdata->_headerWidth = 100;


        /* count total height first */
        for(i = 0; i < gdata->_trackCount; i++)
        {
            UWORD trackHeight=0;
            TrackChild *strack;
            strack = &gdata->_tracks[i];
            trackHeight = strack->_prefHeight;
            if(trackHeight==0) trackHeight = gdata->_defaulTrackHeight;
            totalDomainHeight += trackHeight;
        }

        if((totalDomainHeight-gdata->_scrollY) < height)
        {
            gdata->_scrollY = totalDomainHeight-height;
        }
        if(gdata->_scrollY<0) gdata->_scrollY=0;
        /* Start from the top, accounting for vertical scroll */
        trackTop = topedge - gdata->_scrollY;


        /* Layout each track row */
        for(i = 0; i < gdata->_trackCount; i++)
        {
            TrackArea *trackArea;
            TrackChild *strack;
            struct Gadget *headerGad;
            struct Gadget *trackGad;
            UWORD trackHeight=0;

            strack = &gdata->_tracks[i];
            headerGad = (struct Gadget *) strack->_trackHeader;
            trackGad = (struct Gadget *) strack->_trackArea;
            strack->_layouted = 0;

            if(trackGad)
            {
                trackArea = INST_DATA(TrackAreaClassPtr, trackGad);
            }
            trackHeight = strack->_prefHeight;
            if(trackHeight==0) trackHeight = gdata->_defaulTrackHeight;

            /* Accumulate total domain height */


            /* Skip tracks that are scrolled out of view (above visible area)
            disable also if is below visible area
            */
            if((trackTop + trackHeight < topedge ) ||
                (trackTop > topedge + height)
                 )
            {
                if(headerGad) headerGad->Width = 4; // how we say it's not layouted.
                trackGad->Width = 4;
                trackTop += trackHeight ;
                continue;
            }

            if(headerGad )
            {
                /* Position TrackHeader on the left */
                headerGad->LeftEdge = leftedge;
                headerGad->TopEdge = trackTop;
                headerGad->Width = gdata->_headerWidth;
                headerGad->Height = trackHeight;

                /* Call child's GM_LAYOUT */
                DoMethodA((Object*)headerGad, (Msg)layout);
            }

            if(trackGad)
            {

                /* Position TrackArea on the right, after header */
                trackGad->LeftEdge = leftedge + gdata->_headerWidth;
                trackGad->TopEdge = trackTop;
                trackGad->Width = width - gdata->_headerWidth;
                trackGad->Height = trackHeight;

                /* Call child's GM_LAYOUT */
               DoMethodA((Object*)trackGad, (Msg)layout);
            }
            strack->_layouted = 1;
            strack->_top = trackTop;
            strack->_bottom = trackTop+trackHeight;
            strack->_xmid = leftedge+ gdata->_headerWidth;
// _defaulTrackHeight
            trackTop += trackHeight;
        }

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

//ULONG TrackHeader_Render(Class *C, struct Gadget *Gad, struct gpRender *Render, ULONG update);
// ULONG TrackArea_Render_rp( struct RastPort *rp,Class *C, struct Gadget *Gad, struct gpRender *Render);
// ULONG TrackHeader_Render_rp( struct RastPort *rp,Class *C, struct Gadget *Gad, struct gpRender *Render);

/* draw yourself, in the appropriate state */
ULONG TrackListArea_Render(Class *C, struct Gadget *Gad, struct gpRender *Render,int filter)
{
  TrackListArea *gdata;
  struct RastPort *rp;
  ULONG retval=1;

  gdata=INST_DATA(C, Gad);
  // also sent from GM_GOINACTIVE (4).
  if(Render->MethodID==GM_RENDER)
  {
    rp=Render->gpr_RPort;
   // update=Render->gpr_Redraw;
  }
  else
  {
    return 0; //
//    rp = ObtainGIRPort(Render->gpr_GInfo);
  }

  if(rp)
  {
  	int bLayerUpdating=FALSE;
    LONG i;
    LONG topedge,leftedge,width,height;
    topedge = Gad->TopEdge;
    leftedge = Gad->LeftEdge;
    width = Gad->Width;
    height = Gad->Height;
    struct Region *oldClipRegion;

	if( ( rp->Layer->Flags & LAYERUPDATING ) != 0L )
	{
		bLayerUpdating = TRUE;
		EndUpdate(rp->Layer, FALSE);
//		bdbprintf(" ****Render->MethodID:%08lx LAYERUPDATING\n",(int)Render->MethodID);
	}

    oldClipRegion = InstallClipRegion( rp->Layer, gdata->_clipRegion);

    if(gdata->_tracks && gdata->_trackCount > 0)
    {

        /* Layout each track row */
        for(i = 0; i < gdata->_trackCount; i++)
        {
            TrackChild *strack;
            struct Gadget *headerGad;
            struct Gadget *trackGad;
            strack = &gdata->_tracks[i];
            headerGad = (struct Gadget*)strack->_trackHeader;
            trackGad = (struct Gadget*)strack->_trackArea;

            // if layouted
            if(!strack->_layouted) continue;

            /* Call child's GM_RENDER */

          // recurse
//re, test
//         if(headerGad && ((filter & 2)!=0)) // if layouted && selected fore refresh
//         {
//            DoMethodA((Object*)headerGad, (Msg)Render); // not DoGadgetMethodA in that case
//         }
         if(trackGad && ((filter & 1)!=0)) // if layouted && selected fore refresh
         {
            DoMethodA((Object*)trackGad, (Msg)Render); // not DoGadgetMethodA in that case
         }
        } // end loop per track
    } // end if any track

    InstallClipRegion( rp->Layer,oldClipRegion); // important to pass NULL if oldClipRegion is NULL.

    if(bLayerUpdating)
    {
        BeginUpdate(rp->Layer);
    }

    // if (Render->MethodID != GM_RENDER)
    //   ReleaseGIRPort(rp);

    // vertical drawing management:
    // todo: recursively draw tracks on their projected rectangle

    // then draw eventually clear a rectangle of the empty scrool Area.
  } // end if rp
  return(retval);
}



/** Helper - dispose all allocated gadgets */
void TrackListArea_DisposeGadgets(TrackListArea *gdata)
{
    ULONG i;
    if(!gdata) return;

    /* Dispose all TrackHeader gadgets */
    if(gdata->_tracks)
    {
        for(i = 0; i < gdata->_trackCount; i++)
        {
            if(gdata->_tracks[i]._trackHeader)
            {
                DisposeObject(gdata->_tracks[i]._trackHeader);
            }
            if(gdata->_tracks[i]._trackArea)
            {
                DisposeObject(gdata->_tracks[i]._trackArea);
            }
        }

        FreeVec(gdata->_tracks);
        gdata->_tracks = NULL;
    }
    gdata->_trackCount = 0;

}
extern Class *AppModelClass;
static int TrackListArea_CreateTrackLine(
            TrackListArea *gdata,
            TrackChild *strack, AukTrack *dataTrack, int iTrack)
{
    char *trackname=NULL;
    ULONG TRACKHEADER_Nametag =TAG_END;

    struct AukStyle *styleSheet = gdata->_styleSheet;

    if(dataTrack && dataTrack->name) trackname = dataTrack->name;
    if(trackname) TRACKHEADER_Nametag = TRACKHEADER_Name;
    bdbprintf("TrackListArea_CreateTrackLine styleSheet:%08x\n",(int)styleSheet);
    /* Create TrackHeader gadget */
    strack->_trackHeader = NULL;
//        NewObject(TRACKHEADER_GetClass(), NULL,
//                                     TRACKHEADER_StyleSheet, (ULONG)styleSheet,
//                                     TRACKHEADER_TrackIndex,iTrack,
//                                     ICA_TARGET,AppModelClass,
//                                     TRACKHEADER_Nametag,trackname, // optional, must be last
//                                     TAG_END);
    //if(!strack->_trackHeader ) return 0;
    /* Create TrackArea - pass data track for reference counted retention */
    strack->_trackArea = NewObject(TRACKAREA_GetClass(), NULL,
                                   INFINITESCROLL_PPosition,(ULONG) &gdata->_timeProjection._pixAtLeft,
                                   TRACKAREA_StyleSheet, (ULONG)styleSheet,
                                   TRACKAREA_PTimeProjection,(ULONG)&gdata->_timeProjection,
                                   TRACKAREA_DataTrack,(ULONG)dataTrack,
                                   TAG_END);
    if(!strack->_trackArea ) return 0;
    /* data we sync (weak reference for quick access): */
    strack->_dataTrack = dataTrack;
    // default value
    strack->_prefHeight = 96;

    return 1;
}

/** private,
* manage synchronisation of tracks
* alloc/free/realloc Tracks, when needed and recursively
* implicitely ask for sounds ...
*/
static void TrackListArea_updateTrackListUiToData(struct Gadget *Gad)
{
    TrackListArea *gdata;
    AukAProject *project;
    ULONG dataTrackCount;
    ULONG i,nbTracksAlreadyInSync;

    if(!TrackListClassPtr || !Gad) return;
    gdata = INST_DATA(TrackListClassPtr, Gad);

    project = gdata->_project;
    if(!project)
    {
        /* No project, clean up everything */
        TrackListArea_DisposeGadgets(gdata);
        return;
    }

    /* Get the track count from project */
    dataTrackCount = AukArray_GetCount(project->tracks);

    /* verify how much it changes */
//    nbTracksAlreadyInSyncAtStart=0;
//    nbTracksMinusOneAtEnd=0;
//    for(i = 0; i < trackCount; i++)
//    {
//        _trackCount
//    }

    /* If count changed, reallocate arrays */
    if(dataTrackCount != gdata->_trackCount)
    {       
        /* Dispose old gadgets first */
        TrackListArea_DisposeGadgets(gdata);

        if(dataTrackCount > 0)
        {
            // UWORD ipos = 65534;

            /* Allocate new arrays */
            gdata->_tracks = (TrackChild*)AllocVec(dataTrackCount * sizeof(TrackChild), MEMF_CLEAR);

            if(!gdata->_tracks)
            {
                /* Allocation failed, cleanup */
                TrackListArea_DisposeGadgets(gdata);
                return;
            }

            gdata->_trackCount = dataTrackCount;

            /* Create gadgets for each track */
            for(i = 0; i < dataTrackCount; i++)
            {
                if(!TrackListArea_CreateTrackLine(gdata, &gdata->_tracks[i], project->tracks->items[i],i ))
                {
                    /* Failed to create gadgets, cleanup and abort */
                    TrackListArea_DisposeGadgets(gdata);
                    return;
                }
            }
        }
    }

}


// set main project - TrackListArea NULL means clean everything, back to empty state.
void TrackListArea_setTrackList(struct Gadget *Gad,AukAProject *tracklist)
{
    TrackListArea *gdata;

    if(!TrackListClassPtr || !Gad) return;

    gdata=INST_DATA(TrackListClassPtr, Gad);

    AukObjectPtr_Set(&gdata->_project,tracklist);

    TrackListArea_updateTrackListUiToData(Gad);
}

/* events */
void TrackListArea_addTrack( struct Gadget *Gad,AukTrack *track)
{
    /* When a track is added, resync the entire gadget array */
    //TrackListArea_updateTrackListUiToData(Gad);
    TrackListArea *gdata;
    AukAProject *project;
    ULONG dataTrackCount;
    ULONG i,nbTracksAlreadyInSync;
    TrackChild*ntracks;

    if(!TrackListClassPtr || !Gad) return;
    gdata = INST_DATA(TrackListClassPtr, Gad);

    project = gdata->_project;
    if(!project)
    {
        /* No project, clean up everything */
        TrackListArea_DisposeGadgets(gdata);
        return;
    }

    /* Get the track count from project */
    dataTrackCount = AukArray_GetCount(project->tracks);
    if(dataTrackCount != gdata->_trackCount +1 )
    {
        // general update
        TrackListArea_updateTrackListUiToData(Gad);
        return;
    }

    /* If count changed, reallocate arrays */

    /* Allocate new arrays */
    ntracks = (TrackChild*)AllocVec(dataTrackCount * sizeof(TrackChild), MEMF_CLEAR);
    if(!ntracks)
    {
        /* Allocation failed, cleanup */
        TrackListArea_DisposeGadgets(gdata);
        return;
    }
    if( gdata->_trackCount>0)
    {
        memcpy(ntracks,gdata->_tracks,sizeof(TrackChild)*gdata->_trackCount);
    }
    FreeVec(gdata->_tracks);

    gdata->_tracks = ntracks;

    /* Create gadgets for this track */
    i =  gdata->_trackCount;
    if(!TrackListArea_CreateTrackLine(gdata, &gdata->_tracks[i], project->tracks->items[i], i ))
    {
        /* Failed to create gadgets, cleanup and abort */
        TrackListArea_DisposeGadgets(gdata);
        return;
    }

    gdata->_trackCount = dataTrackCount;

    // - - - - -


}

void TrackListArea_removeTrack(struct Gadget *Gad,AukTrack *track)
{
    /* When a track is removed, resync the entire gadget array */
    TrackListArea_updateTrackListUiToData(Gad);
}

void TrackListArea_trackModified(struct Gadget *Gad,AukTrack *track)
{
    /* Track content modified - for now we don't need to do anything
     * as the TrackGadgets will handle their own rendering based on data */
}
// void TrackListArea_Refresh(struct Gadget *Gad, struct Window *window)
// {
//     RethinkLayout(Gad,window,NULL,0);
// }
