
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <intuition/screens.h>
#include <intuition/icclass.h>
#include <intuition/gadgetclass.h>

#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/utility.h>

#include <proto/layout.h>
#include <gadgets/layout.h>

#include <proto/button.h>
#include <gadgets/button.h>

#include "compilers.h"
#include "HeaderView.h"
#include "gadgetid.h"
#include "auklocale.h"

/* This can be reallocated, so this is shared like this */
extern struct Window *CurrentMainWindow;

void cleanexit(const char *pmessage);

void CreateHeaderView(HeaderView *hv, struct DrawInfo *drawInfo,
                      Object *appModel,
                       AukStyle *stylesheet)
{
    Object *editCol1,*editCol2,*editCol3;

    hv->pstyleSheet = stylesheet;
    hv->appModel = appModel;

    /* Transport control buttons */
    hv->btRewind = NewObject(BUTTON_GetClass(), NULL,
                        GA_DrawInfo, drawInfo,
                        GA_ID, GAD_HEADER_REWIND,
                        GA_RelVerify, TRUE,
                        GA_Text, (ULONG)"|<<",
                        BUTTON_Justification, BCJ_CENTER,
                        ICA_TARGET, appModel,
                        TAG_END);

    hv->btStop = NewObject(BUTTON_GetClass(), NULL,
                        GA_DrawInfo, drawInfo,
                        GA_ID, GAD_HEADER_STOP,
                        GA_RelVerify, TRUE,
                        GA_Text, (ULONG)"[]",
                        BUTTON_Justification, BCJ_CENTER,
                        ICA_TARGET, appModel,
                        TAG_END);

    hv->btPlay = NewObject(BUTTON_GetClass(), NULL,
                        GA_DrawInfo, drawInfo,
                        GA_ID, GAD_HEADER_PLAY,
                        GA_RelVerify, TRUE,
                        GA_Text, (ULONG)">",
                        BUTTON_Justification, BCJ_CENTER,
                        ICA_TARGET, appModel,
                        TAG_END);

    hv->btPause = NewObject(BUTTON_GetClass(), NULL,
                        GA_DrawInfo, drawInfo,
                        GA_ID, GAD_HEADER_PAUSE,
                        GA_RelVerify, TRUE,
                        GA_Text, (ULONG)"||",
                        BUTTON_Justification, BCJ_CENTER,
                        ICA_TARGET, appModel,
                        TAG_END);

    hv->btForward = NewObject(BUTTON_GetClass(), NULL,
                        GA_DrawInfo, drawInfo,
                        GA_ID, GAD_HEADER_FORWARD,
                        GA_RelVerify, TRUE,
                        GA_Text, (ULONG)">>|",
                        BUTTON_Justification, BCJ_CENTER,
                        ICA_TARGET, appModel,
                        TAG_END);

    /* Transport controls horizontal layout */
    hv->transportLayout = NewObject(LAYOUT_GetClass(), NULL,
                        LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
                        LAYOUT_EvenSize, TRUE,
                        LAYOUT_BevelStyle, BVS_GROUP,
                        LAYOUT_SpaceOuter, TRUE,
                        LAYOUT_SpaceInner, TRUE,
                        LAYOUT_AddChild, hv->btRewind,
                        LAYOUT_AddChild, hv->btStop,
                        LAYOUT_AddChild, hv->btPlay,
                        LAYOUT_AddChild, hv->btPause,
                        LAYOUT_AddChild, hv->btForward,
                        TAG_END);

    /* Edit mode buttons (3x2 grid) - PushButton (toggle) style, mutually exclusive */
    hv->btEditModes[EDITMODE_SELECT] = NewObject(BUTTON_GetClass(), NULL,
                        GA_DrawInfo, drawInfo,
                        GA_ID, GAD_HEADER_SELECTTOOL,
                        GA_RelVerify, TRUE,
                        GA_Selected, TRUE,  /* Selection tool selected by default */
                        GA_Text, (ULONG)LOC(MSG_EDITMODE_SELECTTOOL),
                        BUTTON_PushButton, TRUE,                        
                        BUTTON_Justification, BCJ_CENTER,
                        ICA_TARGET, appModel,
                        TAG_END);

    hv->btEditModes[EDITMODE_VOLUME] = NewObject(BUTTON_GetClass(), NULL,
                        GA_DrawInfo, drawInfo,
                        GA_ID, GAD_HEADER_VOLUMEENV,
                        GA_RelVerify, TRUE,
                        GA_Text, (ULONG)LOC(MSG_EDITMODE_VOLUMEENV),
                        BUTTON_PushButton, TRUE,
                        BUTTON_Justification, BCJ_CENTER,
                        ICA_TARGET, appModel,
                        TAG_END);

    hv->btEditModes[EDITMODE_COPY] = NewObject(BUTTON_GetClass(), NULL,
                        GA_DrawInfo, drawInfo,
                        GA_ID, GAD_HEADER_COPY,
                        GA_RelVerify, TRUE,
                        GA_Text, " ", //(ULONG)LOC(MSG_EDITMODE_COPY),
                        BUTTON_PushButton, TRUE,
                        BUTTON_Justification, BCJ_CENTER,
                        ICA_TARGET, appModel,
                        TAG_END);

    hv->btEditModes[EDITMODE_ZOOM] = NewObject(BUTTON_GetClass(), NULL,
                        GA_DrawInfo, drawInfo,
                        GA_ID, GAD_HEADER_ZOOMTOOL,
                        GA_RelVerify, TRUE,
                        GA_Text, (ULONG)LOC(MSG_EDITMODE_ZOOMTOOL),
                        BUTTON_PushButton, TRUE,
                        BUTTON_Justification, BCJ_CENTER,
                        ICA_TARGET, appModel,
                        TAG_END);

    hv->btEditModes[EDITMODE_TIMESLIDE] = NewObject(BUTTON_GetClass(), NULL,
                        GA_DrawInfo, drawInfo,
                        GA_ID, GAD_HEADER_TIMESLIDE,
                        GA_RelVerify, TRUE,
                        GA_Text, (ULONG)LOC(MSG_EDITMODE_TIMESLIDE),
                        BUTTON_PushButton, TRUE,
                        BUTTON_Justification, BCJ_CENTER,
                        ICA_TARGET, appModel,
                        TAG_END);

    hv->btEditModes[EDITMODE_PASTE] = NewObject(BUTTON_GetClass(), NULL,
                        GA_DrawInfo, drawInfo,
                        GA_ID, GAD_HEADER_PASTE,
                        GA_RelVerify, TRUE,
                        GA_Text, " ", // (ULONG)LOC(MSG_EDITMODE_PASTE),
                        BUTTON_PushButton, TRUE,
                        BUTTON_Justification, BCJ_CENTER,
                        ICA_TARGET, appModel,
                        TAG_END);

    /* Set initial edit mode to Selection Tool */
    hv->currentEditMode = EDITMODE_SELECT;

    /* Create nested vertical layouts for the 3x2 grid */
    /* Layout: SelectTool  VolumeEnv  Copy   */
    /*         ZoomTool    TimeSlide  Paste  */
    editCol1 = NewObject(LAYOUT_GetClass(), NULL,
                        LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
                        LAYOUT_EvenSize, TRUE,
                        LAYOUT_SpaceInner, FALSE,
                        LAYOUT_AddChild, hv->btEditModes[EDITMODE_SELECT],
                        LAYOUT_AddChild, hv->btEditModes[EDITMODE_ZOOM],
                        TAG_END);

    editCol2 = NewObject(LAYOUT_GetClass(), NULL,
                        LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
                        LAYOUT_EvenSize, TRUE,
                        LAYOUT_SpaceInner, FALSE,
                        LAYOUT_AddChild,hv->btEditModes[EDITMODE_VOLUME],
                        LAYOUT_AddChild, hv->btEditModes[EDITMODE_TIMESLIDE],
                        TAG_END);

    editCol3 = NewObject(LAYOUT_GetClass(), NULL,
                        LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
                        LAYOUT_EvenSize, TRUE,
                        LAYOUT_SpaceInner, FALSE,
                        LAYOUT_AddChild, hv->btEditModes[EDITMODE_COPY],
                        LAYOUT_AddChild, hv->btEditModes[EDITMODE_PASTE],
                        TAG_END);

    /* Edit mode 3x2 grid layout */
    hv->editModeLayout = NewObject(LAYOUT_GetClass(), NULL,
                        LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
                        LAYOUT_EvenSize, TRUE,
                        LAYOUT_BevelStyle, BVS_GROUP,
                        LAYOUT_SpaceOuter, TRUE,
                        LAYOUT_SpaceInner, TRUE,
                        LAYOUT_AddChild, editCol1,
                        LAYOUT_AddChild, editCol2,
                        LAYOUT_AddChild, editCol3,
                        TAG_END);

    /* Empty spacer layout (filler) */
    hv->spacerLayout = NewObject(LAYOUT_GetClass(), NULL,
                        LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
                        LAYOUT_BevelStyle, BVS_NONE,
                        TAG_END);

    /* Main horizontal layout combining all sections */
    hv->mainHl = NewObject(LAYOUT_GetClass(), NULL,
                        LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
                        LAYOUT_BevelStyle, BVS_NONE,
                        LAYOUT_SpaceOuter, TRUE,
                        LAYOUT_SpaceInner, TRUE,
                        LAYOUT_AddChild, hv->transportLayout,
                            CHILD_WeightedWidth, 0,
                        LAYOUT_AddChild, hv->editModeLayout,
                            CHILD_WeightedWidth, 0,
                        LAYOUT_AddChild, hv->spacerLayout,
                            CHILD_WeightedWidth, 1,
                        TAG_END);

    if (!hv->mainHl) cleanexit("Can't create HeaderView main layout");
}

void CloseHeaderView(HeaderView *hv)
{
    // note: no need, it's done by the main dispose.

    /* Disposing mainHl will cascade to all child objects */
//    if (hv->mainHl) {
//        DisposeObject(hv->mainHl);
//        hv->mainHl = NULL;
//    }
}

/* return new mode when it changes, else -1 keep same */
int HeaderView_ListenMessage(HeaderView *hv,struct opUpdate *M, ULONG gadId)
{
    struct TagItem *ptag;
    ULONG selected=-1;
    int modeForThisButton = (gadId - GAD_HEADER_EDITMODE_FIRST);
    if(modeForThisButton<0 || modeForThisButton>=EDITMODE_COUNT) return -1;
    /*
        Here we know it's a message from a button, but could be
        any information. We seek change to GA_SELECTED;
    */
    if((ptag = FindTagItem( GA_SELECTED,M->opu_AttrList ))!=NULL)
        selected = ptag->ti_Data;

    if(selected<0) return -1; /* no GA_SELECTED information in the message */

    if(selected==0)
    {
        /* message is bt unselected...  */
        ULONG currentBtState=0;
        Object *thatButton;
        if((int)hv->currentEditMode != (int)modeForThisButton) return -1; // normal
        /**/
        thatButton = hv->btEditModes[hv->currentEditMode];
        if(!thatButton) return -1;
        /* reclick the selected mode shouldnt remove its state (hack) */
        GetAttr(GA_SELECTED,thatButton,&currentBtState);

        if(currentBtState != GA_SELECTED)
        {
            SetGadgetAttrs((struct Gadget *)
                hv->btEditModes[hv->currentEditMode],
                    CurrentMainWindow,NULL,GA_SELECTED,TRUE);
        }
        return -1;
    }
    else // selected == 1
    {
        int i;
        /* message is bt selected...  */
        if(hv->currentEditMode == modeForThisButton) return -1; // already correct
        hv->currentEditMode = modeForThisButton;
        /* force unselect the other */
        for(i=0;i<EDITMODE_COUNT;i++)
        {
            if(i ==  (int) hv->currentEditMode) continue;
            SetGadgetAttrs((struct Gadget *)
                hv->btEditModes[i],
                    CurrentMainWindow,NULL,GA_SELECTED,FALSE);
        }
        /* should send message here */
        return modeForThisButton;
    }
    return -1;
}
AukEditMode HeaderView_GetEditMode(HeaderView *hv)
{
    if (!hv) return EDITMODE_SELECT;
    return hv->currentEditMode;
}
