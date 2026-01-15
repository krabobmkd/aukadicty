/**
 * This file contains lib inits that create the class
 * and would OpenLibrary() for other dependencies.
 * word XXX_STATICLINK decides if this is used as the header for a shared .class file
 * or if it is statically linked.
 */

#include <exec/alerts.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/layers.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>

#include <proto/layout.h>
#include <gadgets/layout.h>

#include <proto/button.h>
#include <gadgets/button.h>

#include <proto/slider.h>
#include <gadgets/slider.h>

#include "class_trackheader_private.h"

#ifdef USE_BEVEL_FRAME
    #include <proto/bevel.h>
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

/* Message union for dispatcher */
typedef union MsgUnion
{
  ULONG  MethodID;
  struct opSet        opSet;
  struct opUpdate     opUpdate;
  struct opGet        opGet;
  struct gpHitTest    gpHitTest;
  struct gpRender     gpRender;
  struct gpInput      gpInput;
  struct gpGoInactive gpGoInactive;
  struct gpLayout     gpLayout;
  struct gpDomain     gpDomain;
} *Msgs;


typedef ULONG (*REHOOKFUNC)();

#include <stdlib.h>
#include <string.h>


// this is the only global writtable we should see in the whole class binary !
struct IClass   *TrackHeaderClassPtr=NULL;
struct IClass   *HeaderButtonClassPtr=NULL;
struct IClass   *HeaderSliderClassPtr=NULL;

const char TrackHeaderSuperClassID[]=TrackHeader_SUPERCLASS_ID;


BOOL TrackHeader_OpenLibs_Dependencies(void)
{

    return TRUE;
}

void TrackHeader_CloseLibs_Dependencies(void)
{

}
//==========================================================================================
// does not need to be exact, we just want the function pointer:
ULONG ASM SAVEDS TrackHeader_Dispatcher(
                    REG(a0,struct IClass *C),
                    REG(a2,struct Gadget *Gad),
                    REG(a1,union MsgUnion *M));

ULONG ASM SAVEDS HeaderButton_Dispatcher(
                    REG(a0,struct IClass *C),
                    REG(a2,struct Gadget *Gad),
                    REG(a1,union MsgUnion *M));

ULONG ASM SAVEDS HeaderSlider_Dispatcher(
                    REG(a0,struct IClass *C),
                    REG(a2,struct Gadget *Gad),
                    REG(a1,union MsgUnion *M));

// static version:
struct IClass   *TRACKHEADER_GetClass()
{
    return TrackHeaderClassPtr;
}
struct IClass   *HEADERBUTTON_GetClass()
{
    return HeaderButtonClassPtr;
}
struct IClass   *HEADERSLIDER_GetClass()
{
    return HeaderSliderClassPtr;
}

//====================================================================================


// just use this one once when static link
int TrackHeaderStaticInit()
{ 
   if(!TrackHeader_OpenLibs_Dependencies()) return 0;
    //if(TrackHeaderClassPtr=MakeClass(NULL,TrackHeaderSuperClassID,0,sizeof(TrackHeader),0))
    // MakeClass( ClassID, SuperClassID, SuperClassPtr,InstanceSize, Flags )


    if((TrackHeaderClassPtr=MakeClass(NULL,NULL,LAYOUT_GetClass(),sizeof(TrackHeader),0))!=NULL)
//    if(TrackHeaderClassPtr=MakeClass(NULL,"gadgetclass",0,sizeof(TrackHeader),0))
    {
      TrackHeaderClassPtr->cl_Dispatcher.h_Entry=(REHOOKFUNC)TrackHeader_Dispatcher;
     // do not AddClass() when static, no need to publish, TrackHeaderClassPtr will be enough.

        HeaderButtonClassPtr=MakeClass(NULL,NULL,BUTTON_GetClass(),sizeof(TrackHeaderButton),0);
        if(HeaderButtonClassPtr)
        {
            HeaderButtonClassPtr->cl_Dispatcher.h_Entry=(REHOOKFUNC)HeaderButton_Dispatcher;
        }

        HeaderSliderClassPtr=MakeClass(NULL,NULL,SLIDER_GetClass(),sizeof(TrackHeaderSlider),0);
        if(HeaderSliderClassPtr)
        {
            HeaderSliderClassPtr->cl_Dispatcher.h_Entry=(REHOOKFUNC)HeaderSlider_Dispatcher;
        }
      /* Success */
      return(1);
    }
    return 0;
}

void TrackHeaderStaticClose()
{
    TrackHeader_CloseLibs_Dependencies();
    if(HeaderSliderClassPtr)
    {
      FreeClass(HeaderSliderClassPtr);
      HeaderSliderClassPtr = NULL;
    }
    if(HeaderButtonClassPtr)
    {
      FreeClass(HeaderButtonClassPtr);
      HeaderButtonClassPtr = NULL;
    }
    if(TrackHeaderClassPtr)
    {
      FreeClass(TrackHeaderClassPtr);
      TrackHeaderClassPtr = NULL;
    }
}


