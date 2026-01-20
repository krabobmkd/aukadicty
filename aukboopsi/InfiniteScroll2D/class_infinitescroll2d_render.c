
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/layers.h>

#include <clib/alib_protos.h>

#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/gadgetclass.h>
#include <utility/tagitem.h>

#include "class_infinitescroll2d.h"
#include "class_infinitescroll2d_private.h"

#include "../bdbprintf.h"

extern struct IClass *InfiniteScroll2DClassPtr;

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* GM_DOMAIN */
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

ULONG InfiniteScroll2D_Domain(Class *C, struct Gadget *Gad, struct gpDomain *D)
{
  D->gpd_Domain.Left=0;
  D->gpd_Domain.Top=0;

  switch(D->gpd_Which)
  {
    case GDOMAIN_NOMINAL:
      D->gpd_Domain.Width=256;
      D->gpd_Domain.Height=256;
      break;

    case GDOMAIN_MAXIMUM:
      D->gpd_Domain.Width=16000;
      D->gpd_Domain.Height=16000;
      break;

    case GDOMAIN_MINIMUM:
    default:
        D->gpd_Domain.Width=  TILE2D_SIZE;
        D->gpd_Domain.Height= TILE2D_SIZE;
      break;
  }
  return(1);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* Tile Management Helpers */
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/**
 * Dispose all allocated tiles
 */
void InfiniteScroll2D_DisposeTiles(InfiniteScroll2D *gdata)
{
    ULONG i, total;
    if(!gdata) return;

    if(gdata->_tiles)
    {
        total = (ULONG)gdata->_tilesX * (ULONG)gdata->_tilesY;
        for(i = 0; i < total; i++)
        {
            if(gdata->_tiles[i].isRendered)
            {
                OffscreenBitMap_Close(&gdata->_tiles[i].bitmap);
                gdata->_tiles[i].isRendered = FALSE;
            }
        }
        FreeVec(gdata->_tiles);
        gdata->_tiles = NULL;
    }

    gdata->_tilesX = 0;
    gdata->_tilesY = 0;
    gdata->_torusOffsetX = 0;
    gdata->_torusOffsetY = 0;
    gdata->_renderedTilesX = 0;
    gdata->_renderedTilesY = 0;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* GM_LAYOUT */
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/**
 * method GM_LAYOUT
 * The gadget knows its final coordinates and size.
 * Allocate 2D tile grid based on width and height.
 */
ULONG InfiniteScroll2D_Layout(Class *C, struct Gadget *Gad, struct gpLayout *layout)
{
    InfiniteScroll2D *gdata;
    UWORD neededTilesX, neededTilesY;
    ULONG totalTiles, i;

    gdata = INST_DATA(C, Gad);

    if(gdata->_layoutedForWidth == Gad->Width &&
       gdata->_layoutedForHeight == Gad->Height)
    {
        /* already ok */
        return 1;
    }

    /* Calculate needed tile count: (dimension / tileSize) + 2 */
    /* +2 for smooth scrolling (one tile on each side can be pre-rendered) */
    neededTilesX = (Gad->Width / TILE2D_SIZE) + 2;
    neededTilesY = (Gad->Height / TILE2D_SIZE) + 2;

    /* Check if we need to reallocate tiles */
    if(neededTilesX != gdata->_tilesX || neededTilesY != gdata->_tilesY)
    {
        /* Dispose old tiles */
        InfiniteScroll2D_DisposeTiles(gdata);

        totalTiles = (ULONG)neededTilesX * (ULONG)neededTilesY;

        /* Allocate new tile array (flat, accessed as [y * tilesX + x]) */
        if(totalTiles > 0)
        {
            gdata->_tiles = (InfiniteScroll2DTile*)AllocVec(
                totalTiles * sizeof(InfiniteScroll2DTile),
                MEMF_CLEAR);

            if(!gdata->_tiles)
            {
                gdata->_tilesX = 0;
                gdata->_tilesY = 0;
                return 1; /* Failed but don't crash */
            }

            gdata->_tilesX = neededTilesX;
            gdata->_tilesY = neededTilesY;

            /* Get friend bitmap from screen for correct display mode */
            if(layout->gpl_GInfo && layout->gpl_GInfo->gi_Screen)
            {
                gdata->_friendBitmap = layout->gpl_GInfo->gi_Screen->RastPort.BitMap;
            }

            /* Initialize each tile with 128x128 bitmap */
            for(i = 0; i < totalTiles; i++)
            {
                OffscreenBitMap_Init(&gdata->_tiles[i].bitmap,
                                     TILE2D_SIZE,
                                     TILE2D_SIZE,
                                     0, /* depth from friend */
                                     BMF_CLEAR,
                                     gdata->_friendBitmap);

                gdata->_tiles[i].isRendered = FALSE;
                gdata->_tiles[i].position._scrollx = 0;
                gdata->_tiles[i].position._scrolly = 0;
            }
        }
    }

    /* Reset torus state - force full redraw */
    gdata->_torusOffsetX = 0;
    gdata->_torusOffsetY = 0;
    gdata->_renderedTilesX = 0;
    gdata->_renderedTilesY = 0;
    gdata->_layoutedForWidth = Gad->Width;
    gdata->_layoutedForHeight = Gad->Height;

    return(1);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* GM_RENDER - 2D Torus Tile Management */
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/**
 * Render a single tile at physical grid position (px, py).
 * Sets its logical position to (scrollx, scrolly).
 */
static void InfiniteScroll2D_RenderSingleTile(
    InfiniteScroll2DRenderParams *renderParams,
    InfiniteScroll2D *gdata,
    int px, int py,
    long long scrollx, long long scrolly)
{
    InfiniteScroll2DTile *tile;
    int flatIndex;

    /* Wrap physical indices (no modulo, use subtraction) */
    if(px >= gdata->_tilesX) px -= gdata->_tilesX;
    if(px < 0) px += gdata->_tilesX;
    if(py >= gdata->_tilesY) py -= gdata->_tilesY;
    if(py < 0) py += gdata->_tilesY;

    flatIndex = py * gdata->_tilesX + px;
    tile = &gdata->_tiles[flatIndex];

    if(gdata->_renderFunction)
    {
        renderParams->_start._scrollx = scrollx;
        renderParams->_start._scrolly = scrolly;
        renderParams->rp = tile->bitmap._rp;
        renderParams->destX = 0;
        renderParams->destY = 0;
        renderParams->destWidth = TILE2D_SIZE;
        renderParams->destHeight = TILE2D_SIZE;
        renderParams->_itileX = px;
        renderParams->_itileY = py;
        gdata->_renderFunction(renderParams);
    }

    tile->position._scrollx = scrollx;
    tile->position._scrolly = scrolly;
    tile->isRendered = TRUE;
}

/**
 * Full redraw: render all visible tiles.
 */
static void InfiniteScroll2D_FullRedraw(
    struct Gadget *Gad,
    InfiniteScroll2D *gdata,
    InfiniteScroll2DRenderParams *renderParams)
{
    int nbTilesX = (Gad->Width / TILE2D_SIZE) + 1;
    int nbTilesY = (Gad->Height / TILE2D_SIZE) + 1;
    int x, y;
    long long startX, startY;

    /* Align to tile boundary */
    startX = (gdata->_pposition->_scrollx / TILE2D_SIZE) * TILE2D_SIZE;
    startY = (gdata->_pposition->_scrolly / TILE2D_SIZE) * TILE2D_SIZE;

    for(y = 0; y < nbTilesY; y++)
    {
        for(x = 0; x < nbTilesX; x++)
        {
            InfiniteScroll2D_RenderSingleTile(renderParams, gdata,
                x, y,
                startX + x * TILE2D_SIZE,
                startY + y * TILE2D_SIZE);
        }
    }

    gdata->_torusOffsetX = 0;
    gdata->_torusOffsetY = 0;
    gdata->_renderedTilesX = nbTilesX;
    gdata->_renderedTilesY = nbTilesY;
}

/**
 * Render tiles in a horizontal strip (for vertical scroll changes).
 * Renders from tile (startX, startY) for countX tiles horizontally.
 */
static void InfiniteScroll2D_RenderHorizontalStrip(
    InfiniteScroll2DRenderParams *renderParams,
    InfiniteScroll2D *gdata,
    int startPX, int startPY, int countX,
    long long scrollX, long long scrollY)
{
    int x;
    for(x = 0; x < countX; x++)
    {
        InfiniteScroll2D_RenderSingleTile(renderParams, gdata,
            startPX + x, startPY,
            scrollX + x * TILE2D_SIZE, scrollY);
    }
}

/**
 * Render tiles in a vertical strip (for horizontal scroll changes).
 * Renders from tile (startX, startY) for countY tiles vertically.
 */
static void InfiniteScroll2D_RenderVerticalStrip(
    InfiniteScroll2DRenderParams *renderParams,
    InfiniteScroll2D *gdata,
    int startPX, int startPY, int countY,
    long long scrollX, long long scrollY)
{
    int y;
    for(y = 0; y < countY; y++)
    {
        InfiniteScroll2D_RenderSingleTile(renderParams, gdata,
            startPX, startPY + y,
            scrollX, scrollY + y * TILE2D_SIZE);
    }
}

/**
 * Main render function with 2D torus logic.
 *
 * When scrolling, tiles that go off one edge reappear on the opposite edge.
 * The "cross" pattern: when scrolling both X and Y, we need to refresh:
 * - A horizontal strip (top or bottom) for Y movement
 * - A vertical strip (left or right) for X movement
 * - The corner tile(s) are in both strips but only rendered once
 */
ULONG InfiniteScroll2D_Render(Class *C, struct Gadget *Gad, struct gpRender *Render)
{
    InfiniteScroll2D *gdata;
    struct RastPort *rp;
    ULONG retval = 1;
    InfiniteScroll2DRenderParams renderParams;
    int x, y;

    if(Render->MethodID != GM_RENDER) return retval;

    gdata = INST_DATA(C, Gad);

    if(!gdata->_tiles) return retval;

    renderParams.Gad = Gad;

    /* Check if we need full redraw */
    if(gdata->_renderedTilesX == 0 || gdata->_renderedTilesY == 0)
    {
        InfiniteScroll2D_FullRedraw(Gad, gdata, &renderParams);
    }
    else
    {
        /* Get tile-aligned boundaries for old and new positions */
        InfiniteScroll2DTile *topLeftTile = InfiniteScroll2D_GetTile(gdata, 0, 0);
        long long oldStartX = topLeftTile->position._scrollx;
        long long oldStartY = topLeftTile->position._scrolly;
        long long oldEndX = oldStartX + gdata->_renderedTilesX * TILE2D_SIZE;
        long long oldEndY = oldStartY + gdata->_renderedTilesY * TILE2D_SIZE;

        long long newStartX = (gdata->_pposition->_scrollx / TILE2D_SIZE) * TILE2D_SIZE;
        long long newStartY = (gdata->_pposition->_scrolly / TILE2D_SIZE) * TILE2D_SIZE;
        long long newEndX = newStartX + Gad->Width + TILE2D_SIZE;
        long long newEndY = newStartY + Gad->Height + TILE2D_SIZE;

        /* Check if ranges intersect (can reuse some tiles) */
        int intersectsX = (newStartX < oldEndX) && (newEndX > oldStartX);
        int intersectsY = (newStartY < oldEndY) && (newEndY > oldStartY);

        if(!intersectsX || !intersectsY)
        {
            /* No intersection, need full redraw */
            InfiniteScroll2D_FullRedraw(Gad, gdata, &renderParams);
        }
        else
        {
            /* Partial update: handle X and Y scrolling */
            int tilesScrolledX = (int)((newStartX - oldStartX) / TILE2D_SIZE);
            int tilesScrolledY = (int)((newStartY - oldStartY) / TILE2D_SIZE);

            /* Handle X scrolling (render vertical strips) */
            if(tilesScrolledX > 0)
            {
                /* Scrolled right: render new tiles at right edge */
                int newTilesX = tilesScrolledX;
                if(newTilesX > gdata->_renderedTilesX) newTilesX = gdata->_renderedTilesX;

                for(x = 0; x < newTilesX; x++)
                {
                    int px = gdata->_torusOffsetX + x;
                    if(px >= gdata->_tilesX) px -= gdata->_tilesX;

                    InfiniteScroll2D_RenderVerticalStrip(&renderParams, gdata,
                        px, gdata->_torusOffsetY, gdata->_renderedTilesY,
                        oldEndX + x * TILE2D_SIZE, oldStartY);
                }

                /* Update torus offset */
                gdata->_torusOffsetX += newTilesX;
                if(gdata->_torusOffsetX >= gdata->_tilesX)
                    gdata->_torusOffsetX -= gdata->_tilesX;
            }
            else if(tilesScrolledX < 0)
            {
                /* Scrolled left: render new tiles at left edge */
                int newTilesX = -tilesScrolledX;
                if(newTilesX > gdata->_renderedTilesX) newTilesX = gdata->_renderedTilesX;

                for(x = 0; x < newTilesX; x++)
                {
                    int px = gdata->_torusOffsetX - 1 - x;
                    if(px < 0) px += gdata->_tilesX;

                    InfiniteScroll2D_RenderVerticalStrip(&renderParams, gdata,
                        px, gdata->_torusOffsetY, gdata->_renderedTilesY,
                        oldStartX - (x + 1) * TILE2D_SIZE, oldStartY);
                }

                /* Update torus offset */
                gdata->_torusOffsetX -= newTilesX;
                if(gdata->_torusOffsetX < 0)
                    gdata->_torusOffsetX += gdata->_tilesX;
            }

            /* Handle Y scrolling (render horizontal strips) */
            if(tilesScrolledY > 0)
            {
                /* Scrolled down: render new tiles at bottom edge */
                int newTilesY = tilesScrolledY;
                if(newTilesY > gdata->_renderedTilesY) newTilesY = gdata->_renderedTilesY;

                for(y = 0; y < newTilesY; y++)
                {
                    int py = gdata->_torusOffsetY + y;
                    if(py >= gdata->_tilesY) py -= gdata->_tilesY;

                    /* Use updated oldStartX after X scrolling adjustment */
                    long long currentStartX = oldStartX + tilesScrolledX * TILE2D_SIZE;
                    InfiniteScroll2D_RenderHorizontalStrip(&renderParams, gdata,
                        gdata->_torusOffsetX, py, gdata->_renderedTilesX,
                        currentStartX, oldEndY + y * TILE2D_SIZE);
                }

                /* Update torus offset */
                gdata->_torusOffsetY += newTilesY;
                if(gdata->_torusOffsetY >= gdata->_tilesY)
                    gdata->_torusOffsetY -= gdata->_tilesY;
            }
            else if(tilesScrolledY < 0)
            {
                /* Scrolled up: render new tiles at top edge */
                int newTilesY = -tilesScrolledY;
                if(newTilesY > gdata->_renderedTilesY) newTilesY = gdata->_renderedTilesY;

                for(y = 0; y < newTilesY; y++)
                {
                    int py = gdata->_torusOffsetY - 1 - y;
                    if(py < 0) py += gdata->_tilesY;

                    long long currentStartX = oldStartX + tilesScrolledX * TILE2D_SIZE;
                    InfiniteScroll2D_RenderHorizontalStrip(&renderParams, gdata,
                        gdata->_torusOffsetX, py, gdata->_renderedTilesX,
                        currentStartX, oldStartY - (y + 1) * TILE2D_SIZE);
                }

                /* Update torus offset */
                gdata->_torusOffsetY -= newTilesY;
                if(gdata->_torusOffsetY < 0)
                    gdata->_torusOffsetY += gdata->_tilesY;
            }
        }
    }

    /* Get RastPort */
    rp = Render->gpr_RPort;
    if(!rp) return 1;

    /* Calculate inner area (excluding margins) */
    {
        int innerLeft = Gad->LeftEdge + gdata->_marginLeft;
        int innerTop = Gad->TopEdge + gdata->_marginTop;
        int innerWidth = Gad->Width - gdata->_marginLeft - gdata->_marginRight;
        int innerHeight = Gad->Height - gdata->_marginTop - gdata->_marginBottom;

        /* Blit tiles only to inner area */
        if(innerWidth > 0 && innerHeight > 0)
        {
            for(y = 0; y < gdata->_renderedTilesY; y++)
            {
                for(x = 0; x < gdata->_renderedTilesX; x++)
                {
                    InfiniteScroll2DTile *tile = InfiniteScroll2D_GetTile(gdata, x, y);
                    int dx, dy;
                    int srcX, srcY, width, height;

                    if(!tile->isRendered) continue;

                    /* Calculate position relative to inner area */
                    dx = (int)(tile->position._scrollx - gdata->_pposition->_scrollx);
                    dy = (int)(tile->position._scrolly - gdata->_pposition->_scrolly);

                    /* Skip if completely outside inner bounds */
                    if(dx + TILE2D_SIZE <= 0 || dx >= innerWidth) continue;
                    if(dy + TILE2D_SIZE <= 0 || dy >= innerHeight) continue;

                    /* Clip to inner bounds */
                    srcX = 0;
                    srcY = 0;
                    width = TILE2D_SIZE;
                    height = TILE2D_SIZE;

                    if(dx < 0)
                    {
                        width += dx;
                        srcX = -dx;
                        dx = 0;
                    }
                    if(dy < 0)
                    {
                        height += dy;
                        srcY = -dy;
                        dy = 0;
                    }
                    if(dx + width > innerWidth)
                    {
                        width = innerWidth - dx;
                    }
                    if(dy + height > innerHeight)
                    {
                        height = innerHeight - dy;
                    }

                    if(width > 0 && height > 0)
                    {
                        BltBitMapRastPort(tile->bitmap._bm,
                                          srcX, srcY,
                                          rp,
                                          innerLeft + dx, innerTop + dy,
                                          width, height,
                                          0xC0);  /* minterm: straight copy */
                    }
                }
            }
        }

        /* Draw margins with margin pen */
        SetAPen(rp, gdata->_marginPen);

        /* Left margin */
        if(gdata->_marginLeft > 0)
        {
            RectFill(rp,
                     Gad->LeftEdge,
                     Gad->TopEdge,
                     Gad->LeftEdge + gdata->_marginLeft - 1,
                     Gad->TopEdge + Gad->Height - 1);
        }

        /* Right margin */
        if(gdata->_marginRight > 0)
        {
            RectFill(rp,
                     Gad->LeftEdge + Gad->Width - gdata->_marginRight,
                     Gad->TopEdge,
                     Gad->LeftEdge + Gad->Width - 1,
                     Gad->TopEdge + Gad->Height - 1);
        }

        /* Top margin (between left and right margins) */
        if(gdata->_marginTop > 0)
        {
            RectFill(rp,
                     Gad->LeftEdge + gdata->_marginLeft,
                     Gad->TopEdge,
                     Gad->LeftEdge + Gad->Width - gdata->_marginRight - 1,
                     Gad->TopEdge + gdata->_marginTop - 1);
        }

        /* Bottom margin (between left and right margins) */
        if(gdata->_marginBottom > 0)
        {
            RectFill(rp,
                     Gad->LeftEdge + gdata->_marginLeft,
                     Gad->TopEdge + Gad->Height - gdata->_marginBottom,
                     Gad->LeftEdge + Gad->Width - gdata->_marginRight - 1,
                     Gad->TopEdge + Gad->Height - 1);
        }
    }

    return(retval);
}

