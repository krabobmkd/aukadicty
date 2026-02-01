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

/* ManagedColor - Color with pen allocation tracking */
struct ManagedColor
{
    ULONG rgbcolor;     /* RGB 0x00RRGGBB format - serialized */
    WORD pen;           /* Pen index for drawing - runtime only */
    WORD allocated;     /* 1 if pen obtained via ObtainBestPenA, 0 if FindColor */
};
typedef struct ManagedColor ManagedColor;

/* Color role enumeration - indices into pens[] array */
typedef enum {
    AUK_COLOR_BACKGROUND = 0,          /* Main background color (gray) */
    AUK_COLOR_TRACK_BACKGROUND,        /* Track empty background (dark gray) */
    AUK_COLOR_SOUND_BACKGROUND,        /* Sound clip background (lighter) */
    AUK_COLOR_SELECTED_BACKGROUND,     /* Selected region background */
    AUK_COLOR_SELECTED_SOUND_BG,       /* Sound clip background when selected */
    AUK_COLOR_TRACK_HIGHLIGHT,         /* Track selection highlight */
    AUK_COLOR_TRACK_HIGHLIGHT2,        /* Track selection highlight secondary */
    AUK_COLOR_WAVEFORM_DARK,           /* Waveform min/max dark blue */
    AUK_COLOR_WAVEFORM_LIGHT,          /* Waveform RMS lighter blue */
    AUK_COLOR_TRACK_HEADER_BG,         /* Track header background */
    AUK_COLOR_TEXT,                    /* Text color */
    AUK_COLOR_WHITE,                   /* Always white */
    AUK_COLOR_BLACK,                   /* Always black */
    AUK_COLOR_COUNT                    /* Number of colors - must be last */
} AukColorRole;

/* AukStyle structure - Plain C struct with no inheritance */
struct AukStyle
{
    /* Managed colors (RGB + pen + allocation flag) - Audacity-like palette */
    ManagedColor pens[AUK_COLOR_COUNT];

    /* Font pointers - Amiga TextFont structures (runtime, NOT serialized) */
    struct TextFont *fontTiny;    /* Small font for compact UI elements */
    struct TextAttr fontTiny_TA; /* Boopsi buttons needs this to change font. Inited if font pointer ok */

    struct TextFont *fontNormal;  /* Standard font for general text */
    struct TextAttr fontNormal_TA;

    struct TextFont *fontBig;     /* Large font for headers/emphasis */
    struct TextAttr fontBig_TA;

    /* in pixel. */
    int borderSelectionWidth;

    /* Reference font height for "em"-like sizing */
    int fontHeight;             /* Height of normal font, used as base unit for sizing */
};

/* Typedef for convenience */
typedef struct AukStyle AukStyle;

#ifdef __cplusplus
}
#endif

#endif /* AUKSTYLE_H */
