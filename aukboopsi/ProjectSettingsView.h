#ifndef ProjectSettingsView_H
#define ProjectSettingsView_H

#include <intuition/classusr.h>
#include <intuition/intuition.h>

struct DrawInfo;

/*
    Manages the Settings window.
    Contains two groups: "App Settings" and "Project Settings".
    When closed, it hides (does not quit the app).
*/
typedef struct ProjectSettingsView
{
    Object *windowObj;          /* Window BOOPSI object */
    struct Window *window;      /* Current window (NULL when closed/hidden) */

    /* Main layout */
    Object *mainLayout;

    /* App Settings group */
    Object *appSettingsLayout;
    Object *tempDirGetFile;     /* GetFile gadget for temp directory */
    Object *tempDirLabel;       /* Label for temp dir */

    /* Project Settings group */
    Object *projSettingsLayout;

    /* References */
    struct Screen *screen;      /* Screen the window is on */
    //struct DrawInfo *drawInfo;  /* DrawInfo for gadgets */

} ProjectSettingsView;

/* Gadget IDs for this window */
#define GAD_PROJSETTINGS_TEMPDIR 100

/*
 * Initialize the Settings window.
 * Creates the window object but does not open it.
 */
BOOL ProjectSettingsView_Init(ProjectSettingsView *psv,
                              struct Screen *screen,
                              const char *title);

/* Open the Settings window. Does nothing if already open. */
void ProjectSettingsView_Open(ProjectSettingsView *psv);

/* Close (hide) the Settings window. */
void ProjectSettingsView_Close(ProjectSettingsView *psv);

/* Handle input messages. Call in main loop when window is open. */
BOOL ProjectSettingsView_HandleInput(ProjectSettingsView *psv);

/* Get signal bit for waiting on this window. Returns 0 if not open. */
ULONG ProjectSettingsView_GetSignalMask(ProjectSettingsView *psv);

/* Get current temp directory path. */
const char *ProjectSettingsView_GetTempDir(ProjectSettingsView *psv);

/* Set the temp directory path. */
void ProjectSettingsView_SetTempDir(ProjectSettingsView *psv, const char *path);

/* Dispose all resources. */
void ProjectSettingsView_Dispose(ProjectSettingsView *psv);

#endif
