
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
#include "TrackListAreaUi/class_tracklistareaui.h"

#include <aukobject.h>
// audio tracks project
#include <aukaproject.h>
#include <auktrack.h>
#include "bdbprintf.h"

#ifdef Remove
#undef Remove
#endif

void cleanexit(const char *pmessage);

void CreateTrackListView(TrackListView *pm,struct DrawInfo *drawInfo, int headerwidth, int fontheight)
{
    // init private boopsi gadget & layout classes.

    if(!InfiniteScrollStaticInit()) cleanexit("InfiniteScroll failed");
    if(!TimeRuleStaticInit()) cleanexit("TimeRule failed");
    if(!TrackAreaStaticInit()) cleanexit("TrackLayout init failed");
    if(!TrackHeaderStaticInit()) cleanexit("TrackLayout init failed");
    if(!TrackListStaticInit()) cleanexit("TrackLayout init failed");
    // - - - - - A

    pm->timerule = (Object *)NewObject( TIMERULE_GetClass(), NULL,
                            TIMERULE_DefHeight,(fontheight*3)/2,
                            TAG_END);


    // - - - - - B
        pm->trackList = (Object *)NewObject( TRACKLIST_GetClass(), NULL,
                                TAG_END);

        pm->scrollerV = (Object *)NewObject( SCROLLER_GetClass(), NULL,
                                    GA_DrawInfo, drawInfo,
                                    //GA_ID,GAD_SCROLLER_VALUE,
                                    GA_RelVerify, TRUE, // needed
                                SCROLLER_Top, 0,
                                SCROLLER_Total, 40,
                                SCROLLER_Visible, 10,
                                SCROLLER_Orientation, FREEVERT,
                                SCROLLER_Stretch,TRUE,
                             //   ICA_TARGET,app->testBaseName,
                             //   ICA_MAP,(ULONG)map_slider_to_basename_value,
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
                                //GA_ID,GAD_SCROLLER_VALUE,
                                GA_RelVerify, TRUE, // needed
                            SCROLLER_Top, 0,
                            SCROLLER_Total, 40,
                            SCROLLER_Visible, 10,
                            SCROLLER_Orientation, FREEHORIZ,
                            SCROLLER_Stretch,TRUE,
                         //   ICA_TARGET,app->testBaseName,
                         //   ICA_MAP,(ULONG)map_slider_to_basename_value,
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
                CHILD_MaxHeight,(fontheight*3)/2,
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
    if(!pm || !track) return;
    bdbprintf(" **** AukUpdate_Track ! \n");
    switch(message->type)
    {
        case AUK_MSG_TRACKMODIFIED_TIMECHANGE:
        {

            //TODO update GUI, add ui track
        }
        break;
        case AUK_MSG_TRACKMODIFIED_SOUNDADDED:
        {

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
    bdbprintf(" **** AUK_MSG_TRACKADDED ! \n");
            AukMessage_AProject *m = (AukMessage_AProject *)message;
            AukTrack *track = m->_track;
            if(track)  AukObject_AddListener(track,
                  pm->updateListener, // AukObject* listenerObject,
                  (void*)pm, // userData
                  &AukUpdate_Track //AukUpdateCallback callback
                  );

            // update GUI, add ui track
            TrackListAreaUi_addTrack(trackListAreaUi,track);
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
            TrackListAreaUi_removeTrack(trackListAreaUi,track);
        }
        break;
        default:
    bdbprintf(" **** AUK_MSG_XXX %d! \n",(int)message->type);
        break;
    }

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
    TrackListAreaUi_setTrackList(pm->trackList ,project );

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
