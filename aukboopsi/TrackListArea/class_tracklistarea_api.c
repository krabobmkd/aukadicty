
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
#include <stdio.h>

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

extern struct IClass   *TrackListClassPtr;
extern struct IClass   *TrackHeaderClassPtr;
extern struct IClass   *TrackAreaClassPtr;

/* This can be reallocated, so this is shared like this */
extern struct Window *CurrentMainWindow;


extern BoopsiDisposeQueue *ObjectLateDisposer;

//struct Region *TrackListArea_clipRegion=NULL;
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

extern Object *TargetInstance;

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
                            ICA_TARGET,TargetInstance,
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
                                   TRACKAREA_TimeSelection,(ULONG)&(gdata->_project->selection), /* the shared selection state */
                                   ICA_TARGET,TargetInstance,
                                   //TRACKHEADER_TrackIndex,iTrack,
                                   TAG_END);
    if(schan->_trackArea)
    {
        SetAttrs(Gad,LAYOUT_AddChild,(ULONG)schan->_trackArea,
                            CHILD_NoDispose,TRUE,
                            TAG_END);
    }

    /* retain data we sync: */
    AukObjectPtr_Set((AukObjectPtr *)&strack->_dataTrack,(AukObject *)dataTrack);
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

    AukObjectPtr_Set((AukObjectPtr*)&gdata->_project,(AukObject *)tracklist);

   // TrackListArea_updateTrackListUiToData(Gad);
}

/* Insert track at specified index, shifting existing tracks up */
void TrackListArea_insertTrack(struct Gadget *Gad, AukTrack *track, int indexToInsert)
{
    TrackListArea *gdata;

    if(!TrackListClassPtr || !Gad || !track || track->channelCount<=0) return;
    gdata = INST_DATA(TrackListClassPtr, Gad);

// printf("TrackListArea_insertTrack track->channelCount:%d\n",track->channelCount);

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

// should be just insert
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

/* used by zoom, force full tile refresh on all tracks */
void TrackListArea_FullTrackRedraw(struct Gadget *Gad)
{
    ULONG itrack;
   TrackListArea *gdata;
    TrackChild *strack;
    AukTrackPtr aukTrack;
    ULONG nnbc;

    if(!Gad ) return;
    gdata=INST_DATA(OCLASS(Gad), Gad);

    for( itrack=0 ; itrack<gdata->_trackCount ; itrack++ )
    {
        ULONG ichan;
        strack = &gdata->_tracks[itrack];

        for( ichan=0 ; ichan<strack->_nbChannels ; ichan++)
        {
            TrackChannelChild *chanchild = &strack->_channels[ichan];
            if(chanchild->_layouted && chanchild->_trackArea)
            {
                //todo optimize, if not selected should redraw.
                /* because they are InifniteScroll, need explicit tile refresh */
                SetAttrs(chanchild->_trackArea, INFINITESCROLL_FullTilesRefresh,TRUE,TAG_END);
            }
        }
    }

    SetGadgetAttrs(Gad,CurrentMainWindow,NULL, TRACKLIST_JustTracksRefresh,TRUE,TAG_END);

}

/* used by slide, .. , force full tile refresh on 1 track */
void TrackListArea_TrackRedraw(struct Gadget *Gad, int itrack)
{
   TrackListArea *gdata;
    TrackChild *strack;
    AukTrackPtr aukTrack;
    ULONG nnbc;
    ULONG ichan;

    if(!Gad ) return;
    gdata=INST_DATA(OCLASS(Gad), Gad);

    if(itrack<0 || itrack>=gdata->_trackCount) return;
    strack = &gdata->_tracks[itrack];

    for( ichan=0 ; ichan<strack->_nbChannels ; ichan++)
    {
        TrackChannelChild *chanchild = &strack->_channels[ichan];
        if(chanchild->_layouted && chanchild->_trackArea)
        {
            //todo optimize, if not selected should redraw.
            /* because they are InifniteScroll, need explicit tile refresh */
            SetAttrs(chanchild->_trackArea, INFINITESCROLL_FullTilesRefresh,TRUE,TAG_END);
        }
    }


    SetGadgetAttrs(Gad,CurrentMainWindow,NULL, TRACKLIST_JustTracksRefresh,TRUE,TAG_END);

}

/* sent during moving the select selector */
void TrackListArea_SetCurrentSelection(struct Gadget *Gad, AukSelection *selection)
{
    /*
        How to refresh on selection change ?
        Very hard question, there could have some TrackArea changing and other not.
        Also the TimeRule display selection.
        As TrackArea and TimeRule are InfiniteScroll, refresh affect all buffering.
        Strategy:
         - send very precise message about selection change
         - send GM_RENDER at our level


        // redrawing everything could be a littyle too much.
    */


    /* propagate state with full redraw */
    TrackListArea_FullTrackRedraw(Gad);

}
/* sent during moving the zoom selctor */
void TrackListArea_SetZoomSelectorRun(struct Gadget *Gad, AukSelection *zoomsel)
{
   TrackListArea *gdata;
    TrackChild *strack;
    AukTrackPtr aukTrack;
    ULONG nnbc;

    if(!Gad) return;
    gdata=INST_DATA(OCLASS(Gad), Gad);


}
/* Apply last value sent to TrackListArea_SetZoomSelectorRun() at bt up */
void TrackListArea_ApplyZoomSelectorRun(struct Gadget *Gad)
{
   TrackListArea *gdata;
    TrackChild *strack;
    AukTrackPtr aukTrack;
    ULONG nnbc;

    if(!Gad) return;
    gdata=INST_DATA(OCLASS(Gad), Gad);
}
