#ifndef GADGETS_INFINITESCROLL2D_H
#define GADGETS_INFINITESCROLL2D_H
/**
 * Definitions for Abstract Gadget Class InfiniteScroll2D
 *
 * This is an abstract base class that manages 2D scrolling (horizontal and vertical)
 * with bitmap tile caching. It uses signed 64-bit values for scroll domain
 * and zoom/position, and caches rendered content in offscreen bitmap tiles.
 *
 * Tiles are 128x128 pixels arranged in a 2D torus (circular in both X and Y).
 * No 64-bit modulo operations - uses subtraction for index wrapping.
 */
#include <exec/types.h>
#include <intuition/gadgetclass.h>
#include <intuition/classes.h>
#include <inline/macros.h>
#include "compilers.h"

#define VERSION_INFINITESCROLL2D 1
#define InfiniteScroll2D_SUPERCLASS_ID "gadgetclass"

#ifdef INFINITESCROLL2D_STATICLINK
    extern int InfiniteScroll2DStaticInit(void);
    extern void InfiniteScroll2DStaticClose(void);
    extern Class *INFINITESCROLL2D_GetClass(void);
#else
    /* InfiniteScroll2D_CLASS_ID is the identifier for this class, when shared. */
    /* NewObject() can use either INFINITESCROLL2D_GetClass() or InfiniteScroll2D_CLASS_ID. */
    #define InfiniteScroll2D_CLASS_ID "infinitescroll2d.gadget"

    /* the following is to define function with implicit library call for INFINITESCROLL2D_GetClass(). */
    /* note it should be in includes generated from a fd files. */
    Class * __stdargs INFINITESCROLL2D_GetClass( void );

    #ifndef _NO_INLINE
        # if defined(__GNUC__)
            #include <inline/macros.h>
            #define INFINITESCROLL2D_GetClass() LP0(0x1e, Class *, INFINITESCROLL2D_GetClass ,, InfiniteScroll2DBase)
        # endif
        #if defined(LATTICE) || defined(__SASC) || defined(_DCC)
           #pragma libcall InfiniteScroll2DBase INFINITESCROLL2D_GetClass 1e 00
        # endif
        #if defined(__VBCC__)
            Class * __INFINITESCROLL2D_GetClass(__reg("a6") void *)="\tjsr\t-$1e(a6)";
            #define INFINITESCROLL2D_GetClass() __INFINITESCROLL2D_GetClass(InfiniteScroll2DBase)
        #endif
    #endif /* _NO_INLINE */

    #ifndef __NOLIBBASE__
      extern struct Library *
        # ifdef __CONSTLIBBASEDECL__
       __CONSTLIBBASEDECL__
        # endif /* __CONSTLIBBASEDECL__ */
      InfiniteScroll2DBase;
    #endif /* !__NOLIBBASE__ */

/* end if dynamic link */
#endif


/**
 * 2D scroll position structure with both X and Y components.
 * Both are signed 64-bit to support large virtual scroll spaces.
 */
typedef struct InfiniteScroll2DPosition {
    long long _scrollx;
    long long _scrolly;
} InfiniteScroll2DPosition;

INLINE int InfiniteScroll2DPosition_isSame(InfiniteScroll2DPosition *a, InfiniteScroll2DPosition *b)
{
    return (int)(a->_scrollx == b->_scrollx && a->_scrolly == b->_scrolly);
}

typedef struct Scroll2DDomain {
    /* start, included  */
    InfiniteScroll2DPosition _start;
    /* note end is excluded, last valid pixel is just behind.  */
    InfiniteScroll2DPosition _end;
} Scroll2DDomain;

/** Params for InfiniteScroll2D render function */
typedef struct InfiniteScroll2DRenderParams {
    struct Gadget *Gad;
    struct RastPort *rp;
    /* start position of scroll to render, for that tile */
    InfiniteScroll2DPosition _start;
    /* rectangle to render in Rastport (always 128x128 for tiles) */
    WORD destX, destY, destWidth, destHeight;
    /* debug purpose: tile indices */
    int _itileX, _itileY;
} InfiniteScroll2DRenderParams;

/**
 * Function pointer type to ask rendering a portion of 2D space.
 * This is to be passed by inherited class.
 */
typedef void (*InfiniteScroll2DRenderf)(InfiniteScroll2DRenderParams *p);


/**  Attributes defined by the gadget class,
 * all attribs from gadgetclass.h are also valid.
 */
/* different classes may not use same base. */
#define INFINITESCROLL2D_Dummy          (TAG_USER+0x03220000)

/* Scrollable position as pointer to InfiniteScroll2DPosition
    Apply to OM_NEW OM_SET OM_GET.
*/
#define INFINITESCROLL2D_Position       (INFINITESCROLL2D_Dummy+1)

/**
    defer position variable to external source via pointer.
    default point to own Position. If PPosition is set elsewhere, internal position becomes obsolete.
*/
#define INFINITESCROLL2D_PPosition      (INFINITESCROLL2D_Dummy+2)

/* This abstract class need a Render function to actually draw,
  of type InfiniteScroll2DRenderf.
  Implementer will use InfiniteScroll2DRenderParams._start
  and destX,destY,destWidth,destHeight to know where to render in Rastport.

    Apply to OM_NEW OM_SET OM_GET.
*/
#define INFINITESCROLL2D_RenderFunction (INFINITESCROLL2D_Dummy+3)

/*
 Tell next rendering can't use the tile cache, redraw all.
*/
#define INFINITESCROLL2D_FullTilesRefresh   (INFINITESCROLL2D_Dummy+4)

/* external layout to render us, as notify message. */
#define INFINITESCROLL2D_Redraw         (INFINITESCROLL2D_Dummy+5)

/* Margin sizes in pixels for each border (UWORD).
   Tiles are rendered minus the margin rectangle.
   Default is 0 for all margins.
*/
#define INFINITESCROLL2D_MarginLeft     (INFINITESCROLL2D_Dummy+6)
#define INFINITESCROLL2D_MarginRight    (INFINITESCROLL2D_Dummy+7)
#define INFINITESCROLL2D_MarginTop      (INFINITESCROLL2D_Dummy+8)
#define INFINITESCROLL2D_MarginBottom   (INFINITESCROLL2D_Dummy+9)

/* Pen index used to draw margin areas (WORD).
   Default is 1.
*/
#define INFINITESCROLL2D_MarginPen      (INFINITESCROLL2D_Dummy+10)

/* Methods */
#define INFINITESCROLL2D_GMDummy        (INFINITESCROLL2D_Dummy+0x100)


#endif
