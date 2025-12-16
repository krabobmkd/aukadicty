
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

#include "tracklayout.h"

#include <proto/scroller.h>
#include <gadgets/scroller.h>

#include <proto/label.h>
#include <images/label.h>

#include <proto/layout.h>
#include <gadgets/layout.h>

#include "TimeRule/class_timerule.h"
#include "TrackGadget/class_track.h"
#include "TrackHeader/class_trackheader.h"
#include "TrackHeaderList/class_trackheaderlist.h"
#include "TrackList/class_tracklist.h"

void cleanexit(const char *pmessage);

void CreateTrackLayout(TrackLayoutManager *pm,struct DrawInfo *drawInfo, int headerwidth)
{

    if(TimeRuleStaticInit()) cleanexit("boopsi init1");
    if(TrackStaticInit()) cleanexit("boopsi init2");
    if(TrackHeaderStaticInit()) cleanexit("boopsi init3");
    if(TrackHeaderListStaticInit()) cleanexit("boopsi init4");
    if(TrackListStaticInit()) cleanexit("boopsi init5");
    // - - - - - A
//        Object* spacer1 = (Object *)NewObject( LABEL_GetClass(), NULL,
//                        LABEL_DrawInfo,drawInfo,
//                        LABEL_Text,(ULONG)" ",
//                    TAG_END);


        pm->timerule = (Object *)NewObject( TIMERULE_GetClass(), NULL,
                                TAG_END);
//        Object* spacer2 = (Object *)NewObject( LABEL_GetClass(), NULL,
//                        LABEL_DrawInfo,drawInfo,
//                        LABEL_Text,(ULONG)" ",
//                    TAG_END);

//   pm->subAHl = (Object *)NewObject( LAYOUT_GetClass(), NULL,
//                    LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
//                    LAYOUT_BevelStyle, /*BVS_GROUP*/BVS_NONE,

//             LAYOUT_SpaceOuter, FALSE,
//             LAYOUT_SpaceInner, FALSE,
//             LAYOUT_BottomSpacing, 0,
//             LAYOUT_TopSpacing,0,
//             LAYOUT_LeftSpacing,0,
//             LAYOUT_RightSpacing,0,
//             LAYOUT_InnerSpacing,0,

//                    LAYOUT_AddChild, spacer1,
//                CHILD_WeightedWidth,0,
//                CHILD_MinWidth,headerwidth,
//                    LAYOUT_AddChild, pm->timerule,
//                CHILD_WeightedWidth,1,
//                    LAYOUT_AddChild,spacer2,
//                CHILD_WeightedWidth,0,
//                    TAG_DONE);



    // - - - - - B
        pm->trackHeaderList = (Object *)NewObject( TRACKHEADERLIST_GetClass(), NULL,
                                TAG_END);

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

                LAYOUT_AddChild,pm->trackHeaderList /*pm->subAHl*/,
            CHILD_WeightedWidth,0,
            CHILD_MinWidth,headerwidth,
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
                    LAYOUT_AddChild, pm->subBHl,
                CHILD_WeightedHeight,1,
                    LAYOUT_AddChild, pm->scrollerH,
                CHILD_WeightedHeight,0,
                    TAG_DONE);
/*
        Object *subAHl;
            Object *timerule;
        Object *subBHl;
            Object *trackHeaderList;
            Object *trackList;
            Object *scrollerV;
        Object *scrollerH;

    Object *mainVl;
*/
}
