#ifndef GADGETID_H
#define GADGETID_H

/**
 * Gadget ID definitions for BOOPSI gadgets
 *
 * These IDs are used with the GA_ID attribute to identify specific
 * gadget instances in the application. They are useful for handling
 * events and identifying which gadget sent a notification.
 */

/* Main track list area gadget */
#define GAD_TRACKLIST           1

/* Vertical scroller for track list */
#define GAD_SCROLLER_V          2

/* Horizontal scroller for track list */
#define GAD_SCROLLER_H          3

/* Time rule gadget (horizontal timeline) */
#define GAD_TIMERULE            4



#define GAD_BUTTON_ABOUT 16

/* Individual track area gadgets start from this base */
#define GAD_TRACKAREA_BASE      4096

/* Individual track header gadgets start from this base */
#define GAD_TRACKHEADER_BASE    8192

#define GAD_TRACKHEADER_CLOSE    (GAD_TRACKHEADER_BASE+1)
#define GAD_TRACKHEADER_NAME    (GAD_TRACKHEADER_BASE+2)


#endif /* GADGETID_H */
