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

    extern int TrackAreaStaticInit(void);
    extern void TrackAreaStaticClose(void);
    extern Class *TRACKAREA_GetClass(void);


/**  Attributes defined by the gadget class,
 * all attribs from gadgetclass.h are also valid.
 */
/* different classes may not use same base. */
#define TRACKAREA_Dummy			(TAG_USER+0x04180000)

/* Pointer to AukStyle for visual styling */
#define	TRACKAREA_StyleSheet		(TRACKAREA_Dummy+1)



/** DEVTODO: adds attributes definitions here and
 * manage them in class_trackarea_attribs.c
 */
#endif
