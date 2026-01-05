#ifndef FooterView_H
#define FooterView_H

#include <intuition/classusr.h>
#include "aukstylesheet.h"

/*
    Manages the bottom status bar section of the GUI.
    Displays project mixing frequency, selection start/end times,
    and current playback position.
*/
typedef struct FooterView
{
    /* BOOPSI gadgets for the footer view */
    Object *labelFrequency;     /* Display mixing frequency (e.g. "44100 Hz") */
    Object *labelSelStart;      /* Selection start time */
    Object *labelSelEnd;        /* Selection end time */
    Object *labelPlayPos;       /* Current playback position */

    Object *mainHl;             /* Main horizontal layout containing all labels */

    Object *appModel;           /* Reference to application model */
    struct Window *window;      /* Reference to main window */

    /* Visual style configuration */
    AukStyleSheet *pstyleSheet;

} FooterView;

void CreateFooterView(FooterView *fv, struct DrawInfo *drawInfo,
                      Object *appModel,
                      AukStyleSheet *stylesheet);

void FooterView_UpdateFrequency(FooterView *fv, ULONG frequency);
void FooterView_UpdateSelection(FooterView *fv, const char *startTime, const char *endTime);
void FooterView_UpdatePlayPosition(FooterView *fv, const char *position);

void CloseFooterView(FooterView *fv);

#endif
