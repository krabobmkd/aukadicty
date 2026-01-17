#ifndef HeaderView_H
#define HeaderView_H

#include <intuition/classusr.h>
#include "aukstyle.h"

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
        Object *btEditMode1;    /* Edit mode button 1 */
        Object *btEditMode2;    /* Edit mode button 2 */
        Object *btEditMode3;    /* Edit mode button 3 */
        Object *btEditMode4;    /* Edit mode button 4 */
        Object *btEditMode5;    /* Edit mode button 5 */
        Object *btEditMode6;    /* Edit mode button 6 */

    Object *spacerLayout;       /* Empty space filler */

    Object *mainHl;             /* Main horizontal layout containing all sections */

    Object *appModel;           /* Reference to application model */
     //NEVER KEEP WINDOW !struct Window *window;      /* Reference to main window */

    /* Visual style configuration */
    AukStyle *pstyleSheet;

} HeaderView;

void CreateHeaderView(HeaderView *hv, struct DrawInfo *drawInfo,
                      Object *appModel,
                       AukStyle *stylesheet);

void CloseHeaderView(HeaderView *hv);

#endif
