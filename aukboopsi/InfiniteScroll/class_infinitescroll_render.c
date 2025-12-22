
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
     if(gdata)
     {
       D->gpd_Domain.Width =gdata->_minimalWidth;
       D->gpd_Domain.Height=gdata->_minimalHeight;
     }
     else
      {
        D->gpd_Domain.Width=  128;
        D->gpd_Domain.Height= 32;
      }
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
}

/**
 * Invalidate tiles in a given range (or all if range is 0,0 to 0,0)
 */
void InfiniteScroll_InvalidateTiles(Class *C, struct Gadget *Gad, struct gpInvalidateTiles *Msg)
{
    InfiniteScroll *gdata;
    ULONG i;
    BOOL invalidateAll;

    if(!C || !Gad) return;
    gdata = INST_DATA(C, Gad);
    if(!gdata || !gdata->_tiles) return;

    /* Check if we should invalidate all tiles */
    invalidateAll = (Msg->RangeMinHi == 0 && Msg->RangeMinLo == 0 &&
                     Msg->RangeMaxHi == 0 && Msg->RangeMaxLo == 0);

    /* Mark tiles as dirty */
    for(i = 0; i < gdata->_tileCount; i++)
    {
        if(gdata->_tiles[i].isValid)
        {
            if(invalidateAll)
            {
                gdata->_tiles[i].isDirty = TRUE;
            }
            else
            {
                /* TODO: Check if tile's abstract position overlaps with range */
                /* For now, just invalidate all */
                gdata->_tiles[i].isDirty = TRUE;
            }
        }
    }
}

/**
 * Refresh dirty tiles by calling child class render method
 */
void InfiniteScroll_RefreshTiles(Class *C, struct Gadget *Gad)
{
    InfiniteScroll *gdata;
    ULONG i;

    if(!C || !Gad) return;
    gdata = INST_DATA(C, Gad);
    if(!gdata || !gdata->_tiles) return;

    /* Render each dirty tile */
    for(i = 0; i < gdata->_tileCount; i++)
    {
        if(gdata->_tiles[i].isValid && gdata->_tiles[i].isDirty)
        {
            InfiniteScroll_RenderTile(C, Gad, &gdata->_tiles[i], i);
            gdata->_tiles[i].isDirty = FALSE;
        }
    }
}

/**
 * Render a single tile - to be overridden by child classes
 * Default implementation clears the tile to background color
 */
void InfiniteScroll_RenderTile(Class *C, struct Gadget *Gad, InfiniteScrollTile *tile, ULONG tileIndex)
{
    struct RastPort *rp;

    if(!tile || !tile->isValid || !tile->bitmap._rp) return;

    rp = tile->bitmap._rp;

    /* Default: Clear to background color (pen 0) */
    SetAPen(rp, 0);
    SetBPen(rp, 0);
    RectFill(rp, 0, 0, tile->bitmap._bm->BytesPerRow * 8 - 1,
             tile->bitmap._bm->Rows - 1);

    /* Child classes should override this method to render actual content */
}

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

    gdata->_framerec.MinX = leftedge;
    gdata->_framerec.MinY = topedge;
    gdata->_framerec.MaxX = leftedge + width  -1;
    gdata->_framerec.MaxY = topedge  + height -1;

    /* Calculate needed tile count: (width / tileWidth) + 2 */
    /* +2 for smooth scrolling (one tile on each side can be pre-rendered) */
    if(gdata->_tileWidth > 0)
    {
        neededTileCount = (width / gdata->_tileWidth) + 2;
    }
    else
    {
        neededTileCount = 0;
    }

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
                gdata->_friendBitmap = &layout->gpl_GInfo->gi_Screen->BitMap;
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

                if(gdata->_tiles[i].bitmap._bm)
                {
                    gdata->_tiles[i].isValid = TRUE;
                    gdata->_tiles[i].isDirty = TRUE;
                    gdata->_tiles[i].abstractPosHi = 0;
                    gdata->_tiles[i].abstractPosLo = i * gdata->_tileWidth;
                }
                else
                {
                    /* Allocation failed */
                    gdata->_tiles[i].isValid = FALSE;
                }
            }
        }
    }

  return(1);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* GM_RENDER */
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/**
 * Draw yourself, in the appropriate state
 * Refresh dirty tiles and blit visible portions to screen
 */
ULONG InfiniteScroll_Render(Class *C, struct Gadget *Gad, struct gpRender *Render, ULONG update)
{
  InfiniteScroll *gdata;
  struct RastPort *rp;
  ULONG retval=1;
  ULONG i;
  LONG tileScreenX;

  gdata=INST_DATA(C, Gad);

  /* Get RastPort */
  if(Render->MethodID==GM_RENDER)
  {
    rp=Render->gpr_RPort;
    update=Render->gpr_Redraw;
  }
  else
  {
    rp = ObtainGIRPort(Render->gpr_GInfo);
  }

  if(rp && gdata->_tiles)
  {
    /* First, refresh any dirty tiles */
    InfiniteScroll_RefreshTiles(C, Gad);

    /* Now blit tiles to screen */
    for(i = 0; i < gdata->_tileCount; i++)
    {
        if(!gdata->_tiles[i].isValid) continue;

        /* Calculate screen X position for this tile */
        /* TODO: This is simplified - needs proper abstract->screen conversion */
        tileScreenX = gdata->_framerec.MinX + (i * gdata->_tileWidth);

        /* Skip tiles that are completely off-screen to the left */
        if(tileScreenX + gdata->_tileWidth < gdata->_framerec.MinX)
            continue;

        /* Skip tiles that are completely off-screen to the right */
        if(tileScreenX > gdata->_framerec.MaxX)
            break;

        /* Blit this tile to the screen */
        BltBitMapRastPort(gdata->_tiles[i].bitmap._bm,
                          0, 0,  /* source x, y */
                          rp,
                          tileScreenX, gdata->_framerec.MinY,  /* dest x, y */
                          gdata->_tileWidth, gdata->_tileHeight,  /* width, height */
                          0xC0);  /* minterm: straight copy */
    }
  }

  if(Render->MethodID != GM_RENDER)
  {
      if(rp) ReleaseGIRPort(rp);
  }

  return(retval);
}
