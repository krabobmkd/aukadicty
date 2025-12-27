
#include <proto/exec.h>
#include <proto/intuition.h>

//#ifdef __SASC
////    #include "minialib.h"
//    #include <clib/alib_protos.h>
//#else
//    // GCC
//    #include "minialib.h"
//#endif

#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/gadgetclass.h>
//#include <utility/tagitem.h>

#include "class_tracklistarea.h"
#include "class_tracklistarea_private.h"

ULONG TrackListArea_HandleHitTest(Class *C, struct Gadget *Gad,struct gpHitTest *m)
{
   TrackListArea *gdata;
    ULONG retval=GMR_NOREUSE; // GMR_REUSE;
    LONG topedge,leftedge,width,height;
    int x,y;
    topedge = Gad->TopEdge;
    leftedge = Gad->LeftEdge;
    width = Gad->Width;
    height = Gad->Height;
    gdata=INST_DATA(C, Gad);

    x = m->gpht_Mouse.X + leftedge;
    y = m->gpht_Mouse.Y + topedge;

// bdbprintf("HandleHitTest xy reported to x:%d y:%d\n",x,y);

    if(gdata->_tracks && gdata->_trackCount > 0)
    {
        int i;
        /* Layout each track row */
        for(i = 0; i < gdata->_trackCount; i++)
        {
            TrackChild *strack;
            struct Gadget *headerGad;
            struct Gadget *trackGad;
            strack = &gdata->_tracks[i];
            headerGad = (struct Gadget*)strack->_trackHeader;
            trackGad = (struct Gadget*)strack->_trackArea;
            if(!headerGad || !trackGad) continue;

    // bdbprintf("i:%d headerGad id %08x \n",i,headerGad->GadgetID);

            /* Skip tracks that are scrolled out of view (above visible area) */
            if( ( headerGad->TopEdge + headerGad->Height) < topedge)
            {
                continue;
            }
            /* Stop if track is below visible area */
            if(headerGad->TopEdge > topedge + height)
            {
                break;
            }
            // not even layouted
            if(headerGad->Width==0 ||trackGad->Width==0) continue;

   //     bdbprintf("t:%d y:%d hg Top %d bt: %d \n",y,headerGad->TopEdge,(headerGad->TopEdge+headerGad->Height));
        if(y>=headerGad->TopEdge && y<=(headerGad->TopEdge+headerGad->Height) &&
          x>=headerGad->LeftEdge && x<= (trackGad->LeftEdge+trackGad->Width))
        {
            if(x<(headerGad->LeftEdge+headerGad->Width))
            {

                struct gpHitTest n;
                ULONG r;
                n.MethodID = GM_HITTEST;
                n.gpht_GInfo = m->gpht_GInfo;
                n.gpht_Mouse.X = x - headerGad->LeftEdge;
                n.gpht_Mouse.Y = y - headerGad->TopEdge;
                 r = DoMethodA((Object*)headerGad, (Msg)&n);
                return r;
            } else
            {
                struct gpHitTest n;
                n.MethodID = GM_HITTEST;
                n.gpht_GInfo = m->gpht_GInfo;
                n.gpht_Mouse.X = x - trackGad->LeftEdge;
                n.gpht_Mouse.Y = y - trackGad->TopEdge;
                return DoMethodA((Object*)trackGad, (Msg)&n);
            }
        } // y test
        } // end loop per track
    } // end if any track

    // "not hit"
    return 0;
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
    struct Region *oldClipRegion=NULL;
    TrackListArea *gdata;
    ULONG retval=GMR_NOREUSE; // GMR_REUSE;
    LONG topedge,leftedge,width,height;


    topedge = Gad->TopEdge;
    leftedge = Gad->LeftEdge;
    width = Gad->Width;
    height = Gad->Height;
    gdata=INST_DATA(C, Gad);

    x += leftedge;
    y += topedge;
// bdbprintf("HandleInput xy  x:%d y:%d\n",x,y);

//  bdbprintf(" gi->ly %08x gi->rp-ly:%08x \n",M->gpi_GInfo->gi_Layer,M->gpi_GInfo->gi_RastPort->Layer);

    // if(M->gpi_GInfo && M->gpi_GInfo->gi_Window && gdata->_clipRegion)
    // {
    //     oldClipRegion = InstallClipRegion(  M->gpi_GInfo->gi_Window->RPort->Layer, gdata->_clipRegion);
    // }



    if(gdata->_tracks && gdata->_trackCount > 0)
    {
        int i;
        /* Layout each track row */
        for(i = 0; i < gdata->_trackCount; i++)
        {
            TrackChild *strack;
            struct Gadget *headerGad;
            struct Gadget *trackGad;
            strack = &gdata->_tracks[i];
            headerGad = (struct Gadget*)strack->_trackHeader;
            trackGad = (struct Gadget*)strack->_trackArea;
            if(!headerGad || !trackGad) continue;
            /* Skip tracks that are scrolled out of view (above visible area) */
            if( ( headerGad->TopEdge + headerGad->Height) < topedge)
            {
                continue;
            }
            /* Stop if track is below visible area */
            if(headerGad->TopEdge > topedge + height)
            {
                break;
            }
            if(headerGad->Width==0 ||trackGad->Width==0) continue;

//bdbprintf("i:%d top:%d bt:%d\n",i,headerGad->TopEdge,headerGad->TopEdge+headerGad->Height);

        if(y>=headerGad->TopEdge && y<=(headerGad->TopEdge+headerGad->Height) &&
          x>=headerGad->LeftEdge && x<= (trackGad->LeftEdge+trackGad->Width))
        {
            if(x<(headerGad->LeftEdge+headerGad->Width))
            {
                M->gpi_Mouse.X -= headerGad->LeftEdge -leftedge ;
                M->gpi_Mouse.Y -= headerGad->TopEdge  - topedge;
                retval = DoMethodA((Object*)headerGad, (Msg)M);
                M->gpi_Mouse.X += headerGad->LeftEdge-leftedge ;
                M->gpi_Mouse.Y += headerGad->TopEdge- topedge;
                break;
            } else
            {
                M->gpi_Mouse.X -= trackGad->LeftEdge;
                M->gpi_Mouse.Y -= trackGad->TopEdge;
                retval = DoMethodA((Object*)trackGad, (Msg)M);
                M->gpi_Mouse.X += trackGad->LeftEdge;
                M->gpi_Mouse.Y += trackGad->TopEdge;
                break;
            }
        } // y test
        } // end loop per track
    } // end if any track

 // if(M->gpi_GInfo &&   M->gpi_GInfo->gi_Window->RPort->Layer && gdata->_clipRegion)
 // {
 //    InstallClipRegion( M->gpi_GInfo->gi_Window->RPort->Layer,oldClipRegion); // important to pass NULL if oldClipRegion is NULL.
 // }


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
    struct Region *oldClipRegion=NULL;
    TrackListArea *gdata;
    LONG topedge,leftedge,width,height;
    topedge = Gad->TopEdge;
    leftedge = Gad->LeftEdge;
    width = Gad->Width;
    height = Gad->Height;
    gdata=INST_DATA(C, Gad);

 // bdbprintf("TrackListArea_GoInactive %08x\n",M->gpgi_GInfo);


    // if(M->gpgi_GInfo && M->gpgi_GInfo->gi_Window && gdata->_clipRegion)
    // {
    //     oldClipRegion = InstallClipRegion(  M->gpgi_GInfo->gi_Window->RPort->Layer, gdata->_clipRegion);
    // }

    if(gdata->_tracks && gdata->_trackCount > 0)
    {
        int i;
        /* Layout each track row */
        for(i = 0; i < gdata->_trackCount; i++)
        {
            TrackChild *strack;
            struct Gadget *headerGad;
            struct Gadget *trackGad;
            strack = &gdata->_tracks[i];
            headerGad = (struct Gadget*)strack->_trackHeader;
            trackGad = (struct Gadget*)strack->_trackArea;
            if(!headerGad || !trackGad) continue;
            /* Skip tracks that are scrolled out of view (above visible area) */
            if( ( headerGad->TopEdge + headerGad->Height) < topedge)
            {
                continue;
            }
            /* Stop if track is below visible area */
            if(headerGad->TopEdge > topedge + height)
            {
                break;
            }
            if(headerGad->Width==0 ||trackGad->Width==0) continue;

            if(headerGad->Activation & GACT_ACTIVEGADGET)
            {
                //headerGad->Activation &= ~GACT_ACTIVEGADGET;
                DoMethodA((Object*)headerGad, (Msg)M);
            }
            if(trackGad->Activation & GACT_ACTIVEGADGET)
            {
                DoMethodA((Object*)trackGad, (Msg)M);
            }

        } // end loop per track

    } // end if any track

 // if(M->gpgi_GInfo &&   M->gpgi_GInfo->gi_Window->RPort->Layer && gdata->_clipRegion)
 // {
 //    InstallClipRegion( M->gpgi_GInfo->gi_Window->RPort->Layer,oldClipRegion); // important to pass NULL if oldClipRegion is NULL.

 // }


    return 0;
}
