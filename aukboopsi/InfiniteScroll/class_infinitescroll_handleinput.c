
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>

#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/gadgetclass.h>

#include "class_infinitescroll.h"
#include "class_infinitescroll_private.h"

#include "../bdbprintf.h"

/**
 * GM_HANDLEINPUT handler
 * Handle mouse and keyboard input
 * Child classes may override this to implement scrolling, zooming, etc.
 */
ULONG InfiniteScroll_HandleInput(Class *C, struct Gadget *Gad, struct gpInput *Input)
{
    InfiniteScroll *gdata;
    ULONG retval = GMR_MEACTIVE;

    gdata = INST_DATA(C, Gad);

    /* Default: just stay active, child classes should implement actual input handling */

    return retval;
}
