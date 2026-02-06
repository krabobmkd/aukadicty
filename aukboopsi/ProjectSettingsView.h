#ifndef ProjectSettingsView_H
#define ProjectSettingsView_H

#include <intuition/classusr.h>
#include <intuition/intuition.h>

struct DrawInfo;

/*
    Manages the Project Settings window.
    This is a subsidiary window that can be opened/closed independently.
    When closed, it hides (does not quit the app).
*/
typedef struct ProjectSettingsView
{
    Object *windowObj;          /* Window BOOPSI object */
    struct Window *window;      /* Current window (NULL when closed/hidden) */

    /* Main layout */
    Object *mainLayout;

    /* Form gadgets */
    Object *tempDirGetFile;     /* GetFile gadget for temp directory */
    Object *tempDirLabel;       /* Label for temp dir */

    /* References */
    struct Screen *screen;      /* Screen the window is on */
    struct DrawInfo *drawInfo;  /* DrawInfo for gadgets */

} ProjectSettingsView;

/* Gadget IDs for this window */
#define GAD_PROJSETTINGS_TEMPDIR 100

/*
 * Initialize the Project Settings window.
 * Creates the window object but does not open it.
 *
 * @param psv       The ProjectSettingsView struct to initialize
 * @param screen    The screen to open on
 * @param drawInfo  DrawInfo for gadget rendering
 * @param title     Window title string
 * @return          TRUE on success, FALSE on failure
 */
BOOL ProjectSettingsView_Init(ProjectSettingsView *psv,
                              struct Screen *screen,
                              struct DrawInfo *drawInfo,
                              const char *title);

/*
 * Open the Project Settings window.
 * Does nothing if already open.
 */
void ProjectSettingsView_Open(ProjectSettingsView *psv);

/*
 * Close (hide) the Project Settings window.
 * The window can be reopened with ProjectSettingsView_Open.
 */
void ProjectSettingsView_Close(ProjectSettingsView *psv);

/*
 * Handle input messages for the Project Settings window.
 * Call this in the main loop when the window is open.
 *
 * @param psv   The ProjectSettingsView
 * @return      TRUE if the window should remain open, FALSE if closed
 */
BOOL ProjectSettingsView_HandleInput(ProjectSettingsView *psv);

/*
 * Get the signal bit for waiting on this window.
 * Returns 0 if window is not open.
 */
ULONG ProjectSettingsView_GetSignalMask(ProjectSettingsView *psv);

/*
 * Get the current temp directory path.
 * @return  The path string (owned by the gadget, do not free)
 */
const char *ProjectSettingsView_GetTempDir(ProjectSettingsView *psv);

/*
 * Set the temp directory path.
 */
void ProjectSettingsView_SetTempDir(ProjectSettingsView *psv, const char *path);

/*
 * Dispose all resources for the Project Settings window.
 */
void ProjectSettingsView_Dispose(ProjectSettingsView *psv);

#endif
