
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/layers.h>

#ifdef __SASC
    #include <clib/alib_protos.h>
#else
    /* GCC */
    #include "../minialib.h"
#endif

#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/gadgetclass.h>
#include <utility/tagitem.h>

#include "class_infinitescroll.h"
#include "class_infinitescroll_private.h"

#include "../bdbprintf.h"

extern struct IClass *InfiniteScrollClassPtr;

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* GM_DOMAIN */
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

ULONG InfiniteScroll_Domain(Class *C, struct Gadget *Gad, struct gpDomain *D)
{
  InfiniteScroll *gdata=0;

  if(Gad) gdata=INST_DATA(C, Gad);

  D->gpd_Domain.Left=0;
  D->gpd_Domain.Top=0;

  switch(D->gpd_Which)
  {
    case GDOMAIN_NOMINAL:
      D->gpd_Domain.Width=256;
      D->gpd_Domain.Height=64;
      break;

    case GDOMAIN_MAXIMUM:
      D->gpd_Domain.Width=16000;
      D->gpd_Domain.Height=16000;
      break;

    case GDOMAIN_MINIMUM:
    default:
        D->gpd_Domain.Width=  16;
        D->gpd_Domain.Height= 16;
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
void InfiniteScroll_DisposeTiles(InfiniteScroll *gdata)
{
    ULONG i;
    if(!gdata) return;

    if(gdata->_tiles)
    {
        for(i = 0; i < gdata->_tileCount; i++)
        {
            if(gdata->_tiles[i].isValid)
            {
                OffscreenBitMap_Close(&gdata->_tiles[i].bitmap);
                gdata->_tiles[i].isValid = FALSE;
            }
        }
        FreeVec(gdata->_tiles);
        gdata->_tiles = NULL;
    }

    gdata->_tileCount = 0;
    gdata->_initedForWidth = 0;
    gdata->_initedForHeight = 0;
}

/**
 * Invalidate tiles in a given range (or all if range is 0,0 to 0,0)
 */
// void InfiniteScroll_InvalidateTiles(Class *C, struct Gadget *Gad, struct gpInvalidateTiles *Msg)
// {
//     InfiniteScroll *gdata;
//     ULONG i;
//     BOOL invalidateAll;

//     if(!C || !Gad) return;
//     gdata = INST_DATA(C, Gad);
//     if(!gdata || !gdata->_tiles) return;

//     /* Check if we should invalidate all tiles */
//     invalidateAll = (Msg->RangeMinHi == 0 && Msg->RangeMinLo == 0 &&
//                      Msg->RangeMaxHi == 0 && Msg->RangeMaxLo == 0);

//     /* Mark tiles as dirty */
//     for(i = 0; i < gdata->_tileCount; i++)
//     {
//         if(gdata->_tiles[i].isValid)
//         {
//             if(invalidateAll)
//             {
//                 gdata->_tiles[i].isDirty = TRUE;
//             }
//             else
//             {
//                 /* TODO: Check if tile's abstract position overlaps with range */
//                 /* For now, just invalidate all */
//                 gdata->_tiles[i].isDirty = TRUE;
//             }
//         }
//     }
// }

/**
 * Refresh dirty tiles by calling child class render method
 */
// void InfiniteScroll_RefreshTiles(Class *C, struct Gadget *Gad)
// {
//     InfiniteScroll *gdata;
//     ULONG i;

//     if(!C || !Gad) return;
//     gdata = INST_DATA(C, Gad);
//     if(!gdata || !gdata->_tiles) return;

//     /* Render each dirty tile */
//     for(i = 0; i < gdata->_tileCount; i++)
//     {
//         if(gdata->_tiles[i].isValid && gdata->_tiles[i].isDirty)
//         {
//             InfiniteScroll_RenderTile(C, Gad, &gdata->_tiles[i], i);
//             gdata->_tiles[i].isDirty = FALSE;
//         }
//     }
// }

/**
 * Render a single tile - sends GM_INFINITESCROLL_RENDERTILE to allow subclass override
 * Default implementation (handled in dispatcher) clears the tile to background color
 */
//void InfiniteScroll_RenderTile(Class *C, struct Gadget *Gad, InfiniteScrollTile *tile, ULONG tileIndex)
//{
    // InfiniteScroll *gdata;
    // struct gpRenderTile msg;

    // if(!tile || !tile->isValid || !tile->bitmap._rp) return;

    // gdata = INST_DATA(C, Gad);

    // /* Build message for subclass to handle */
    // msg.MethodID = GM_INFINITESCROLL_RENDERTILE;
    // msg.RPort = tile->bitmap._rp;
    // msg.TileWidth = gdata->_tileWidth;
    // msg.TileHeight = gdata->_tileHeight;
    // msg.AbstractPosHi = tile->abstractPosHi;
    // msg.AbstractPosLo = tile->abstractPosLo;
    // msg.TileIndex = tileIndex;

    // /* Send to object - subclass dispatcher can handle or pass to super */
    // DoMethodA((Object *)Gad, (Msg)&msg);
//}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* GM_LAYOUT */
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/**
 * method GM_LAYOUT
 * The gadget knows its final coordinates and size.
 * Allocate tiles based on width.
 */
ULONG InfiniteScroll_Layout(Class *C, struct Gadget *Gad, struct gpLayout *layout)
{
  InfiniteScroll *gdata;
  LONG topedge, leftedge, width, height;
  ULONG neededTileCount;
  ULONG i;

    gdata=INST_DATA(C, Gad);

    topedge = Gad->TopEdge;
    leftedge = Gad->LeftEdge;
    width = Gad->Width;
    height = Gad->Height;

    if(gdata->_initedForWidth == width &&
        gdata->_initedForHeight == height
        )
        {
            // layout already inited with right size, don't change.
            return 1;
        }

    /* Calculate needed tile count: (width / tileWidth) + 2 */
    /* +2 for smooth scrolling (one tile on each side can be pre-rendered) */
    if(gdata->_tileWidth == 0) gdata->_tileWidth=128;

    neededTileCount = (width / gdata->_tileWidth) + 2;
    // let's round that to allow less realloc when resizing...
    // neededTileCount = (neededTileCount+3) & (~3UL);

    /* Check if we need to reallocate tiles */
    if(neededTileCount != gdata->_tileCount || gdata->_tileHeight != height)
    {
        /* Dispose old tiles */
        InfiniteScroll_DisposeTiles(gdata);

        /* Update tile height */
        gdata->_tileHeight = height;

        /* Allocate new tile array */
        if(neededTileCount > 0)
        {
            gdata->_tiles = (InfiniteScrollTile*)AllocVec(
                neededTileCount * sizeof(InfiniteScrollTile),
                MEMF_CLEAR);

            if(!gdata->_tiles)
            {
                gdata->_tileCount = 0;
                return 1; /* Failed but don't crash */
            }

            gdata->_tileCount = neededTileCount;

            /* Get friend bitmap from screen for correct display mode */
            if(layout->gpl_GInfo && layout->gpl_GInfo->gi_Screen)
            {
                gdata->_friendBitmap = &layout->gpl_GInfo->gi_Screen->RastPort.BitMap;
            }

            /* Initialize each tile */
            for(i = 0; i < gdata->_tileCount; i++)
            {
                /* Allocate offscreen bitmap for this tile */
                OffscreenBitMap_Init(&gdata->_tiles[i].bitmap,
                                     gdata->_tileWidth,
                                     gdata->_tileHeight,
                                     0, /* depth from friend */
                                     BMF_CLEAR,
                                     gdata->_friendBitmap);

                if(gdata->_tiles[i].bitmap._rp)
                {
                    gdata->_tiles[i].isValid = FALSE; // because not rendered
                    gdata->_tiles[i].position._scrollx = 0;
                }
                else
                {
                    /* Allocation failed */
                    gdata->_tiles[i].isValid = FALSE;
                }
                gdata->_currentLeftBorderTileIndex = -1; // means, no tile affected yet.
            }
        }
    }

  return(1);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* GM_RENDER */
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/**
 * Reattribute and render a serie of contiguous tiles
 */
static void InfiniteScroll_RenderTilesRow(
        InfiniteScrollRenderParams *renderParams,
        InfiniteScroll *gdata,int itileStart,int nbTiles,
        InfiniteScrollPosition *startpos
     )
{
    int i=0;
    renderParams->destX=0;
    renderParams->destY=0;
    renderParams->destWidth=gdata->_tileWidth;
    renderParams->destY=gdata->_tileHeight;

    InfiniteScrollPosition pos = *startpos;
    for(i=0;i<nbTiles;i++)
    {
        int iTile = i+itileStart;
        if(iTile>=gdata->_tileCount) iTile -=gdata->_tileCount;
        InfiniteScrollTile *tile = &gdata->_tiles[iTile];

        if(gdata->_renderFunction)
        {
            renderParams->_start = pos;
            renderParams->rp = tile->bitmap._rp;
            gdata->_renderFunction(renderParams);

        }
        tile->position = pos;
        tile->isValid = TRUE;

        pos._scrollx += gdata->_tileWidth;
        itileStart++;
    }
}
static void InfiniteScroll_FullRedraw(
        struct Gadget *Gad,
        InfiniteScroll *gdata,
        InfiniteScrollRenderParams *renderParams
     )
{
    // need full redraw...
    int nbTilesX = (Gad->Width / gdata->_tileWidth)+1;
    InfiniteScroll_RenderTilesRow(renderParams, gdata,0,nbTilesX,&gdata->_position);
    gdata->_currentLeftBorderTileIndex = 0;
}
/**
 * Draw yourself, in the appropriate state
 * Refresh dirty tiles and blit visible portions to screen
 */
ULONG InfiniteScroll_Render(Class *C, struct Gadget *Gad, struct gpRender *Render)
{
    InfiniteScroll *gdata;
    struct RastPort *rp;
    ULONG retval=1;
    LONG i;

    InfiniteScrollRenderParams renderParams;
    // We render only under GM_RENDER.
    if(!Render->MethodID==GM_RENDER) return;

    gdata=INST_DATA(C, Gad);

    if( !gdata->_tiles) return;
    // common params for tile rendering
    renderParams.C = C;
    renderParams.Gad = Gad;


    if(gdata->_currentLeftBorderTileIndex<0)
    {
        InfiniteScroll_FullRedraw(Gad,gdata,&renderParams);

    } else
    {
        // get [prevStart,prevEnd] span already rendered in previous draw:
        InfiniteScrollTile *prevTile=&gdata->_tiles[gdata->_currentLeftBorderTileIndex];
        InfiniteScrollPosition prevStart = prevTile->position;
        InfiniteScrollPosition prevEnd =prevStart;
        prevEnd._scrollx += gdata->_tileWidth;

        for(i =1; i < gdata->_tileCount; i++)
        {
            InfiniteScrollTile *tile;
            int j = i+ gdata->_currentLeftBorderTileIndex;
            if(j>=gdata->_tileCount) j-=gdata->_tileCount;
            tile = &gdata->_tiles[j];
            if(!tile->isValid) break; // not rendered
            if(prevEnd._scrollx != tile->position._scrollx) break; // not continuous.
            prevEnd._scrollx += gdata->_tileWidth;
        }

        /* Check intersection between [prevStart, prevEnd] and [_position, endVisiblePosition] */
        {
            long long newStart = gdata->_position._scrollx;
            long long newEnd = newStart + Gad->Width;
            long long oldStart = prevStart._scrollx;
            long long oldEnd = prevEnd._scrollx;

            /* Ranges intersect if:  */
            if( newStart >= oldStart && newStart < oldEnd )
            {
                int firstInvalidTileIndex;
                InfiniteScrollTile *tile = &gdata->_tiles[gdata->_currentLeftBorderTileIndex];
               while(newStart < (tile->position._scrollx + gdata->_tileWidth))
               {
                    gdata->_currentLeftBorderTileIndex++;
                    if(gdata->_currentLeftBorderTileIndex == gdata->_tileCount) gdata->_currentLeftBorderTileIndex=0;
                    tile = &gdata->_tiles[gdata->_currentLeftBorderTileIndex];
               }
               firstInvalidTileIndex = gdata->_currentLeftBorderTileIndex+1;
                if(firstInvalidTileIndex == gdata->_tileCount) firstInvalidTileIndex=0;
                tile = &gdata->_tiles[firstInvalidTileIndex];
               while(tile->isValid && tile->position._scrollx<newEnd )
               {
                    firstInvalidTileIndex++;
                    if(firstInvalidTileIndex == gdata->_tileCount) firstInvalidTileIndex=0;
                    tile = &gdata->_tiles[firstInvalidTileIndex];
               }



            } else  /* Ranges intersect if:  */
            if((newEnd-1)<oldEnd && (newEnd-1)>=oldStart)
            {
                /* Scroll
                 can reuse tiles by shifting, then need re-using tiles at right.*/
                // can reuse tiles, and add some at right

            } else
            {   // need full redraw
                InfiniteScroll_FullRedraw(Gad,gdata,&renderParams);
            }

    } // end if test for tile updates


    /* Get RastPort */
    rp=Render->gpr_RPort;

    if(!rp) return;

    /* Now blit tiles to gadget  */
    for(i =0; i < gdata->_tileCount; i++)
    {
        InfiniteScrollTile *tile;
        int j = i+ gdata->_currentLeftBorderTileIndex;
        int dx;
        if(j>=gdata->_tileCount) j-=gdata->_tileCount;

        tile = &gdata->_tiles[j];
        if(!tile->isValid) continue;
        dx = (int)(tile->position._scrollx - gdata->_position._scrollx);
        if(dx+gdata->_tileWidth <=0 || dx>Gad->Width) continue;

        /* we have to do a bit of clipping ourselves */
        {
            int width = gdata->_tileWidth;
            int sourcex = 0;
            if(dx<0) {
                width += dx;
                sourcex -= dx;
                dx=0;
            }
            if(dx+width>Gad->Width)
            {
                width = Gad->Width-dx;
            }
            BltBitMapRastPort(tile->bitmap._bm,
                              sourcex, 0,  /* source x, y */
                              rp,
                              Gad->LeftEdge+ dx, Gad->TopEdge,  /* dest x, y */
                              width, gdata->_tileHeight,  /* width, height */
                              0xC0);  /* minterm: straight copy */
        }
    }


  return(retval);
}
