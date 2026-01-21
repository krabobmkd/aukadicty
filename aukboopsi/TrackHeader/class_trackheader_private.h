#ifndef _CLASS_TRACKHEADERPRIVATE_H_
#define _CLASS_TRACKHEADERPRIVATE_H_

#include "compilers.h"
#include "class_trackheader.h"
#include "aukstyle.h"

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

/* subs gadget indices - direct children, no nested layouts */
#define THS_CloseButton   0
#define THS_NameButton    1
#define THS_SilencerBt    2
#define THS_SoloBt        3
#define THS_VolLabel      4
#define THS_VolumeSlider  5
#define THS_PanLabel      6
#define THS_PanSlider     7
#define THS_InfoLabel     8
#define THS_VolumeRule    9

#define THS_Total  10

typedef struct ITrackHeader {

    Object *subs[THS_Total];

    /* frame rectangle for clipping */
    struct Rectangle _framerec;

    /* Pointer to AukStyle for visual styling */
    AukStyle *_style;
    /* track */
    int _trackIndex;

} TrackHeader;

typedef struct TrackHeaderButton {
    int _isPushButton;
} TrackHeaderButton;

typedef struct TrackHeaderSlider {
    int _d;
} TrackHeaderSlider;

#ifdef __cplusplus
}
#endif

#endif
