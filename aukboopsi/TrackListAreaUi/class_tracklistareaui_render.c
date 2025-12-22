
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/layers.h>

#ifdef __SASC
//    #include "minialib.h"
    #include <clib/alib_protos.h>
#else
    // GCC
    #include "minialib.h"
#endif

#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/gadgetclass.h>
#include <utility/tagitem.h>

#include "class_tracklistareaui.h"
#include "class_tracklistareaui_private.h"

/* Include child gadget classes */
#include "../TrackArea/class_trackarea.h"
#include "../TrackHeader/class_trackheader.h"

#ifdef USE_BEVEL_FRAME
    #include <proto/bevel.h>
    #include <images/bevel.h>
#endif

/* Most of the calls to boopsi methods are not done from the App's context,
 * but from a specific intuition context, and because of that we can't use DOS calls
 * like dos/Printf() , and also stdlib printf().
 * So we may print debug informations with a special buffer,and function bdbprintf(),
 * hen flushbdbprint() in main process will print for real to standard output.
 * remove word USE_DEBUG_BDBPRINT to desactivate all bdbprintf()/flushbdbprint() calls.
 * Template projects that links boopsi classes statically use USE_DEBUG_BDBPRINT by default.
 * Template projects that uses boopsi classes with LoadLibrary() do not.
 */
#include "bdbprintf.h"

/* The GM_DOMAIN method is used to obtain the sizing requirements of an
 * object for a class before ever creating an object. */

/* GM_DOMAIN */
//struct gpDomain
//{
//    ULONG		 MethodID;
//    struct GadgetInfo	*gpd_GInfo;
//    struct RastPort	*gpd_RPort;	/* RastPort to layout for */
//    LONG		 gpd_Which;
//    struct IBox		 gpd_Domain;	/* Resulting domain */
//    struct TagItem	*gpd_Attrs;	/* Additional attributes */
//};

ULONG TrackListAreaUi_Domain(Class *C, struct Gadget *Gad, struct gpDomain *D)
{
  TrackListAreaUi *gdata=0;

  if(Gad) gdata=INST_DATA(C, Gad);
// Printf("TrackListAreaUi_Domain data:%lx\n",(int)gdata);

  D->gpd_Domain.Left=0;
  D->gpd_Domain.Top=0;

  switch(D->gpd_Which)
  {
    case GDOMAIN_NOMINAL:
     // if(gdata)
     // {
     //   D->gpd_Domain.Width =gdata->_minimalWidth;
     //   D->gpd_Domain.Height=gdata->_minimalHeight;
     // }
     // else
      {
        D->gpd_Domain.Width=100;
        D->gpd_Domain.Height=50;
      }
      break;

    case GDOMAIN_MAXIMUM:
      D->gpd_Domain.Width=16000;
      D->gpd_Domain.Height=16000;
      break;

    case GDOMAIN_MINIMUM:
    default:
     if(gdata)
     {
       D->gpd_Domain.Width =gdata->_minimalWidth; // sqrt(gdata->Pens) * 8 + 8;
       D->gpd_Domain.Height=gdata->_minimalHeight; // sqrt(gdata->Pens) * 8 + 8;
     }
     else
      {
        D->gpd_Domain.Width=  50;
        D->gpd_Domain.Height= 50;
      }
      break;

  }
  return(1);
}

/**
 * method GM_LAYOUT
 * The gadget knows its final coordinates,
 * So we may have to resize what's inside our gadget.
 */
ULONG TrackListAreaUi_Layout(Class *C, struct Gadget *Gad, struct gpLayout *layout)
{
  TrackListAreaUi *gdata;
  LONG topedge,leftedge,width,height;
  LONG trackTop;
  ULONG i;
  struct gpLayout childLayout;

    gdata=INST_DATA(C, Gad);

    topedge = Gad->TopEdge;
    leftedge = Gad->LeftEdge;
    width = Gad->Width;
    height = Gad->Height;

#ifdef USE_BEVEL_FRAME
    if(gdata->Bevel)
    {   /* all other attribs that doesnt change are set at NewObject() */
        SetAttrs((Object *)gdata->Bevel,
            IA_Left, leftedge,
            IA_Top,        topedge,
            IA_Width,      width,
            IA_Height,     height,
            BEVEL_ColorMap,(ULONG)layout->gpl_GInfo->gi_Screen->ViewPort.ColorMap,
            BEVEL_Transparent,TRUE, /* we will draw iside the frame ourselve. */
            BEVEL_Style,BVS_BUTTON,
            TAG_DONE);
        /* consider the effective rectangle is inside the frame. */
        GetAttr(BEVEL_InnerTop,     gdata->Bevel,(ULONG *) &topedge);
        GetAttr(BEVEL_InnerLeft,    gdata->Bevel,(ULONG *) &leftedge);
        GetAttr(BEVEL_InnerWidth,   gdata->Bevel,(ULONG *) &width);
        GetAttr(BEVEL_InnerHeight,  gdata->Bevel,(ULONG *) &height);
    }
#endif
    gdata->_framerec.MinX = leftedge;
    gdata->_framerec.MinY = topedge;
    gdata->_framerec.MaxX = leftedge + width  -1;
    gdata->_framerec.MaxY = topedge  + height -1;

    /* Layout child gadgets (TrackHeaders and TrackGadgets) */
    if(gdata->_trackHeaders && gdata->_trackAreas && gdata->_trackCount > 0)
    {
        /* Default track height if not set */
        if(gdata->_trackHeight == 0) gdata->_trackHeight = 40;
        /* Default header width if not set */
        if(gdata->_headerWidth == 0) gdata->_headerWidth = 100;

        /* Start from the top, accounting for vertical scroll */
        trackTop = topedge - gdata->_scrollTop;

        /* Prepare child layout message */
        childLayout.MethodID = GM_LAYOUT;
        childLayout.gpl_GInfo = layout->gpl_GInfo;
        childLayout.gpl_Initial = 0;

        /* Layout each track row */
        for(i = 0; i < gdata->_trackCount; i++)
        {
            struct Gadget *headerGad;
            struct Gadget *trackGad;

            /* Skip tracks that are scrolled out of view (above visible area) */
            if(trackTop + gdata->_trackHeight < topedge)
            {
                trackTop += gdata->_trackHeight;
                continue;
            }

            /* Stop if track is below visible area */
            if(trackTop > topedge + height)
            {
                break;
            }

            headerGad = (struct Gadget*)gdata->_trackHeaders[i];
            trackGad = (struct Gadget*)gdata->_trackAreas[i];

            if(headerGad)
            {
                /* Position TrackHeader on the left */
                headerGad->LeftEdge = leftedge;
                headerGad->TopEdge = trackTop;
                headerGad->Width = gdata->_headerWidth;
                headerGad->Height = gdata->_trackHeight;

                /* Call child's GM_LAYOUT */
                DoMethodA((Object*)headerGad, (Msg)&childLayout);
            }

            if(trackGad)
            {
                /* Position TrackArea on the right, after header */
                trackGad->LeftEdge = leftedge + gdata->_headerWidth;
                trackGad->TopEdge = trackTop;
                trackGad->Width = width - gdata->_headerWidth;
                trackGad->Height = gdata->_trackHeight;

                /* Call child's GM_LAYOUT */
                DoMethodA((Object*)trackGad, (Msg)&childLayout);
            }

            trackTop += gdata->_trackHeight;
        }
    }

  return(1);
}


/* draw yourself, in the appropriate state */
ULONG TrackListAreaUi_Render(Class *C, struct Gadget *Gad, struct gpRender *Render, ULONG update)
{
  TrackListAreaUi *gdata;
  struct RastPort *rp; 
  ULONG retval=1;

  gdata=INST_DATA(C, Gad);

  // also sent from GM_GOINACTIVE (4).
  if(Render->MethodID==GM_RENDER)
  {
    rp=Render->gpr_RPort;
    update=Render->gpr_Redraw;
  }
  else
  {
    rp = ObtainGIRPort(Render->gpr_GInfo);
  }

  if(rp)
  {
    // vertical drawing management:
    // todo: recursively draw tracks on their projected rectangle

    // then draw eventually clear a rectangle of the empty scrool Area.
  }
  return(retval);
}

extern struct IClass   *TrackListClassPtr;
extern struct IClass   *TrackHeaderClassPtr;
extern struct IClass   *TrackGadgetClassPtr;

/** Helper - dispose all allocated gadgets */
void TrackListAreaUi_DisposeGadgets(TrackListAreaUi *gdata)
{
    ULONG i;
    if(!gdata) return;

    /* Dispose all TrackHeader gadgets */
    if(gdata->_trackHeaders)
    {
        for(i = 0; i < gdata->_trackCount; i++)
        {
            if(gdata->_trackHeaders[i])
            {
                DisposeObject(gdata->_trackHeaders[i]);
                gdata->_trackHeaders[i] = NULL;
            }
        }
        FreeVec(gdata->_trackHeaders);
        gdata->_trackHeaders = NULL;
    }

    /* Dispose all TrackArea gadgets */
    if(gdata->_trackAreas)
    {
        for(i = 0; i < gdata->_trackCount; i++)
        {
            if(gdata->_trackAreas[i])
            {
                DisposeObject(gdata->_trackAreas[i]);
                gdata->_trackAreas[i] = NULL;
            }
        }
        FreeVec(gdata->_trackAreas);
        gdata->_trackAreas = NULL;
    }

    gdata->_trackCount = 0;
}

/** private,
* manage synchronisation of tracks
* alloc/free/realloc Tracks, when needed and recursively
* implicitely ask for sounds ...
*/
static void TrackListAreaUi_updateTrackListUiToData(struct Gadget *Gad)
{
    TrackListAreaUi *gdata;
    AukAProject *project;
    ULONG trackCount;
    ULONG i;

    if(!TrackListClassPtr || !Gad) return;
    gdata = INST_DATA(TrackListClassPtr, Gad);

    project = gdata->_project;
    if(!project)
    {
        /* No project, clean up everything */
        TrackListAreaUi_DisposeGadgets(gdata);
        return;
    }

    /* Get the track count from project */
    trackCount = AukArray_GetCount(project->tracks);

    /* If count changed, reallocate arrays */
    if(trackCount != gdata->_trackCount)
    {
        /* Dispose old gadgets first */
        TrackListAreaUi_DisposeGadgets(gdata);

        if(trackCount > 0)
        {
            /* Allocate new arrays */
            gdata->_trackHeaders = (Object**)AllocVec(trackCount * sizeof(Object*), MEMF_CLEAR);
            gdata->_trackAreas = (Object**)AllocVec(trackCount * sizeof(Object*), MEMF_CLEAR);

            if(!gdata->_trackHeaders || !gdata->_trackAreas)
            {
                /* Allocation failed, cleanup */
                TrackListAreaUi_DisposeGadgets(gdata);
                return;
            }

            gdata->_trackCount = trackCount;

            /* Create gadgets for each track */
            for(i = 0; i < trackCount; i++)
            {
                /* Create TrackHeader gadget */
                gdata->_trackHeaders[i] = NewObject(TRACKHEADER_GetClass(), NULL, TAG_END);

                /* Create TrackArea */
                gdata->_trackAreas[i] = NewObject(TRACKAREA_GetClass(), NULL, TAG_END);

                if(!gdata->_trackHeaders[i] || !gdata->_trackAreas[i])
                {
                    /* Failed to create gadgets, cleanup and abort */
                    TrackListAreaUi_DisposeGadgets(gdata);
                    return;
                }
            }
        }
    }
}


// set main project - TrackListAreaUi NULL means clean everything, back to empty state.
void TrackListAreaUi_setTrackList(struct Gadget *Gad,AukAProject *tracklist)
{
    TrackListAreaUi *gdata;

    if(!TrackListClassPtr || !Gad) return;

    gdata=INST_DATA(TrackListClassPtr, Gad);

    AukObjectPtr_Set(&gdata->_project,tracklist);

    TrackListAreaUi_updateTrackListUiToData(Gad);
}

/* events */
void TrackListAreaUi_addTrack( struct Gadget *Gad,AukTrack *track)
{
    /* When a track is added, resync the entire gadget array */
    TrackListAreaUi_updateTrackListUiToData(Gad);
}

void TrackListAreaUi_removeTrack(struct Gadget *Gad,AukTrack *track)
{
    /* When a track is removed, resync the entire gadget array */
    TrackListAreaUi_updateTrackListUiToData(Gad);
}

void TrackListAreaUi_trackModified(struct Gadget *Gad,AukTrack *track)
{
    /* Track content modified - for now we don't need to do anything
     * as the TrackGadgets will handle their own rendering based on data */
}
