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

/* AukStyle structure - Plain C struct with no inheritance */
struct AukStyle
{
    /* Managed colors (RGB + pen + allocation flag) - Audacity-like palette */
    ManagedColor background;           /* Main background color (gray) */
    ManagedColor trackBackground;      /* Track empty background (dark gray) */
    ManagedColor soundBackground;      /* Sound clip background (lighter) */
    ManagedColor selectedBackground;   /* Selected region background */
    ManagedColor waveformDark;         /* Waveform min/max dark blue */
    ManagedColor waveformLight;        /* Waveform RMS lighter blue */
    ManagedColor textColor;            /* Text color */
    ManagedColor white;                /* Always white */
    ManagedColor black;                /* Always black */

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
