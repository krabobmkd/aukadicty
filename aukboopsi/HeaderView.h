#ifndef HeaderView_H
#define HeaderView_H

#include <intuition/classusr.h>
#include "aukstyle.h"
#include "../aukeditmode.h"
/*
    Manages the top toolbar section of the GUI.
    Contains transport controls (pause/play/stop/rewind/forward)
    and edit mode buttons (6 buttons in 3x2 layout).
    Similar to Audacity's top toolbar but with edit mode buttons.
*/
typedef struct HeaderView
{
    /* BOOPSI gadgets for the header view */
    Object *transportLayout;    /* Horizontal layout for transport controls */
        Object *btRewind;       /* Rewind button */
        Object *btStop;         /* Stop button */
        Object *btPlay;         /* Play button */
        Object *btPause;        /* Pause button */
        Object *btForward;      /* Forward button */

    Object *editModeLayout;     /* 3x2 grid layout for edit mode buttons */
        Object *btEditModes[EDITMODE_COUNT];
        // Object *btSelectTool;    /* Edit mode button 1 */
        // Object *btVolumeEnv;    /* Edit mode button 2 */
        // Object *btCopy;    /* Edit mode button 3 */
        // Object *btZoomTool;    /* Edit mode button 4 */
        // Object *btTimeSlide;    /* Edit mode button 5 */
        // Object *btPaste;    /* Edit mode button 6 */

    Object *spacerLayout;       /* Empty space filler */

    Object *mainHl;             /* Main horizontal layout containing all sections */

    Object *appModel;           /* Reference to application model */
     //NEVER KEEP WINDOW !struct Window *window;      /* Reference to main window */

    /* Visual style configuration */
    AukStyle *pstyleSheet;

    /* Current edit mode - tracks which button is selected */
    AukEditMode currentEditMode;

} HeaderView;

void CreateHeaderView(HeaderView *hv, struct DrawInfo *drawInfo,
                      Object *appModel,
                       AukStyle *stylesheet);

void CloseHeaderView(HeaderView *hv);

void HeaderView_ListenMessage(HeaderView *hv,struct opUpdate *M, ULONG gadId);

#endif
