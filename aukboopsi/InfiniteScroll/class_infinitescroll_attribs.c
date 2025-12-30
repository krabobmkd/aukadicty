
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


ULONG InfiniteScroll_NotifyAttribValue(Class *C,struct Gadget *Gad, struct GadgetInfo *GInfo,ULONG attrib, ULONG value)
{
    struct opUpdate notifymsg;
    // InfiniteScroll *gdata;
    // gdata = INST_DATA(C, Gad);
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
    notifymsg.opu_GInfo = GInfo; // "always there for gadget, in all messages"
    notifymsg.opu_Flags = 0;

    return DoSuperMethodA(C,(APTR)Gad,(Msg)&notifymsg );
}


/**
 * OM_SET / OM_UPDATE handler
 */
ULONG InfiniteScroll_SetAttrs(Class *C, struct Gadget *Gad, struct opSet *Set)
{
    InfiniteScroll *gdata;
    struct TagItem *tags, *tag;
    BOOL needsRefresh = FALSE;

    gdata = INST_DATA(C, Gad);
//    tags = Set->ops_AttrList;
//bdbprintf("  *** trytofoolme? %08x\n",Set->MethodID);
//   while(tags->ti_Tag !=0)
//   {
//    bdbprintf("   %08x %08x\n",(int)tags->ti_Tag ,(int)tags->ti_Data);
//    tags++;
//   }
        tags = Set->ops_AttrList;
    while(tag = NextTagItem(&tags))
    {
        switch(tag->ti_Tag)
        {
            case INFINITESCROLL_Position:
            {
                InfiniteScrollPosition *pp = (InfiniteScrollPosition*)tag->ti_Data;
                if(pp)
                {
                    gdata->_position._scrollx = 0;
                    needsRefresh = TRUE;
                }
                else
                {
                    if(!InfiniteScrollPosition_isSame(pp,&gdata->_position))
                    {
                        gdata->_position = *pp;
                        needsRefresh = TRUE;
                    }
                }
            }
            break;
            case INFINITESCROLL_RenderFunction:
            {
                ULONG f = (ULONG)tag->ti_Data;
                bdbprintf(" //// set INFINITESCROLL_RenderFunction:%08x\n",(int)f);
                if(f !=  gdata->_renderFunction)
                {
                   gdata->_renderFunction = f;
                   needsRefresh = TRUE;
                }
            } break;
            default:
                break;
        }
    }

    if(needsRefresh)
    {
        // notify external source to render us.
        InfiniteScroll_NotifyAttribValue(C,Gad,Set->ops_GInfo,INFINITESCROLL_Redraw,1);
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
       case INFINITESCROLL_Position:
        {
            InfiniteScrollPosition *pp = (InfiniteScrollPosition*)Get->opg_Storage;
            if(pp)
            {
               // pp->_scrollx = gdata->_position._scrollx;
               *pp = gdata->_position; // copy all struct
            }
        }
        break;
        case INFINITESCROLL_RenderFunction:
        {
            InfiniteScrollRenderf *pp = (InfiniteScrollRenderf*)Get->opg_Storage;
            if(pp)
            {
               *pp = gdata->_renderFunction;
            }
        }
        break;
        default:
            retval = 0;
            break;
    }

    return retval;
}
