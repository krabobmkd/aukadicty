#ifndef _CLASS_TRACKLISTPRIVATE_H_
#define _CLASS_TRACKLISTPRIVATE_H_

#include "compilers.h"
#include "class_tracklistarea.h"

// not much sense because c++ static runtime are hard to link.
#ifdef __cplusplus
extern "C" {
#endif

#include <exec/types.h>
#include <exec/libraries.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/gadgetclass.h>
#include <intuition/imageclass.h>
#include <graphics/gfx.h>
#include <graphics/regions.h>

#include "aukaproject.h"

// enable or not some parts of code...
#define USE_REGION_CLIPPING 1

/** retain gadget children for a given data track */
typedef struct TrackChild{
    AukTrack *_dataTrack; // corresponding data
    Object *_trackHeader; // TrackHeader gadget
    Object *_trackArea; // TrackArea gadget
    /* configurable height for this track, pixel. */
    UWORD _prefHeight;
} TrackChild;

/**
*
* an important principle of boopsi is that structure for the class is hidden to the consumers.
*  - yes, but as TrackListArea is an internal class, we use the private methods in the main for the moment.
*  We use TrackListArea as a layout boopsi gadget, that allocates/place/free automatically boopsi Track gadgets,
*  to mirror the data tracks, and we layout them like if we were a vertical scroll area.
*  then boopsi Track gadgets acts as layouts placnig
*  boopsi objects Tracks, itself acting
*
*  in charge of allocating, layouting and  boopsi class
*/
typedef struct TrackListArea {

    // would have minimal size here.
    UWORD _minimalWidth,_minimalHeight;

    // get pixel rectangle iof the gadget at layout
    struct Rectangle _framerec;

    /* The document data which own the track list as AukArray ->tracks */
    AukAProjectPtr _project;

    /* retain the synchronisation to data track list to gadgets... */
    TrackChild *_tracks;
    /* Number of allocated track gadgets/headers */
    ULONG _trackCount;

    /* Fixed width for track headers on the left */
    UWORD _headerWidth;
    /* Default height for each track row, then value in TrackArea gadget */
    UWORD _defaulTrackHeight;

    /* Vertical scroll position (first visible pixel line) */
    LONG _scrollY;

    /* Total domain height (sum of all track heights), updated in Layout */
    ULONG _domainHeight;

    /* Total domain width in pixels, computed from project duration / timePerPixelWidth */
    unsigned long long _domainWidth;
    /* Amount of seconds for a pixel width in fixed 32.32 value, aka the time zoom rate.
     * Lower value = more zoomed in. Must not be zero.
     * Minimum allowed: 1/(44100*2) to ensure 44kHz sample gets at least 2 pixels.
     */
    AukFixed _timePerPixelWidth;

    /* Horizontal scroll position: time at the left border of scroll area.
     * Signed 64-bit AukFixed value - can be negative for time before zero.
     */
    long long _scrollX;

    /* check for change at layout*/
    ULONG _prevHeight;
    /* Pointer to AukStyleSheet for visual styling */
    struct AukStyleSheet *_styleSheet;

    /* allow our scroll strategy */
    struct Region *_clipRegion;

} TrackListArea;

// internal tool
ULONG TrackListArea_NotifyAttribValue(struct Gadget *Gad, struct GadgetInfo	*GInfo,ULONG attrib, ULONG value);

/* internal use */
ULONG TrackListArea_SetAttrs(Class *C, struct Gadget *Gad, struct opSet *Set);
ULONG TrackListArea_GetAttr(Class *C, struct Gadget *Gad, struct opGet *Get);
ULONG TrackListArea_Layout(Class *C, struct Gadget *Gad, struct gpLayout *layout);
ULONG TrackListArea_Render(Class *C, struct Gadget *Gad, struct gpRender *Render);
ULONG TrackListArea_HandleInput(Class *C, struct Gadget *Gad,struct gpInput *M, int x, int y);
ULONG TrackListArea_GoInactive(Class *C, struct Gadget *Gad,struct gpGoInactive *M);
ULONG TrackListArea_HandleHitTest(Class *C, struct Gadget *Gad, struct gpHitTest *m);
ULONG TrackListArea_Domain(Class *C, struct Gadget *Gad, struct gpDomain *D);

/* esxternal way to refresh (experimental */
//void TrackListArea_Refresh(struct Gadget *Gad, struct Window *window);

/* Helper to dispose all gadget arrays */
void TrackListArea_DisposeGadgets(TrackListArea *gdata);

/* - - - - -- - */

/** for dispatcher, very wise use of union.
 *  each  struct also starts with MethodID.
 * and they are the very parameters for each methods.
 */
// typedef union MsgUnion
// {
//   ULONG  MethodID;
//   // from classusr.h or gadgetclass.h, all starts with MethodID.
//   struct opSet        opSet;
//   struct opUpdate     opUpdate;
//   struct opGet        opGet;
//   struct gpHitTest    gpHitTest;
//   struct gpRender     gpRender;
//   struct gpInput      gpInput;
//   struct gpGoInactive gpGoInactive;
//   struct gpLayout     gpLayout;
// } *Msgs;

/**
* This is to publish ou data when they change.
* It may be better to just notify what change and have many notify functions per theme.
* Some examples use only one Notify which send all attribs.
*/
ULONG TrackListArea_NotifyCoords(Class *C, struct Gadget *Gad, struct GadgetInfo	*GInfo);

/** this is the struct that is the extended struct Library
 * That is created with OpenLibrary().
 * But as it just manages a BOOPSI class there are just the open/close functions.
 * which themselves only manages registering the class with MakeClass()/AddClass()
 * versioning, and closing itself. This is *not* the boopsi class definition which is up there.
 * So it doesnt have to evolve, and can keep same name for each projects.
 * That said, layout.gadget has tool methods like any library.
 * must be mirrored to equivalent in classinit.s
 */
// struct ExtClassLib
// {
//     struct ClassLibrary cb_ClassLibrary;

//     APTR  cb_SysBase; // this is passed as LibInit
//     APTR  cb_SegList; // this is passed at OpenLib and needed at expunge.
//     // note: old libraries examples adds bases for graphics/intuition/utility after this
//     // but C compiler will only search then in globals...
// };

#ifdef __cplusplus
}
#endif

#endif
