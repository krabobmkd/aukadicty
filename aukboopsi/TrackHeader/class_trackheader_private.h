#ifndef _CLASS_TRACKHEADERPRIVATE_H_
#define _CLASS_TRACKHEADERPRIVATE_H_

#include "compilers.h"
#include "class_trackheader.h"

// not much sense because c++ static runtime are hard to link.
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
*  this is the internal private gadget struct that own the data of the object instances.
* an important principle of boopsi is that structure for the class is hidden to the consumers.
* the consumers will only see the public header, and will do setAtribs()/GetAttribs()/Domethod().
* Also: for the same Gadget, superclass members are in struct Gadget * passed to functions.
* (These are just concatenated structs in a system private way.)
* DEVTODO: make this class evolve to retain the data needed to draw and interact with your gadget.
*/
typedef struct ITrackHeader {
    Object *CloseButton;
    Object *NameLabel;

    Object  *VolumeSlider;
    Object  *PanSlider;

    // ------------- H
    Object *VolumeRule;

} TrackHeader;

#ifdef __cplusplus
}
#endif

#endif
