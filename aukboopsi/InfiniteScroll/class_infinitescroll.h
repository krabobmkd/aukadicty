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
#include "compilers.h"
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


/**
 * NOW, we have a struct to define bitmap scroll position allocated elsewhere,
 * and an attrib as a pointer to it, to define projection.
 * We don't manage a domain at this level, just tile cache and scrolling.
*/
typedef struct InfiniteScrollPosition {

    long long _scrollx;
} InfiniteScrollPosition;

INLINE int InfiniteScrollPosition_isSame(InfiniteScrollPosition *a, InfiniteScrollPosition *b)
{
    return (int)(a->_scrollx == b->_scrollx);
}

typedef struct ScrollDomain {
    /* start, included  */
    InfiniteScrollPosition _start;
    /* note end is excluded, last valid pixel is just behind.  */
    InfiniteScrollPosition _end;
} ScrollDomain;

/** NOW, params for InfiniteScrollRender function */
typedef struct InfiniteScrollRenderParams {
    Class *C;
    struct Gadget *Gad;
    struct RastPort *rp;   
    /* start position of scroll to render, for that tile */
    InfiniteScrollPosition _start;
    /* rectangle to render in Rastport */
    WORD destX,destY,destWidth,destHeight;

} InfiniteScrollRenderParams;

/**
 * NOW, we have  function pointer type to ask rendering a portion of space.
 * This is to be passed by inherited class.
*/
typedef void (*InfiniteScrollRenderf)(InfiniteScrollRenderParams *p);


/**  Attributes defined by the gadget class,
 * all attribs from gadgetclass.h are also valid.
 */
/* different classes may not use same base. */
#define INFINITESCROLL_Dummy			(TAG_USER+0x03110000)

/* Scrollable domain min value as pointer to a signed 64-bit (InfiniteScrollPosition *)
    Apply to OM_NEW OM_SET OM_GET.
*/
#define	INFINITESCROLL_Position		(INFINITESCROLL_Dummy+1)

/* This abstract class need a Render function to actually draw,
  of type InfiniteScrollRenderf.
  Implementer will use InfiniteScrollRenderParams._start
  and destX,destY,destWidth,destHeight to know where to render in Rastport.

    Apply to OM_NEW OM_SET OM_GET.
*/
#define	INFINITESCROLL_RenderFunction		(INFINITESCROLL_Dummy+2)

/* external layout to render us, as notify message. */
#define	INFINITESCROLL_Redraw		(INFINITESCROLL_Dummy+3)

/* Methods */
#define INFINITESCROLL_GMDummy			(INFINITESCROLL_Dummy+0x100)


#endif
