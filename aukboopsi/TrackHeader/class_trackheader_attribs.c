
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/dos.h>
#include <stdio.h>
#include <string.h>

#include <clib/alib_protos.h>
#include <intuition/intuition.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/gadgetclass.h>

#include "class_trackheader.h"
#include "class_trackheader_private.h"

#include <gadgets/slider.h>
#include <utility/tagitem.h>

#include "gadgetid.h"

/* This can be reallocated, so this is shared like this */
extern struct Window *CurrentMainWindow;

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

void HeaderButton_Notify(Class *C, struct Gadget *Gad, struct GadgetInfo *ginfo);

ULONG TrackHeader_GetAttr(Class *C, struct Gadget *Gad, struct opGet *Get)
{
  ULONG retval=1;
  int   DoSuperCall=0;
  TrackHeader *gdata;
  ULONG *data;

  gdata=INST_DATA(C, Gad);

  data=Get->opg_Storage;

  switch(Get->opg_AttrID)
  {
     case TRACKHEADER_TrackIndex:
         *data = (LONG)gdata->_trackIndex;
     break;
     case TRACKHEADER_Name:
         *data = (LONG)0;
     break;
     case TRACKHEADER_Pan:
         *data = (LONG)0;
     break;
     case TRACKHEADER_Volume:
         *data = (LONG)0;
     break;

    // super class gadget things. would manage attribs selected/hightlighted, ...
    default:
        DoSuperCall = 1;
      // everything we don't manage directly is managed by supercall.
  }
  if(DoSuperCall)  retval=DoSuperMethodA(C, (APTR)Gad, (APTR)Get);

  return(retval);
}

ULONG TrackHeader_SetAttrs(Class *C, struct Gadget *Gad, struct opSet *Set)
{
  struct TagItem *tag;
  ULONG data; // for SetAttribs, retval means if anything needed redraw.
  TrackHeader *gdata;
  ULONG actuallydone=0;

  gdata=INST_DATA(C, Gad);

 // set can use a list of attribs to change, so we manage this with a loop.
 // this also allows to have just one draw refresh for a set of change.
  for( tag = Set->ops_AttrList ;
        tag->ti_Tag != TAG_END ;
        tag++
   )
  {
    data=tag->ti_Data;

    switch(tag->ti_Tag)
    {
      case TRACKHEADER_StyleSheet:
        /* data points to AukStyle, extract the style member */
        gdata->_style = (struct AukStyle *)data;
        actuallydone = 1;
        break;
        case TRACKHEADER_TrackIndex:
        {
            ULONG iTrack = data;
            gdata->_trackIndex = iTrack;

            /* track id shifted, propagate and change GA_ID to all active children */
            if(gdata->subs[THS_CloseButton])
                SetAttrs(gdata->subs[THS_CloseButton],GA_ID,
                    GAD_TRACKHEADER_BASE|GAD_TRACKHEADER_CLOSE|(iTrack<<4),
                    TAG_END);

            if(gdata->subs[THS_NameButton])
                SetAttrs(gdata->subs[THS_NameButton],GA_ID,
                    GAD_TRACKHEADER_BASE|GAD_TRACKHEADER_NAME|(iTrack<<4),
                    TAG_END);

            if(gdata->subs[THS_SilencerBt])
                SetAttrs(gdata->subs[THS_SilencerBt],GA_ID,
                    GAD_TRACKHEADER_BASE|GAD_TRACKHEADER_SILENCER|(iTrack<<4),
                    TAG_END);

            if(gdata->subs[THS_SoloBt])
                SetAttrs(gdata->subs[THS_SoloBt],GA_ID,
                    GAD_TRACKHEADER_BASE|GAD_TRACKHEADER_SOLO|(iTrack<<4),
                    TAG_END);


            if(gdata->subs[THS_VolumeSlider])
                SetAttrs(gdata->subs[THS_VolumeSlider],GA_ID,
                    GAD_TRACKHEADER_BASE|GAD_TRACKHEADER_VOL|(iTrack<<4),
                    TAG_END);

            if(gdata->subs[THS_PanSlider])
                SetAttrs(gdata->subs[THS_PanSlider],GA_ID,
                    GAD_TRACKHEADER_BASE|GAD_TRACKHEADER_PAN|(iTrack<<4),
                    TAG_END);

        } break;
       case  TRACKHEADER_Name:
       {
          struct Gadget *btname =  (struct Gadget *) gdata->subs[THS_NameButton];
          if(btname && data!=0)
          {
            char tname[32];
            char *name= (char *)data;
            if(strlen(name)>7)
            {
                snprintf(tname,7,"%s",name);
                strcat(tname,"..");
                name = &tname[0];
            }
 bdbprintf("*** TRACKHEADER_Name %s\n",name);
            SetAttrs(btname,GA_Text,(ULONG)name,TAG_END);
          }
          actuallydone = 1;
       }
       break;
       case TRACKHEADER_Pan:
       {
            ULONG prevSliderLevel;
            ULONG newlevel = data>>9; // data scale is 1<<16 , slider is 1<<7
            struct Gadget *slider =  (struct Gadget *) gdata->subs[THS_PanSlider];
            if(slider)
            {
                GetAttr(SLIDER_Level,slider,&prevSliderLevel);
                if(prevSliderLevel != newlevel)
                {
                    SetGadgetAttrs(slider,CurrentMainWindow,NULL,SLIDER_Level,newlevel);
                }
            }
       }
       break;
       case TRACKHEADER_Volume:
       {
            ULONG prevSliderLevel;
            ULONG newlevel = data>>9; // data scale is 1<<16 , slider is 1<<7
            struct Gadget *slider =  (struct Gadget *) gdata->subs[THS_VolumeSlider];
            if(slider)
            {
                GetAttr(SLIDER_Level,slider,&prevSliderLevel);
                if(prevSliderLevel != newlevel)
                {
                    SetGadgetAttrs(slider,CurrentMainWindow,NULL,SLIDER_Level,newlevel);
                }
            }

       }
       break;
       case TRACKHEADER_Flags:
        {
            int flags = data;
            struct Gadget *silbt =  (struct Gadget *) gdata->subs[THS_SilencerBt];
            struct Gadget *solobt =  (struct Gadget *) gdata->subs[THS_SoloBt];
            bdbprintf(" * * * set TRACKHEADER_Flags:%08x\n",flags);
            if(silbt)
            {
                int curstate;
                int setstate = flags & 1; // AukTrack AukTrackFlag_Silent
                GetAttr(GA_SELECTED,silbt,&curstate);
                if(curstate != setstate)
                {
                    SetGadgetAttrs(silbt,CurrentMainWindow,NULL,GA_SELECTED,setstate);
                }
            }
            if(solobt)
            {
                int curstate;
                int setstate = (flags & 2) !=0; // AukTrack AukTrackFlag_Silent
                GetAttr(GA_SELECTED,solobt,&curstate);
                if(curstate != setstate)
                {
                    SetGadgetAttrs(solobt,CurrentMainWindow,NULL,GA_SELECTED,setstate);
                }
            }
        }
       break;
    default:
        break;

    } // end switch
  } // end for

  return(actuallydone);
}

