#ifndef GADGETS_TIMERULE_H
#define GADGETS_TIMERULE_H
/**
 * Definitions for Gadget TimeRule
 * (This is the public file that can be released when publishing just the .gadget)
 *
 * TimeRule inherits from InfiniteScroll to use bitmap tile caching.
 * It draws time graduations with lines and text showing time units.
 */
#include <exec/types.h>
#include <intuition/gadgetclass.h>
#include <intuition/classes.h>
#include <inline/macros.h>

/* TimeRule inherits from InfiniteScroll for tile caching */
#include "../InfiniteScroll/class_infinitescroll.h"

#define VERSION_TIMERULE 1
/* Use InfiniteScroll as superclass - requires INFINITESCROLL_GetClass() */
#define TimeRule_SUPERCLASS_ID NULL  /* Use class pointer, not string */


    extern int TimeRuleStaticInit();
    extern void TimeRuleStaticClose();
    extern Class *TIMERULE_GetClass();

/**  Attributes defined by the gadget class,
 * all attribs from gadgetclass.h and InfiniteScroll are also valid.
 */
/* different classes may not use same base. */
#define TIMERULE_Dummy			(TAG_USER+0x04510000)

/* pointer to a long long. Note scroll poistion per pixel is at InfiniteScroll level  */
#define	TIMERULE_TimePerPixelWidth	(TIMERULE_Dummy+1)

/* Pointer to AukStyle for visual styling (fonts, colors) */
#define TIMERULE_StyleSheet (TIMERULE_Dummy+2)

#define TIMERULE_Refresh (TIMERULE_Dummy+3)

#endif
