
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

#include "class_trackarea_private.h"

#include "class_tracklistareaui.h"
#include "class_tracklistareaui_private.h"

/* Include child gadget classes */
#include "../TrackArea/class_trackarea.h"
#include "../TrackHeader/class_trackheader.h"

#include <proto/layout.h>
#include <gadgets/layout.h>

#include <proto/button.h>
#include <gadgets/button.h>

#include <aukarray.h>

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


extern struct IClass   *TrackListClassPtr;
extern struct IClass   *TrackHeaderClassPtr;
extern struct IClass   *TrackAreaClassPtr;

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

    gdata=INST_DATA(C, Gad);

    topedge = Gad->TopEdge;
    leftedge = Gad->LeftEdge;
    width = Gad->Width;
    height = Gad->Height;

    gdata->_framerec.MinX = leftedge;
    gdata->_framerec.MinY = topedge;
    gdata->_framerec.MaxX = leftedge + width  -1;
    gdata->_framerec.MaxY = topedge  + height -1;

    if(layout->gpl_GInfo)
    {
        gdata->window = layout->gpl_GInfo->gi_Window;
    }

 bdbprintf(" *** TrackListAreaUi_Layout  tracks:%d\n",(int)gdata->_trackCount);
//

//    if(layout->gpl_GInfo)
//    {
//        subGadgetInfo = *(layout->gpl_GInfo);
//    }
    /* Layout child gadgets (TrackHeaders and TrackGadgets) */
    if(gdata->_tracks && gdata->_trackCount > 0)
    {
        /* Default track height if not set */
        if(gdata->_defaulTrackHeight == 0) gdata->_defaulTrackHeight = 40;
        /* Default header width if not set */
        if(gdata->_headerWidth == 0) gdata->_headerWidth = 100;

        /* Start from the top, accounting for vertical scroll */
        trackTop = topedge - gdata->_scrollTop;


        /* Layout each track row */
        for(i = 0; i < gdata->_trackCount; i++)
        {
            TrackArea *trackArea;
            TrackChild *strack;
            struct Gadget *headerGad;
            struct Gadget *trackGad;
            UWORD trackHeight=0;

            strack = &gdata->_tracks[i];
            headerGad = (struct Gadget *) strack->_trackHeader;
            trackGad = (struct Gadget *) strack->_trackArea;

            if(trackGad)
            {
                trackArea = INST_DATA(TrackAreaClassPtr, trackGad);
                trackHeight = trackArea->_prefHeight;
                if(trackHeight==0) trackHeight = gdata->_defaulTrackHeight;
            }

            /* Skip tracks that are scrolled out of view (above visible area)
            disable also if is below visible area
            */
            if((trackTop + trackHeight < topedge ) ||
                (trackTop > topedge + height)
                 )
            {
                headerGad->Width = 0; // how we say it's not layouted.
                trackGad->Width = 0;
                trackTop += trackHeight ;
                continue;
            }

            if(headerGad)
            {
                /* Position TrackHeader on the left */
                headerGad->LeftEdge = leftedge;
                headerGad->TopEdge = trackTop;
                headerGad->Width = gdata->_headerWidth;
                headerGad->Height = trackHeight;

                /* Call child's GM_LAYOUT */
                DoMethodA((Object*)headerGad, (Msg)layout);
            //   DoGadgetMethodA(headerGad,gdata->window,NULL,(Msg)&childLayout);
            }

            if(trackGad)
            {

                /* Position TrackArea on the right, after header */
                trackGad->LeftEdge = leftedge + gdata->_headerWidth;
                trackGad->TopEdge = trackTop;
                trackGad->Width = width - gdata->_headerWidth;
                trackGad->Height = trackHeight;

                /* Call child's GM_LAYOUT */
              // DoMethodA((Object*)trackGad, (Msg)&childLayout);
           //    DoGadgetMethodA(trackGad,gdata->window,NULL,(Msg)&childLayout);
            }
// _defaulTrackHeight
            trackTop += trackHeight;
        }
    }

    if(gdata->_clipRegion)
    {
        ClearRegion(gdata->_clipRegion);
        OrRectRegion(gdata->_clipRegion, &gdata->_framerec);
    }

  return(1);
}

//ULONG TrackHeader_Render(Class *C, struct Gadget *Gad, struct gpRender *Render, ULONG update);
// ULONG TrackArea_Render_rp( struct RastPort *rp,Class *C, struct Gadget *Gad, struct gpRender *Render);
// ULONG TrackHeader_Render_rp( struct RastPort *rp,Class *C, struct Gadget *Gad, struct gpRender *Render);

/* draw yourself, in the appropriate state */
ULONG TrackListAreaUi_Render(Class *C, struct Gadget *Gad, struct gpRender *Render, ULONG update)
{
  TrackListAreaUi *gdata;
  struct RastPort *rp;
  ULONG retval=1;

  gdata=INST_DATA(C, Gad);

 bdbprintf("TrackListAreaUi_Render\n");

  // also sent from GM_GOINACTIVE (4).
  if(Render->MethodID==GM_RENDER)
  {
    rp=Render->gpr_RPort;
    update=Render->gpr_Redraw;
  }
  else
  {
    return 0; //
//    rp = ObtainGIRPort(Render->gpr_GInfo);
  }

  if(rp)
  {
  	int bLayerUpdating=FALSE;
    LONG i;
    LONG topedge,leftedge,width,height;
    topedge = Gad->TopEdge;
    leftedge = Gad->LeftEdge;
    width = Gad->Width;
    height = Gad->Height;
    struct Region *oldClipRegion;

	if( ( rp->Layer->Flags & LAYERUPDATING ) != 0L )
	{
		bLayerUpdating = TRUE;
		EndUpdate(rp->Layer, FALSE);
		//bdbprintf(" ****Render->MethodID:%08lx LAYERUPDATING\n",(int)Render->MethodID);
	}

    oldClipRegion = InstallClipRegion( rp->Layer, gdata->_clipRegion);

    if(gdata->_tracks && gdata->_trackCount > 0)
    {

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
 bdbprintf(" **would draw\n");
            /* Call child's GM_RENDER */
          //  DoMethodA((Object*)headerGad, (Msg)Render);
            // C,Gad,(struct gpRender *)M
            // ULONG TrackListAreaUi_Render(Class *C, struct Gadget *Gad, struct gpRender *Render, ULONG update)

            /* Call child's GM_RENDER */

          // recurse
         // note wouldn't work for GM_GOINACTIVE redirected to GM_RENDER
         if(headerGad->Width>0) // if layouted
         {
            DoMethodA((Object*)headerGad, (Msg)Render); // not DoGadgetMethodA in that case
         }
         if(trackGad->Width>0) // if layouted
         {
            DoMethodA((Object*)trackGad, (Msg)Render); // not DoGadgetMethodA in that case
         }
        } // end loop per track
    } // end if any track

    InstallClipRegion( rp->Layer,oldClipRegion); // important to pass NULL if oldClipRegion is NULL.

    if(bLayerUpdating)
    {
        BeginUpdate(rp->Layer);
    }

    // if (Render->MethodID != GM_RENDER)
    //   ReleaseGIRPort(rp);

    // vertical drawing management:
    // todo: recursively draw tracks on their projected rectangle

    // then draw eventually clear a rectangle of the empty scrool Area.
  } // end if rp
  return(retval);
}



/** Helper - dispose all allocated gadgets */
void TrackListAreaUi_DisposeGadgets(TrackListAreaUi *gdata)
{
    ULONG i;
    if(!gdata) return;

    /* Dispose all TrackHeader gadgets */
    if(gdata->_tracks)
    {
        for(i = 0; i < gdata->_trackCount; i++)
        {
            if(gdata->_tracks[i]._trackHeader)
            {
                //if(gdata->window) RemoveGadget(gdata->window,gdata->_tracks[i]._trackHeader);
                DisposeObject(gdata->_tracks[i]._trackHeader);
            }
            if(gdata->_tracks[i]._trackArea)
            {
                //if(gdata->window) RemoveGadget(gdata->window,gdata->_tracks[i]._trackArea);
                DisposeObject(gdata->_tracks[i]._trackArea);
            }
        }

        FreeVec(gdata->_tracks);
        gdata->_tracks = NULL;
    }
    gdata->_trackCount = 0;

}

// static Object *CreateHeader()
// {
//     Object *LeftVertlayout;
//     Object *Bt = NewObject( BUTTON_GetClass(),NULL,
//                                     GA_Text, "Button",
//                                   //  GA_ID,GAD_BUTTON_ABOUT,
//                                     GA_RelVerify, TRUE,
//                          //           GA_Disabled,TRUE,
//                         // BUTTON_BevelStyle,BVS_NONE,
//                         // BUTTON_Transparent, TRUE,
//                                 TAG_END);

//         // in this paragraph we create the layout hierarchy
//         LeftVertlayout  = (Object *)NewObject( LAYOUT_GetClass(), NULL,
//                     LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
//                     LAYOUT_AddChild, Bt,
//                     TAG_DONE);



//     return LeftVertlayout;
// }

/** private,
* manage synchronisation of tracks
* alloc/free/realloc Tracks, when needed and recursively
* implicitely ask for sounds ...
*/
static void TrackListAreaUi_updateTrackListUiToData(struct Gadget *Gad)
{
    TrackListAreaUi *gdata;
    AukAProject *project;
    ULONG dataTrackCount;
    ULONG i,nbTracksAlreadyInSync;

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
    dataTrackCount = AukArray_GetCount(project->tracks);

    /* verify how much it changes */
//    nbTracksAlreadyInSyncAtStart=0;
//    nbTracksMinusOneAtEnd=0;
//    for(i = 0; i < trackCount; i++)
//    {
//        _trackCount
//    }

    /* If count changed, reallocate arrays */
    if(dataTrackCount != gdata->_trackCount)
    {       
        /* Dispose old gadgets first */
        TrackListAreaUi_DisposeGadgets(gdata);

        if(dataTrackCount > 0)
        {
            // UWORD ipos = 65534;

            /* Allocate new arrays */
            gdata->_tracks = (TrackChild*)AllocVec(dataTrackCount * sizeof(TrackChild), MEMF_CLEAR);

            if(!gdata->_tracks)
            {
                /* Allocation failed, cleanup */
                TrackListAreaUi_DisposeGadgets(gdata);
                return;
            }

            gdata->_trackCount = dataTrackCount;

            /* Create gadgets for each track */
            for(i = 0; i < dataTrackCount; i++)
            {

                /* Create TrackHeader gadget */
                gdata->_tracks[i]._trackHeader = //CreateHeader();

                 NewObject(TRACKHEADER_GetClass(), NULL, TAG_END);

                /* Create TrackArea */
                gdata->_tracks[i]._trackArea = NewObject(TRACKAREA_GetClass(), NULL, TAG_END);

                /* data we sync: */
                gdata->_tracks[i]._dataTrack = project->tracks->items[i];

                if(!gdata->_tracks[i]._trackHeader || !gdata->_tracks[i]._trackArea)
                {
                    /* Failed to create gadgets, cleanup and abort */
                    TrackListAreaUi_DisposeGadgets(gdata);
                    return;
                }
                //
//                if(gdata->window)
//                {
//                    bdbprintf(" *** added with windows !\n");
//                    ipos = AddGadget(gdata->window,gdata->_tracks[i]._trackHeader,0/*ipos+1*/ );
//                    ipos = AddGadget(gdata->window,gdata->_tracks[i]._trackArea,0/*ipos+1*/);
//                } else
//                {
//                    bdbprintf(" *** no window yet ??\n");
//                }

            }
        }
    }
    bdbprintf("before RefreshGadget(gad)\n");
    //RefreshGList(Gad,gdata->window,NULL,1);


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
    //TrackListAreaUi_updateTrackListUiToData(Gad);
    TrackListAreaUi *gdata;
    AukAProject *project;
    ULONG dataTrackCount;
    ULONG i,nbTracksAlreadyInSync;
    TrackChild*ntracks;

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
    dataTrackCount = AukArray_GetCount(project->tracks);
    if(dataTrackCount != gdata->_trackCount +1 )
    {
        // general update
        TrackListAreaUi_updateTrackListUiToData(Gad);
        return;
    }

    /* If count changed, reallocate arrays */

    /* Allocate new arrays */
    ntracks = (TrackChild*)AllocVec(dataTrackCount * sizeof(TrackChild), MEMF_CLEAR);
    if(!ntracks)
    {
        /* Allocation failed, cleanup */
        TrackListAreaUi_DisposeGadgets(gdata);
        return;
    }
    if( gdata->_trackCount>0)
    {
        memcpy(ntracks,gdata->_tracks,sizeof(TrackChild)*gdata->_trackCount);
    }
    FreeVec(gdata->_tracks);
    gdata->_tracks = ntracks;




    /* Create gadgets for this track */
    i = gdata->_trackCount;
    {
         UWORD ipos = 65534;
        /* Create TrackHeader gadget */
        gdata->_tracks[i]._trackHeader = // CreateHeader();

         NewObject(TRACKHEADER_GetClass(), NULL, TAG_END);

        /* Create TrackArea */
        gdata->_tracks[i]._trackArea = NewObject(TRACKAREA_GetClass(), NULL, TAG_END);

        /* data we sync: */
        gdata->_tracks[i]._dataTrack = project->tracks->items[i];

        if(!gdata->_tracks[i]._trackHeader || !gdata->_tracks[i]._trackArea)
        {
            /* Failed to create gadgets, cleanup and abort */
            TrackListAreaUi_DisposeGadgets(gdata);
            return;
        }

//        if(gdata->window)
//        {
//            bdbprintf(" *** added with windows ! %d\n",(int)ipos);
//            ipos = AddGadget(gdata->window,gdata->_tracks[i]._trackHeader,0/*ipos+1*/ );
//            ipos = AddGadget(gdata->window,gdata->_tracks[i]._trackArea,0/*ipos+1*/);
//        } else
//        {
//            bdbprintf(" *** no window yet ??\n");
//        }
    }

    gdata->_trackCount = dataTrackCount;

    // - - - - -
    bdbprintf("before RefreshGadget(gad)\n");
    //RefreshGList(Gad,gdata->window,NULL,1);

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
