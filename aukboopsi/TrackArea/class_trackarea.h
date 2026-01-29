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
#include "aukselection.h"

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

/* Pointer to AukSelection (selection start and end times).
 * can be set once to shared value, then render refresh by the other refresh means.
 * NULL or both values == 0 means no selection.
 * Apply to OM_NEW OM_SET OM_GET.
 */
#define TRACKAREA_TimeSelection (TRACKAREA_Dummy+5)

/* this is to notify change, when clicks on us , so change are applied  . It's pointer to 2x long long  */
#define TRACKAREA_TimeSelectionChange (TRACKAREA_Dummy+6)
/* this is to notify change . It's pointer to 2x long long  */
#define TRACKAREA_TimeZoomChange (TRACKAREA_Dummy+7)

#endif
