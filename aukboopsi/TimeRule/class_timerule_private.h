#ifndef _CLASS_TIMERULEPRIVATE_H_
#define _CLASS_TIMERULEPRIVATE_H_

#include "compilers.h"
#include "class_timerule.h"
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
 * This is the internal private gadget struct that own the data of the object instances.
 * An important principle of BOOPSI is that structure for the class is hidden to the consumers.
 * The consumers will only see the public header, and will do SetAttribs()/GetAttribs()/DoMethod().
 * Also: for the same Gadget, superclass members are in struct Gadget * passed to functions.
 * (These are just concatenated structs in a system private way.)
 *
 * TimeRule inherits from InfiniteScroll - the superclass manages:
 * - IT CHANGEd.
 */
typedef struct ITimeRule {

    /* Gives current projection */
    long long _timePerPixelWidth;

    /* next GM_RENDER will do accoringly */
    WORD _justScroll,_fullRedraw;

    /* Pointer to AukStyle for visual styling (fonts, colors) */
    AukStyle *_style;

    /* computed for a _timePerPixelWidth value  */
    long long majorTickInterval;  /* Time between major ticks */
    long long minorTickInterval;  /* Time between minor ticks */
    int  tickSubDiv; /* basically  majorTickInterval/minorTickInterval */
    int  timeScale;  /* TimeScale enum value for formatting (USEC, MSEC, SEC, MIN) */

    UWORD majorTickHeight, minorTickHeight;

    /* Pointer to time selection span (start and end times).
     * NULL or both values == 0 means no selection.
     */
    AukSelection *_timeSelection;

} TimeRule;

ULONG TimeRule_SetAttrs(Class *C, struct Gadget *Gad, struct opSet *Set);
ULONG TimeRule_GetAttr(Class *C, struct Gadget *Gad, struct opGet *Get);
ULONG TimeRule_Domain(Class *C, struct Gadget *Gad, struct gpDomain *D);
ULONG TimeRule_Layout(Class *C, struct Gadget *Gad, struct gpLayout *layout);

void TimeRule_UpdateTimeInterval(TimeRule *gdata);

/* TimeRule overrides InfiniteScroll tile rendering to draw graduations */
/* NOW, the InfiniteScroll inheritage just need that render function
*/
void TimeRule_RenderDelegate(InfiniteScrollRenderParams *p);


/* Helper to format time value as text */
void TimeRule_FormatTime(long long stime, char *buffer, int scale);
// - - - - -- -

/** for dispatcher, very wise use of union.
 *  each  struct also starts with MethodID.
 * and they are the very parameters for each methods.
 */
typedef union MsgUnion
{
  ULONG  MethodID;
  /* from classusr.h or gadgetclass.h, all starts with MethodID. */
  struct opSet        opSet;
  struct opUpdate     opUpdate;
  struct opGet        opGet;
  struct gpHitTest    gpHitTest;
  struct gpRender     gpRender;
  struct gpInput      gpInput;
  struct gpGoInactive gpGoInactive;
  struct gpLayout     gpLayout;
  struct gpDomain     gpDomain;
} *Msgs;

/**
* This is to publish ou data when they change.
* It may be better to just notify what change and have many notify functions per theme.
* Some examples use only one Notify which send all attribs.
*/
ULONG TimeRule_NotifyCoords(Class *C, struct Gadget *Gad, struct GadgetInfo	*GInfo);

/** this is the struct that is the extended struct Library
 * That is created with OpenLibrary().
 * But as it just manages a BOOPSI class there are just the open/close functions.
 * which themselves only manages registering the class with MakeClass()/AddClass()
 * versioning, and closing itself. This is *not* the boopsi class definition which is up there.
 * So it doesnt have to evolve, and can keep same name for each projects.
 * That said, layout.gadget has tool methods like any library.
 * must be mirrored to equivalent in classinit.s
 */
struct ExtClassLib
{
    struct ClassLibrary cb_ClassLibrary;

    APTR  cb_SysBase; // this is passed as LibInit
    APTR  cb_SegList; // this is passed at OpenLib and needed at expunge.
    // note: old libraries examples adds bases for graphics/intuition/utility after this
    // but C compiler will only search then in globals...
};

#ifdef __cplusplus
}
#endif

#endif
