#ifndef _CLASS_VOLUMERULEPRIVATE_H_
#define _CLASS_VOLUMERULEPRIVATE_H_

#include "compilers.h"
#include "class_volumerule.h"
#include "aukstyle.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <exec/types.h>
#include <exec/libraries.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/gadgetclass.h>
#include <intuition/imageclass.h>
#include <graphics/gfx.h>
#include <graphics/regions.h>

/**
 * This is the internal private gadget struct that owns the data of the object instances.
 * An important principle of BOOPSI is that structure for the class is hidden to the consumers.
 * The consumers will only see the public header, and will do SetAttribs()/GetAttribs()/DoMethod().
 *
 * VolumeRule displays a volume scale from -1 to +1 with horizontal marks.
 */
typedef struct IVolumeRule {

    /* Pointer to AukStyle for visual styling (fonts, colors) */
    AukStyle *_style;

} VolumeRule;

ULONG VolumeRule_SetAttrs(Class *C, struct Gadget *Gad, struct opSet *Set);
ULONG VolumeRule_GetAttr(Class *C, struct Gadget *Gad, struct opGet *Get);
ULONG VolumeRule_Domain(Class *C, struct Gadget *Gad, struct gpDomain *D);
ULONG VolumeRule_Render(Class *C, struct Gadget *Gad, struct gpRender *R);


#ifdef __cplusplus
}
#endif

#endif
