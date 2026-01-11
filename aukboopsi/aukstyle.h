#ifndef AUKSTYLE_H
#define AUKSTYLE_H

/*
 * AukStyle - Lightweight style data for BOOPSI gadgets
 *
 * Contains only color values and font pointers needed for rendering.
 * This struct can be included by BOOPSI classes without pulling in
 * the entire AukObject system.
 *
 * Font pointers are runtime values (NOT serialized).
 * Color values are ARGB or RGB format depending on platform.
 */

#include <exec/types.h>
#include <graphics/text.h>

#ifdef __cplusplus
extern "C" {
#endif

/* AukStyle structure - Plain C struct with no inheritance */
struct AukStyle
{
    /* Color values (RGB 0x00RRGGBB format) - Audacity-like palette */
    ULONG background;           /* Main background color (gray) */
    ULONG trackBackground;      /* Track empty background (dark gray) */
    ULONG soundBackground;      /* Sound clip background (lighter) */
    ULONG selectedBackground;   /* Selected region background */
    ULONG waveformDark;         /* Waveform min/max dark blue */
    ULONG waveformLight;        /* Waveform RMS lighter blue */
    ULONG textColor;            /* Text color */

    /* Pen indices obtained via ObtainBestPenA - runtime only, NOT serialized */
    WORD penBackground;         /* Pen for main background */
    WORD penTrackBackground;    /* Pen for track empty area */
    WORD penSoundBackground;    /* Pen for sound clip area */
    WORD penSelectedBackground; /* Pen for selected region */
    WORD penWaveformDark;       /* Pen for waveform min/max */
    WORD penWaveformLight;      /* Pen for waveform RMS */
    WORD penText;               /* Pen for text */
    WORD penWhite;              /* Always white pen */
    WORD penBlack;              /* Always black pen */
    WORD _penPadding;           /* Padding for alignment */

    /* Font pointers - Amiga TextFont structures (runtime, NOT serialized) */
    struct TextFont *fontTiny;    /* Small font for compact UI elements */
    struct TextAttr fontTiny_TA; /* Boopsi buttons needs this to change font. Inited if font pointer ok */

    struct TextFont *fontNormal;  /* Standard font for general text */
    struct TextAttr fontNormal_TA;

    struct TextFont *fontBig;     /* Large font for headers/emphasis */
    struct TextAttr fontBig_TA;

    /* Reference font height for "em"-like sizing */
    int fontHeight;             /* Height of normal font, used as base unit for sizing */
};

/* Typedef for convenience */
typedef struct AukStyle AukStyle;

#ifdef __cplusplus
}
#endif

#endif /* AUKSTYLE_H */
