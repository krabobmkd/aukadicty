#ifndef _CLASS_TIMERULEPRIVATE_H_
#define _CLASS_TIMERULEPRIVATE_H_

#include "compilers.h"
#include "class_timerule.h"

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

// enable or not some parts of code...
#define USE_REGION_CLIPPING 1

/**
 * This is the internal private gadget struct that own the data of the object instances.
 * An important principle of BOOPSI is that structure for the class is hidden to the consumers.
 * The consumers will only see the public header, and will do SetAttribs()/GetAttribs()/DoMethod().
 * Also: for the same Gadget, superclass members are in struct Gadget * passed to functions.
 * (These are just concatenated structs in a system private way.)
 *
 * TimeRule inherits from InfiniteScroll - the superclass manages:
 * - _tiles array with offscreen bitmaps
 * - _framerec rectangle
 * - _clipRegion for clipping
 * - _domainMin/Max, _viewPos, _viewZoom
 * - _tileWidth, _tileHeight, _tileCount
 */
typedef struct ITimeRule {
    /* Default height for the ruler */
    UWORD _defaultHeight;

    /* Mouse interaction state (for future use) */
    ULONG _MouseMode;
    ULONG _EditMode;

    /* Time at left border of TimeRule (signed 64-bit AukFixed) */
    LONG _timeLeftHi;
    LONG _timeLeftLo;

    /* Time at right border of TimeRule (signed 64-bit AukFixed) */
    LONG _timeRightHi;
    LONG _timeRightLo;

    /* X offset from TimeRule left to where TrackListArea starts */
    /* This is typically the TrackHeader width */
    UWORD _trackAreaOffsetX;

    /* Pointer to AukStyleSheet for visual styling (fonts, colors) */
    struct AukStyleSheet *_styleSheet;

} TimeRule;

ULONG TimeRule_SetAttrs(Class *C, struct Gadget *Gad, struct opSet *Set);
ULONG TimeRule_GetAttr(Class *C, struct Gadget *Gad, struct opGet *Get);
ULONG TimeRule_Domain(Class *C, struct Gadget *Gad, struct gpDomain *D);

/* TimeRule overrides InfiniteScroll tile rendering to draw graduations */
/* This handles GM_INFINITESCROLL_RENDERTILE method */
ULONG TimeRule_RenderTile(Class *C, struct Gadget *Gad, struct gpRenderTile *M);

/* Helper to format time value as text */
void TimeRule_FormatTime(LONG timeHi, LONG timeLo, char *buffer, BOOL showMs);

// - - - - -- -

/** for dispatcher, very wise use of union.
 *  each  struct also starts with MethodID.
 * and they are the very parameters for each methods.
 */
typedef union MsgUnion
{
  ULONG  MethodID;
  /* from classusr.h or gadgetclass.h, all starts with MethodID. */
  struct opSet        opSet;
  struct opUpdate     opUpdate;
  struct opGet        opGet;
  struct gpHitTest    gpHitTest;
  struct gpRender     gpRender;
  struct gpInput      gpInput;
  struct gpGoInactive gpGoInactive;
  struct gpLayout     gpLayout;
  struct gpDomain     gpDomain;
  struct gpRenderTile gpRenderTile;  /* From InfiniteScroll */
} *Msgs;

/**
* This is to publish ou data when they change.
* It may be better to just notify what change and have many notify functions per theme.
* Some examples use only one Notify which send all attribs.
*/
ULONG TimeRule_NotifyCoords(Class *C, struct Gadget *Gad, struct GadgetInfo	*GInfo);

/** this is the struct that is the extended struct Library
 * That is created with OpenLibrary().
 * But as it just manages a BOOPSI class there are just the open/close functions.
 * which themselves only manages registering the class with MakeClass()/AddClass()
 * versioning, and closing itself. This is *not* the boopsi class definition which is up there.
 * So it doesnt have to evolve, and can keep same name for each projects.
 * That said, layout.gadget has tool methods like any library.
 * must be mirrored to equivalent in classinit.s
 */
struct ExtClassLib
{
    struct ClassLibrary cb_ClassLibrary;

    APTR  cb_SysBase; // this is passed as LibInit
    APTR  cb_SegList; // this is passed at OpenLib and needed at expunge.
    // note: old libraries examples adds bases for graphics/intuition/utility after this
    // but C compiler will only search then in globals...
};

#ifdef __cplusplus
}
#endif

#endif
