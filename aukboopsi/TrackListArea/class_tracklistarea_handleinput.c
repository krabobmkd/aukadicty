
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



//#define MRK_BUFFER_SIZE 3
/*
#define GMR_MEACTIVE	(0)
#define GMR_NOREUSE	(1 << 1)
#define GMR_REUSE	(1 << 2)
#define GMR_VERIFY	(1 << 3)	you MUST set gpi_Termination

*/
ULONG TrackListArea_HandleInput(Class *C, struct Gadget *Gad,Msg M, int x, int y)
{
    TrackListArea *gdata;
    ULONG retval=GMR_NOREUSE; // GMR_REUSE;
    LONG topedge,leftedge,width,height;

    topedge = Gad->TopEdge;
    leftedge = Gad->LeftEdge;
    width = Gad->Width;
    height = Gad->Height;
    gdata=INST_DATA(C, Gad);

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

        if(y>=headerGad->TopEdge && y<=(headerGad->TopEdge+headerGad->Height) &&
          x>=headerGad->LeftEdge && x<= (trackGad->LeftEdge+trackGad->Width))
        {
            if(x<(headerGad->LeftEdge+headerGad->Width))
            {
                bdbprintf("use a headerGad\n");
                return DoMethodA((Object*)headerGad, (Msg)M);
            } else
            {
                bdbprintf("use a trackGad\n");
                return DoMethodA((Object*)trackGad, (Msg)M); // not DoGadgetMethodA in that case
            }
        } // y test
        } // end loop per track
    } // end if any track

  return(retval);
}

