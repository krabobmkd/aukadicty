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

/* Header view transport control buttons */
#define GAD_HEADER_REWIND       5
#define GAD_HEADER_STOP         6
#define GAD_HEADER_PLAY         7
#define GAD_HEADER_PAUSE        8
#define GAD_HEADER_FORWARD      9

/* Header view edit mode buttons (3x2 grid) - mutually exclusive toggle buttons */
#define GAD_HEADER_SELECTTOOL   10  /* Selection Tool */
#define GAD_HEADER_VOLUMEENV    11  /* Volume Envelope */
#define GAD_HEADER_COPY         12  /* Copy */
#define GAD_HEADER_ZOOMTOOL     13  /* Zoom Tool */
#define GAD_HEADER_TIMESLIDE    14  /* Time Slide */
#define GAD_HEADER_PASTE        15  /* Paste */

/* First and last edit mode button IDs for range checking */
#define GAD_HEADER_EDITMODE_FIRST GAD_HEADER_SELECTTOOL
#define GAD_HEADER_EDITMODE_LAST  GAD_HEADER_PASTE

#define GAD_BUTTON_ABOUT 16

/* Individual track area gadgets start from this base */
//#define GAD_TRACKAREA_BASE      4096

/* Individual track header gadgets start from this base */
#define GAD_TRACKHEADER_BASE        0x08000

#define GAD_TRACKHEADER_IDMASK      0x0000f
#define GAD_TRACKHEADER_TRACKMASK   0x07ff0

#define GAD_TRACKHEADER_TRACKAREA    0
#define GAD_TRACKHEADER_CLOSE    1
#define GAD_TRACKHEADER_NAME    2
#define GAD_TRACKHEADER_VOL    3
#define GAD_TRACKHEADER_PAN    4

#define GAD_TRACKHEADER_SILENCER 5
#define GAD_TRACKHEADER_SOLO 6
#define GAD_TRACKHEADER_SELECT 7
#endif /* GADGETID_H */
