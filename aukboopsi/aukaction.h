#ifndef AUKACTION_H
#define AUKACTION_H

#include <exec/types.h>
#include <aukaproject.h>

/*
    Action system for Aukadicty
    Defines all application actions (menu commands, keyboard shortcuts, etc.)
    Actions are independent of UI and can be triggered from menus, buttons, or keys
*/

/* Forward declarations */
typedef struct AukAction AukAction;
typedef struct AukActionContext AukActionContext;

struct TrackListView;
struct AppSettings;

/* Action function signature - returns TRUE if successful */
typedef BOOL (*AukActionFunc)(AukActionContext *context);

/* Action context - passed to action functions */
struct AukActionContext {
    AukAProjectPtr *pproject;       /* Current project */
    void *appWindow;            /* Application window (struct Window*) */
    void *appData;              /* Application-specific data */
    struct TrackListView *trackListView; /* For View actions */
    struct AppSettings *appSettings;    /* For recent file actions */
    int recentFileIndex;               /* Index for ACTION_RECENT_FILE_* */
};

/* Action definition */
struct AukAction {
    AukActionFunc func;         /* Function to execute */
    ULONG nameStringID;         /* Localized name (MSG_* constant) */
    const char *name;           /* Cached localized name (set at init) */
    WORD shortcutKey;           /* Keyboard shortcut (raw key code, 0 = none) */
    UWORD shortcutQual;         /* Qualifier keys (IEQUALIFIER_* flags) */
};

/* Action IDs - used to reference actions */
enum {
    /* Project actions */
    ACTION_PROJECT_NEW = 0,
    ACTION_PROJECT_OPEN,
    ACTION_PROJECT_SAVE,
    ACTION_PROJECT_SAVEAS,
    ACTION_PROJECT_EXPORT,
    ACTION_PROJECT_ABOUT,
    ACTION_PROJECT_QUIT,

    /* Edition actions */
    ACTION_EDIT_UNDO,
    ACTION_EDIT_REDO,
    ACTION_EDIT_SELECTALL,
    ACTION_EDIT_SELECTNONE,
    ACTION_EDIT_COPY,
    ACTION_EDIT_CUT,
    ACTION_EDIT_PASTE,

    /* Track actions (now under Edition menu) */
    ACTION_TRACKS_ADD,

    /* View actions */
    ACTION_VIEW_ZOOMIN,
    ACTION_VIEW_ZOOMOUT,
    ACTION_VIEW_ZOOM_PROJECT,
    ACTION_VIEW_COLLAPSE_TRACKS,
    ACTION_VIEW_EXPAND_TRACKS,
    ACTION_VIEW_ICONIFY,

    /* Settings actions */
    ACTION_SETTINGS_PROJECT,
    ACTION_SETTINGS_VIEW,

    /* Help actions */
    ACTION_HELP_HELP,

    /* Recent file actions (menu-only, 8 max) */
    ACTION_RECENT_FILE_0,
    ACTION_RECENT_FILE_1,
    ACTION_RECENT_FILE_2,
    ACTION_RECENT_FILE_3,
    ACTION_RECENT_FILE_4,
    ACTION_RECENT_FILE_5,
    ACTION_RECENT_FILE_6,
    ACTION_RECENT_FILE_7,

    /* Must be last */
    ACTION_COUNT
};

/* Initialize action system and localize action names */
void AukAction_Init(void);

/* Get action by ID */
AukAction *AukAction_Get(ULONG actionID);

/* Execute an action */
BOOL AukAction_Execute(ULONG actionID, AukActionContext *context);

/* Action function declarations */
BOOL Action_ProjectNew(AukActionContext *context);
BOOL Action_ProjectOpen(AukActionContext *context);
BOOL Action_ProjectSave(AukActionContext *context);
BOOL Action_ProjectSaveAs(AukActionContext *context);
BOOL Action_ProjectExport(AukActionContext *context);
BOOL Action_ProjectAbout(AukActionContext *context);
BOOL Action_ProjectQuit(AukActionContext *context);

BOOL Action_EditUndo(AukActionContext *context);
BOOL Action_EditRedo(AukActionContext *context);
BOOL Action_EditSelectAll(AukActionContext *context);
BOOL Action_EditSelectNone(AukActionContext *context);
BOOL Action_EditCopy(AukActionContext *context);
BOOL Action_EditCut(AukActionContext *context);
BOOL Action_EditPaste(AukActionContext *context);

BOOL Action_TracksAdd(AukActionContext *context);

BOOL Action_ViewZoomIn(AukActionContext *context);
BOOL Action_ViewZoomOut(AukActionContext *context);
BOOL Action_ViewZoomProject(AukActionContext *context);
BOOL Action_ViewCollapseTracks(AukActionContext *context);
BOOL Action_ViewExpandTracks(AukActionContext *context);
BOOL Action_ViewIconify(AukActionContext *context);

BOOL Action_SettingsProject(AukActionContext *context);
BOOL Action_SettingsView(AukActionContext *context);

BOOL Action_HelpHelp(AukActionContext *context);

BOOL Action_RecentFileOpen(AukActionContext *context);

#endif /* AUKACTION_H */
