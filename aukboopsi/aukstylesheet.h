#ifndef AUKSTYLESHEET_H
#define AUKSTYLESHEET_H

#include <exec/types.h>

/**
 * AukStyleSheet - Visual style configuration for the UI
 *
 * Contains color values and font pointers for rendering the interface.
 * Font pointers are Amiga TextFont structures used throughout the UI.
 */
typedef struct AukStyleSheet
{
    /* Color values (ARGB or RGB format depending on platform) */
    ULONG background;       /* Main background color */
    ULONG trackBackground;  /* Track area background color */
    ULONG waveShape;        /* Waveform drawing color */
    ULONG textColor;        /* Text color */

    /* Font pointers - Amiga TextFont structures */
    struct TextFont *fontTiny;    /* Small font for compact UI elements */
    struct TextFont *fontNormal;  /* Standard font for general text */
    struct TextFont *fontBig;     /* Large font for headers/emphasis */

    /* Reference font height for "em"-like sizing */
    int fontHeight;         /* Height of normal font, used as base unit for sizing */

} AukStyleSheet;

#endif /* AUKSTYLESHEET_H */
