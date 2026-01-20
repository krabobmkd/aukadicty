#ifndef GADGETS_TRACKAREA_H
#define GADGETS_TRACKAREA_H
/**
 * Definitions for Gadget TrackArea
 * (This is the public file that can be released when publishing just the .gadget)
 *
 * TrackArea inherits from InfiniteScroll - the superclass manages:
 * - Horizontal scroll position and tile caching
 * - GM_LAYOUT, GM_RENDER for tile-based rendering
 */
#include <exec/types.h>
#include <intuition/gadgetclass.h>
#include <intuition/classes.h>

/* Include TrackListArea header for TimeProjection definition */
#include "../TrackListArea/class_tracklistarea.h"
#include "../auktimesel.h"

#define VERSION_TRACKAREA 1
/* TrackArea inherits from InfiniteScroll (class pointer, not string) */
/* #define TrackArea_SUPERCLASS_ID - not used, we use class pointer */

    extern int TrackAreaStaticInit(void);
    extern void TrackAreaStaticClose(void);
    extern Class *TRACKAREA_GetClass(void);


/**  Attributes defined by the gadget class,
 * all attribs from gadgetclass.h are also valid,
 * plus InfiniteScroll attributes.
 */
/* different classes may not use same base. */
#define TRACKAREA_Dummy			(TAG_USER+0x04180000)

/* Pointer to AukStyle for visual styling */
#define	TRACKAREA_StyleSheet		(TRACKAREA_Dummy+1)

/* Pointer to TimeProjection in TrackListArea for time/pixel mapping.
 * This allows TrackArea to know horizontal scroll position and zoom level.
 * Apply to OM_NEW OM_SET OM_GET.
 */
#define	TRACKAREA_PTimeProjection		(TRACKAREA_Dummy+2)

/* Pointer to AukTrack data this gadget reflects.
 * The object is retained via reference counting (AukObjectPtr_Set/Release).
 * Apply to OM_NEW OM_SET OM_GET.
 */
#define	TRACKAREA_DataTrack		(TRACKAREA_Dummy+3)

/* Pointer to AukTimeCursor (64-bit fixed-point time position).
 * NULL means no cursor displayed. Used for playback/edit cursor.
 * Apply to OM_NEW OM_SET OM_GET.
 */
#define TRACKAREA_TimeCursor (TRACKAREA_Dummy+4)

/* Pointer to AukTimeSpan (selection start and end times).
 * NULL or both values == 0 means no selection.
 * Apply to OM_NEW OM_SET OM_GET.
 */
#define TRACKAREA_TimeSelection (TRACKAREA_Dummy+5)

#endif
