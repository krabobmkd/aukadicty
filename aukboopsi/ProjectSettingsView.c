
#include <stdio.h>
#include <string.h>

#include <clib/alib_protos.h>

#include <intuition/screens.h>
#include <intuition/icclass.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>

#include <proto/window.h>
#include <classes/window.h>

#include <proto/layout.h>
#include <gadgets/layout.h>

#include <proto/button.h>
#include <gadgets/button.h>

#include <proto/label.h>
#include <images/label.h>

#include <proto/getfile.h>
#include <gadgets/getfile.h>

#include "compilers.h"
#include "ProjectSettingsView.h"

/* Helper to open a BOOPSI window object */
INLINE struct Window *boopsi_OpenWindow(Object *owin) {
    return (struct Window *)DoMethod(owin, WM_OPEN, NULL);
}

BOOL ProjectSettingsView_Init(ProjectSettingsView *psv,
                              struct Screen *screen,
                              const char *title)
{
    Object *projPlaceholder;

    if(!psv || !screen) return FALSE;

    memset(psv, 0, sizeof(ProjectSettingsView));
    psv->screen = screen;
    //psv->drawInfo = drawInfo;

    /* Create the GetFile gadget for temp directory selection */
    psv->tempDirGetFile = NewObject(GETFILE_GetClass(), NULL,
                            GA_ID, GAD_PROJSETTINGS_TEMPDIR,
                            GA_RelVerify, TRUE,
                            GETFILE_TitleText, (ULONG)"Select Temporary Directory",
                            GETFILE_ReadOnly, FALSE,
                            GETFILE_DrawersOnly, TRUE,
                            GETFILE_Drawer, (ULONG)"T:",  /* Default Amiga temp */
                            TAG_END);

    if(!psv->tempDirGetFile) return FALSE;

    /* Create label for the temp dir gadget */
    psv->tempDirLabel = NewObject(LABEL_GetClass(), NULL,
                            LABEL_Text, (ULONG)"Temp Directory:",
                            TAG_END);

    /* App Settings group layout */
    psv->appSettingsLayout = NewObject(LAYOUT_GetClass(), NULL,
                       // GA_DrawInfo, (ULONG)drawInfo,
                        LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
                        LAYOUT_BevelStyle, BVS_GROUP,
                        LAYOUT_Label, (ULONG)"App Settings",
                        LAYOUT_SpaceOuter, TRUE,
                        LAYOUT_SpaceInner, TRUE,

                        LAYOUT_AddChild, (ULONG)psv->tempDirGetFile,
                        CHILD_WeightedHeight, 0,
                        CHILD_Label, (ULONG)psv->tempDirLabel,

                        TAG_END);

    if(!psv->appSettingsLayout) {
        DisposeObject(psv->tempDirGetFile);
        if(psv->tempDirLabel) DisposeObject(psv->tempDirLabel);
        return FALSE;
    }

    /* Project Settings group layout (placeholder for now) */
    projPlaceholder = NewObject(BUTTON_GetClass(), NULL,
                //        GA_DrawInfo, (ULONG)drawInfo,
                        GA_ReadOnly, TRUE,
                        BUTTON_BevelStyle, BVS_NONE,
                        BUTTON_Transparent, TRUE,
                        GA_Text, (ULONG)"(no project settings yet)",
                        TAG_END);

    psv->projSettingsLayout = NewObject(LAYOUT_GetClass(), NULL,
                       // GA_DrawInfo, (ULONG)drawInfo,
                        LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
                        LAYOUT_BevelStyle, BVS_GROUP,
                        LAYOUT_Label, (ULONG)"Project Settings",
                        LAYOUT_SpaceOuter, TRUE,
                        LAYOUT_SpaceInner, TRUE,

                        LAYOUT_AddChild, (ULONG)projPlaceholder,
                        CHILD_WeightedHeight, 0,

                        TAG_END);

    if(!psv->projSettingsLayout) {
        DisposeObject(psv->appSettingsLayout);
        psv->appSettingsLayout = NULL;
        psv->tempDirGetFile = NULL;
        psv->tempDirLabel = NULL;
        if(projPlaceholder) DisposeObject(projPlaceholder);
        return FALSE;
    }

    /* Main vertical layout containing both groups */
    psv->mainLayout = NewObject(LAYOUT_GetClass(), NULL,
                      //  GA_DrawInfo, (ULONG)drawInfo,
                        LAYOUT_DeferLayout, TRUE,
                        LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
                        LAYOUT_BevelStyle, BVS_NONE,
                        LAYOUT_SpaceOuter, TRUE,
                        LAYOUT_SpaceInner, TRUE,

                        LAYOUT_AddChild, (ULONG)psv->appSettingsLayout,
                        CHILD_WeightedHeight, 0,

                        LAYOUT_AddChild, (ULONG)psv->projSettingsLayout,
                        CHILD_WeightedHeight, 0,

                        TAG_END);

    if(!psv->mainLayout) {
        DisposeObject(psv->projSettingsLayout);
        DisposeObject(psv->appSettingsLayout);
        psv->appSettingsLayout = NULL;
        psv->projSettingsLayout = NULL;
        psv->tempDirGetFile = NULL;
        psv->tempDirLabel = NULL;
        return FALSE;
    }

    /* Create the window object */
    psv->windowObj = NewObject(WINDOW_GetClass(), NULL,
                        WA_Left, 140,
                        WA_Top, 80,
                        WA_Width, 340,
                        WA_Height, 200,
                        WA_CustomScreen, (ULONG)screen,
                        WA_IDCMP, IDCMP_CLOSEWINDOW | IDCMP_GADGETUP | IDCMP_RAWKEY,
                        WA_Flags, WFLG_DRAGBAR | WFLG_DEPTHGADGET | WFLG_CLOSEGADGET |
                                  WFLG_SIZEGADGET | WFLG_ACTIVATE | WFLG_SMART_REFRESH,
                        WA_Title, (ULONG)title,
                        WINDOW_ParentGroup, (ULONG)psv->mainLayout,
                        TAG_END);

    if(!psv->windowObj) {
        DisposeObject(psv->mainLayout);
        psv->mainLayout = NULL;
        psv->appSettingsLayout = NULL;
        psv->projSettingsLayout = NULL;
        psv->tempDirGetFile = NULL;
        psv->tempDirLabel = NULL;
        return FALSE;
    }

    return TRUE;
}

void ProjectSettingsView_Open(ProjectSettingsView *psv)
{
    if(!psv || !psv->windowObj) return;
    if(psv->window) return;  /* Already open */

    psv->window = boopsi_OpenWindow(psv->windowObj);
}

void ProjectSettingsView_Close(ProjectSettingsView *psv)
{
    if(!psv || !psv->windowObj || !psv->window) return;

    DoMethod(psv->windowObj, WM_CLOSE, NULL);
    psv->window = NULL;
}

BOOL ProjectSettingsView_HandleInput(ProjectSettingsView *psv)
{
    ULONG result;

    if(!psv || !psv->windowObj) return FALSE;
    if(!psv->window) return TRUE;  /* Window closed, but that's ok */

    while((result = DoMethod(psv->windowObj, WM_HANDLEINPUT, NULL)) != WMHI_LASTMSG)
    {
        switch(result & WMHI_CLASSMASK)
        {
            case WMHI_CLOSEWINDOW:
                ProjectSettingsView_Close(psv);
                return TRUE;  /* Window just hidden, not destroyed */

            case WMHI_GADGETUP:
            {
                ULONG gadId = result & WMHI_GADGETMASK;
                if(gadId == GAD_PROJSETTINGS_TEMPDIR)
                {
                    /* User clicked the getfile button - open file requester */
                    DoMethod(psv->tempDirGetFile, GFILE_REQUEST, (ULONG)psv->window);
                }
                break;
            }

            case WMHI_RAWKEY:
                break;

            default:
                break;
        }
    }

    return TRUE;
}

ULONG ProjectSettingsView_GetSignalMask(ProjectSettingsView *psv)
{
    if(!psv || !psv->window) return 0;
    return (1L << psv->window->UserPort->mp_SigBit);
}

const char *ProjectSettingsView_GetTempDir(ProjectSettingsView *psv)
{
    const char *path = NULL;

    if(!psv || !psv->tempDirGetFile) return NULL;

    GetAttr(GETFILE_Drawer, psv->tempDirGetFile, (ULONG *)&path);
    return path;
}

void ProjectSettingsView_SetTempDir(ProjectSettingsView *psv, const char *path)
{
    if(!psv || !psv->tempDirGetFile || !path) return;

    SetGadgetAttrs((struct Gadget *)psv->tempDirGetFile, psv->window, NULL,
                   GETFILE_Drawer, (ULONG)path,
                   TAG_END);
}

void ProjectSettingsView_Dispose(ProjectSettingsView *psv)
{
    if(!psv) return;

    /* Close window if open */
    if(psv->window) {
        DoMethod(psv->windowObj, WM_CLOSE, NULL);
        psv->window = NULL;
    }

    /* Dispose window object - this cascades to child gadgets */
    if(psv->windowObj) {
        DisposeObject(psv->windowObj);
        psv->windowObj = NULL;
    }

    psv->mainLayout = NULL;
    psv->appSettingsLayout = NULL;
    psv->projSettingsLayout = NULL;
    psv->tempDirGetFile = NULL;
    psv->tempDirLabel = NULL;
}
