
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <intuition/screens.h>
#include <intuition/icclass.h>

#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/dos.h>

#include <proto/layout.h>
#include <gadgets/layout.h>

#include "compilers.h"
#include "bdbprintf.h"

#include "TrackListView.h"

#include <proto/scroller.h>
#include <gadgets/scroller.h>

#include <proto/label.h>
#include <images/label.h>

#include <proto/layout.h>
#include <gadgets/layout.h>

#include "TimeRule/class_timerule.h"
#include "TrackArea/class_trackarea.h"
#include "TrackHeader/class_trackheader.h"
#include "TrackListArea/class_tracklistarea.h"

#include "gadgetid.h"

#include <aukobject.h>
// audio tracks project
#include <aukaproject.h>
#include <auktrack.h>
#include "bdbprintf.h"

#ifdef Remove
#undef Remove
#endif

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

void CreateTrackListView(TrackListView *pm,struct DrawInfo *drawInfo,
                Object *appModel,
                AukStyle *stylesheet)
{
    // init private boopsi gadget & layout classes.

    if(!InfiniteScrollStaticInit()) cleanexit("InfiniteScroll failed");
    if(!TimeRuleStaticInit()) cleanexit("TimeRule failed");
    if(!TrackAreaStaticInit()) cleanexit("TrackLayout init failed");
    if(!TrackHeaderStaticInit()) cleanexit("TrackLayout init failed");
    if(!TrackListStaticInit()) cleanexit("TrackLayout init failed");

    pm->pstyleSheet = stylesheet;

    // - - - - - A
    pm->timerule = (Object *)NewObject( TIMERULE_GetClass(), NULL,
                            TIMERULE_StyleSheet,(ULONG)stylesheet,
                            ICA_TARGET,appModel,
                            TAG_END);


    // - - - - - B
        pm->trackList = (Object *)NewObject( TRACKLIST_GetClass(), NULL,
                                TRACKLIST_StyleSheet, (ULONG)stylesheet,
                                GA_ID,GAD_TRACKLIST, /* allows to redirect notify messages */
                                ICA_TARGET,appModel, /* will send messages, that will be received by the main app boopsi object model */
                                TAG_END);

        pm->scrollerV = (Object *)NewObject( SCROLLER_GetClass(), NULL,
                                    GA_DrawInfo, drawInfo,
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
                                GA_DrawInfo, drawInfo,
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
static void AukUpdate_Track(AukObject* listenerObject, AukObject* modifiedObject,void *userData, AukMessage *message)
{
    TrackListView *pm = (TrackListView *)userData;
    AukTrack *track = (AukTrack*)modifiedObject;
    AukMessage_AProject *projmess;
    if(!pm || !track) return;
    bdbprintf(" **** AukUpdate_Track ! \n");
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
             */
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
            const char *name;
            projmess = (AukMessage_AProject *)message;
            TrackListArea_SetTrackName(pm->trackList, pm->window, projmess->_track_id,projmess->_track->name);
        }
        break;
        default:
        break;
    }

}

static void AukUpdate_TrackList(AukObject* listenerObject, AukObject* modifiedObject,void *userData, AukMessage *message)
{
    struct Gadget *trackListAreaUi;
    AukAProject *tracklist = (AukAProject*)modifiedObject;
    TrackListView *pm = (TrackListView *)userData;

    bdbprintf(" **** AukUpdate_TrackList ! \n");
    if(!pm || !tracklist || !message) return;

    trackListAreaUi = (struct Gadget *)pm->trackList;
    switch(message->type)
    {
        case AUK_MSG_TRACKADDED:
        {
 //   bdbprintf(" **** AUK_MSG_TRACKADDED ! \n");
            AukMessage_AProject *m = (AukMessage_AProject *)message;
            AukTrack *track = m->_track;
            if(track)  AukObject_AddListener(track,
                  pm->updateListener, // AukObject* listenerObject,
                  (void*)pm, // userData
                  &AukUpdate_Track //AukUpdateCallback callback
                  );

            // update GUI, add ui track
            TrackListArea_addTrack(trackListAreaUi,track);

            /* Track added may affect project duration, update horizontal scroll domain */
            pm->updateBits |= TLVB_UPDATE_HORIZSCROLLDOMAIN;
            if(myTask) Signal(myTask, SIGBREAKF_CTRL_F);
        }
        break;
        case AUK_MSG_TRACKREMOVED:
        {
    bdbprintf(" **** AUK_MSG_TRACKREMOVED ! \n");
            AukMessage_AProject *m = (AukMessage_AProject *)message;
            AukTrack *track = m->_track;
            if(track)  AukObject_RemoveListener(track,
                        pm->updateListener // AukObject* listenerObject,
                  );
            // update GUI, remove ui track
            TrackListArea_removeTrack(trackListAreaUi,track);

            /* Track removed may affect project duration, update horizontal scroll domain */
            pm->updateBits |= TLVB_UPDATE_HORIZSCROLLDOMAIN;
            if(myTask) Signal(myTask, SIGBREAKF_CTRL_F);
        }
        break;
        default:
  //note: does things  bdbprintf(" **** AUK_MSG_XXX %d! \n",(int)message->type);
        break;
    }

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


        SetGadgetAttrsA((struct Gadget *)pm->scrollerV, pm->window, NULL,&tags[0]);
    }

}


/* Minimum timePerPixelWidth: 1 sample at 44100Hz should give at least 2 pixels.
 * min = 1/(44100*2) seconds/pixel = 1/88200 sec/px
 * In fixed-point 32.32: (1LL << 32) / 88200 = approximately 48693
 */
#define MIN_TIME_PER_PIXEL_WIDTH 48693LL

/**
 * Update the Horizontal scroller's domain based on the trackList's max time length
 * Should be called after track count changes, track modified, or layout changes.
 *
 * Domain (total) = project duration / timePerPixelWidth (in pixels)
 * Visible = current trackArea width (gadget width - header width)
 *
 * Also updates TimeRule's time border attributes to match visible time range.
 */
void updateHorizontalScrollDomain(TrackListView *pm)
{
    TimeProjection trackListTimeproj;
    struct Gadget *trackListGad;
    AukAProject *project;
    long long duration;
    long long timePerPixelWidth;
    long long timeLeft, timeRight;
    long long domainWidthPix;
    ULONG visibleWidthRelative;
    ULONG totalScroll;
    ULONG visibleScroll;
    LONG arrowDelta;

    if(!pm || !pm->trackList || !pm->scrollerH) return;

    trackListGad = (struct Gadget *)pm->trackList;
    project = (AukAProject *)pm->project;

    if(!trackListGad || !project) return;
    if( trackListGad->Width <=0) return;

    /* Get project duration (AukFixed 32.32 format, in seconds) */
    duration = project->GetDuration(project);

    if(duration <= 0) {
        /* No duration, set scroller to full visible (disabled state) */
        SetGadgetAttrs((struct Gadget *)pm->scrollerH, pm->window, NULL,
            SCROLLER_Total, 1,
            SCROLLER_Visible, 1,
            SCROLLER_ArrowDelta,0,
            TAG_END);

        return;
    }


    /* Get visible width from the gadget. TrackListArea uses headerWidth for left side,
     * so visible track area width = gadget width.
     * The actual visible time area would be gadget width, but we use domainWidth directly.
     */
     GetAttr(TRACKLIST_TimeProjection, pm->trackList, &trackListTimeproj);
     timePerPixelWidth = trackListTimeproj._timePerPixelWidth;

    /* Clamp timePerPixelWidth to minimum (prevent divide by zero and over-zoom) */
    if(timePerPixelWidth < MIN_TIME_PER_PIXEL_WIDTH) {
        timePerPixelWidth = MIN_TIME_PER_PIXEL_WIDTH;
    }

    /* Calculate domain width in pixels: domainWidth = duration / timePerPixelWidth
     * Both are in AukFixed 32.32 format, so dividing them gives an integer result in pixels.
     */

     if(domainWidthPix<=0) domainWidthPix=1;

  //  visibleWidth = (ULONG) (((long long)trackListGad->Width)*SCROLLERH_FIXEDTOTAL)/domainWidthPix;
 //aka
    visibleWidthRelative = (ULONG) (((long long)trackListGad->Width*timePerPixelWidth*SCROLLERH_FIXEDTOTAL)/duration);


    if(visibleWidthRelative == 0) visibleWidthRelative = 1;

    arrowDelta = visibleWidthRelative*16/trackListGad->Width; // should do 16 pixels
    if(arrowDelta==0) arrowDelta=1;

    SetGadgetAttrs((struct Gadget *)pm->scrollerH, pm->window, NULL,
        SCROLLER_Total,/* totalScroll*/ SCROLLERH_FIXEDTOTAL, // WORD 16384
        SCROLLER_Visible, visibleWidthRelative,
        SCROLLER_ArrowDelta,arrowDelta,
        TAG_END);

    SetGadgetAttrs((struct Gadget *)pm->timerule, pm->window, NULL,
        TIMERULE_TimePerPixelWidth,&timePerPixelWidth,
        // not here INFINITESCROLL_Position, &trackListTimeproj._timeAtLeft,
        TAG_END);

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
    }
    // retain project
    AukObjectPtr_Set(&pm->project,&project->base.base);

    // link data to UI
    TrackListArea_setTrackList(pm->trackList ,project );

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
        pm->window = M->opu_GInfo->gi_Window;
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
        pm->updateBits |= TLVB_UPDATE_REDRAW_TRACKLIST;
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
        SetGadgetAttrs((struct Gadget *)pm->trackList, pm->window, NULL,
                    TRACKLIST_ScrollY,scrollY,
                    TAG_END
                );
    }

}

static void TrackListView_SetHScrollPos(TrackListView *pm,TimeProjection *timeproj )
{
    ULONG headerWidth = 0;
    GetAttr(TRACKLIST_HeaderWidth, pm->trackList, &headerWidth);

    SetGadgetAttrs((struct Gadget *)pm->trackList, pm->window, NULL,
                TRACKLIST_TimeProjection,(ULONG *) timeproj,
                TAG_END
            );

    timeproj->_pixAtLeft -= (long long)headerWidth;


    pm->updateBits |= TLVB_UPDATE_REDRAW_TIMERULE;
    SetGadgetAttrs((struct Gadget *)pm->timerule, pm->window, NULL,
        TIMERULE_TimePerPixelWidth,&timeproj->_timePerPixelWidth,
        INFINITESCROLL_Position, &timeproj->_pixAtLeft,
        TAG_END);

}

/* Update trackList horizontal scroll position from horizontal scroller.
 * The scroller SCROLLER_Top is in "scroller units" which we convert to time. -> total domain is SCROLLERH_FIXEDTOTAL
 * The scroller's domain is computed in updateHorizontalScrollDomain().
 * Also updates TimeRule's time border attributes to match new scroll position.
 */
void TrackListView_ListenScrollHMessage(TrackListView *pm, struct opUpdate *M)
{
// bdbprintf("//// TrackListView_ListenScrollHMessage\n");
	AukAProject *project;
	project = (AukAProject *)pm->project;
	if(!project) return;

    struct TagItem *ptag;
    if((ptag = FindTagItem( SCROLLER_Top, M->opu_AttrList )) != NULL)
    {
        long long duration;
        ULONG scrollerTop = ptag->ti_Data;
        ULONG timePerPixLo = 0, timePerPixHi = 0;

        ULONG visibleWidth;
        struct Gadget *trackListGad;
        TimeProjection trackListTimeproj;

        GetAttr(TRACKLIST_TimeProjection, pm->trackList, &trackListTimeproj);

        if(trackListTimeproj._timePerPixelWidth<=0 ) return;
        duration = project->GetDuration(project);
        if(duration<=0) return;
       // timePerPixelWidth = trackListTimeproj._timePerPixelWidth; // ((long long)timePerPixHi << 32) | timePerPixLo;

        /* scrollX = value in pixel 64b, not in time */
        trackListTimeproj._pixAtLeft = ((long long)scrollerTop * duration) / (trackListTimeproj._timePerPixelWidth *SCROLLERH_FIXEDTOTAL);
  bdbprintf("trackListTimeproj._pixAtLeft:%lld",trackListTimeproj._pixAtLeft);
        TrackListView_SetHScrollPos(pm,&trackListTimeproj);

    }
}

void TrackListView_ListenTrackHeaderMessage(TrackListView *pm,struct opUpdate *M, ULONG gadId)
{
    ULONG buttonId = gadId & GAD_TRACKHEADER_IDMASK;
    ULONG trackId = (gadId & GAD_TRACKHEADER_TRACKMASK)>>4; // 4096 tracks possible, 16 buttons

    switch(buttonId)
    {
        case GAD_TRACKHEADER_CLOSE:
        // TODO close track
        break;
        case GAD_TRACKHEADER_NAME:
            // TODO track name edit.
        break;
        case GAD_TRACKHEADER_VOL:
            // TODO volume slide has changed
        break;
        case GAD_TRACKHEADER_PAN:
            // TODO pan slide has changed
        break;
        default:
        break;

    }

}
void TrackListView_CheckUpdates(TrackListView *pm)
{
    if(pm->updateBits & TLVB_UPDATE_VERTSCROLLDOMAIN) updateVerticalScrollDomain(pm);
    if(pm->updateBits & TLVB_UPDATE_HORIZSCROLLDOMAIN) updateHorizontalScrollDomain(pm);

    if(pm->updateBits & TLVB_UPDATE_REDRAW_TRACKLIST)
    {
        SetGadgetAttrs(pm->trackList, pm->window, NULL,TRACKLIST_Refresh,TRUE,TAG_END);
    }
    if(pm->updateBits & TLVB_UPDATE_REDRAW_TIMERULE)
    {
        SetGadgetAttrs(pm->timerule, pm->window, NULL,TIMERULE_Refresh,TRUE,TAG_END);
    }


    pm->updateBits = 0;
}

void TrackListView_UpdateTrackList(TrackListView *pm)
{
    if(!pm->trackList) return;
    SetGadgetAttrs(pm->trackList, pm->window, NULL,TRACKLIST_Refresh,TRUE,TAG_END);
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
    TrackAreaStaticClose();
    TimeRuleStaticClose();
    InfiniteScrollStaticClose();
}

/*
 debug report note:
  if send OM_UPDATE to notify a size at the end of a GM_LAYOUT, and modify scroller domain in it, will not work
  -> delay update for setGadgetAttrib()


*/
