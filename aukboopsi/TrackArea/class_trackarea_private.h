#ifndef _CLASS_TRACKAREAPRIVATE_H_
#define _CLASS_TRACKAREAPRIVATE_H_

#include "compilers.h"
#include "class_trackarea.h"
#include "aukstyle.h"
#include "aukselection.h"
/* Include InfiniteScroll for superclass */
#include "../InfiniteScroll/class_infinitescroll.h"

/* Include auktrack for AukTrackPtr and AukObjectPtr_Set/Release */
#include "auktrack.h"

/* not much sense because c++ static runtime are hard to link. */
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

/**
 * This is the internal private gadget struct that owns the data of the object instances.
 * An important principle of BOOPSI is that structure for the class is hidden to the consumers.
 * The consumers will only see the public header, and will do SetAttribs()/GetAttribs()/DoMethod().
 * Also: for the same Gadget, superclass members are in struct Gadget * passed to functions.
 * (These are just concatenated structs in a system private way.)
 *
 * TrackArea inherits from InfiniteScroll - the superclass manages:
 * - Horizontal scroll position (_position) and tile caching
 * - GM_LAYOUT, GM_RENDER for tile-based rendering
 */
typedef struct ITrackArea {

    /* Pointer to AukStyle for visual styling */
    AukStyle *_style;

    /* Pointer to TimeProjection in TrackListArea for time/pixel mapping.
     * This allows TrackArea to know horizontal scroll position and zoom level.
     */
    TimeProjection *_pTimeProjection;

    /* Pointer to the data track this gadget reflects.
     * Retained via AukObjectPtr_Set/Release for proper reference counting.
     */
    AukTrackPtr _dataTrack;

    /* next GM_RENDER will do accordingly */
    WORD _justScroll, _fullRedraw;

    /* Pointer to time selection span (start and end times).
     * NULL or both values == 0 means no selection.
     * This is used to draw selection span
     */
    AukSelection *_dataSelection;

#define TRCKMOVE_NoMove 0
#define TRCKMOVE_Selection 1
#define TRCKMOVE_PanZoom 2
#define TRCKMOVE_Slide 2

    /* interaction automats. Totally internal. */

    UBYTE _MoveType; /* what happens when click down and move. */
    UBYTE b,c,d;
    /* TRCKMOVE_Selection/TRCKMOVE_PanZoom during move.This is the value told by input, yet not set in data
        Thus, don't use this elsewhere than input methods and notify.
        The really applied selection is pointed by _dataSelection.
    */
    AukSelection _inputselection;


} TrackArea;

ULONG TrackArea_SetAttrs(Class *C, struct Gadget *Gad, struct opSet *Set);
ULONG TrackArea_GetAttr(Class *C, struct Gadget *Gad, struct opGet *Get);
ULONG TrackArea_Layout(Class *C, struct Gadget *Gad, struct gpLayout *layout);
ULONG TrackArea_HandleInput(Class *C, struct Gadget *Gad, struct gpInput *Input, int isFirstActivate );
ULONG TrackArea_GoInactive(Class *C, struct Gadget *Gad,struct gpGoInactive *M);

ULONG TrackArea_Domain(Class *C, struct Gadget *Gad, struct gpDomain *D);

/* TrackArea overrides InfiniteScroll tile rendering to draw track content */
void TrackArea_RenderDelegate(InfiniteScrollRenderParams *p);

/* need a few overriding */
ULONG TrackArea_Render(Class *C, struct Gadget *Gad, struct gpRender *Render);
/* - - - - -- - */

/**
* This is to publish our data when they change.
* It may be better to just notify what change and have many notify functions per theme.
* Some examples use only one Notify which send all attribs.
*/
ULONG TrackArea_NotifySelection(Class *C, struct Gadget *Gad, struct GadgetInfo	*GInfo);

/** this is the struct that is the extended struct Library
 * That is created with OpenLibrary().
 * But as it just manages a BOOPSI class there are just the open/close functions.
 * which themselves only manages registering the class with MakeClass()/AddClass()
 * versioning, and closing itself. This is *not* the BOOPSI class definition which is up there.
 * So it doesnt have to evolve, and can keep same name for each projects.
 * That said, layout.gadget has tool methods like any library.
 * must be mirrored to equivalent in classinit.s
 */
struct ExtClassLib
{
    struct ClassLibrary cb_ClassLibrary;

    APTR  cb_SysBase; /* this is passed as LibInit */
    APTR  cb_SegList; /* this is passed at OpenLib and needed at expunge. */
    /* note: old libraries examples adds bases for graphics/intuition/utility after this */
    /* but C compiler will only search then in globals... */
};

#ifdef __cplusplus
}
#endif

#endif
