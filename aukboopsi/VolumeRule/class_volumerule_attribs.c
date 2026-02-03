
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/utility.h>


#include <clib/alib_protos.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/gadgetclass.h>
#include <utility/tagitem.h>

#include "class_volumerule.h"
#include "class_volumerule_private.h"
#include "../aukstyle.h"

#include "bdbprintf.h"

/**
 * Handle OM_GET - retrieve attribute values
 */
ULONG VolumeRule_GetAttr(Class *C, struct Gadget *Gad, struct opGet *Get)
{
    VolumeRule *gdata;
    ULONG *data;
    ULONG retval = 0;

    gdata = INST_DATA(C, Gad);
    data = Get->opg_Storage;

    switch(Get->opg_AttrID)
    {
        case VOLUMERULE_StyleSheet:
            *data = (ULONG)gdata->_style;
            retval = 1;
            break;

        default:
            retval = DoSuperMethodA(C, (Object *)Gad, (Msg)Get);
            break;
    }

    return retval;
}

/**
 * Handle OM_SET/OM_UPDATE - set attribute values
 */
ULONG VolumeRule_SetAttrs(Class *C, struct Gadget *Gad, struct opSet *Set)
{
    VolumeRule *gdata;
    struct TagItem *tag;
    ULONG actuallydone = 0;

    gdata = INST_DATA(C, Gad);

    for(tag = Set->ops_AttrList; tag->ti_Tag != TAG_END; tag++)
    {
        switch(tag->ti_Tag)
        {
            case VOLUMERULE_StyleSheet:
                gdata->_style = (AukStyle *)tag->ti_Data;
             //   bdbprintf("VolumeRule_SetAttrs VOLUMERULE_StyleSheet %08x\n",(int)gdata->_style);
                actuallydone = 1;
                break;

            case VOLUMERULE_Refresh:
                /* Force redraw if requested */
                actuallydone = 1;
                break;

            case TAG_MORE:
                tag = (struct TagItem *)tag->ti_Data;
                break;

            case TAG_SKIP:
                tag += tag->ti_Data;
                break;

            case TAG_IGNORE:
            default:
                break;
        }
    }

    return actuallydone;
}
