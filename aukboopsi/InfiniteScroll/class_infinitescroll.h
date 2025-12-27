#ifndef GADGETS_INFINITESCROLL_H
#define GADGETS_INFINITESCROLL_H
/**
 * Definitions for Abstract Gadget Class InfiniteScroll
 *
 * This is an abstract base class that manages horizontal scrolling with
 * bitmap tile caching. It uses signed 64-bit values for scroll domain
 * and zoom/position, and caches rendered content in offscreen bitmap tiles.
 *
 * TimeRule and TrackGadget will inherit from this class.
 */
#include <exec/types.h>
#include <intuition/gadgetclass.h>
#include <intuition/classes.h>
#include <inline/macros.h>

#define VERSION_INFINITESCROLL 1
#define InfiniteScroll_SUPERCLASS_ID "gadgetclass"

#ifdef INFINITESCROLL_STATICLINK
    extern int InfiniteScrollStaticInit(void);
    extern void InfiniteScrollStaticClose(void);
    extern Class *INFINITESCROLL_GetClass(void);
#else
    /* InfiniteScroll_CLASS_ID is the identifier for this class, when shared. */
    /* NewObject() can use either INFINITESCROLL_GetClass() or InfiniteScroll_CLASS_ID. */
    #define InfiniteScroll_CLASS_ID "infinitescroll.gadget"

    /* the following is to define function with implicit library call for INFINITESCROLL_GetClass(). */
    /* note it should be in includes generated from a fd files. */
    Class * __stdargs INFINITESCROLL_GetClass( void );

    #ifndef _NO_INLINE
        # if defined(__GNUC__)
            #include <inline/macros.h>
            #define INFINITESCROLL_GetClass() LP0(0x1e, Class *, INFINITESCROLL_GetClass ,, InfiniteScrollBase)
        # endif
        #if defined(LATTICE) || defined(__SASC) || defined(_DCC)
           #pragma libcall InfiniteScrollBase INFINITESCROLL_GetClass 1e 00
        # endif
        #if defined(__VBCC__)
            Class * __INFINITESCROLL_GetClass(__reg("a6") void *)="\tjsr\t-$1e(a6)";
            #define INFINITESCROLL_GetClass() __INFINITESCROLL_GetClass(InfiniteScrollBase)
        #endif
    #endif /* _NO_INLINE */

    #ifndef __NOLIBBASE__
      extern struct Library *
        # ifdef __CONSTLIBBASEDECL__
       __CONSTLIBBASEDECL__
        # endif /* __CONSTLIBBASEDECL__ */
      InfiniteScrollBase;
    #endif /* !__NOLIBBASE__ */

/* end if dynamic link */
#endif

/**  Attributes defined by the gadget class,
 * all attribs from gadgetclass.h are also valid.
 */
/* different classes may not use same base. */
#define INFINITESCROLL_Dummy			(TAG_USER+0x04130000)

/* Scrollable domain min value (signed 64-bit, stored as two LONG) */
#define	INFINITESCROLL_DomainMinHi		(INFINITESCROLL_Dummy+1)
#define	INFINITESCROLL_DomainMinLo		(INFINITESCROLL_Dummy+2)

/* Scrollable domain max value (signed 64-bit, stored as two LONG) */
#define	INFINITESCROLL_DomainMaxHi		(INFINITESCROLL_Dummy+3)
#define	INFINITESCROLL_DomainMaxLo		(INFINITESCROLL_Dummy+4)

/* Current view position (signed 64-bit, stored as two LONG) */
#define	INFINITESCROLL_ViewPosHi		(INFINITESCROLL_Dummy+5)
#define	INFINITESCROLL_ViewPosLo		(INFINITESCROLL_Dummy+6)

/* Current view zoom (signed 64-bit, stored as two LONG) */
/* Higher value = more zoomed in, lower value = more zoomed out */
#define	INFINITESCROLL_ViewZoomHi		(INFINITESCROLL_Dummy+7)
#define	INFINITESCROLL_ViewZoomLo		(INFINITESCROLL_Dummy+8)

/* Tile width in pixels (default 128) */
#define	INFINITESCROLL_TileWidth		(INFINITESCROLL_Dummy+9)

/* Methods */
#define INFINITESCROLL_GMDummy			(INFINITESCROLL_Dummy+0x100)

/* GM_INFINITESCROLL_INVALIDATETILES - Mark tiles dirty to force re-render */
#define GM_INFINITESCROLL_INVALIDATETILES (INFINITESCROLL_GMDummy+1)

/* GM_INFINITESCROLL_RENDERTILE - Render a single tile (override in subclass) */
#define GM_INFINITESCROLL_RENDERTILE (INFINITESCROLL_GMDummy+2)

/* Message structure for GM_INFINITESCROLL_INVALIDATETILES */
struct gpInvalidateTiles
{
    ULONG MethodID;  /* GM_INFINITESCROLL_INVALIDATETILES */
    /* If both Hi/Lo are 0, invalidate all tiles */
    LONG RangeMinHi; /* Start of range to invalidate (abstract 64-bit) */
    LONG RangeMinLo;
    LONG RangeMaxHi; /* End of range to invalidate (abstract 64-bit) */
    LONG RangeMaxLo;
};

/* Message structure for GM_INFINITESCROLL_RENDERTILE */
struct gpRenderTile
{
    ULONG MethodID;            /* GM_INFINITESCROLL_RENDERTILE */
    struct RastPort *RPort;    /* RastPort for tile bitmap (draw at 0,0) */
    UWORD TileWidth;           /* Width of tile in pixels */
    UWORD TileHeight;          /* Height of tile in pixels */
    LONG AbstractPosHi;        /* Abstract position of tile left edge (64-bit) */
    LONG AbstractPosLo;
    ULONG TileIndex;           /* Index of this tile in the array */
};

#endif
