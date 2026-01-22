
#include <proto/exec.h>
#include <proto/intuition.h>


#include <clib/alib_protos.h>

#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/gadgetclass.h>
//#include <utility/tagitem.h>

#include "class_tracklistarea.h"
#include "class_tracklistarea_private.h"

ULONG TrackListArea_HandleHitTest(Class *C, struct Gadget *Gad,struct gpHitTest *m)
{
   TrackListArea *gdata;
 //   ULONG retval=GMR_NOREUSE; // GMR_REUSE;
    LONG topedge,leftedge; //,width,height;
    int x,y;
    topedge = Gad->TopEdge;
    leftedge = Gad->LeftEdge;
//    width = Gad->Width;
//    height = Gad->Height;
    gdata=INST_DATA(C, Gad);

    x = m->gpht_Mouse.X + leftedge;
    y = m->gpht_Mouse.Y + topedge;

// bdbprintf("HandleHitTest xy reported to x:%d y:%d\n",x,y);

    if(gdata->_tracks && gdata->_trackCount > 0)
    {
        ULONG itrack,iChannel;
        for(itrack = 0; itrack < gdata->_trackCount; itrack++)
        {
            TrackChild *strack;
            strack = &gdata->_tracks[itrack];
            if(!strack->_layouted) continue;
            for(iChannel = 0; iChannel < strack->_nbChannels; iChannel++)
            {
                TrackChannelChild *chan = &strack->_channels[iChannel];
                struct Gadget *headerGad,*volumeRule;
                struct Gadget *trackGad;

                headerGad = (struct Gadget*)chan->_trackHeader;
                volumeRule =  (struct Gadget*)chan->_volumeRule;
                trackGad = (struct Gadget*)chan->_trackArea;


                if(y>=chan->_top && y< chan->_bottom )
                {
                    if(headerGad && x</*chan->_xmid*/(headerGad->LeftEdge+headerGad->Width) )
                    {

                        struct gpHitTest n;
                        ULONG r;
                        n.MethodID = GM_HITTEST;
                        n.gpht_GInfo = m->gpht_GInfo;
                        n.gpht_Mouse.X = x - headerGad->LeftEdge;
                        n.gpht_Mouse.Y = y - headerGad->TopEdge;
                         r = DoMethodA((Object*)headerGad, (Msg)&n);
                        return r;
                    } else if(volumeRule && x< (volumeRule->LeftEdge+volumeRule->Width) )
                    {
                        return GMR_NOREUSE;
                    } else
                    if(chan->_layouted && trackGad && x>=chan->_xmid)
                    {
                        struct gpHitTest n;
                        n.MethodID = GM_HITTEST;
                        n.gpht_GInfo = m->gpht_GInfo;
                        n.gpht_Mouse.X = x - trackGad->LeftEdge;
                        n.gpht_Mouse.Y = y - trackGad->TopEdge;
                        return DoMethodA((Object*)trackGad, (Msg)&n);
                    }
                } // y test
            } // end loop per chan
        } // end loop per track
    } // end if any track

    // "not hit"
    return GMR_NOREUSE; //0;
}

//#define MRK_BUFFER_SIZE 3
/*
#define GMR_MEACTIVE	(0)
#define GMR_NOREUSE	(1 << 1)
#define GMR_REUSE	(1 << 2)
#define GMR_VERIFY	(1 << 3)	you MUST set gpi_Termination

*/
ULONG TrackListArea_HandleInput(Class *C, struct Gadget *Gad,struct gpInput *M, int x, int y)
{
    //struct Region *oldClipRegion=NULL;
    TrackListArea *gdata;
    ULONG retval=GMR_NOREUSE; // GMR_REUSE;
    LONG topedge,leftedge; //,width,height;


    topedge = Gad->TopEdge;
    leftedge = Gad->LeftEdge;
//    width = Gad->Width;
//    height = Gad->Height;
    gdata=INST_DATA(C, Gad);

    x += leftedge;
    y += topedge;

    if(gdata->_tracks && gdata->_trackCount > 0)
    {
        ULONG itrack,iChannel;
        for(itrack = 0; itrack < gdata->_trackCount; itrack++)
        {
            TrackChild *strack;
            strack = &gdata->_tracks[itrack];
            if(!strack->_layouted) continue;
            for(iChannel = 0; iChannel < strack->_nbChannels; iChannel++)
            {
                TrackChannelChild *chan = &strack->_channels[iChannel];
                struct Gadget *headerGad,*volumeRule;
                struct Gadget *trackGad;

                headerGad = (struct Gadget*)chan->_trackHeader;
                volumeRule =  (struct Gadget*)chan->_volumeRule;
                trackGad = (struct Gadget*)chan->_trackArea;
                //if(!chan->_layouted) continue;

                if(y>=chan->_top && y< chan->_bottom )
                {
                    if(headerGad && x< (headerGad->LeftEdge+headerGad->Width))
                    {
                        M->gpi_Mouse.X -= headerGad->LeftEdge -leftedge ;
                        M->gpi_Mouse.Y -= headerGad->TopEdge  - topedge;
                        retval = DoMethodA((Object*)headerGad, (Msg)M);
                        M->gpi_Mouse.X += headerGad->LeftEdge-leftedge ;
                        M->gpi_Mouse.Y += headerGad->TopEdge- topedge;
                        break;
                    } else if(volumeRule && x< (volumeRule->LeftEdge+volumeRule->Width) )
                    {
                        return 0; // dunno
                    } else
                    if(trackGad && x>=chan->_xmid )
                    {
                        M->gpi_Mouse.X -= trackGad->LeftEdge;
                        M->gpi_Mouse.Y -= trackGad->TopEdge;
                        retval = DoMethodA((Object*)trackGad, (Msg)M);
                        M->gpi_Mouse.X += trackGad->LeftEdge;
                        M->gpi_Mouse.Y += trackGad->TopEdge;
                        break;
                    }
                } // y test
            }
        } // end loop per track
    } // end if any track

  return(retval);
}
ULONG TrackListArea_GoInactive(Class *C, struct Gadget *Gad,struct gpGoInactive *M)
{
/*
The gpgi_Abort field contains either a 0 or 1. If 0, the gadget became inactive
on its own power (because the GM_GOACTIVE or GM_HANDLEINPUT method returned
something besides GMR_MEACTIVE). If gpgi_Abort is 1, Intuition aborted this active gadget.
Some instances where Intuition aborts a gadget include: the user clicked in another
window or screen, an application removed the active gadget with RemoveGList(),
 and an application called ActiveWindow() on a window other than the gadget's window.
*/
//    struct Region *oldClipRegion=NULL;
    TrackListArea *gdata;
//    LONG topedge,leftedge,width,height;
//    topedge = Gad->TopEdge;
//    leftedge = Gad->LeftEdge;
//    width = Gad->Width;
//    height = Gad->Height;
    gdata=INST_DATA(C, Gad);

    if(gdata->_tracks && gdata->_trackCount > 0)
    {
        ULONG itrack,iChannel;
        for(itrack = 0; itrack < gdata->_trackCount; itrack++)
        {
            TrackChild *strack;
            strack = &gdata->_tracks[itrack];
            if(!strack->_layouted) continue;
            for(iChannel = 0; iChannel < strack->_nbChannels; iChannel++)
            {
                TrackChannelChild *chan = &strack->_channels[iChannel];
                struct Gadget *headerGad,*volumeRule;
                struct Gadget *trackGad;

                headerGad = (struct Gadget*)chan->_trackHeader;
                volumeRule = (struct Gadget*)chan->_volumeRule;
                trackGad = (struct Gadget*)chan->_trackArea;

                if(headerGad && headerGad->Activation & GACT_ACTIVEGADGET)
                {
                    DoMethodA((Object*)headerGad, (Msg)M);
                }
                if(volumeRule && volumeRule->Activation & GACT_ACTIVEGADGET)
                {
                    DoMethodA((Object*)volumeRule, (Msg)M);
                }
                if(trackGad && trackGad->Activation & GACT_ACTIVEGADGET)
                {
                    DoMethodA((Object*)trackGad, (Msg)M);
                }
            }
        } // end loop per track

    } // end if any track

    return 0;
}
