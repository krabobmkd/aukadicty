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

    /* position this tile represents (left edge, 64-bit signed) */
    InfiniteScrollPosition position;

    /* isRendered flag - TRUE if tile is allocated and rendered */
    int isRendered;

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

    /* Scrollable position in pixel */
    InfiniteScrollPosition _position;

    /* Tile configuration */
    UWORD _tileWidth;                  /* Width of each tile in pixels (default 128) */
    UWORD _tileHeight;                 /* Height matches gadget height */

    /* Tile array */
    InfiniteScrollTile *_tiles;        /* Array of tiles */
    ULONG _tileCount;                  /* Number of allocated tiles */

    /* current index of tile used for the current leftmost tile in _tiles table,
      for which last rendered position fits between tile->position and tile->position+_tileWidth.
      Important: if <0, means no tile used yet, they need to be reattributed to some location.
     */
    WORD _currentLeftBorderTileIndex,_renderedTilesCount;

    /* The actual render function of the inherited implementation. */
    InfiniteScrollRenderf _renderFunction;

    /* internal use, allow using target screen pixel format in our bitmap */
    struct BitMap *_friendBitmap;
} InfiniteScroll;

ULONG InfiniteScroll_SetAttrs(Class *C, struct Gadget *Gad, struct opSet *Set);
ULONG InfiniteScroll_GetAttr(Class *C, struct Gadget *Gad, struct opGet *Get);
ULONG InfiniteScroll_Layout(Class *C, struct Gadget *Gad, struct gpLayout *layout);
ULONG InfiniteScroll_Render(Class *C, struct Gadget *Gad, struct gpRender *Render);
ULONG InfiniteScroll_HandleInput(Class *C, struct Gadget *Gad, struct gpInput *Input);
ULONG InfiniteScroll_Domain(Class *C, struct Gadget *Gad, struct gpDomain *D);

/* Tile management helpers */
void InfiniteScroll_DisposeTiles(InfiniteScroll *gdata);
// void InfiniteScroll_InvalidateTiles(Class *C, struct Gadget *Gad, struct gpInvalidateTiles *Msg);
// void InfiniteScroll_RefreshTiles(Class *C, struct Gadget *Gad);

/* Helper to render a single tile (calls child class implementation) */
//void InfiniteScroll_RenderTile(Class *C, struct Gadget *Gad, InfiniteScrollTile *tile, ULONG tileIndex);

/* - - - - -- - */



/**
 * This is to publish our data when they change.
 * It may be better to just notify what change and have many notify functions per theme.
 * Some examples use only one Notify which send all attribs.
 */
ULONG InfiniteScroll_NotifyAttrs(Class *C, struct Gadget *Gad, struct GadgetInfo *GInfo);


#ifdef __cplusplus
}
#endif

#endif
