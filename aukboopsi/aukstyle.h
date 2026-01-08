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
    /* Color values (ARGB or RGB format depending on platform) */
    ULONG background;           /* Main background color */
    ULONG trackBackground;      /* Track area background color */
    ULONG waveShape;            /* Waveform drawing color */
    ULONG textColor;            /* Text color */

    /* Font pointers - Amiga TextFont structures (runtime, NOT serialized) */
    struct TextFont *fontTiny;    /* Small font for compact UI elements */
    struct TextFont *fontNormal;  /* Standard font for general text */
    struct TextFont *fontBig;     /* Large font for headers/emphasis */

    /* Reference font height for "em"-like sizing */
    int fontHeight;             /* Height of normal font, used as base unit for sizing */
};

/* Typedef for convenience */
typedef struct AukStyle AukStyle;

#ifdef __cplusplus
}
#endif

#endif /* AUKSTYLE_H */
