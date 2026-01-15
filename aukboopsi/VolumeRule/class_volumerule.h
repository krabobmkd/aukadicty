#ifndef GADGETS_VOLUMERULE_H
#define GADGETS_VOLUMERULE_H
/**
 * Definitions for Gadget VolumeRule
 * (This is the public file that can be released when publishing just the .gadget)
 *
 * VolumeRule draws a vertical audio volume scale with marks at
 * -1, -0.5, 0, 0.5, 1.0 values, with horizontal lines.
 * The gadget height represents the volume range [-1, 1].
 */
#include <exec/types.h>
#include <intuition/gadgetclass.h>
#include <intuition/classes.h>

#define VERSION_VOLUMERULE 1
/* VolumeRule inherits directly from gadgetclass */
#define VolumeRule_SUPERCLASS_ID "gadgetclass"


    extern int VolumeRuleStaticInit();
    extern void VolumeRuleStaticClose();
    extern Class *VOLUMERULE_GetClass();

/**  Attributes defined by the gadget class,
 * all attribs from gadgetclass.h are also valid.
 */
#define VOLUMERULE_Dummy        (TAG_USER+0x04520000)

/* Pointer to AukStyle for visual styling (fonts, colors) */
#define VOLUMERULE_StyleSheet   (VOLUMERULE_Dummy+1)

/* Force full redraw */
#define VOLUMERULE_Refresh      (VOLUMERULE_Dummy+2)

#endif
