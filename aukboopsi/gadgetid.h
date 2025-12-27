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
//#define GAD_TRACKAREA_BASE      4096

/* Individual track header gadgets start from this base */
#define GAD_TRACKHEADER_BASE        0x10000

#define GAD_TRACKHEADER_IDMASK      0x0000f
#define GAD_TRACKHEADER_TRACKMASK   0x0fff0

#define GAD_TRACKHEADER_CLOSE    1
#define GAD_TRACKHEADER_NAME    2
#define GAD_TRACKHEADER_VOL    3
#define GAD_TRACKHEADER_PAN    4

#endif /* GADGETID_H */
