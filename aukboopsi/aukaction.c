
#include <string.h>
#include <stdlib.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/dos.h>
#include <proto/asl.h>
#include <libraries/asl.h>
#include <devices/inputevent.h>

#include "aukaction.h"
#include "auklocale.h"
#include "aukerrors.h"
#include "compilers.h"
#include "aukiffserializer.h"
#include "auktyperegistry.h"
#include <stdio.h>

#include "TrackListView.h"

/* External references to app globals from aukboopsi.c */
extern struct Library *AslBase;
extern struct Window *CurrentMainWindow;

/* Helper: case-insensitive check if string ends with suffix */
static int EndsWithNoCase(const char *str, const char *suffix)
{
    size_t strLen, suffixLen;
    const char *strEnd;

    if (!str || !suffix) return 0;

    strLen = strlen(str);
    suffixLen = strlen(suffix);

    if (suffixLen > strLen) return 0;

    strEnd = str + strLen - suffixLen;

    /* Case-insensitive compare */
    while (*strEnd) {
        char c1 = *strEnd;
        char c2 = *suffix;
        /* Convert to lowercase for comparison */
        if (c1 >= 'A' && c1 <= 'Z') c1 += 32;
        if (c2 >= 'A' && c2 <= 'Z') c2 += 32;
        if (c1 != c2) return 0;
        strEnd++;
        suffix++;
    }
    return 1;
}

/* Action implementations */

BOOL Action_ProjectNew(AukActionContext *context) {
    AukLog_Message(AUKLOG_INFO, AUKERR_ACTION_PROJECT_NEW);

    if (!context || !context->pproject || !*context->pproject) {
        AukLog_Message(AUKLOG_ERROR, AUKERR_ACTION_NO_PROJECT);
        return FALSE;
    }

    /* Clear the existing project - removes all tracks, resets to defaults */
    AukAProject_Clear(*context->pproject);

    AukLog_Message(AUKLOG_INFO, AUKERR_ACTION_PROJECT_CLEARED);
    return TRUE;
}

BOOL Action_ProjectOpen(AukActionContext *context) {
    struct FileRequester *request;
    char fullPath[512];
    BPTR file;
    ISerializer *ser;
    const TypeNameToContructor *typeRegistry;
    AukAProject **pproject;

    AukLog_Message(AUKLOG_INFO, AUKERR_ACTION_PROJECT_OPEN);

    if (!context || !context->pproject || !*context->pproject) {
        AukLog_Message(AUKLOG_ERROR, AUKERR_ACTION_NO_PROJECT);
        return FALSE;
    }

    if (!AslBase) {
        AukLog_Message(AUKLOG_ERROR, AUKERR_ACTION_NO_ASLBASE);
        return FALSE;
    }

    pproject = context->pproject;
// printf("Action_ProjectOpen: listeners:%08x\n", (*pproject)->base.base.listeners);
    /* Allocate and show file requester */
    request = AllocFileRequest();
    if (!request) {
        AukLog_Message(AUKLOG_ERROR, AUKERR_ACTION_FILE_ALLOC_FAILED);
        return FALSE;
    }

    if (!AslRequestTags(request,
            ASLFR_TitleText, (ULONG)"Open Aukadicty Project",
            ASLFR_InitialPattern, (ULONG)"#?.auka",
            ASLFR_DoPatterns, TRUE,
            ASLFR_DoSaveMode, FALSE,
            ASLFR_RejectIcons, TRUE,
            ASLFR_Window, (ULONG)CurrentMainWindow,
            TAG_END)) {
        /* User cancelled */
        FreeAslRequest(request);
        AukLog_Message(AUKLOG_INFO, AUKERR_ACTION_PROJECT_OPEN_CANCELLED);
        return FALSE;
    }

    /* Build full path from drawer and file */
    if (request->fr_Drawer && request->fr_File) {
        strncpy(fullPath, request->fr_Drawer, sizeof(fullPath) - 1);
        fullPath[sizeof(fullPath) - 1] = '\0';

        /* Add path separator if needed */
        if (strlen(fullPath) > 0) {
            char lastChar = fullPath[strlen(fullPath) - 1];
            if (lastChar != ':' && lastChar != '/') {
                strncat(fullPath, "/", sizeof(fullPath) - strlen(fullPath) - 1);
            }
        }
        strncat(fullPath, request->fr_File, sizeof(fullPath) - strlen(fullPath) - 1);
    } else {
        FreeAslRequest(request);
        AukLog_Message(AUKLOG_ERROR, AUKERR_ACTION_FILE_INVALID);
        return FALSE;
    }

    FreeAslRequest(request);

    /* Open file for reading */
    file = Open((STRPTR)fullPath, MODE_OLDFILE);
    if (!file) {
        AukLog_Message(AUKLOG_ERROR, AUKERR_ACTION_FILE_OPEN_FAILED);
        return FALSE;
    }

    /* Get type registry */
    typeRegistry = AukProject_GetTypeRegistry();

    /* Create IFF reader serializer */
    ser = AukIFFSerializer_CreateReader(file, typeRegistry);
    if (!ser) {
        AukLog_Message(AUKLOG_ERROR, AUKERR_ACTION_IFF_READ_FAILED);
        Close(file);
        return FALSE;
    }

    /* Clear existing project data first */
    //no need ?
    AukAProject_Clear(*pproject);

    /* Deserialize into existing project */
    //ser->t_object(ser, "project",pproject);
    /* This version keep same object, so keep listener list */
    (*pproject)->base.base.Serialize((AukObject *)*pproject,ser,"AukAProject");

    /* Clean up serializer and file */
    ser->Destroy(ser);
    Close(file);



    AukLog_MessageStr(AUKLOG_INFO, AUKERR_ACTION_PROJECT_OPENED, fullPath);
    return TRUE;
}

BOOL Action_ProjectSave(AukActionContext *context) {
    struct FileRequester *request;
    char fullPath[512];
    BPTR file;
    ISerializer *ser;
    AukAProject **pproject;

    AukLog_Message(AUKLOG_INFO, AUKERR_ACTION_PROJECT_SAVE);

    if (!context || !context->pproject || !*context->pproject) {
        AukLog_Message(AUKLOG_ERROR, AUKERR_ACTION_NO_PROJECT);
        return FALSE;
    }

    if (!AslBase) {
        AukLog_Message(AUKLOG_ERROR, AUKERR_ACTION_NO_ASLBASE);
        return FALSE;
    }

    pproject = context->pproject;

    /* Allocate and show file requester */
    request = AllocFileRequest();
    if (!request) {
        AukLog_Message(AUKLOG_ERROR, AUKERR_ACTION_FILE_ALLOC_FAILED);
        return FALSE;
    }

    /* Get initial filename from project name */
    {
        const char *projectName = (*pproject)->base.GetName(*pproject);
        char initialFile[128];
        if (projectName && strlen(projectName) > 0) {
            snprintf(initialFile, sizeof(initialFile), "%s.auka", projectName);
        } else {
            strncpy(initialFile, "untitled.auka", sizeof(initialFile));
        }

        if (!AslRequestTags(request,
                ASLFR_TitleText, (ULONG)"Save Aukadicty Project",
                ASLFR_InitialFile, (ULONG)initialFile,
                ASLFR_InitialPattern, (ULONG)"#?.auka",
                ASLFR_DoPatterns, TRUE,
                ASLFR_DoSaveMode, TRUE,
                ASLFR_RejectIcons, TRUE,
                ASLFR_Window, (ULONG)CurrentMainWindow,
                TAG_END)) {
            /* User cancelled */
            FreeAslRequest(request);
            AukLog_Message(AUKLOG_INFO, AUKERR_ACTION_PROJECT_SAVE_CANCELLED);
            return FALSE;
        }
    }

    /* Build full path from drawer and file */
    if (request->fr_Drawer && request->fr_File) {
        strncpy(fullPath, request->fr_Drawer, sizeof(fullPath) - 1);
        fullPath[sizeof(fullPath) - 1] = '\0';

        /* Add path separator if needed */
        if (strlen(fullPath) > 0) {
            char lastChar = fullPath[strlen(fullPath) - 1];
            if (lastChar != ':' && lastChar != '/') {
                strncat(fullPath, "/", sizeof(fullPath) - strlen(fullPath) - 1);
            }
        }
        strncat(fullPath, request->fr_File, sizeof(fullPath) - strlen(fullPath) - 1);

        /* Add .auka extension if not present (case-insensitive check) */
        if (!EndsWithNoCase(fullPath, ".auka")) {
            strncat(fullPath, ".auka", sizeof(fullPath) - strlen(fullPath) - 1);
        }
    } else {
        FreeAslRequest(request);
        AukLog_Message(AUKLOG_ERROR, AUKERR_ACTION_FILE_INVALID);
        return FALSE;
    }

    FreeAslRequest(request);

    /* Open file for writing */
    file = Open((STRPTR)fullPath, MODE_NEWFILE);
    if (!file) {
        AukLog_Message(AUKLOG_ERROR, AUKERR_ACTION_FILE_WRITE_FAILED);
        return FALSE;
    }

    /* Create IFF writer serializer */
    ser = AukIFFSerializer_CreateWriter(file);
    if (!ser) {
        AukLog_Message(AUKLOG_ERROR, AUKERR_ACTION_IFF_WRITE_FAILED);
        Close(file);
        return FALSE;
    }

    /* Serialize the project */
    //ser->t_object(ser, "project",pproject);
    /* version that keeps same object instance */
    (*pproject)->base.base.Serialize((AukObject *)*pproject,ser,"AukAProject");

    /* Finalize IFF (writes correct FORM size) */
    if (!AukIFFSerializer_Finalize(ser)) {
        AukLog_Message(AUKLOG_ERROR, AUKERR_ACTION_IFF_FINALIZE_FAILED);
        ser->Destroy(ser);
        Close(file);
        return FALSE;
    }

    /* Clean up */
    ser->Destroy(ser);
    Close(file);

    AukLog_MessageStr(AUKLOG_INFO, AUKERR_ACTION_PROJECT_SAVED, fullPath);
    return TRUE;
}

BOOL Action_ProjectSaveAs(AukActionContext *context) {
    /* Save As is the same as Save - always shows file requester */
    return Action_ProjectSave(context);
}

BOOL Action_ProjectExport(AukActionContext *context) {
    (void)context;
    /* TODO: Implement audio export */
    return TRUE;
}

BOOL Action_ProjectAbout(AukActionContext *context) {
    (void)context;
    /* TODO: Show about requester */
    return TRUE;
}

BOOL Action_ProjectQuit(AukActionContext *context) {
    (void)context;
    /* TODO: Confirm and quit application -> if modified */

    /* atexit() magic */
    exit(0);

    return TRUE;
}

BOOL Action_EditUndo(AukActionContext *context) {
    (void)context;
    /* TODO: Implement undo */
    return TRUE;
}

BOOL Action_EditRedo(AukActionContext *context) {
    (void)context;
    /* TODO: Implement redo */
    return TRUE;
}

BOOL Action_EditSelectAll(AukActionContext *context) {
    (void)context;
    /* TODO: Implement select all */
    return TRUE;
}

BOOL Action_EditSelectNone(AukActionContext *context) {
    (void)context;
    /* TODO: Implement deselect all */
    return TRUE;
}

BOOL Action_EditCopy(AukActionContext *context) {
    (void)context;
    /* TODO: Implement copy to clipboard */
    return TRUE;
}

BOOL Action_EditCut(AukActionContext *context) {
    (void)context;
    /* TODO: Implement cut to clipboard */
    return TRUE;
}

BOOL Action_EditPaste(AukActionContext *context) {
    (void)context;
    /* TODO: Implement paste from clipboard */
    return TRUE;
}

BOOL Action_TracksAdd(AukActionContext *context) {
    /* TODO: Implement add track to project */
    if (context && context->pproject && *context->pproject) {
        AukAProject *p = *context->pproject ;
        p->CreateTrack(p);
    }
    return TRUE;
}

BOOL Action_ViewZoomIn(AukActionContext *context) {
    (void)context;

    TrackListView_ZoomIn(context->trackListView);
    /*  Implement zoom in - will call TrackListView_ZoomIn */
    return TRUE;
}

BOOL Action_ViewZoomOut(AukActionContext *context) {
    (void)context;
    TrackListView_ZoomOut(context->trackListView);
    /* TODO: Implement zoom out - will call TrackListView_ZoomOut */
    return TRUE;
}

BOOL Action_ViewCollapseTracks(AukActionContext *context) {
    (void)context;
    /* TODO: Implement collapse all tracks */
    return TRUE;
}

BOOL Action_ViewExpandTracks(AukActionContext *context) {
    (void)context;
    /* TODO: Implement expand all tracks */
    return TRUE;
}

BOOL Action_ViewIconify(AukActionContext *context) {
    (void)context;
    /* TODO: Implement iconify window */
    return TRUE;
}

BOOL Action_SettingsProject(AukActionContext *context) {
    (void)context;
    /* TODO: Show project settings dialog */
    return TRUE;
}

BOOL Action_SettingsView(AukActionContext *context) {
    (void)context;
    /* TODO: Show view settings dialog */
    return TRUE;
}

BOOL Action_HelpHelp(AukActionContext *context) {
    (void)context;
    /* TODO: Show help documentation */
    return TRUE;
}

/* Global action table */
static AukAction actionTable[ACTION_COUNT] = {
    /* Project actions */
    [ACTION_PROJECT_NEW]    = {Action_ProjectNew,    MSG_FILE_NEW,    NULL, 0, 0},
    [ACTION_PROJECT_OPEN]   = {Action_ProjectOpen,   MSG_FILE_OPEN,   NULL, 0, 0},
    [ACTION_PROJECT_SAVE]   = {Action_ProjectSave,   MSG_FILE_SAVE,   NULL, 0, 0},
    [ACTION_PROJECT_SAVEAS] = {Action_ProjectSaveAs, MSG_FILE_SAVEAS, NULL, 0, 0},
    [ACTION_PROJECT_EXPORT] = {Action_ProjectExport, MSG_FILE_EXPORT, NULL, 0, 0},
    [ACTION_PROJECT_ABOUT]  = {Action_ProjectAbout,  MSG_MENU_ABOUT,  NULL, 0, 0},
    [ACTION_PROJECT_QUIT]   = {Action_ProjectQuit,   MSG_MENU_QUIT,   NULL, 0x45, 0}, /* ESC key */

    /* Edition actions */
    [ACTION_EDIT_UNDO]       = {Action_EditUndo,       MSG_EDIT_UNDO,       NULL, 0, 0},
    [ACTION_EDIT_REDO]       = {Action_EditRedo,       MSG_EDIT_REDO,       NULL, 0, 0},
    [ACTION_EDIT_SELECTALL]  = {Action_EditSelectAll,  MSG_EDIT_SELECTALL,  NULL, 0, 0},
    [ACTION_EDIT_SELECTNONE] = {Action_EditSelectNone, MSG_EDIT_SELECTNONE, NULL, 0, 0},
    [ACTION_EDIT_COPY]       = {Action_EditCopy,       MSG_EDIT_COPY,       NULL, 0, 0},
    [ACTION_EDIT_CUT]        = {Action_EditCut,        MSG_EDIT_CUT,        NULL, 0, 0},
    [ACTION_EDIT_PASTE]      = {Action_EditPaste,      MSG_EDIT_PASTE,      NULL, 0, 0},

    /* Track actions (now under Edition menu) */
    [ACTION_TRACKS_ADD] = {Action_TracksAdd, MSG_TRACKS_ADD, NULL, 0, 0},

    /* View actions */
    [ACTION_VIEW_ZOOMIN]          = {Action_ViewZoomIn,         MSG_VIEW_ZOOMIN,          NULL, 0x5D, 0}, /* Numpad + */
    [ACTION_VIEW_ZOOMOUT]         = {Action_ViewZoomOut,        MSG_VIEW_ZOOMOUT,         NULL, 0x4A, 0}, /* Numpad - */
    [ACTION_VIEW_COLLAPSE_TRACKS] = {Action_ViewCollapseTracks, MSG_VIEW_COLLAPSE_TRACKS, NULL, 0, 0},
    [ACTION_VIEW_EXPAND_TRACKS]   = {Action_ViewExpandTracks,   MSG_VIEW_EXPAND_TRACKS,   NULL, 0, 0},
    [ACTION_VIEW_ICONIFY]         = {Action_ViewIconify,        MSG_VIEW_ICONIFY,         NULL, 0, 0},

    /* Settings actions */
    [ACTION_SETTINGS_PROJECT] = {Action_SettingsProject, MSG_SETTINGS_PROJECT, NULL, 0, 0},
    [ACTION_SETTINGS_VIEW]    = {Action_SettingsView,    MSG_SETTINGS_VIEW,    NULL, 0, 0},

    /* Help actions */
    [ACTION_HELP_HELP] = {Action_HelpHelp, MSG_MENU_HELP, NULL, 0, 0},
};

void AukAction_Init(void)
{
    ULONG i;

    /* Localize all action names */
    for (i = 0; i < ACTION_COUNT; i++) {
        actionTable[i].name = LOC(actionTable[i].nameStringID);
    }
}

AukAction *AukAction_Get(ULONG actionID)
{
    if (actionID >= ACTION_COUNT) {
        return NULL;
    }
    return &actionTable[actionID];
}

BOOL AukAction_Execute(ULONG actionID, AukActionContext *context)
{
    AukAction *action;

    if (actionID >= ACTION_COUNT) {
        return FALSE;
    }

    action = &actionTable[actionID];
    if (!action->func) {
        return FALSE;
    }

    /* Execute the action */
    return action->func(context);
}
