#ifndef TRACKLAYOUT_H
#define TRACKLAYOUT_H

#include <intuition/classusr.h>

typedef struct TrackLayoutManager
{
        //Object *subAHl;
            Object *timerule;
        Object *subBHl;
            Object *trackHeaderList;
            Object *trackList;
            Object *scrollerV;
        Object *scrollerH;

    Object *mainVl;
}TrackLayoutManager;

void CreateTrackLayout(TrackLayoutManager *pm,struct DrawInfo *drawInfo, int headerwidth);


#endif
