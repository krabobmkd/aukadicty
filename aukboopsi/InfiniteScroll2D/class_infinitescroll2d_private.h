#ifndef _CLASS_INFINITESCROLL2DPRIVATE_H_
#define _CLASS_INFINITESCROLL2DPRIVATE_H_

#include "compilers.h"
#include "class_infinitescroll2d.h"

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

/* Tile size is fixed at 128x128 */
#define TILE2D_SIZE 128

/**
 * Represents a single tile in the 2D scroll cache.
 * Tiles are arranged in a 2D torus topology.
 */
typedef struct InfiniteScroll2DTile
{
    /* Offscreen bitmap for this tile (128x128) */
    OffscreenBitMap bitmap;

    /* position this tile represents (top-left corner, 64-bit signed) */
    InfiniteScroll2DPosition position;

    /* isRendered flag - TRUE if tile is allocated and rendered */
    int isRendered;

} InfiniteScroll2DTile;

/**
 * InfiniteScroll2D - Abstract base gadget class for 2D scrolling content
 *
 * This is the internal private gadget struct that owns the data of the object instances.
 * Tiles are arranged in a 2D rectangular grid that wraps in a torus topology.
 *
 * Torus indexing (no 64-bit modulo):
 *   actual_x = x + offsetX; if(actual_x >= tilesX) actual_x -= tilesX;
 *   actual_y = y + offsetY; if(actual_y >= tilesY) actual_y -= tilesY;
 */
typedef struct IInfiniteScroll2D {

    /* Scrollable position in pixels (64-bit signed for both X and Y) */
    InfiniteScroll2DPosition _position;
    /* pointer to _position, or elsewhere if deferred */
    InfiniteScroll2DPosition *_pposition;

    /* Tile configuration: fixed 128x128 per tile */
    UWORD _tileSize;               /* Always 128 */

    /* 2D Tile grid - allocated as a flat array, accessed as [y * tilesX + x] */
    InfiniteScroll2DTile *_tiles;  /* Flat array of tiles */
    UWORD _tilesX;                 /* Number of tiles in X direction */
    UWORD _tilesY;                 /* Number of tiles in Y direction */

    /* Torus offset: which tile index is currently at the top-left corner.
     * When scrolling, these offsets change instead of moving tile data.
     * Use subtraction-based wrapping (no modulo):
     *   if(index >= count) index -= count;
     *   if(index < 0) index += count;
     */
    WORD _torusOffsetX;
    WORD _torusOffsetY;

    /* Track which range of tiles is currently valid/rendered.
     * These are in "logical" coordinates before torus offset applied.
     * _renderedTilesX/Y: how many tiles are currently rendered.
     */
    WORD _renderedTilesX, _renderedTilesY;

    /* Cached layout dimensions */
    WORD _layoutedForWidth, _layoutedForHeight;

    /* The actual render function of the inherited implementation. */
    InfiniteScroll2DRenderf _renderFunction;

    /* internal use, allow using target screen pixel format in our bitmap */
    struct BitMap *_friendBitmap;

    /* Margin sizes in pixels for each border */
    UWORD _marginLeft;
    UWORD _marginRight;
    UWORD _marginTop;
    UWORD _marginBottom;

    /* Pen used to draw margin areas */
    WORD _marginPen;

} InfiniteScroll2D;

/* Attribute handlers */
ULONG InfiniteScroll2D_SetAttrs(Class *C, struct Gadget *Gad, struct opSet *Set);
ULONG InfiniteScroll2D_GetAttr(Class *C, struct Gadget *Gad, struct opGet *Get);

/* Layout and rendering */
ULONG InfiniteScroll2D_Layout(Class *C, struct Gadget *Gad, struct gpLayout *layout);
ULONG InfiniteScroll2D_Render(Class *C, struct Gadget *Gad, struct gpRender *Render);
ULONG InfiniteScroll2D_Domain(Class *C, struct Gadget *Gad, struct gpDomain *D);

/* Tile management helpers */
void InfiniteScroll2D_DisposeTiles(InfiniteScroll2D *gdata);

/* Notification */
ULONG InfiniteScroll2D_NotifyAttrs(Class *C, struct Gadget *Gad, struct GadgetInfo *GInfo);

/**
 * Helper: Get tile pointer at logical (x,y) with torus wrapping.
 * Uses subtraction-based wrapping to avoid 64-bit modulo on 68000.
 */
INLINE InfiniteScroll2DTile* InfiniteScroll2D_GetTile(InfiniteScroll2D *gdata, int x, int y)
{
    int ax = x + gdata->_torusOffsetX;
    int ay = y + gdata->_torusOffsetY;
    if(ax >= gdata->_tilesX) ax -= gdata->_tilesX;
    if(ax < 0) ax += gdata->_tilesX;
    if(ay >= gdata->_tilesY) ay -= gdata->_tilesY;
    if(ay < 0) ay += gdata->_tilesY;
    return &gdata->_tiles[ay * gdata->_tilesX + ax];
}

#ifdef __cplusplus
}
#endif

#endif
