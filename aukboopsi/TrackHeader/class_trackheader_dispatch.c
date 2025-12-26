

#ifdef __SASC
    #include <clib/alib_protos.h>
#else
    // GCC, vbcc
    #include "minialib.h"
#endif
#include <proto/dos.h>
//#include <proto/utility.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/gadgetclass.h>

#include "class_trackheader.h"
#include "class_trackheader_private.h"

#include <proto/layout.h>
#include <gadgets/layout.h>

#include <proto/button.h>
#include <gadgets/button.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/utility.h>

typedef union MsgUnion
{
  ULONG  MethodID;
  // from classusr.h or gadgetclass.h, all starts with MethodID.
  struct opSet        opSet;
  struct opUpdate     opUpdate;
  struct opGet        opGet;
  struct gpHitTest    gpHitTest;
  struct gpRender     gpRender;
  struct gpInput      gpInput;
  struct gpGoInactive gpGoInactive;
  struct gpLayout     gpLayout;
} *Msgs;

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

/** WATCH OUT ! boopsi docs says:
*  "the rkmmodelclass dispatcher must be able to run on Intuition's context,
*  which puts some limitations on what the dispatcher is permitted to do:
*  it can't use dos.library, it can't wait on application signals or message ports
* and it can't call any Intuition functions which might wait on Intuition."
*/
ULONG ASM SAVEDS TrackHeader_Dispatcher(
                    REG(a0,struct IClass *C),
                    REG(a2,struct Gadget *Gad),
                    REG(a1,union MsgUnion *M))
{
  TrackHeader *gdata;
  ULONG retval=0;
  gdata=INST_DATA(C, Gad);

  switch(M->MethodID)
  {
    case OM_NEW:
      {
        Object *VolumeRule,*CloseButton,*NameLabel,*VolumeSlider,*PanSlider,
                *LeftVertlayout,*CloseAndNameHl;

        CloseButton = NewObject( BUTTON_GetClass(),NULL,
                                    GA_Text, "X",
                                  //  GA_ID,GAD_BUTTON_ABOUT,
                                    GA_RelVerify, TRUE,
                         //           GA_Disabled,TRUE,
                        // BUTTON_BevelStyle,BVS_NONE,
                        // BUTTON_Transparent, TRUE,
                                TAG_END);
        NameLabel = NewObject( BUTTON_GetClass(),NULL,
                                    GA_Text, "Name",
                                  //  GA_ID,GAD_BUTTON_ABOUT,
                                    GA_RelVerify, TRUE,
                         //           GA_Disabled,TRUE,
                        // BUTTON_BevelStyle,BVS_NONE,
                        // BUTTON_Transparent, TRUE,
                                TAG_END);


        CloseAndNameHl  = (Object *)NewObject( LAYOUT_GetClass(), NULL,
                    LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
            LAYOUT_DeferLayout, TRUE, // Layout refreshes done on task's context (by thewindow class)
            LAYOUT_BottomSpacing, 0,
            LAYOUT_TopSpacing,0,
            LAYOUT_LeftSpacing,0,
            LAYOUT_RightSpacing,0,
            LAYOUT_InnerSpacing,1,
                    LAYOUT_AddChild, CloseButton,
                     CHILD_WeightedWidth,0,
                    LAYOUT_AddChild, NameLabel,
                     CHILD_WeightedWidth,1,
                    TAG_DONE);

        VolumeSlider = NewObject( BUTTON_GetClass(),NULL,
                                    GA_Text, "VolSlider",
                                  //  GA_ID,GAD_BUTTON_ABOUT,
                                    GA_RelVerify, TRUE,
                         //           GA_Disabled,TRUE,
                        // BUTTON_BevelStyle,BVS_NONE,
                        // BUTTON_Transparent, TRUE,
                                TAG_END);

        // in this paragraph we create the layout hierarchy
        LeftVertlayout  = (Object *)NewObject( LAYOUT_GetClass(), NULL,
                    LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
            LAYOUT_BevelStyle,BVS_NONE,
            LAYOUT_DeferLayout, TRUE, // Layout refreshes done on task's context (by thewindow class)
            LAYOUT_BottomSpacing, 0,
            LAYOUT_TopSpacing,0,
            LAYOUT_LeftSpacing,0,
            LAYOUT_RightSpacing,0,
            LAYOUT_InnerSpacing,1,

//                    LAYOUT_BevelStyle, /*BVS_GROUP*/BVS_NONE,
                    LAYOUT_AddChild, CloseAndNameHl,
                     CHILD_WeightedHeight,0,

                    LAYOUT_AddChild, VolumeSlider,
                     CHILD_WeightedHeight,1,

                    TAG_DONE);

        VolumeRule = NewObject( BUTTON_GetClass(),NULL,
                                    GA_Text, "VR",
                                  //  GA_ID,GAD_BUTTON_ABOUT,
                                    GA_RelVerify, TRUE,
                         //           GA_Disabled,TRUE,
                        // BUTTON_BevelStyle,BVS_NONE,
                        // BUTTON_Transparent, TRUE,
                                TAG_END);

/*
    Object *CloseButton;
    Object *NameLabel;

    Object  *VolumeSlider;
    Object  *PanSlider;

    // ------------- H
    Object *VolumeRule;

*/
        {

        // we are a layout that forces its parameter...
        ULONG tags[]={
            LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
            LAYOUT_DeferLayout, TRUE, // Layout refreshes done on task's context (by thewindow class)
            LAYOUT_BottomSpacing, 0,
            LAYOUT_TopSpacing,0,
            LAYOUT_LeftSpacing,0,
            LAYOUT_RightSpacing,0,
            LAYOUT_InnerSpacing,0,

                    LAYOUT_BevelStyle, /*BVS_GROUP*/BVS_NONE,
                    LAYOUT_AddChild, LeftVertlayout,
                     CHILD_WeightedWidth,1,
                    LAYOUT_AddChild, VolumeRule,
                     CHILD_WeightedWidth,0,
                    TAG_DONE
        };
        struct opSet opset;
        opset.MethodID = OM_NEW;
        opset.ops_GInfo = M->opSet.ops_GInfo;
        opset.ops_AttrList = &tags[0];

          if(Gad=(struct Gadget *)DoSuperMethodA(C,(Object *)Gad,&opset))
          {
            gdata=INST_DATA(C, Gad);
            bdbprintf_new("TrackHeader", Gad);

            gdata->CloseButton = CloseButton;
            /* means new object OK so far: */
            retval=(ULONG)Gad;
          }
        }
      }
      break;

    case OM_UPDATE:
    case OM_SET:
      retval=DoSuperMethodA(C,(Object *)Gad,(Msg)M);
      TrackHeader_SetAttrs(C,Gad,(struct opSet *)M);
     break;

    case OM_GET:
      retval=DoSuperMethodA(C,(Object *)Gad,(Msg)M);
      //TrackHeader_GetAttr(C,Gad,(struct opGet *)M);
     break;

    case OM_DISPOSE:
        bdbprintf_dispose("TrackHeader", Gad);
      retval=DoSuperMethodA(C,(Object *)Gad,(Msg)M);
      break;
    default:
      // for anything, use default layout behaviour.
      retval=DoSuperMethodA(C,(Object *)Gad,(Msg)M);
      break;
  }
  return(retval);
}

