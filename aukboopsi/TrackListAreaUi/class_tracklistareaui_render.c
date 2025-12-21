
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

    gdata=INST_DATA(C, Gad);

    topedge = Gad->TopEdge;
    leftedge = Gad->LeftEdge;
    width = Gad->Width;
    height = Gad->Height;

#ifdef USE_BEVEL_FRAME
    if(gdata->Bevel)
    {   // all other attribs that doesnt change are set at NewObject()
        SetAttrs((Object *)gdata->Bevel,
            IA_Left, leftedge,
            IA_Top,        topedge,
            IA_Width,      width,
            IA_Height,     height,
            BEVEL_ColorMap,(ULONG)layout->gpl_GInfo->gi_Screen->ViewPort.ColorMap,
            BEVEL_Transparent,TRUE, // we will draw iside the frame ourselve.
            BEVEL_Style,BVS_BUTTON,
            TAG_DONE);
        // consider the effective rectangle is inside the frame.
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


/** private,
* manage synchronisation of tracks
* alloc/free/realloc Tracks, when needed and recursively
* implicitely ask for sounds ...
*/
static void TrackListAreaUi_updateTrackListUiToData(struct Gadget *Gad)
{
    TrackListAreaUi *gdata;
    if(!TrackListClassPtr || !Gad) return;
    gdata=INST_DATA(TrackListClassPtr, Gad);




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

// events
void TrackListAreaUi_addTrack( struct Gadget *Gad,AukTrack *track)
{

}
void TrackListAreaUi_removeTrack(struct Gadget *Gad,AukTrack *track)
{

}
void TrackListAreaUi_trackModified(struct Gadget *Gad,AukTrack *track)
{

}
