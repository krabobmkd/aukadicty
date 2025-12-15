#ifndef GADGETS_TRACKHEADERLIST_H
#define GADGETS_TRACKHEADERLIST_H
/**
 * Definitions for Gadget TrackHeaderList
 * (This is the public file that can be released when publishing just the .gadget)
 */
#include <exec/types.h>
#include <intuition/gadgetclass.h>
#include <intuition/classes.h>
            #include <inline/macros.h>
#define VERSION_TRACKHEADERLIST 1
#define TrackHeaderList_SUPERCLASS_ID "gadgetclass"

#ifdef TRACKHEADERLIST_STATICLINK
    extern int TrackHeaderListStaticInit();
    extern void TrackHeaderListStaticClose();
    extern Class *TRACKHEADERLIST_GetClass();
#else
    // TrackHeaderList_CLASS_ID is the identifier for this class, when shared.
    // NewObject() can use either TRACKHEADERLIST_GetClass() or TrackHeaderList_CLASS_ID.
    #define TrackHeaderList_CLASS_ID "trackheaderlist.gadget"

    // the following is to define function with implicit library call for TRACKHEADERLIST_GetClass().
    // note it should be in includes generated from a fd files.
    Class * __stdargs TRACKHEADERLIST_GetClass( void );
    // ... could have other functions here

    #ifndef _NO_INLINE
        # if defined(__GNUC__)
            #include <inline/macros.h>
            #define TRACKHEADERLIST_GetClass() LP0(0x1e, Class *, TRACKHEADERLIST_GetClass ,, TrackHeaderListBase)
            // ... could have other functions here
        # endif
        #if defined(LATTICE) || defined(__SASC) || defined(_DCC)
           #pragma libcall TrackHeaderListBase TRACKHEADERLIST_GetClass 1e 00
            // ... could have other functions here
        # endif
        #if defined(__VBCC__)
            Class * __TRACKHEADERLIST_GetClass(__reg("a6") void *)="\tjsr\t-$1e(a6)";
            #define TRACKHEADERLIST_GetClass() __TRACKHEADERLIST_GetClass(TrackHeaderListBase)
			// ... could have other functions here			
        #endif
    #endif /* _NO_INLINE */

    #ifndef __NOLIBBASE__
      extern struct Library *
        # ifdef __CONSTLIBBASEDECL__
       __CONSTLIBBASEDECL__
        # endif /* __CONSTLIBBASEDECL__ */
      TrackHeaderListBase;
    #endif /* !__NOLIBBASE__ */

// end if dynamic link
#endif

/**  Attributes defined by the gadget class,
 * all attribs from gadgetclass.h are also valid.
 */
// different classes may not use same base.
//DEVTODO: have another offset for your new class to not collide super class ones and optimize...
#define TRACKHEADERLIST_Dummy			(TAG_USER+0x04110000)

// abstract coordinate from 0 to 65535 , whatever width is.
#define	TRACKHEADERLIST_CenterX		(TRACKHEADERLIST_Dummy+1)
// abstract coordinate from 0 to 65535, whatever height is.
#define	TRACKHEADERLIST_CenterY		(TRACKHEADERLIST_Dummy+2)

/** DEVTODO: adds attributes definitions here and
 * manage them in class_trackheaderlist_attribs.c
 */
#endif
