#ifndef _CLASS_INFINITESCROLLPRIVATE_H_
#define _CLASS_INFINITESCROLLPRIVATE_H_

#include "compilers.h"
#include "class_infinitescroll.h"

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

#include "../offscreenbm.h"

/* Enable or not some parts of code... */
#define USE_REGION_CLIPPING 1

/**
 * Represents a single tile in the horizontal scroll cache
 */
typedef struct InfiniteScrollTile
{
    /* Offscreen bitmap for this tile */
    OffscreenBitMap bitmap;

    /* Abstract position this tile represents (left edge, 64-bit signed) */
    LONG abstractPosHi;
    LONG abstractPosLo;

    /* Dirty flag - TRUE if tile needs re-rendering */
    BOOL isDirty;

    /* Valid flag - TRUE if tile is allocated and initialized */
    BOOL isValid;

} InfiniteScrollTile;

/**
 * InfiniteScroll - Abstract base gadget class for horizontally scrolling content
 *
 * This is the internal private gadget struct that owns the data of the object instances.
 * An important principle of BOOPSI is that structure for the class is hidden to the consumers.
 * The consumers will only see the public header, and will do SetAttribs()/GetAttribs()/DoMethod().
 * Also: for the same Gadget, superclass members are in struct Gadget * passed to functions.
 * (These are just concatenated structs in a system private way.)
 */
typedef struct IInfiniteScroll {
    /* Would have minimal size here */
    UWORD _minimalWidth, _minimalHeight;

    /* Gadget rectangle set at layout */
    struct Rectangle _framerec;

#ifdef USE_REGION_CLIPPING
    struct Region *_clipRegion;
#endif

    /* Scrollable domain (signed 64-bit values) */
    LONG _domainMinHi, _domainMinLo;  /* Minimum abstract position */
    LONG _domainMaxHi, _domainMaxLo;  /* Maximum abstract position */

    /* Current view state (signed 64-bit values) */
    LONG _viewPosHi, _viewPosLo;      /* Current scroll position */
    LONG _viewZoomHi, _viewZoomLo;    /* Current zoom level */

    /* Tile configuration */
    UWORD _tileWidth;                  /* Width of each tile in pixels (default 128) */
    UWORD _tileHeight;                 /* Height matches gadget height */

    /* Tile array */
    InfiniteScrollTile *_tiles;        /* Array of tiles */
    ULONG _tileCount;                  /* Number of allocated tiles */

    /* Cached bitmap mode info for tile allocation */
    struct BitMap *_friendBitmap;      /* Friend bitmap for mode (from screen) */

} InfiniteScroll;

ULONG InfiniteScroll_SetAttrs(Class *C, struct Gadget *Gad, struct opSet *Set);
ULONG InfiniteScroll_GetAttr(Class *C, struct Gadget *Gad, struct opGet *Get);
ULONG InfiniteScroll_Layout(Class *C, struct Gadget *Gad, struct gpLayout *layout);
ULONG InfiniteScroll_Render(Class *C, struct Gadget *Gad, struct gpRender *Render, ULONG update);
ULONG InfiniteScroll_HandleInput(Class *C, struct Gadget *Gad, struct gpInput *Input);
ULONG InfiniteScroll_Domain(Class *C, struct Gadget *Gad, struct gpDomain *D);

/* Tile management helpers */
void InfiniteScroll_DisposeTiles(InfiniteScroll *gdata);
void InfiniteScroll_InvalidateTiles(Class *C, struct Gadget *Gad, struct gpInvalidateTiles *Msg);
void InfiniteScroll_RefreshTiles(Class *C, struct Gadget *Gad);

/* Helper to render a single tile (calls child class implementation) */
void InfiniteScroll_RenderTile(Class *C, struct Gadget *Gad, InfiniteScrollTile *tile, ULONG tileIndex);

/* - - - - -- - */

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
  struct gpInvalidateTiles gpInvalidateTiles;
} *Msgs;

/**
 * This is to publish our data when they change.
 * It may be better to just notify what change and have many notify functions per theme.
 * Some examples use only one Notify which send all attribs.
 */
ULONG InfiniteScroll_NotifyAttrs(Class *C, struct Gadget *Gad, struct GadgetInfo *GInfo);

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
