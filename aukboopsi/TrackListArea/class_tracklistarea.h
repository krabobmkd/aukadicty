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
/* Different classes may not use same base. */
#define TRACKLIST_Dummy			(TAG_USER+0x04120000)

/* Minimum height for a track row */
#define	TRACKLIST_TRACKMINHEIGHT		(TRACKLIST_Dummy+1)

/* Track reference */
#define	TRACKLIST_TRACK		(TRACKLIST_Dummy+2)

/* Vertical scroll position (first visible line in pixels) */
#define	TRACKLIST_ScrollY		(TRACKLIST_Dummy+3)

/* Domain width - total virtual pixel width of the longest track time length, reported to pixels */
#define	TRACKLIST_DomainWidth		(TRACKLIST_Dummy+4)

/* Domain height - total pixel height of all tracks (GetAttr only) */
#define	TRACKLIST_DomainHeight		(TRACKLIST_Dummy+5)

/* Pointer to AukStyleSheet for visual styling */
#define	TRACKLIST_StyleSheet		(TRACKLIST_Dummy+6)


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
void TrackListArea_setTrackList(struct Gadget *Gad,AukAProject *tracklist);
// events
void TrackListArea_addTrack( struct Gadget *Gad,AukTrack *track);
void TrackListArea_removeTrack( struct Gadget *Gad,AukTrack *track);
void TrackListArea_trackModified( struct Gadget *Gad,AukTrack *track);




/** DEVTODO: adds attributes definitions here and
 * manage them in class_tracklist_attribs.c
 */
#endif
