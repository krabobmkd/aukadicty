
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

#include <aukobject.h>
#include <aukaproject.h>
#include "TimeRule/class_timerule_private.h"

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

/* Listener callback for project updates */
static void AukUpdate_FooterView(AukObject* listenerObject, AukObject* modifiedObject, void *userData, AukMessage *message)
{
    FooterView *fv = (FooterView *)userData;
    AukAProject *project = (AukAProject*)modifiedObject;

    (void)listenerObject; /* unused */

    if(!fv || !project || !message) return;

    switch(message->type)
    {
        case AUK_MSG_SELECTIONCHANGED:
        {
            AukSelection *sel = &project->selection;
            char startBuf[32], endBuf[32];
            /* TIMESCALE_MSEC = 1 for millisecond precision */
            TimeRule_FormatTime(sel->_start, startBuf, 1);
            TimeRule_FormatTime(sel->_end, endBuf, 1);
            FooterView_UpdateSelection(fv, startBuf, endBuf);
        }
        break;
        default:
        break;
    }
}

void FooterView_SetProject(FooterView *fv, AukObjectPtr project)
{
    AukAProject *proj;
    if(!fv || !project) return;

    proj = (AukAProject *)project;

    /* Remove listener from previous project if any */
    if(fv->project && fv->updateListener)
    {
        AukObject_RemoveListener((AukObject *)fv->project, fv->updateListener);
    }

    fv->project = project;

    /* Create listener object if not already created */
    if(!fv->updateListener)
    {
        AukObject_New(&fv->updateListener);
    }

    /* Register listener on new project */
    if(fv->updateListener)
    {
        AukObject_AddListener(&proj->base.base,
                            fv->updateListener,
                            (void*)fv,
                            &AukUpdate_FooterView);

        /* Initialize footer with current selection */
        {
            AukSelection *sel = &proj->selection;
            char startBuf[32], endBuf[32];
            TimeRule_FormatTime(sel->_start, startBuf, 1);
            TimeRule_FormatTime(sel->_end, endBuf, 1);
            FooterView_UpdateSelection(fv, startBuf, endBuf);
        }
    }
}

void CloseFooterView(FooterView *fv)
{
    if(!fv) return;

    /* Remove listener from project */
    if(fv->project && fv->updateListener)
    {
        AukObject_RemoveListener((AukObject *)fv->project, fv->updateListener);
    }

    /* Release the listener and project references */
    AukObjectPtr_Release(&fv->updateListener);
    AukObjectPtr_Release(&fv->project);

    fv->project = NULL;

    /* Disposing mainHl will cascade to all child objects */
    /* Note: Actually handled by main window disposal */
}
