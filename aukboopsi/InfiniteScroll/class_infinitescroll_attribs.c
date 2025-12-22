
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/utility.h>

#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/gadgetclass.h>
#include <utility/tagitem.h>

#include "class_infinitescroll.h"
#include "class_infinitescroll_private.h"

#include "../bdbprintf.h"

/**
 * OM_SET / OM_UPDATE handler
 */
ULONG InfiniteScroll_SetAttrs(Class *C, struct Gadget *Gad, struct opSet *Set)
{
    InfiniteScroll *gdata;
    struct TagItem *tags, *tag;
    BOOL needsRefresh = FALSE;

    gdata = INST_DATA(C, Gad);
    tags = Set->ops_AttrList;

    while(tag = NextTagItem(&tags))
    {
        switch(tag->ti_Tag)
        {
            case INFINITESCROLL_DomainMinHi:
                gdata->_domainMinHi = (LONG)tag->ti_Data;
                needsRefresh = TRUE;
                break;

            case INFINITESCROLL_DomainMinLo:
                gdata->_domainMinLo = (LONG)tag->ti_Data;
                needsRefresh = TRUE;
                break;

            case INFINITESCROLL_DomainMaxHi:
                gdata->_domainMaxHi = (LONG)tag->ti_Data;
                needsRefresh = TRUE;
                break;

            case INFINITESCROLL_DomainMaxLo:
                gdata->_domainMaxLo = (LONG)tag->ti_Data;
                needsRefresh = TRUE;
                break;

            case INFINITESCROLL_ViewPosHi:
                if(gdata->_viewPosHi != (LONG)tag->ti_Data)
                {
                    gdata->_viewPosHi = (LONG)tag->ti_Data;
                    needsRefresh = TRUE;
                }
                break;

            case INFINITESCROLL_ViewPosLo:
                if(gdata->_viewPosLo != (LONG)tag->ti_Data)
                {
                    gdata->_viewPosLo = (LONG)tag->ti_Data;
                    needsRefresh = TRUE;
                }
                break;

            case INFINITESCROLL_ViewZoomHi:
                if(gdata->_viewZoomHi != (LONG)tag->ti_Data)
                {
                    gdata->_viewZoomHi = (LONG)tag->ti_Data;
                    needsRefresh = TRUE;
                }
                break;

            case INFINITESCROLL_ViewZoomLo:
                if(gdata->_viewZoomLo != (LONG)tag->ti_Data)
                {
                    gdata->_viewZoomLo = (LONG)tag->ti_Data;
                    needsRefresh = TRUE;
                }
                break;

            case INFINITESCROLL_TileWidth:
                if(gdata->_tileWidth != (UWORD)tag->ti_Data && tag->ti_Data > 0)
                {
                    gdata->_tileWidth = (UWORD)tag->ti_Data;
                    /* Changing tile width requires reallocation at next layout */
                    InfiniteScroll_DisposeTiles(gdata);
                }
                break;

            default:
                break;
        }
    }

    /* If view changed, mark all tiles as dirty */
    if(needsRefresh)
    {
        struct gpInvalidateTiles invalidMsg;
        invalidMsg.MethodID = GM_INFINITESCROLL_INVALIDATETILES;
        invalidMsg.RangeMinHi = 0;
        invalidMsg.RangeMinLo = 0;
        invalidMsg.RangeMaxHi = 0;
        invalidMsg.RangeMaxLo = 0;
        InfiniteScroll_InvalidateTiles(C, Gad, &invalidMsg);
    }

    return 1;
}

/**
 * OM_GET handler
 */
ULONG InfiniteScroll_GetAttr(Class *C, struct Gadget *Gad, struct opGet *Get)
{
    InfiniteScroll *gdata;
    ULONG retval = 1;

    gdata = INST_DATA(C, Gad);

    switch(Get->opg_AttrID)
    {
        case INFINITESCROLL_DomainMinHi:
            *Get->opg_Storage = (ULONG)gdata->_domainMinHi;
            break;

        case INFINITESCROLL_DomainMinLo:
            *Get->opg_Storage = (ULONG)gdata->_domainMinLo;
            break;

        case INFINITESCROLL_DomainMaxHi:
            *Get->opg_Storage = (ULONG)gdata->_domainMaxHi;
            break;

        case INFINITESCROLL_DomainMaxLo:
            *Get->opg_Storage = (ULONG)gdata->_domainMaxLo;
            break;

        case INFINITESCROLL_ViewPosHi:
            *Get->opg_Storage = (ULONG)gdata->_viewPosHi;
            break;

        case INFINITESCROLL_ViewPosLo:
            *Get->opg_Storage = (ULONG)gdata->_viewPosLo;
            break;

        case INFINITESCROLL_ViewZoomHi:
            *Get->opg_Storage = (ULONG)gdata->_viewZoomHi;
            break;

        case INFINITESCROLL_ViewZoomLo:
            *Get->opg_Storage = (ULONG)gdata->_viewZoomLo;
            break;

        case INFINITESCROLL_TileWidth:
            *Get->opg_Storage = (ULONG)gdata->_tileWidth;
            break;

        default:
            retval = 0;
            break;
    }

    return retval;
}
