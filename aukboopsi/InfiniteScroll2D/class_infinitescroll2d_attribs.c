
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/alib.h>

#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/gadgetclass.h>
#include <utility/tagitem.h>

#include "class_infinitescroll2d.h"
#include "class_infinitescroll2d_private.h"

#include "../bdbprintf.h"


ULONG InfiniteScroll2D_NotifyAttribValue(Class *C, struct Gadget *Gad, struct GadgetInfo *GInfo, ULONG attrib, ULONG value)
{
    struct opUpdate notifymsg;
    ULONG tags[]={
     GA_ID,0,
     0,0,
     TAG_DONE
    };
    tags[1] = Gad->GadgetID;
    tags[2] = attrib;
    tags[3] = value;
    notifymsg.MethodID = OM_NOTIFY;
    notifymsg.opu_AttrList = (struct TagItem *)&tags[0];
    notifymsg.opu_GInfo = GInfo;
    notifymsg.opu_Flags = 0;

    return DoSuperMethodA(C,(APTR)Gad,(Msg)&notifymsg );
}


/**
 * OM_SET / OM_UPDATE handler
 */
ULONG InfiniteScroll2D_SetAttrs(Class *C, struct Gadget *Gad, struct opSet *Set)
{
    InfiniteScroll2D *gdata;
    struct TagItem *tags, *tag;
    BOOL needsRefresh = FALSE;

    gdata = INST_DATA(C, Gad);
    tags = Set->ops_AttrList;

    while((tag = NextTagItem(&tags))!=NULL)
    {
        switch(tag->ti_Tag)
        {
            case INFINITESCROLL2D_Position:
            {
                InfiniteScroll2DPosition *pp = (InfiniteScroll2DPosition*)tag->ti_Data;
                if(!pp)
                {
                    gdata->_position._scrollx = 0;
                    gdata->_position._scrolly = 0;
                    needsRefresh = TRUE;
                }
                else
                {
                    if(!InfiniteScroll2DPosition_isSame(pp, &gdata->_position))
                    {
                        gdata->_position = *pp;
                        needsRefresh = TRUE;
                    }
                }
            }
            break;
            case INFINITESCROLL2D_PPosition:
            {
                gdata->_pposition = (InfiniteScroll2DPosition *)tag->ti_Data;
                needsRefresh = TRUE;
            }
            break;
            case INFINITESCROLL2D_RenderFunction:
            {
                InfiniteScroll2DRenderf f = (InfiniteScroll2DRenderf)tag->ti_Data;
                if(f != gdata->_renderFunction)
                {
                   gdata->_renderFunction = f;
                   needsRefresh = TRUE;
                }
            }
            break;
            case INFINITESCROLL2D_FullTilesRefresh:
            {
                /* Invalidate all tiles - force full redraw */
                gdata->_torusOffsetX = 0;
                gdata->_torusOffsetY = 0;
                gdata->_renderedTilesX = 0;
                gdata->_renderedTilesY = 0;
                needsRefresh = TRUE;
            }
            break;
            case INFINITESCROLL2D_MarginLeft:
            {
                UWORD val = (UWORD)tag->ti_Data;
                if(val != gdata->_marginLeft)
                {
                    gdata->_marginLeft = val;
                    needsRefresh = TRUE;
                }
            }
            break;
            case INFINITESCROLL2D_MarginRight:
            {
                UWORD val = (UWORD)tag->ti_Data;
                if(val != gdata->_marginRight)
                {
                    gdata->_marginRight = val;
                    needsRefresh = TRUE;
                }
            }
            break;
            case INFINITESCROLL2D_MarginTop:
            {
                UWORD val = (UWORD)tag->ti_Data;
                if(val != gdata->_marginTop)
                {
                    gdata->_marginTop = val;
                    needsRefresh = TRUE;
                }
            }
            break;
            case INFINITESCROLL2D_MarginBottom:
            {
                UWORD val = (UWORD)tag->ti_Data;
                if(val != gdata->_marginBottom)
                {
                    gdata->_marginBottom = val;
                    needsRefresh = TRUE;
                }
            }
            break;
            case INFINITESCROLL2D_MarginPen:
            {
                WORD val = (WORD)tag->ti_Data;
                if(val != gdata->_marginPen)
                {
                    gdata->_marginPen = val;
                    needsRefresh = TRUE;
                }
            }
            break;
            default:
                break;
        }
    }

    if(needsRefresh)
    {
        /* notify external source to render us. */
        InfiniteScroll2D_NotifyAttribValue(C, Gad, Set->ops_GInfo, INFINITESCROLL2D_Redraw, 1);
    }

    return 1;
}

/**
 * OM_GET handler
 */
ULONG InfiniteScroll2D_GetAttr(Class *C, struct Gadget *Gad, struct opGet *Get)
{
    InfiniteScroll2D *gdata;
    ULONG retval = 1;

    gdata = INST_DATA(C, Gad);

    switch(Get->opg_AttrID)
    {
       case INFINITESCROLL2D_Position:
        {
            InfiniteScroll2DPosition *pp = (InfiniteScroll2DPosition*)Get->opg_Storage;
            if(pp)
            {
               *pp = *gdata->_pposition; /* copy all struct */
            }
        }
        break;
        case INFINITESCROLL2D_RenderFunction:
        {
            InfiniteScroll2DRenderf *pp = (InfiniteScroll2DRenderf*)Get->opg_Storage;
            if(pp)
            {
               *pp = gdata->_renderFunction;
            }
        }
        break;
        case INFINITESCROLL2D_MarginLeft:
            *(UWORD*)Get->opg_Storage = gdata->_marginLeft;
            break;
        case INFINITESCROLL2D_MarginRight:
            *(UWORD*)Get->opg_Storage = gdata->_marginRight;
            break;
        case INFINITESCROLL2D_MarginTop:
            *(UWORD*)Get->opg_Storage = gdata->_marginTop;
            break;
        case INFINITESCROLL2D_MarginBottom:
            *(UWORD*)Get->opg_Storage = gdata->_marginBottom;
            break;
        case INFINITESCROLL2D_MarginPen:
            *(WORD*)Get->opg_Storage = gdata->_marginPen;
            break;
        default:
            retval = 0;
            break;
    }

    return retval;
}

