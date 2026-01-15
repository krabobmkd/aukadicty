
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <intuition/screens.h>
#include <intuition/icclass.h>

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

void cleanexit(const char *pmessage);

void CreateHeaderView(HeaderView *hv, struct DrawInfo *drawInfo,
                      Object *appModel,
                       AukStyle *stylesheet)
{
    Object *editCol1,*editCol2,*editCol3;

    hv->pstyleSheet = stylesheet;
    hv->appModel = appModel;
    hv->window = NULL;

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

    /* Edit mode buttons (3x2 grid) */
    hv->btEditMode1 = NewObject(BUTTON_GetClass(), NULL,
                        GA_DrawInfo, drawInfo,
                        GA_ID, GAD_HEADER_EDITMODE1,
                        GA_RelVerify, TRUE,
                        GA_Text, (ULONG)"1",
                        BUTTON_Justification, BCJ_CENTER,
                        ICA_TARGET, appModel,
                        TAG_END);

    hv->btEditMode2 = NewObject(BUTTON_GetClass(), NULL,
                        GA_DrawInfo, drawInfo,
                        GA_ID, GAD_HEADER_EDITMODE2,
                        GA_RelVerify, TRUE,
                        GA_Text, (ULONG)"2",
                        BUTTON_Justification, BCJ_CENTER,
                        ICA_TARGET, appModel,
                        TAG_END);

    hv->btEditMode3 = NewObject(BUTTON_GetClass(), NULL,
                        GA_DrawInfo, drawInfo,
                        GA_ID, GAD_HEADER_EDITMODE3,
                        GA_RelVerify, TRUE,
                        GA_Text, (ULONG)"3",
                        BUTTON_Justification, BCJ_CENTER,
                        ICA_TARGET, appModel,
                        TAG_END);

    hv->btEditMode4 = NewObject(BUTTON_GetClass(), NULL,
                        GA_DrawInfo, drawInfo,
                        GA_ID, GAD_HEADER_EDITMODE4,
                        GA_RelVerify, TRUE,
                        GA_Text, (ULONG)"4",
                        BUTTON_Justification, BCJ_CENTER,
                        ICA_TARGET, appModel,
                        TAG_END);

    hv->btEditMode5 = NewObject(BUTTON_GetClass(), NULL,
                        GA_DrawInfo, drawInfo,
                        GA_ID, GAD_HEADER_EDITMODE5,
                        GA_RelVerify, TRUE,
                        GA_Text, (ULONG)"5",
                        BUTTON_Justification, BCJ_CENTER,
                        ICA_TARGET, appModel,
                        TAG_END);

    hv->btEditMode6 = NewObject(BUTTON_GetClass(), NULL,
                        GA_DrawInfo, drawInfo,
                        GA_ID, GAD_HEADER_EDITMODE6,
                        GA_RelVerify, TRUE,
                        GA_Text, (ULONG)"6",
                        BUTTON_Justification, BCJ_CENTER,
                        ICA_TARGET, appModel,
                        TAG_END);

    /* Create nested vertical layouts for the 3x2 grid */
    editCol1 = NewObject(LAYOUT_GetClass(), NULL,
                        LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
                        LAYOUT_EvenSize, TRUE,
                        LAYOUT_SpaceInner, FALSE,
                        LAYOUT_AddChild, hv->btEditMode1,
                        LAYOUT_AddChild, hv->btEditMode4,
                        TAG_END);

    editCol2 = NewObject(LAYOUT_GetClass(), NULL,
                        LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
                        LAYOUT_EvenSize, TRUE,
                        LAYOUT_SpaceInner, FALSE,
                        LAYOUT_AddChild, hv->btEditMode2,
                        LAYOUT_AddChild, hv->btEditMode5,
                        TAG_END);

    editCol3 = NewObject(LAYOUT_GetClass(), NULL,
                        LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
                        LAYOUT_EvenSize, TRUE,
                        LAYOUT_SpaceInner, FALSE,
                        LAYOUT_AddChild, hv->btEditMode3,
                        LAYOUT_AddChild, hv->btEditMode6,
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
