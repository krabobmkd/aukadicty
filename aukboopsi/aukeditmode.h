#ifndef AUKEDITMODE_H
#define AUKEDITMODE_H

/*
    Edit mode enum for Aukadicty
    These modes define the current editing tool selected in the toolbar.
    The buttons are mutually exclusive - only one can be active at a time.
*/

typedef enum {
    EDITMODE_SELECT = 0,    /* Selection Tool - select and move sounds */
    EDITMODE_VOLUME,        /* Volume Envelope - edit track volume envelope */
    EDITMODE_COPY,          /* Copy - copy selected sounds */
    EDITMODE_ZOOM,          /* Zoom Tool - zoom in/out on timeline */
    EDITMODE_TIMESLIDE,     /* Time Slide - slide sounds in time */
    EDITMODE_PASTE,         /* Paste - paste copied sounds */
    EDITMODE_COUNT          /* Number of edit modes */
} AukEditMode;

#endif /* AUKEDITMODE_H */
