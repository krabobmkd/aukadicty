
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <intuition/screens.h>
#include <intuition/icclass.h>

#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/alib.h>
#include <proto/dos.h>

#include <proto/layout.h>
#include <gadgets/layout.h>

#include "compilers.h"
#include "bdbprintf.h"

#include "TrackListView.h"

#include <proto/scroller.h>
#include <gadgets/scroller.h>

#include <proto/requester.h>
#include <classes/requester.h>

#include <proto/label.h>
#include <images/label.h>

#include <proto/layout.h>
#include <gadgets/layout.h>
// to read properties
#include <gadgets/button.h>
#include <gadgets/slider.h>

#include "TimeRule/class_timerule.h"
#include "TimeRule/class_timerule_private.h"
#include "TrackArea/class_trackarea.h"
#include "TrackHeader/class_trackheader.h"
#include "TrackListArea/class_tracklistarea.h"
#include "TrackListArea/class_tracklistarea_private.h"
#include "VolumeRule/class_volumerule.h"
#include "VolumeRule/class_volumerule_private.h"

#include "gadgetid.h"

#include <aukobject.h>
// audio tracks project
#include <aukaproject.h>
#include <auktrack.h>
#include "auklocale.h"
#include "bdbprintf.h"

#ifdef Remove
#undef Remove
#endif

/* This can be reallocated, so this is shared like this */
extern struct Window *CurrentMainWindow;

// because it's word and we need precision on large domain...
#define SCROLLERH_FIXEDTOTAL 16384

extern struct Task	*myTask;

void cleanexit(const char *pmessage);

/* Mapping array to connect scroller to trackList scroll position
    it gaves the wrong Gad at target message notify, it's the sender object with the target dispatcher !
    Totally stop using that, all messages are sent to AppModel to be redispatched.
*/
//static const struct TagItem map_scrollerV_to_scrollY[] = {
//    { SCROLLER_Top, TRACKLIST_ScrollY },
//    { TAG_END, 0 }
//};

void CreateTrackListView(TrackListView *pm,
                Object *appModel,
                AukStyle *stylesheet)
{
    // init private boopsi gadget & layout classes.

    if(!InfiniteScrollStaticInit()) cleanexit("InfiniteScroll failed");
    if(!TimeRuleStaticInit()) cleanexit("TimeRule failed");
    if(!TrackAreaStaticInit()) cleanexit("TrackLayout init failed");
    if(!VolumeRuleStaticInit()) cleanexit("VolumeRule init failed");
    if(!TrackHeaderStaticInit()) cleanexit("TrackHeader init failed");
    if(!TrackListStaticInit()) cleanexit("TrackList init failed");

    pm->pstyleSheet = stylesheet;

    // - - - - - A
    pm->timerule = (Object *)NewObject( TIMERULE_GetClass(), NULL,
                            TIMERULE_StyleSheet,(ULONG)stylesheet,
                            ICA_TARGET,appModel,
                            TIMERULE_TimeSelection,
                            TAG_END);


    // - - - - - B
        pm->trackList = (Object *)NewObject( TRACKLIST_GetClass(), NULL,
                                LAYOUT_DeferLayout, TRUE,
                                TRACKLIST_StyleSheet, (ULONG)stylesheet,
                                GA_ID,GAD_TRACKLIST, /* allows to redirect notify messages */
                                ICA_TARGET,appModel, /* will send messages, that will be received by the main app boopsi object model */
                               // GA_DrawInfo,(ULONG)drawInfo,
                                TAG_END);

        pm->scrollerV = (Object *)NewObject( SCROLLER_GetClass(), NULL,
                                 //   GA_DrawInfo, drawInfo,
                                    GA_RelVerify, TRUE,
                                SCROLLER_Top, 0,
                                SCROLLER_Total, 40,
                                SCROLLER_Visible, 40,
                                SCROLLER_ArrowDelta,16,
                                SCROLLER_Orientation, FREEVERT,
                                SCROLLER_Stretch,TRUE,
                                GA_ID,GAD_SCROLLER_V,
                                ICA_TARGET,appModel,
                                //ICA_TARGET, pm->trackList, // this send uncorrect Gad to target !!
                                //ICA_MAP, (ULONG)map_scrollerV_to_scrollY,
                                TAG_END);

    pm->subBHl = (Object *)NewObject( LAYOUT_GetClass(), NULL,
                LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
                LAYOUT_BevelStyle, /*BVS_GROUP*/BVS_NONE,

         LAYOUT_SpaceOuter, FALSE,
         LAYOUT_SpaceInner, FALSE,
         LAYOUT_BottomSpacing, 0,
         LAYOUT_TopSpacing,0,
         LAYOUT_LeftSpacing,0,
         LAYOUT_RightSpacing,0,
         LAYOUT_InnerSpacing,0,

//                LAYOUT_AddChild,pm->trackHeaderList ,
//            CHILD_WeightedWidth,0,
//            CHILD_MinWidth,headerwidth,
                LAYOUT_AddChild, pm->trackList,
            CHILD_WeightedWidth,1,
                LAYOUT_AddChild, pm->scrollerV,
            CHILD_WeightedWidth,0,
                TAG_DONE);

    // - - - - - C
    pm->scrollerH = (Object *)NewObject( SCROLLER_GetClass(), NULL,
                             //   GA_DrawInfo, drawInfo,
                                GA_ID, GAD_SCROLLER_H,
                                GA_RelVerify, TRUE, // needed
                            SCROLLER_Top, 0,
                            SCROLLER_Total, SCROLLERH_FIXEDTOTAL,
                            SCROLLER_Visible, SCROLLERH_FIXEDTOTAL,
                            SCROLLER_Orientation, FREEHORIZ,
                            SCROLLER_Stretch,TRUE,
                            SCROLLER_ArrowDelta,SCROLLERH_FIXEDTOTAL/16,
                            ICA_TARGET, appModel,
                            TAG_END);
    // - - - - - A+B+C

    pm->mainVl = (Object *)NewObject( LAYOUT_GetClass(), NULL,
                    LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
                    LAYOUT_BevelStyle, /*BVS_GROUP*/BVS_NONE,

             LAYOUT_SpaceOuter, FALSE,
             LAYOUT_SpaceInner, FALSE,
             LAYOUT_BottomSpacing, 0,
             LAYOUT_TopSpacing,0,
             LAYOUT_LeftSpacing,0,
             LAYOUT_RightSpacing,0,
             LAYOUT_InnerSpacing,0,

                    LAYOUT_AddChild,pm->timerule /*pm->subAHl*/,
                CHILD_WeightedHeight,0,
                CHILD_MaxHeight,stylesheet->fontHeight+4,
                    LAYOUT_AddChild, pm->subBHl,
                CHILD_WeightedHeight,1,
                    LAYOUT_AddChild, pm->scrollerH,
                CHILD_WeightedHeight,0,
                    TAG_DONE);

    // this object is the updateListener
    AukObject_New(&pm->updateListener);


}

/* receive update message for within a track */
static void AukUpdate_Track(AukObject* listenerObject, AukObject* modifiedObject,void *userData, AukMessage_AProject *message)
{
    TrackListView *pm = (TrackListView *)userData;

    AukTrack *track = message->_track;
    if(!pm || !track) return;
 //   bdbprintf(" **** AukUpdate_Track ! \n");
    switch(message->type)
    {
        case AUK_MSG_TRACKMODIFIED_TIMECHANGE:
        {
            /* Time of a sound changed, may affect project duration.
             * Schedule horizontal scroll domain update.
             */
            pm->updateBits |= TLVB_UPDATE_HORIZSCROLLDOMAIN;
            if(myTask) Signal(myTask, SIGBREAKF_CTRL_F);
        }
        break;

        case AUK_MSG_TRACKMODIFIED_SOUNDADDED:
        {
            /* Sound added to track, may affect project duration.
             * Schedule horizontal scroll domain update.
             * Also update track info (channel count, sample rate) in header.
             */
            TrackListArea_CheckTrackChannels((struct Gadget *)pm->trackList,track->trackIndex);
            TrackListArea_SetTrackChannelCount((struct Gadget *)pm->trackList,track->trackIndex,track->channelCount);
            TrackListArea_SetTrackSampleRate((struct Gadget *)pm->trackList,track->trackIndex,track->sampleRate);

            pm->updateBits |= TLVB_UPDATE_HORIZSCROLLDOMAIN;
            if(myTask) Signal(myTask, SIGBREAKF_CTRL_F);
        }
        break;
        case AUK_MSG_TRACKMODIFIED_SOUNDREMOVED:
        {
            /* Sound removed from track, may affect project duration.
             * Schedule horizontal scroll domain update.
             */
            pm->updateBits |= TLVB_UPDATE_HORIZSCROLLDOMAIN;
            if(myTask) Signal(myTask, SIGBREAKF_CTRL_F);
        }
        break;
        case AUK_MSG_TRACKMODIFIED_NAMECHANGE:
        {
           //const char *name;
            TrackListArea_SetTrackName((struct Gadget *)pm->trackList, message->_track_id,message->_track->name);
        }
        break;
        case AUK_MSG_TRACKMODIFIED_CHANGEVol:
        {
            TrackListArea_SetTrackOwnVolume((struct Gadget *)pm->trackList,message->_track_id,track->ownVolume);
            //to validate printf("track->ownVolume:%d\n",track->ownVolume);
         }
        break;
        case AUK_MSG_TRACKMODIFIED_CHANGEPan:
        {
            TrackListArea_SetTrackStereoPan((struct Gadget *)pm->trackList,message->_track_id,track->stereoPan);
        }
        break;
        case AUK_MSG_TRACKMODIFIED_CHANGEFlags:
        {
            TrackListArea_SetTrackFlags((struct Gadget *)pm->trackList,message->_track_id,track->stateFlags);
        }
        break;
        case AUK_MSG_TRACKMODIFIED_CHANGESoundPosition:
        {
            TrackListArea_TrackRedraw((struct Gadget *)pm->trackList,message->_track_id);
            pm->updateBits |= TLVB_UPDATE_HORIZSCROLLDOMAIN ;
            if(myTask) Signal(myTask, SIGBREAKF_CTRL_F);
        } break;
        default:
        break;
    }

}

static void AukUpdate_TrackList(AukObject* listenerObject, AukObject* modifiedObject,void *userData, AukMessage *message)
{
    int signalupdate=0;
    struct Gadget *trackListAreaUi;
    AukAProject *project = (AukAProject*)modifiedObject;
    TrackListView *pm = (TrackListView *)userData;

   // printf(" **** AukUpdate_TrackList ! \n");
    if(!pm || !project || !message) return;

    trackListAreaUi = (struct Gadget *)pm->trackList;
    switch(message->type)
    {
        case AUK_MSG_TRACKADDED:
        {
            AukMessage_AProject *m = (AukMessage_AProject *)message;
            AukTrack *track = m->_track;
   // printf(" **** AUK_MSG_TRACKADDED ! \n");
            if(track)  AukObject_AddListener((AukObject *)track,
                  pm->updateListener, // AukObject* listenerObject,
                  (void*)pm, // userData
                  (AukUpdateCallback)&AukUpdate_Track //AukUpdateCallback callback
                  );

            // update GUI, add ui track. This retain the track.
            TrackListArea_insertTrack(trackListAreaUi,track,track->trackIndex);

            /* Set track info labels (channel count, sample rate) for loaded projects */
            TrackListArea_SetTrackChannelCount(trackListAreaUi,track->trackIndex,track->channelCount);
            TrackListArea_SetTrackSampleRate(trackListAreaUi,track->trackIndex,track->sampleRate);

            /* Track added may affect project duration, update horizontal scroll domain */            
            pm->updateBits |= TLVB_UPDATE_HORIZSCROLLDOMAIN
                                | TLVB_UPDATE_VERTSCROLLDOMAIN
                                | TLVB_UPDATE_REDRAW_TRACKLIST;
            if(myTask) signalupdate=1;
        }
        break;
        case AUK_MSG_TRACKREMOVED:
        {
            AukMessage_AProject *m = (AukMessage_AProject *)message;
            AukTrack *track = m->_track;

  //  bdbprintf(" **** receive from data AUK_MSG_TRACKREMOVED ! \n");
            if(track)  AukObject_RemoveListener((AukObject *)track,
                        pm->updateListener // AukObject* listenerObject,
                  );

            // update GUI, remove ui track
            TrackListArea_removeTrack(trackListAreaUi,track->trackIndex);

            /* Track removed may affect project duration, update horizontal scroll domain */
            pm->updateBits |= TLVB_UPDATE_HORIZSCROLLDOMAIN;
            if(myTask) signalupdate=1;
        }
        break;
        case AUK_MSG_TRACKMODIFIED_CHANGESoloTrack:
        {
            /* affect all headers */
            AukMessage_AProject *m = (AukMessage_AProject *)message;

 printf(" *** DATA-> UI TRACKMODIFIED_CHANGESoloTrack %d\n",m->_track_id);
            TrackListArea_SetSoloTrack(trackListAreaUi, m->_track_id);

            // pm->updateBits |= TLVB_UPDATE_REDRAW_JUSTHEADERS;
            // if(myTask) signalupdate=1;
        }
        break;
        case AUK_MSG_SELECTIONCHANGED:
        {
            AukSelection *sel = &project->selection;
            TrackListArea_SetCurrentSelection(trackListAreaUi,sel);
            if(pm->timerule)
            {
                /* "Next refresh, update all tiles" */
                SetAttrs(pm->timerule, INFINITESCROLL_FullTilesRefresh,TRUE,TAG_END);

                SetGadgetAttrs((struct Gadget *)pm->timerule,CurrentMainWindow,NULL,
                     TIMERULE_Refresh,TRUE,TAG_END );
            }
        }
        break;
        default:
  //note: does things  bdbprintf(" **** AUK_MSG_XXX %d! \n",(int)message->type);
        break;
    }
    if(signalupdate) Signal(myTask, SIGBREAKF_CTRL_F);
}





/**
 * Update the vertical scroller's domain based on the trackList's total height
 * Should be called after track count changes or layout changes
 */
void updateVerticalScrollDomain(TrackListView *pm)
{
    struct Gadget *trackListGad;
    ULONG domainHeight = 0;
    ULONG visibleHeight = 0;
    //ULONG scrollTop = 0;

    if(!pm || !pm->trackList || !pm->scrollerV) return;

    trackListGad = (struct Gadget *)pm->trackList;

    /* Get the total domain height from the trackList gadget */
    GetAttr(TRACKLIST_DomainHeight, pm->trackList, &domainHeight);

    /* Get the visible height from the gadget's actual height */
    visibleHeight = (ULONG)trackListGad->Height;

    /* Get current scroll position */
    //GetAttr(TRACKLIST_ScrollY, pm->trackList, &scrollTop);
    if(domainHeight ==0) domainHeight=1;
    if(visibleHeight>domainHeight) visibleHeight = domainHeight;

    //bdbprintf(" **** updateVerticalScrollDomain: domainHeight:%d visibleHeight:%d \n",domainHeight,visibleHeight);

    /* Update the scroller */
    {
        ULONG tags[]={
            SCROLLER_Total, 0,
            SCROLLER_Visible, 0,
            TAG_END
        };
        tags[1] = domainHeight;
        tags[3] = visibleHeight;


        SetGadgetAttrsA((struct Gadget *)pm->scrollerV, CurrentMainWindow, NULL,(struct TagItem *)&tags[0]);
    }

}


/* Minimum timePerPixelWidth: 1 sample at 44100Hz should give at least 2 pixels.
 * min = 1/(44100*2) seconds/pixel = 1/88200 sec/px
 * In fixed-point 32.32: (1LL << 32) / 88200 = approximately 48693
 */
//#define MIN_TIME_PER_PIXEL_WIDTH (48693LL>>8)

/**
 * Update the Horizontal scroller's domain based on the trackList's max time length
 * Should be called after track count changes, track modified, or layout changes.
 *
 * Domain (total) = project duration / timePerPixelWidth (in pixels)
 * Visible = current trackArea width (gadget width - header width)
 *
 * Also updates TimeRule's time border attributes to match visible time range.
 */
void updateHorizontalScrollDomain(TrackListView *pm, int alsoSetoffset)
{
    TimeProjection trackListTimeproj;
    struct Gadget *trackListGad;
    AukAProject *project;
    long long duration;
    long long timePerPixelWidth;
    unsigned long long domainWidthPix;
    ULONG visibleWidthRelative;
    ULONG trackPixelWidth;
    ULONG scrollerTop;
    LONG arrowDelta;

    if(!pm || !pm->trackList || !pm->scrollerH) return;

    trackListGad = (struct Gadget *)pm->trackList;
    project = (AukAProject *)pm->project;

    if(!trackListGad || !project) return;

    /* Get project duration (AukFixed 32.32 format, in seconds) */
    duration = project->GetDuration(project);

    if(duration <= 0) {
        /* No duration, set scroller to full visible (disabled state) */
        SetGadgetAttrs((struct Gadget *)pm->scrollerH,CurrentMainWindow, NULL,
            SCROLLER_Total, 1,
            SCROLLER_Visible, 1,
            SCROLLER_ArrowDelta,1,
            TAG_END);

        return;
    }

    /* Get visible width from the gadget. TrackListArea uses headerWidth for left side,
     * so visible track area width = gadget width.
     * The actual visible time area would be gadget width, but we use domainWidth directly.
     */
     GetAttr(TRACKLIST_TimeProjection, pm->trackList,(ULONG *) &trackListTimeproj);
     timePerPixelWidth = trackListTimeproj._timePerPixelWidth;

     GetAttr(TRACKLIST_TrackAreaWidth, pm->trackList, &trackPixelWidth);
    if(trackPixelWidth==0) return;

    /* Clamp timePerPixelWidth to minimum (prevent divide by zero and over-zoom) */
    if(timePerPixelWidth < TRACKLIST_MINZOOM) {
        timePerPixelWidth = TRACKLIST_MINZOOM;
    }


    /* this optimisation test must be done just before application, after context check */
    if(pm->lastDomainDurationChecked == duration &&
        pm->lastDomainTimePerPixel == timePerPixelWidth &&
        pm->lastDomainWidth == trackPixelWidth)
    {
        return;
    }
    pm->lastDomainDurationChecked = duration;
    pm->lastDomainTimePerPixel = timePerPixelWidth;
    pm->lastDomainWidth = trackPixelWidth;



    domainWidthPix =  ((unsigned long long)duration/(unsigned long long)timePerPixelWidth);
     if(domainWidthPix==0) domainWidthPix=1;

    if(domainWidthPix <=trackPixelWidth) {
        visibleWidthRelative = SCROLLERH_FIXEDTOTAL;
        arrowDelta = 1;
    } else
    {
        visibleWidthRelative = (trackPixelWidth*SCROLLERH_FIXEDTOTAL)/domainWidthPix;

        arrowDelta = (visibleWidthRelative*16)/trackPixelWidth; // should do 16 pixels
        if(arrowDelta==0) arrowDelta=1;

    }
    // shouldnt happen, but well...
    if(visibleWidthRelative>SCROLLERH_FIXEDTOTAL) visibleWidthRelative = SCROLLERH_FIXEDTOTAL;

    // may move scrollerTop or not...
    if(alsoSetoffset)
    {
        scrollerTop = (trackListTimeproj._pixAtLeft*SCROLLERH_FIXEDTOTAL)/
                    domainWidthPix
                     ;
    } else
    {
        /* reuse  */
        GetAttr(SCROLLER_Top, pm->scrollerH, &scrollerTop);
    }

    if(scrollerTop>(SCROLLERH_FIXEDTOTAL-visibleWidthRelative))
    scrollerTop = SCROLLERH_FIXEDTOTAL-visibleWidthRelative;

    // {
    //     long long bottomtime = (trackListTimeproj._pixAtLeft+trackPixelWidth)*
    //                             trackListTimeproj._timePerPixelWidth;
    //     if(bottomtime>duration)
    //     {
    //         scrollerTop = SCROLLERH_FIXEDTOTAL-visibleWidthRelative;

    //     }
    // }
    // if(scrollerTop+visibleWidthRelative>SCROLLERH_FIXEDTOTAL)
    //     scrollerTop = SCROLLERH_FIXEDTOTAL-visibleWidthRelative;



    long long lastDomainDurationChecked;
    long long lastDomainTimePerPixel;
    ULONG lastDomainWidth;

    SetGadgetAttrs((struct Gadget *)pm->timerule, CurrentMainWindow, NULL,
        TIMERULE_TimePerPixelWidth,&timePerPixelWidth,
        // not here INFINITESCROLL_Position, &trackListTimeproj._timeAtLeft,
        TAG_END);

    SetGadgetAttrs((struct Gadget *)pm->scrollerH, CurrentMainWindow, NULL,
        SCROLLER_Total,/* totalScroll*/ SCROLLERH_FIXEDTOTAL, // WORD 16384
        SCROLLER_Visible, visibleWidthRelative,
        SCROLLER_Top,scrollerTop,
        SCROLLER_ArrowDelta,arrowDelta,
        TAG_END);
    // need this update
    TrackListView_UpdateTimeRule(pm);


}


void TrackListView_setProject(TrackListView *pm,AukAProject *project)
{
    // listen project modification
    if(project)
    {
        AukObject_AddListener(&project->base.base,
                      pm->updateListener, // AukObject* listenerObject,
                      (void*)pm, // userData
                      &AukUpdate_TrackList //AukUpdateCallback callback
                      );

       if(pm->timerule)
       {
            SetGadgetAttrs((struct Gadget *)pm->timerule,CurrentMainWindow,NULL,
                            TIMERULE_TimeSelection, (ULONG)&project->selection,
                             TAG_END );

       }
    }
    // retain project
    AukObjectPtr_Set(&pm->project,&project->base.base);
   // printf("  ////// TrackListView_setProject:%08x\n",(int)project);

    // link data to UI
    TrackListArea_setTrackList((struct Gadget *)pm->trackList ,project );

}

void TrackListView_ListenTrackListMessage(TrackListView *pm,struct opUpdate *M)
{
    struct TagItem *ptag;
    ULONG changedDomainHeight=0;
    /* here we know that sender is GA_ID == GAD_TRACKLIST
      We listen to change and actually delay application to next Wait() main loop,
     to avoid inter signal recursions.
    */
    if((ptag = FindTagItem( TRACKLIST_DomainHeight,M->opu_AttrList ))!=NULL) changedDomainHeight = ptag->ti_Data;
    if(changedDomainHeight >0)
    {
        pm->updateBits |= TLVB_UPDATE_VERTSCROLLDOMAIN;
        /* Layout changed, also update horizontal scroll domain
         * (visible width may have changed due to window resize)
         */
        pm->updateBits |= TLVB_UPDATE_HORIZSCROLLDOMAIN;
        if(myTask) Signal(myTask,SIGBREAKF_CTRL_F);
    }

    if((ptag = FindTagItem( TRACKLIST_ScrollY,M->opu_AttrList ))!=NULL)
    {   
        pm->updateBits |= TLVB_UPDATE_REDRAW_TRACKLIST;
        if(myTask) Signal(myTask,SIGBREAKF_CTRL_F);
    }

    if((ptag = FindTagItem( TRACKLIST_TimeProjection,M->opu_AttrList ))!=NULL)
    {
  //  bdbprintf("TLVB_UPDATE_REDRAW_JUSTTRACKS from TRACKLIST_TimeProjection\n");
        pm->updateBits |= TLVB_UPDATE_REDRAW_JUSTTRACKS;
        if(myTask) Signal(myTask,SIGBREAKF_CTRL_F);
    }

    // can manage more update signals here...

}
/* Update trackList drawing from vertical scroll position
*/
void TrackListView_ListenScrollVMessage(TrackListView *pm,struct opUpdate *M)
{
    struct TagItem *ptag;
    if((ptag = FindTagItem( SCROLLER_Top,M->opu_AttrList ))!=NULL)
    {
        LONG scrollY = ptag->ti_Data;
        SetGadgetAttrs((struct Gadget *)pm->trackList, CurrentMainWindow, NULL,
                    TRACKLIST_ScrollY,scrollY,
                    TAG_END
                );
    }

}

static void TrackListView_SetHScrollPos(TrackListView *pm,TimeProjection *timeproj )
{

    ULONG headerWidth = 0;
    GetAttr(TRACKLIST_HeaderWidth, pm->trackList, &headerWidth);

    SetGadgetAttrs((struct Gadget *)pm->trackList, CurrentMainWindow, NULL,
                TRACKLIST_TimeProjection,(ULONG *) timeproj,
                TAG_END
            );

    /* this is the left shift from the TrackArea left position to
      the window left position */
    timeproj->_pixAtLeft -= (long long)headerWidth;

    SetGadgetAttrs((struct Gadget *)pm->timerule, CurrentMainWindow, NULL,
        TIMERULE_TimePerPixelWidth,&timeproj->_timePerPixelWidth,
        INFINITESCROLL_Position, &timeproj->_pixAtLeft,
        TAG_END);

// this should be done by gadgets notifications if needed:
    pm->updateBits |= TLVB_UPDATE_REDRAW_TIMERULE | TLVB_UPDATE_REDRAW_JUSTTRACKS;
    if(myTask) Signal(myTask,SIGBREAKF_CTRL_F);


}

/* Update trackList horizontal scroll position from horizontal scroller.
 * The scroller SCROLLER_Top is in "scroller units" which we convert to time. -> total domain is SCROLLERH_FIXEDTOTAL
 * The scroller's domain is computed in updateHorizontalScrollDomain().
 * Also updates TimeRule's time border attributes to match new scroll position.
 */
void TrackListView_ListenScrollHMessage(TrackListView *pm, struct opUpdate *M)
{
    struct TagItem *ptag;
	AukAProject *project;

// bdbprintf("//// TrackListView_ListenScrollHMessage\n");
	project = (AukAProject *)pm->project;
	if(!project) return;

    if((ptag = FindTagItem( SCROLLER_Top, M->opu_AttrList )) != NULL)
    {
        long long duration;
        ULONG scrollerTop = ptag->ti_Data;
        TimeProjection trackListTimeproj;

        GetAttr(TRACKLIST_TimeProjection, pm->trackList,(ULONG *) &trackListTimeproj);

        if(trackListTimeproj._timePerPixelWidth<=0 ) return;
        duration = project->GetDuration(project);
        if(duration<=0) return;
       // timePerPixelWidth = trackListTimeproj._timePerPixelWidth; // ((long long)timePerPixHi << 32) | timePerPixLo;

        /* scrollX = value in pixel 64b, not in time */
        trackListTimeproj._pixAtLeft = ((long long)scrollerTop * duration) / (trackListTimeproj._timePerPixelWidth *SCROLLERH_FIXEDTOTAL);
//  bdbprintf("trackListTimeproj._pixAtLeft:%lld",trackListTimeproj._pixAtLeft);
        TrackListView_SetHScrollPos(pm,&trackListTimeproj);

    }
}

/* return -1 if no */
int getGadgetMessageAttrib(struct opUpdate *M, int attrib)
{
    struct TagItem *ptag;
    if(!M || !M->opu_AttrList) return -1;
    if((ptag = FindTagItem( attrib,M->opu_AttrList))!=NULL)
    {
        return ptag->ti_Data;
    }
    return -1;
}


/* taken out of context, code that tells a button is released */
#define WMHI_GADGETUP        (2<<16)

/* Trim whitespace (spaces, tabs, newlines, carriage returns) from both ends of string.
 * Modifies string in-place and returns pointer to trimmed start.
 */
static char *aukTrimString(char *str)
{
    char *end;

    if(!str || *str == 0) return str;

    /* Trim leading whitespace */
    while(*str == ' ' || *str == '\t' || *str == '\n' || *str == '\r')
        str++;

    if(*str == 0) return str;

    /* Trim trailing whitespace */
    end = str + strlen(str) - 1;
    while(end > str && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r'))
        end--;

    /* Write new null terminator */
    *(end + 1) = 0;

    return str;
}

int requesterName(char *buffer, int charmax,
            const char *requesterTitle,
            const char *interfaceString)
{

    Object *reqobj;
    int result;

    // default
    *buffer =0;

    // Create a string requester
    reqobj = NewObject(REQUESTER_GetClass(), NULL,
        REQ_Type, REQTYPE_STRING,
        REQ_TitleText,(ULONG) requesterTitle,
        REQ_BodyText,(ULONG) interfaceString,
        REQS_Buffer, buffer,
        REQS_MaxChars, charmax - 1,
        TAG_END);

    if (!reqobj) return 0;

     // Show the requester
    result = DoMethod(reqobj, RM_OPENREQ, NULL, CurrentMainWindow, NULL);

    // Clean ups
    DisposeObject(reqobj);

    return result;
}

/*
    Here, UI ask actions on the data, data modify and send update messages,
    event listeners then adapt UI.
*/
void TrackListView_ListenTrackHeaderMessage(TrackListView *pm,struct opUpdate *M, ULONG gadId)
{
    AukAProject *project;
    AukTrackPtr track=NULL;
    ULONG buttonId = gadId & GAD_TRACKHEADER_IDMASK;
    ULONG trackId = (gadId & GAD_TRACKHEADER_TRACKMASK)>>4; // 4096 tracks possible, 16 buttons

    if(!pm || !pm->project) return;
    project = (AukAProject*)pm->project;

    /* Most likely, actions will affect a given track:
     Note this must be paired with a release call later.
     */

    project->GetTrack(project,&track,trackId);
    if(!track) return;

    switch(buttonId)
    {

        case GAD_TRACKHEADER_TRACKAREA:
        {
            struct TagItem *ptag = M->opu_AttrList;
            while(ptag->ti_Tag)
            {
                switch(ptag->ti_Tag)
                {
                    case TRACKAREA_TimeSelectionChange:
                    {   /* affect selection noted in data, data will send update message */
                        AukSelection *selection = (AukSelection *)ptag->ti_Data;
                        AukAProject_SetSelection(project,selection);
                    } break;
                    case TRACKAREA_TimeZoomChange:
                    {
                        /* when moving just affect drawing, when bt up, change zoom.
                            This is all TrackListArea (UI) internal things...
                        */
                        AukSelection *sel = (AukSelection *)ptag->ti_Data;
                        /* when the zoom move ends, apply zoom change */
                        if(sel->_mode == 0)
                        {
                            TrackListView_ZoomToSpan(pm,sel);
                            TrackListArea_SetZoomSelectorRun((struct Gadget *)pm->trackList,sel);
                        } else
                        {
                            // when moving just display span
                            /* for the display */
                            TrackListArea_SetZoomSelectorRun((struct Gadget *)pm->trackList,sel);

                        }
                    } break;
                    case TRACKAREA_SoundSlideChange:
                    {
                        /* Sound slide notification from TrackArea */
                        AukSoundSlideInfo *slideInfo = (AukSoundSlideInfo *)ptag->ti_Data;
                        if(slideInfo && slideInfo->sound)
                        {
                            /* Use AukTrack_SlideSound to move with constraints */
                            AukTrack_SlideSound(track, slideInfo->sound,
                                               slideInfo->newStartTime,
                                               0, 0x7FFFFFFFFFFFFFFFLL);
                        }
                    } break;
                    default:
                        break;
                }
                ptag++;
            }

             int isSelection = getGadgetMessageAttrib(M,WMHI_GADGETUP);
        }
        break;
        case GAD_TRACKHEADER_CLOSE:
        {
            // watch out receive events that are not necessarily "button action"
            // buttonState is sent all the time button is pressed
//            int buttonstate = getGadgetMessageAttrib(M,GA_SELECTED);
            int buttonReleased = getGadgetMessageAttrib(M,WMHI_GADGETUP);

           // printf("GAD_TRACKHEADER_CLOSE buttonReleased:%d\n",buttonReleased);
            // /* Close track at data level */
            if(buttonReleased >0)
            {
                //printf("go project->RemoveTrack %d %08x %08x\n",trackId,project,track);
                project->RemoveTrack(project,track);
                AukObjectPtr_Release((AukObject **)&track);
            }
        }
        break;
        case GAD_TRACKHEADER_NAME:
        {
            int buttonReleased = getGadgetMessageAttrib(M,WMHI_GADGETUP);
            if(buttonReleased >0)
            {
                char name[32];
                char *trimmed;
                name[0]=0;
                requesterName(name, 31,
                        LOC(MSG_TRACK_RENAME_TITLE),
                        LOC(MSG_TRACK_RENAME_PROMPT));
                trimmed = aukTrimString(name);
                if(trimmed[0] != 0)
                {
                    AukTrack_SetName(track, trimmed);
                }
            }
        }
        break;
        case GAD_TRACKHEADER_SILENCER:
        {

            int buttonstate = getGadgetMessageAttrib(M,GA_SELECTED);
      //  printf("AukTrack_SetSilent %d\n",buttonstate);
         if(buttonstate !=-1)
         {
            track->base._blockUpdates = TRUE;
                AukTrack_SetSilent(track,buttonstate);
            track->base._blockUpdates = FALSE;
         }
        }
        break;
        case GAD_TRACKHEADER_SOLO:
        {
            int buttonstate = getGadgetMessageAttrib(M,GA_SELECTED);
           // printf("UI->data GAD_TRACKHEADER_SOLO: trackId:%d bt selstate%d\n",trackId,buttonstate);
            if(buttonstate != -1)
            {
                // don't block update, change affect all UI
                //  we receive 0 for the other solo bt state that we put off !
                // only send of to the track which is on
                if(project->soloTrack != -1 &&
                    project->soloTrack == (int)trackId &&
                    buttonstate == 0 )
                    {
                        AukAProject_SetSoloTrack(project,-1);
                    } else if(buttonstate == 1)
                    {
                        AukAProject_SetSoloTrack(project,trackId);
                    }

            }
        }
        break;
        case GAD_TRACKHEADER_VOL:
        {
            int sliderlevel = getGadgetMessageAttrib(M,SLIDER_Level);
       // bdbprintf("AukTrack_SetOwnVolume %d\n",sliderlevel);
            if(sliderlevel !=-1) // -1 means slider released
            {
                track->base._blockUpdates = TRUE;
                 AukTrack_SetOwnVolume(track,sliderlevel<<9);
                track->base._blockUpdates = FALSE;
            }
        }
        break;
        case GAD_TRACKHEADER_PAN:
        {
            int sliderlevel = getGadgetMessageAttrib(M,SLIDER_Level);
       // bdbprintf("AukTrack_SetStereoPan %d\n",sliderlevel);
            if(sliderlevel !=-1)
            {
                track->base._blockUpdates = TRUE;
                 AukTrack_SetStereoPan(track,sliderlevel<<9);
                track->base._blockUpdates = FALSE;
            }
        }
        break;
        default:
        break;

    }

    AukObjectPtr_Release((AukObject **)&track);

}

void TrackListView_SetEditMode(TrackListView *pm, int editMode)
{
    if(pm->editMode == editMode) return;

    pm->editMode = editMode;

}

void TrackListView_CheckUpdates(TrackListView *pm)
{
    if(pm->updateBits & TLVB_UPDATE_VERTSCROLLDOMAIN)
    {
        updateVerticalScrollDomain(pm);
    }
    if(pm->updateBits & TLVB_UPDATE_HORIZSCROLLDOMAIN)
    {
        updateHorizontalScrollDomain(pm,0); /* can add TLVB_UPDATE_REDRAW_TRACKLIST, checked just after, or not. */
    }

    if(pm->updateBits & TLVB_UPDATE_REDRAW_TRACKLIST)
    {   /* full redraw */
        SetGadgetAttrs((struct Gadget *)pm->trackList,CurrentMainWindow, NULL,TRACKLIST_Refresh,TRUE,TAG_END);
    } else
    {   /* partial redraw */
        if(pm->updateBits & TLVB_UPDATE_REDRAW_JUSTTRACKS)
        {
            /* same as TLVB_UPDATE_REDRAW_TRACKLIST, but do not redraw headers */
            SetGadgetAttrs((struct Gadget *)pm->trackList, CurrentMainWindow, NULL,TRACKLIST_JustTracksRefresh,TRUE,TAG_END);
        }
        if(pm->updateBits & TLVB_UPDATE_REDRAW_JUSTHEADERS)
        {
            /* same as TLVB_UPDATE_REDRAW_TRACKLIST, but do not redraw headers */
            SetGadgetAttrs((struct Gadget *)pm->trackList, CurrentMainWindow, NULL,TRACKLIST_JustHeadersRefresh,TRUE,TAG_END);
        }
    }

    if(pm->updateBits & TLVB_UPDATE_REDRAW_TIMERULE)
    {
        SetGadgetAttrs((struct Gadget *)pm->timerule,CurrentMainWindow, NULL,TIMERULE_Refresh,TRUE,TAG_END);
    }


    pm->updateBits = 0;
}

void TrackListView_UpdateTrackList(TrackListView *pm)
{
    if(!pm->trackList) return;
    SetGadgetAttrs((struct Gadget *)pm->trackList, CurrentMainWindow, NULL,TRACKLIST_Refresh,TRUE,TAG_END);
}
void TrackListView_UpdateTimeRule(TrackListView *pm)
{
    struct opUpdate m;
    ULONG scrollH=0;
    if(!pm || !pm->trackList || !pm->scrollerH) return;
 // send message like HScrooller would do

    GetAttr(SCROLLER_Top, pm->scrollerH,&scrollH);

    m.MethodID = OM_UPDATE;
    {
        ULONG tags[]={SCROLLER_Top,scrollH,TAG_END};
        m.opu_AttrList = (struct TagItem *)&tags[0];
        TrackListView_ListenScrollHMessage(pm,&m);
    }

}

// public close, free objects
void CloseTrackListView(TrackListView *pm)
{
    if(!pm) return;
// implicit with Releases
//    if(pm->project && pm->updateListener)
//    {
//        AukObject_RemoveListener(&pm->project->base.base,pm->updateListener);
//    }
    AukObjectPtr_Release(&pm->project);
    AukObjectPtr_Release(&pm->updateListener);

}
// public close, free private gadget classes
void CloseTrackListView_StaticClasses()
{
    TrackListStaticClose();
    TrackHeaderStaticClose();
    VolumeRuleStaticClose();
    TrackAreaStaticClose();
    TimeRuleStaticClose();
    InfiniteScrollStaticClose();
}


static void TrackListView_ZoomChange(TrackListView *pm, ULONG factor)
{
    TimeProjection trackListTimeproj;
    ULONG headerWidth = 0;
    struct Gadget *Gadtracklist;
    long long pixcenter;
    ULONG trackAreaWidth;
    if(!pm) return;

        printf("TrackListView_ZoomChange %08x\n",(int)factor);

    GetAttr(TRACKLIST_HeaderWidth, pm->trackList, &headerWidth);
    GetAttr(TRACKLIST_TimeProjection, pm->trackList,(ULONG*) &trackListTimeproj);

// printf(" z timePerPixelWidth: %08x.%08x\n",(int)(trackListTimeproj._timePerPixelWidth>>32),(int)trackListTimeproj._timePerPixelWidth);

//        printf("TrackListView_ZoomChange tpp:%016x\n",trackListTimeproj._timePerPixelWidth);
    if(trackListTimeproj._timePerPixelWidth<=0 ) return;
    Gadtracklist = (struct Gadget *)pm->trackList;

    trackAreaWidth = ((ULONG)Gadtracklist->Width - headerWidth);

    pixcenter = (trackListTimeproj._pixAtLeft + (trackAreaWidth>>1)) ;// * trackListTimeproj._timePerPixelWidth;

    trackListTimeproj._timePerPixelWidth =
            (((unsigned long long )trackListTimeproj._timePerPixelWidth)*factor)>>16;


    if(trackListTimeproj._timePerPixelWidth<TRACKLIST_MINZOOM)
        trackListTimeproj._timePerPixelWidth = TRACKLIST_MINZOOM;
    else if(trackListTimeproj._timePerPixelWidth>TRACKLIST_MAXZOOM)
        trackListTimeproj._timePerPixelWidth = TRACKLIST_MAXZOOM;

 //printf(" za timePerPixelWidth: %08x.%08x\n",(int)(trackListTimeproj._timePerPixelWidth>>32),(int)trackListTimeproj._timePerPixelWidth);

    trackListTimeproj._pixAtLeft  +=
            ((trackAreaWidth*factor)>>16)-trackAreaWidth;

    TrackListView_SetHScrollPos(pm,&trackListTimeproj);

    updateHorizontalScrollDomain(pm,1);
    // same as TLVB_UPDATE_REDRAW_TRACKLIST, but do not redraw headers
    //SetGadgetAttrs((struct Gadget *)pm->trackList, CurrentMainWindow, NULL,TRACKLIST_JustTracksRefresh,TRUE,TAG_END);
    /* need full time refresh on all tracks */
    TrackListArea_FullTrackRedraw((struct Gadget *)pm->trackList);

}
void TrackListView_ZoomIn(TrackListView *pm)
{
    TrackListView_ZoomChange(pm,0x00010000 - (0x00010000>>2) ); // X 0.75
}
void TrackListView_ZoomOut(TrackListView *pm)
{
    TrackListView_ZoomChange(pm,0x00010000 + (0x00010000>>2) ); // X 1.25
}

/* sent during moving the zoom selctor */
void TrackListView_ZoomToSpan(TrackListView *pm,AukSelection *zoomsel)
{
    ULONG trackwidth;
    TimeProjection newtimeproj;
    long long t1,t2;
    LONG headerWidth = 0;
    struct Gadget *Gadtracklist;

   if(!pm || !pm->trackList || !pm->project || !zoomsel) return;

    t1 = zoomsel->_start;
    t2 = zoomsel->_end;

    if(t2 == t1) return;
    if(t2<t1) {
        long long s=t1; t1=t2; t2=s;
    }
    GetAttr(TRACKLIST_HeaderWidth, pm->trackList, &headerWidth);
    Gadtracklist = (struct Gadget *)pm->trackList ;
    trackwidth = Gadtracklist->Width - headerWidth ;
    if(trackwidth ==0 ) return;

    newtimeproj._timePerPixelWidth = (unsigned long long)(t2-t1)/trackwidth ;

    if( newtimeproj._timePerPixelWidth < TRACKLIST_MINZOOM ) newtimeproj._timePerPixelWidth = TRACKLIST_MINZOOM;
    else if( newtimeproj._timePerPixelWidth > TRACKLIST_MAXZOOM ) newtimeproj._timePerPixelWidth = TRACKLIST_MAXZOOM;

    newtimeproj._pixAtLeft = t1 / newtimeproj._timePerPixelWidth;

    TrackListView_SetHScrollPos(pm,&newtimeproj);
    //done by SetAttrs( pm->trackList,TRACKLIST_TimeProjection,(ULONG)&newtimeproj,TAG_END);
    updateHorizontalScrollDomain(pm,1);


    /* need full time refresh on all tracks */
    TrackListArea_FullTrackRedraw((struct Gadget *)pm->trackList);

}

void TrackListView_ZoomToProject(TrackListView *pm)
{
    TimeProjection trackListTimeproj;
    ULONG headerWidth = 0;
    struct Gadget *Gadtracklist;
    ULONG trackAreaWidth;
    AukAProject *project;
    long long duration;

    if(!pm || !pm->trackList || !pm->project) return;

    project = (AukAProject *)pm->project;
    duration = project->GetDuration(project);
    if(duration <= 0) return;

    GetAttr(TRACKLIST_HeaderWidth, pm->trackList, &headerWidth);
    GetAttr(TRACKLIST_TimeProjection, pm->trackList,(ULONG*) &trackListTimeproj);

    Gadtracklist = (struct Gadget *)pm->trackList;
    trackAreaWidth = ((ULONG)Gadtracklist->Width - headerWidth);
    if(trackAreaWidth == 0) return;

    /* Calculate timePerPixelWidth so entire duration fits in visible area */
    trackListTimeproj._timePerPixelWidth = duration / trackAreaWidth;

    /* Clamp to min/max zoom */
    if(trackListTimeproj._timePerPixelWidth < TRACKLIST_MINZOOM)
        trackListTimeproj._timePerPixelWidth = TRACKLIST_MINZOOM;
    else if(trackListTimeproj._timePerPixelWidth > TRACKLIST_MAXZOOM)
        trackListTimeproj._timePerPixelWidth = TRACKLIST_MAXZOOM;

    /* Start at time 0 */
    trackListTimeproj._pixAtLeft = 0;

    TrackListView_SetHScrollPos(pm,&trackListTimeproj);
    updateHorizontalScrollDomain(pm,0);
    TrackListArea_FullTrackRedraw((struct Gadget *)pm->trackList);
}
