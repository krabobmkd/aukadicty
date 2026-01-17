
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
#include "FooterView.h"

/* This can be reallocated, so this is shared like this */
extern struct Window *CurrentMainWindow;

void cleanexit(const char *pmessage);

void CreateFooterView(FooterView *fv, struct DrawInfo *drawInfo,
                      Object *appModel,
                      AukStyle *style)
{
    fv->pstyleSheet = style;
    fv->appModel = appModel;

    /* Mixing frequency label */
    fv->labelFrequency = NewObject(BUTTON_GetClass(), NULL,
                        GA_DrawInfo, drawInfo,
                        GA_ReadOnly, TRUE,
                        BUTTON_BevelStyle, BVS_NONE,
                        BUTTON_Transparent, TRUE,
                        BUTTON_Justification, BCJ_LEFT,
                        GA_Text, (ULONG)"44100 Hz",
                        TAG_END);

    /* Selection start time label */
    fv->labelSelStart = NewObject(BUTTON_GetClass(), NULL,
                        GA_DrawInfo, drawInfo,
                        GA_ReadOnly, TRUE,
                        BUTTON_BevelStyle, BVS_NONE,
                        BUTTON_Transparent, TRUE,
                        BUTTON_Justification, BCJ_CENTER,
                        GA_Text, (ULONG)"Start: 0.000s",
                        TAG_END);

    /* Selection end time label */
    fv->labelSelEnd = NewObject(BUTTON_GetClass(), NULL,
                        GA_DrawInfo, drawInfo,
                        GA_ReadOnly, TRUE,
                        BUTTON_BevelStyle, BVS_NONE,
                        BUTTON_Transparent, TRUE,
                        BUTTON_Justification, BCJ_CENTER,
                        GA_Text, (ULONG)"End: 0.000s",
                        TAG_END);

    /* Playback position label */
    fv->labelPlayPos = NewObject(BUTTON_GetClass(), NULL,
                        GA_DrawInfo, drawInfo,
                        GA_ReadOnly, TRUE,
                        BUTTON_BevelStyle, BVS_NONE,
                        BUTTON_Transparent, TRUE,
                        BUTTON_Justification, BCJ_RIGHT,
                        GA_Text, (ULONG)"Pos: 0.000s",
                        TAG_END);

    /* Main horizontal layout */
    fv->mainHl = NewObject(LAYOUT_GetClass(), NULL,
                        LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
                        LAYOUT_BevelStyle, BVS_NONE,
                        LAYOUT_SpaceOuter, TRUE,
                        LAYOUT_SpaceInner, TRUE,
                        LAYOUT_AddChild, fv->labelFrequency,
                            CHILD_WeightedWidth, 0,
                            CHILD_MinWidth, 80,
                        LAYOUT_AddChild, fv->labelSelStart,
                            CHILD_WeightedWidth, 1,
                        LAYOUT_AddChild, fv->labelSelEnd,
                            CHILD_WeightedWidth, 1,
                        LAYOUT_AddChild, fv->labelPlayPos,
                            CHILD_WeightedWidth, 1,
                        TAG_END);

    if (!fv->mainHl) cleanexit("Can't create FooterView main layout");
}

void FooterView_UpdateFrequency(FooterView *fv, ULONG frequency)
{
    char buffer[32];
    if (!fv || !fv->labelFrequency) return;

    snprintf(buffer, 31, "%lu Hz", frequency);
    SetGadgetAttrs((struct Gadget *)fv->labelFrequency, CurrentMainWindow, NULL,
                   GA_Text, (ULONG)buffer,
                   TAG_END);
}

void FooterView_UpdateSelection(FooterView *fv, const char *startTime, const char *endTime)
{
    char bufferStart[64];
    char bufferEnd[64];
    if (!fv) return;

    if (fv->labelSelStart && startTime) {
        snprintf(bufferStart, 63, "Start: %s", startTime);
        SetGadgetAttrs((struct Gadget *)fv->labelSelStart, CurrentMainWindow, NULL,
                       GA_Text, (ULONG)bufferStart,
                       TAG_END);
    }

    if (fv->labelSelEnd && endTime) {
        snprintf(bufferEnd, 63, "End: %s", endTime);
        SetGadgetAttrs((struct Gadget *)fv->labelSelEnd, CurrentMainWindow, NULL,
                       GA_Text, (ULONG)bufferEnd,
                       TAG_END);
    }
}

void FooterView_UpdatePlayPosition(FooterView *fv, const char *position)
{
    char buffer[64];
    if (!fv || !fv->labelPlayPos || !position) return;

    snprintf(buffer, 63, "Pos: %s", position);
    SetGadgetAttrs((struct Gadget *)fv->labelPlayPos, CurrentMainWindow, NULL,
                   GA_Text, (ULONG)buffer,
                   TAG_END);
}

void CloseFooterView(FooterView *fv)
{
    /* Disposing mainHl will cascade to all child objects */
    /* Note: Actually handled by main window disposal */
}
