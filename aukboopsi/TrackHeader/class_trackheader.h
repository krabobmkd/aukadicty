#ifndef GADGETS_TRACKHEADER_H
#define GADGETS_TRACKHEADER_H
/**
 * Definitions for Gadget TrackHeader
 * (This is the public file that can be released when publishing just the .gadget)
 */
#include <exec/types.h>
#include <intuition/gadgetclass.h>
#include <intuition/classes.h>
            #include <inline/macros.h>
#define VERSION_TRACKHEADER 1
#define TrackHeader_SUPERCLASS_ID "layout.gadget"

#ifdef TRACKHEADER_STATICLINK
    extern int TrackHeaderStaticInit();
    extern void TrackHeaderStaticClose();
    extern Class *TRACKHEADER_GetClass();
    extern Class *HEADERBUTTON_GetClass();
#else
    // TrackHeader_CLASS_ID is the identifier for this class, when shared.
    // NewObject() can use either TRACKHEADER_GetClass() or TrackHeader_CLASS_ID.
    #define TrackHeader_CLASS_ID "trackheader.gadget"

    // the following is to define function with implicit library call for TRACKHEADER_GetClass().
    // note it should be in includes generated from a fd files.
    Class * __stdargs TRACKHEADER_GetClass( void );
    // ... could have other functions here

    #ifndef _NO_INLINE
        # if defined(__GNUC__)
            #include <inline/macros.h>
            #define TRACKHEADER_GetClass() LP0(0x1e, Class *, TRACKHEADER_GetClass ,, TrackHeaderBase)
            // ... could have other functions here
        # endif
        #if defined(LATTICE) || defined(__SASC) || defined(_DCC)
           #pragma libcall TrackHeaderBase TRACKHEADER_GetClass 1e 00
            // ... could have other functions here
        # endif
        #if defined(__VBCC__)
            Class * __TRACKHEADER_GetClass(__reg("a6") void *)="\tjsr\t-$1e(a6)";
            #define TRACKHEADER_GetClass() __TRACKHEADER_GetClass(TrackHeaderBase)
			// ... could have other functions here
        #endif
    #endif /* _NO_INLINE */

    #ifndef __NOLIBBASE__
      extern struct Library *
        # ifdef __CONSTLIBBASEDECL__
       __CONSTLIBBASEDECL__
        # endif /* __CONSTLIBBASEDECL__ */
      TrackHeaderBase;
    #endif /* !__NOLIBBASE__ */

// end if dynamic link
#endif

/**  Attributes defined by the gadget class,
 * all attribs from gadgetclass.h are also valid.
 */
// different classes may not use same base.
//DEVTODO: have another offset for your new class to not collide super class ones and optimize...
#define TRACKHEADER_Dummy			(TAG_USER+0x04240000)

/* needed by OM_NEW */
#define TRACKHEADER_TrackIndex (TRACKHEADER_Dummy+1)

/* */
#define TRACKHEADER_Name (TRACKHEADER_Dummy+2)

/* */
#define TRACKHEADER_Pan (TRACKHEADER_Dummy+3)

/* */
#define TRACKHEADER_Volume (TRACKHEADER_Dummy+4)

/* Pointer to AukStyle for visual styling */
#define	TRACKHEADER_StyleSheet		(TRACKHEADER_Dummy+5)


/** DEVTODO: adds attributes definitions here and
 * manage them in class_trackheader_attribs.c
 */
#endif
