#ifndef GADGETS_TRACKAREA_H
#define GADGETS_TRACKAREA_H
/**
 * Definitions for Gadget TrackArea
 * (This is the public file that can be released when publishing just the .gadget)
 */
#include <exec/types.h>
#include <intuition/gadgetclass.h>
#include <intuition/classes.h>

#define VERSION_TRACKAREA 1
#define TrackArea_SUPERCLASS_ID "gadgetclass"

#ifdef TRACKAREA_STATICLINK
    extern int TrackAreaStaticInit(void);
    extern void TrackAreaStaticClose(void);
    extern Class *TRACKAREA_GetClass(void);
#else
    /* TrackArea_CLASS_ID is the identifier for this class, when shared. */
    /* NewObject() can use either TRACKAREA_GetClass() or TrackArea_CLASS_ID. */
    #define TrackArea_CLASS_ID "trackarea.gadget"

    /* the following is to define function with implicit library call for TRACKAREA_GetClass(). */
    /* note it should be in includes generated from a fd files. */
    Class * __stdargs TRACKAREA_GetClass( void );
    /* ... could have other functions here */

    #ifndef _NO_INLINE
        # if defined(__GNUC__)
            #include <inline/macros.h>
            #define TRACKAREA_GetClass() LP0(0x1e, Class *, TRACKAREA_GetClass ,, TrackAreaBase)
            /* ... could have other functions here */
        # endif
        #if defined(LATTICE) || defined(__SASC) || defined(_DCC)
           #pragma libcall TrackAreaBase TRACKAREA_GetClass 1e 00
            /* ... could have other functions here */
        # endif
        #if defined(__VBCC__)
            Class * __TRACKAREA_GetClass(__reg("a6") void *)="\tjsr\t-$1e(a6)";
            #define TRACKAREA_GetClass() __TRACKAREA_GetClass(TrackAreaBase)
			/* ... could have other functions here */
        #endif
    #endif /* _NO_INLINE */

    #ifndef __NOLIBBASE__
      extern struct Library *
        # ifdef __CONSTLIBBASEDECL__
       __CONSTLIBBASEDECL__
        # endif /* __CONSTLIBBASEDECL__ */
      TrackAreaBase;
    #endif /* !__NOLIBBASE__ */

/* end if dynamic link */
#endif

/**  Attributes defined by the gadget class,
 * all attribs from gadgetclass.h are also valid.
 */
/* different classes may not use same base. */
#define TRACKAREA_Dummy			(TAG_USER+0x04110000)

/* abstract coordinate from 0 to 65535 , whatever width is. */
#define	TRACKAREA_CenterX		(TRACKAREA_Dummy+1)
/* abstract coordinate from 0 to 65535, whatever height is. */
#define	TRACKAREA_CenterY		(TRACKAREA_Dummy+2)

/* Pointer to AukStyleSheet for visual styling */
#define	TRACKAREA_StyleSheet		(TRACKAREA_Dummy+3)

/** DEVTODO: adds attributes definitions here and
 * manage them in class_trackarea_attribs.c
 */
#endif
