
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

#include <proto/layout.h>
#include <gadgets/layout.h>

#include <proto/button.h>
#include <gadgets/button.h>

#include <aukarray.h>
#include <auktrack.h>

// memcpy
#include <string.h>

/* Default capacity for track array allocation */
#define TRACKLIST_DEFAULT_CAPACITY 32

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

/* draw yourself, in the appropriate state */
ULONG TrackListArea_Render(Class *C, struct Gadget *Gad, struct gpRender *Render,int filter)
{
    struct Region *oldClipRegion;
    TrackListArea *gdata;
    struct RastPort *rp;
    int bLayerUpdating=FALSE;
    LONG i;
    LONG topedge,leftedge,width,height;

    if(Render->MethodID==GM_RENDER &&  Render->gpr_RPort )
    {
        rp=Render->gpr_RPort;
    }
    else
    {
        return 1;
    }

    gdata=INST_DATA(C, Gad);

    topedge = Gad->TopEdge;
    leftedge = Gad->LeftEdge;
    width = Gad->Width;
    height = Gad->Height;


	// if( ( rp->Layer->Flags & LAYERUPDATING ) != 0L )
	// {
	// 	bLayerUpdating = TRUE;
	// 	EndUpdate(rp->Layer, FALSE);
	// 	bdbprintf(" ****Render->MethodID:%08lx LAYERUPDATING\n",(int)Render->MethodID);
	// }

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
        if(headerGad && ((filter & 2)!=0) && bLayerUpdating == 0) // if layouted && selected for refresh
        {
//            SetAPen(rp, gdata->_styleSheet->trackHeaderBG.pen);
//            RectFill(rp,headerGad->LeftEdge,
//                        headerGad->TopEdge,
//                        headerGad->LeftEdge + headerGad->Width -1,
//                        headerGad->TopEdge + headerGad->Height -1);

            DoMethodA((Object*)headerGad, (Msg)Render); // not DoGadgetMethodA in that case
        }
         if(trackGad && ((filter & 1)!=0)) // if layouted && selected for refresh
         {
            DoMethodA((Object*)trackGad, (Msg)Render); // not DoGadgetMethodA in that case
         }
        } // end loop per track
    } // end if any track

    InstallClipRegion( rp->Layer,oldClipRegion); // important to pass NULL if oldClipRegion is NULL.

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
    ULONG i;
    if(!gdata) return;
    bdbprintf("TrackListArea_DisposeGadgets() ->all\n");
    /* Dispose all active TrackHeader/TrackArea gadgets */
    if(gdata->_tracks)
    {
        for(i = 0; i < gdata->_trackCount; i++)
        {
            if(gdata->_tracks[i]._trackHeader)
            {
                /* LAYOUT_RemoveChild: This will destroy the object as well. */
                SetGadgetAttrs(Gad,CurrentMainWindow,NULL,
                            LAYOUT_RemoveChild,(ULONG)gdata->_tracks[i]._trackHeader,TAG_END);
            }
            if(gdata->_tracks[i]._trackArea)
            {
                SetGadgetAttrs(Gad,CurrentMainWindow,NULL,
                            LAYOUT_RemoveChild,(ULONG)gdata->_tracks[i]._trackArea,TAG_END);
            }
             /* release data we sync: */
            AukObjectPtr_Release(&gdata->_tracks[i]._dataTrack);

            /* Clear the slot */
            gdata->_tracks[i]._trackHeader = NULL;
            gdata->_tracks[i]._trackArea = NULL;
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
static int TrackListArea_CreateTrackLine(
            struct Gadget *Gad,
            TrackListArea *gdata,
            TrackChild *strack, AukTrack *dataTrack, int iTrack)
{
    char *trackname=NULL;
    ULONG TRACKHEADER_Nametag =TAG_END;

    struct AukStyle *styleSheet = gdata->_styleSheet;

    if(dataTrack && dataTrack->name) trackname = dataTrack->name;
    if(trackname) TRACKHEADER_Nametag = TRACKHEADER_Name;
    bdbprintf("TrackListArea_CreateTrackLine trackname:%s\n",trackname);
    /* Create TrackHeader gadget */
    strack->_trackHeader =
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
    //TODO TRACKHEADER_TrackIndex TRACKHEADER_Name should be later setAttribs()

    if(strack->_trackHeader)
    {
        SetAttrs(Gad,LAYOUT_AddChild,(ULONG)strack->_trackHeader,TAG_END);
    }
    //if(!strack->_trackHeader ) return 0;
    /* Create TrackArea - pass data track for reference counted retention */
    strack->_trackArea = NewObject(TRACKAREA_GetClass(), NULL,
                                   INFINITESCROLL_PPosition,(ULONG) &gdata->_timeProjection._pixAtLeft,
                                   TRACKAREA_StyleSheet, (ULONG)styleSheet,
                                   TRACKAREA_PTimeProjection,(ULONG)&gdata->_timeProjection,
                                   TRACKAREA_DataTrack,(ULONG)dataTrack,
                                   TAG_END);
    if(strack->_trackArea)
    {
        SetAttrs(Gad,LAYOUT_AddChild,(ULONG)strack->_trackArea,TAG_END);
    }

    /* retain data we sync: */
    AukObjectPtr_Set(&strack->_dataTrack,dataTrack);
    // default value
    strack->_prefHeight = 96;

    return 1;
}

/** private,
* Full sync of track UI to data.
* Called on project set or when incremental updates can't handle changes.
*/
static void TrackListArea_updateTrackListUiToData(struct Gadget *Gad)
{
    TrackListArea *gdata;
    AukAProject *project;
    ULONG dataTrackCount;
    ULONG i;

    if(!TrackListClassPtr || !Gad) return;
    gdata = INST_DATA(TrackListClassPtr, Gad);

    project = gdata->_project;
    if(!project)
    {
        /* No project, clean up everything */
        TrackListArea_DisposeGadgets(Gad,gdata);
        return;
    }

    /* Get the track count from project */
    dataTrackCount = AukArray_GetCount(project->tracks);

    /* Check capacity - if data exceeds our capacity, we need a full rebuild */
    if(dataTrackCount > TRACKLIST_DEFAULT_CAPACITY)
    {
        AukLog_MessageInt(AUKLOG_WARNING, AUKERR_TRACKLIST_CAPACITY_REACHED, TRACKLIST_DEFAULT_CAPACITY);
        /* For now, just handle up to capacity */
        dataTrackCount = TRACKLIST_DEFAULT_CAPACITY;
    }

    /* If count changed, rebuild UI */
    if(dataTrackCount != gdata->_trackCount)
    {
        /* Dispose old gadgets first (keeps array if allocated) */
        TrackListArea_DisposeGadgets(Gad, gdata);

        if(dataTrackCount > 0)
        {
            /* Ensure array is allocated */
            if(!TrackListArea_EnsureTrackArray(gdata))
            {
                return;
            }

            /* Create gadgets for each track */
            for(i = 0; i < dataTrackCount; i++)
            {
                if(!TrackListArea_CreateTrackLine(Gad, gdata, &gdata->_tracks[i], project->tracks->items[i], i))
                {
                    /* Failed to create gadgets, cleanup and abort */
                    TrackListArea_DisposeGadgets(Gad, gdata);
                    return;
                }
            }
            gdata->_trackCount = dataTrackCount;
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

/* Insert track at specified index, shifting existing tracks up */
void TrackListArea_insertTrack(struct Gadget *Gad, AukTrack *track, int indexToInsert)
{
    TrackListArea *gdata;
    ULONG i;

    if(!TrackListClassPtr || !Gad || !track) return;
    gdata = INST_DATA(TrackListClassPtr, Gad);

    /* Ensure array is allocated */
    if(!TrackListArea_EnsureTrackArray(gdata))
    {
        return;
    }

    /* Check capacity */
    if(gdata->_trackCount >= gdata->_trackCapacity)
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
        /* Shift from end to insert position */
        for(i = gdata->_trackCount; i > (ULONG)indexToInsert; i--)
        {
            gdata->_tracks[i] = gdata->_tracks[i-1];
        }
    }

    /* Clear the slot for new track */
    memset(&gdata->_tracks[indexToInsert], 0, sizeof(TrackChild));

    /* Create gadgets for the new track at the insert position */
    if(!TrackListArea_CreateTrackLine(Gad, gdata, &gdata->_tracks[indexToInsert], track, indexToInsert))
    {
        /* Failed - shift back down and return */
        for(i = indexToInsert; i < gdata->_trackCount; i++)
        {
            gdata->_tracks[i] = gdata->_tracks[i+1];
        }
        memset(&gdata->_tracks[gdata->_trackCount], 0, sizeof(TrackChild));
        return;
    }

    gdata->_trackCount++;

    /* Update track indices for shifted TrackHeaders */
    for(i = indexToInsert + 1; i < gdata->_trackCount; i++)
    {
        if(gdata->_tracks[i]._trackHeader)
        {
            SetAttrs(gdata->_tracks[i]._trackHeader, TRACKHEADER_TrackIndex, i, TAG_END);
        }
    }

}

/* Remove track at specified index, shifting remaining tracks down */
void TrackListArea_removeTrack(struct Gadget *Gad, AukTrack *track, int indexToRemove)
{
    TrackListArea *gdata;
    Object *trackheader,*trackarea;
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

    /* Dispose gadgets at this index using LAYOUT_RemoveChild */
    trackheader = gdata->_tracks[indexToRemove]._trackHeader;
    if(trackheader)
    {   // do that first !
        gdata->_tracks[indexToRemove]._trackHeader = NULL;
    // CHILD_NoDispose
        SetGadgetAttrs(Gad, CurrentMainWindow, NULL,
                    LAYOUT_RemoveChild, (ULONG)trackheader, TAG_END);
//struct GadgetInfo
//        struct gpGoInactive ina;
//        ina.MethodID = GM_GOINACTIVE;
//        ina.gpgi_GInfo = NULL;
//        ina.gpgi_Abort = 1;

//        DoMethodA(trackheader,&ina);

//        SetAttrs(Gad, CHILD_NoDispose, TRUE, LAYOUT_RemoveChild, (ULONG)trackheader, TAG_END);

    }
 exit(0);
    trackarea = gdata->_tracks[indexToRemove]._trackArea;
    if(trackarea)
    {
         // do that first !
        gdata->_tracks[indexToRemove]._trackArea = NULL;
        SetGadgetAttrs(Gad, CurrentMainWindow, NULL,
                    LAYOUT_RemoveChild, (ULONG)trackarea, TAG_END);
//        SetAttrs(Gad, LAYOUT_RemoveChild, (ULONG)trackarea, TAG_END);

    }
 exit(0);
    AukObjectPtr_Release(&gdata->_tracks[indexToRemove]._dataTrack);

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
        if(gdata->_tracks[i]._trackHeader)
        {
            SetAttrs(gdata->_tracks[i]._trackHeader, TRACKHEADER_TrackIndex, i, TAG_END);
        }
    }
    /* Will need big refesh with alyout and render */
    /*TODO->message it*/
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

    /* Update track indices for swapped TrackHeaders */
    if(gdata->_tracks[indexA]._trackHeader)
    {
        SetAttrs(gdata->_tracks[indexA]._trackHeader, TRACKHEADER_TrackIndex, indexA, TAG_END);
    }
    if(gdata->_tracks[indexB]._trackHeader)
    {
        SetAttrs(gdata->_tracks[indexB]._trackHeader, TRACKHEADER_TrackIndex, indexB, TAG_END);
    }
}

void TrackListArea_trackModified(struct Gadget *Gad,AukTrack *track)
{
    /* Track content modified - for now we don't need to do anything
     * as the TrackGadgets will handle their own rendering based on data */
}
// void TrackListArea_Refresh(struct Gadget *Gad)
// {
//     RethinkLayout(Gad,window,NULL,0);
// }
