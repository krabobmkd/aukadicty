#ifndef TrackListView_H
#define TrackListView_H

#include <intuition/classusr.h>
#include <intuition/intuition.h>
#include <aukdefs.h>
#include "aukstyle.h"



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

    Object *appModel;
    //NEVER KEEP WINDOW, VOLATILE struct Window *window;

    /* Visual style configuration */
    AukStyle *pstyleSheet;

    /* kind of easy message update to delay, def below. If zero, nothing to do*/
    ULONG updateBits;

    /* current edit mode aukeditmode.h */
    int editMode;

} TrackListView;


#define TLVB_UPDATE_VERTSCROLLDOMAIN 1
#define TLVB_UPDATE_HORIZSCROLLDOMAIN 2
#define TLVB_UPDATE_REDRAW_JUSTHEADERS 4
#define TLVB_UPDATE_REDRAW_JUSTTRACKS 8
#define TLVB_UPDATE_REDRAW_TRACKLIST (4+8)
#define TLVB_UPDATE_REDRAW_TIMERULE 16

void CreateTrackListView(TrackListView *pm,struct DrawInfo *drawInfo,
                Object *appModel,
                AukStyle *stylesheet);

/**
    if project NULL, will release.
*/
void TrackListView_setProject(TrackListView *pm,AukAProject *project);

void TrackListView_ListenTrackListMessage(TrackListView *pm,struct opUpdate *M);
void TrackListView_ListenScrollVMessage(TrackListView *pm,struct opUpdate *M);
void TrackListView_ListenScrollHMessage(TrackListView *pm,struct opUpdate *M);
/* also get message from all TrackArea */
void TrackListView_ListenTrackHeaderMessage(TrackListView *pm,struct opUpdate *M, ULONG gadId);

void TrackListView_SetEditMode(TrackListView *pm, int editMode);

void TrackListView_CheckUpdates(TrackListView *pm);
void TrackListView_UpdateTrackList(TrackListView *pm);
void TrackListView_UpdateTimeRule(TrackListView *pm);

void TrackListView_ZoomIn(TrackListView *pm);
void TrackListView_ZoomOut(TrackListView *pm);

void CloseTrackListView(TrackListView *pm);
void CloseTrackListView_StaticClasses();



#endif
