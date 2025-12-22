#ifndef TrackListView_H
#define TrackListView_H

#include <intuition/classusr.h>

#include <aukobject.h>

/*
    keeps anything related to track list view in the UI.
    Keeps main layouts and instance of private gadget classes.
    receive updates from the document, and synchronize UI accordingly.

*/
typedef struct TrackListView
{
    // - - these are the BOOPSI gadgets for the track list view:
            Object *timerule; /* private gadget that scrolls the time rule, first line of layout. */
        Object *subBHl;     /* Horizontal layout on second line  */
            Object *trackList; /* private gadget that manages all tracks layout */
            Object *scrollerV;
        Object *scrollerH; /* Horizontal scroller as 3rd line of layout */

    Object *mainVl; /* the first vertical layout */
    // - - - the aukObject to listen updates from all project objects.
    AukObjectPtr updateListener;
    AukObjectPtr project;
} TrackListView;

void CreateTrackListView(TrackListView *pm,struct DrawInfo *drawInfo,
                int headerwidth,
                int fontheight);

typedef struct AukAProject AukAProject;
typedef struct sAukTrack AukTrack;
/**
    if project NULL, will release.
*/
void TrackListView_setProject(TrackListView *pm,AukAProject *project);

void CloseTrackListView(TrackListView *pm);
void CloseTrackListView_StaticClasses();
#endif
