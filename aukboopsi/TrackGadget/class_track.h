#ifndef GADGETS_TRACK_H
#define GADGETS_TRACK_H
/**
 * Definitions for Gadget Track
 * (This is the public file that can be released when publishing just the .gadget)
 */
#include <exec/types.h>
#include <intuition/gadgetclass.h>
#include <intuition/classes.h>

#define VERSION_TRACK 1
#define Track_SUPERCLASS_ID "gadgetclass"

#ifdef TRACK_STATICLINK
    extern int TrackStaticInit();
    extern void TrackStaticClose();
    extern Class *TRACK_GetClass();
#else
    // Track_CLASS_ID is the identifier for this class, when shared.
    // NewObject() can use either TRACK_GetClass() or Track_CLASS_ID.
    #define Track_CLASS_ID "track.gadget"

    // the following is to define function with implicit library call for TRACK_GetClass().
    // note it should be in includes generated from a fd files.
    Class * __stdargs TRACK_GetClass( void );
    // ... could have other functions here

    #ifndef _NO_INLINE
        # if defined(__GNUC__)
            #include <inline/macros.h>
            #define TRACK_GetClass() LP0(0x1e, Class *, TRACK_GetClass ,, TrackBase)
            // ... could have other functions here
        # endif
        #if defined(LATTICE) || defined(__SASC) || defined(_DCC)
           #pragma libcall TrackBase TRACK_GetClass 1e 00
            // ... could have other functions here
        # endif
    #endif /* _NO_INLINE */

    #ifndef __NOLIBBASE__
      extern struct Library *
        # ifdef __CONSTLIBBASEDECL__
       __CONSTLIBBASEDECL__
        # endif /* __CONSTLIBBASEDECL__ */
      TrackBase;
    #endif /* !__NOLIBBASE__ */

// end if dynamic link
#endif

/**  Attributes defined by the gadget class,
 * all attribs from gadgetclass.h are also valid.
 */
// different classes may not use same base.
#define TRACK_Dummy			(TAG_USER+0x04110000)

// abstract coordinate from 0 to 65535 , whatever width is.
#define	TRACK_CenterX		(TRACK_Dummy+1)
// abstract coordinate from 0 to 65535, whatever height is.
#define	TRACK_CenterY		(TRACK_Dummy+2)

/** DEVTODO: adds attributes definitions here and
 * manage them in class_track_attribs.c
 */
#endif
