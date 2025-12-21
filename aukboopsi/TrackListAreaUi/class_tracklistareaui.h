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
#define TRACKLIST_Dummy			(TAG_USER+0x04120000)

//
#define	TRACKLIST_TRACKMINHEIGHT		(TRACKLIST_Dummy+1)
//
#define	TRACKLIST_TRACK		(TRACKLIST_Dummy+2)
//
//#define	TRACKLIST_		(TRACKLIST_Dummy+)
////
//#define	TRACKLIST_		(TRACKLIST_Dummy+)
////
//#define	TRACKLIST_		(TRACKLIST_Dummy+)
////
//#define	TRACKLIST_		(TRACKLIST_Dummy+)
////
//#define	TRACKLIST_		(TRACKLIST_Dummy+)
////
//#define	TRACKLIST_		(TRACKLIST_Dummy+)


// GM_METHODS
//
#define TRACKLIST_GMDummy			(TRACKLIST_Dummy+0x100)
/*
#define GM_TRACKLIST_ADDTRACK (TRACKLIST_GMDummy+1)
#define GM_TRACKLIST_REMOVETRACK (TRACKLIST_GMDummy+2)
*/
// private class, so keep methids open for dev:

typedef struct AukAProject AukAProject;
typedef struct sAukTrack AukTrack;

// set main project - tracklist NULL means clean everything, back to empty state.
void TrackListAreaUi_setTrackList(struct Gadget *Gad,AukAProject *tracklist);
// events
void TrackListAreaUi_addTrack( struct Gadget *Gad,AukTrack *track);
void TrackListAreaUi_removeTrack( struct Gadget *Gad,AukTrack *track);
void TrackListAreaUi_trackModified( struct Gadget *Gad,AukTrack *track);




/** DEVTODO: adds attributes definitions here and
 * manage them in class_tracklist_attribs.c
 */
#endif
