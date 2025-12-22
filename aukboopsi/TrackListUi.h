#ifndef TrackListUi_H
#define TrackListUi_H

#include <intuition/classusr.h>

#include <aukobject.h>

/*
    keeps anything related to track list view in the UI
*/
typedef struct TrackListUi
{
    // - - these are the BOOPSI gadgets for the track list view:
        //Object *subAHl;
            Object *timerule;
        Object *subBHl;
            Object *trackHeaderList;
            Object *trackList;
            Object *scrollerV;
        Object *scrollerH;

    Object *mainVl;
    // - - - the aukObject to listen updates from all project objects.
    AukObjectPtr updateListener;
    AukObjectPtr project;
} TrackListUi;

void CreateTrackListUi(TrackListUi *pm,struct DrawInfo *drawInfo,
                int headerwidth,
                int fontheight);

typedef struct AukAProject AukAProject;
typedef struct sAukTrack AukTrack;
/**
    if project NULL, will release.
*/
void TrackListUi_setProject(TrackListUi *pm,AukAProject *project);

void CloseTrackListUi(TrackListUi *pm);

#endif
