#ifndef GADGETS_TRACKLIST_H
#define GADGETS_TRACKLIST_H
/**
 * Definitions for Gadget TrackList
 * (This is the public file that can be released when publishing just the .gadget)
 */
#include <exec/types.h>
#include <intuition/gadgetclass.h>
#include <intuition/classes.h>
            #include <inline/macros.h>
#define VERSION_TRACKLIST 1
#define TrackList_SUPERCLASS_ID "gadgetclass"

#ifdef TRACKLIST_STATICLINK
    extern int TrackListStaticInit();
    extern void TrackListStaticClose();
    extern Class *TRACKLIST_GetClass();
#else
    // TrackList_CLASS_ID is the identifier for this class, when shared.
    // NewObject() can use either TRACKLIST_GetClass() or TrackList_CLASS_ID.
    #define TrackList_CLASS_ID "tracklist.gadget"

    // the following is to define function with implicit library call for TRACKLIST_GetClass().
    // note it should be in includes generated from a fd files.
    Class * __stdargs TRACKLIST_GetClass( void );
    // ... could have other functions here

    #ifndef _NO_INLINE
        # if defined(__GNUC__)
            #include <inline/macros.h>
            #define TRACKLIST_GetClass() LP0(0x1e, Class *, TRACKLIST_GetClass ,, TrackListBase)
            // ... could have other functions here
        # endif
        #if defined(LATTICE) || defined(__SASC) || defined(_DCC)
           #pragma libcall TrackListBase TRACKLIST_GetClass 1e 00
            // ... could have other functions here
        # endif
        #if defined(__VBCC__)
            Class * __TRACKLIST_GetClass(__reg("a6") void *)="\tjsr\t-$1e(a6)";
            #define TRACKLIST_GetClass() __TRACKLIST_GetClass(TrackListBase)
			// ... could have other functions here			
        #endif
    #endif /* _NO_INLINE */

    #ifndef __NOLIBBASE__
      extern struct Library *
        # ifdef __CONSTLIBBASEDECL__
       __CONSTLIBBASEDECL__
        # endif /* __CONSTLIBBASEDECL__ */
      TrackListBase;
    #endif /* !__NOLIBBASE__ */

// end if dynamic link
#endif

/**  Attributes defined by the gadget class,
 * all attribs from gadgetclass.h are also valid.
 */
// different classes may not use same base.
//DEVTODO: have another offset for your new class to not collide super class ones and optimize...
#define TRACKLIST_Dummy			(TAG_USER+0x04110000)

// abstract coordinate from 0 to 65535 , whatever width is.
#define	TRACKLIST_CenterX		(TRACKLIST_Dummy+1)
// abstract coordinate from 0 to 65535, whatever height is.
#define	TRACKLIST_CenterY		(TRACKLIST_Dummy+2)

/** DEVTODO: adds attributes definitions here and
 * manage them in class_tracklist_attribs.c
 */
#endif
