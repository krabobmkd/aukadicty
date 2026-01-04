#ifndef AUKSTYLESHEET_H
#define AUKSTYLESHEET_H

/*
 * AukStyleSheet - Visual style configuration for the UI
 * Extends AukObject for serialization and listener support
 *
 * Contains color values and font specifications.
 * Font pointers are Amiga TextFont structures used throughout the UI.
 * Fonts are serialized as name+height pairs and opened at runtime.
 */

#include <exec/types.h>
#include "aukobject.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declaration */
typedef struct AukStyleSheet AukStyleSheet;
typedef AukStyleSheet* AukStyleSheetPtr;

/* AukStyleSheet structure - inherits from AukObject */
struct AukStyleSheet
{
    AukObject base;             /* Must be first - inheritance */

    /* Color values (ARGB or RGB format depending on platform) */
    ULONG background;           /* Main background color */
    ULONG trackBackground;      /* Track area background color */
    ULONG waveShape;            /* Waveform drawing color */
    ULONG textColor;            /* Text color */

    /* Font pointers - Amiga TextFont structures (runtime, NOT serialized) */
    struct TextFont *fontTiny;    /* Small font for compact UI elements */
    struct TextFont *fontNormal;  /* Standard font for general text */
    struct TextFont *fontBig;     /* Large font for headers/emphasis */

    /* Font specifications for serialization */
    char *fontTinyName;           /* Font name for tiny font */
    int fontTinyHeight;           /* Pixel height for tiny font */
    char *fontNormalName;         /* Font name for normal font */
    int fontNormalHeight;         /* Pixel height for normal font */
    char *fontBigName;            /* Font name for big font */
    int fontBigHeight;            /* Pixel height for big font */

    /* Reference font height for "em"-like sizing */
    int fontHeight;             /* Height of normal font, used as base unit for sizing */

    /* Virtual methods specific to AukStyleSheet */
    int (*SetFontTiny)(void* This, const char* name, int height);
    int (*SetFontNormal)(void* This, const char* name, int height);
    int (*SetFontBig)(void* This, const char* name, int height);
    int (*OpenFonts)(void* This);      /* Open fonts from specifications */
    void (*CloseFonts)(void* This);    /* Close opened fonts */
};

/* Constructor/Destructor */
void AukStyleSheet_New(AukStyleSheetPtr* firstPtr);
void AukStyleSheet_Delete(void* This);
const char* AukStyleSheet_GetTypeName(void* This);

/* Initialize AukStyleSheet structure */
void AukStyleSheet_Init(AukStyleSheet* styleSheet);

/* Serialization */
void AukStyleSheet_Serialize(void* This, ISerializer* ser, const char* pName);

/* Methods */
int AukStyleSheet_SetFontTiny(void* This, const char* name, int height);
int AukStyleSheet_SetFontNormal(void* This, const char* name, int height);
int AukStyleSheet_SetFontBig(void* This, const char* name, int height);
int AukStyleSheet_OpenFonts(void* This);
void AukStyleSheet_CloseFonts(void* This);

/* Color setters */
int AukStyleSheet_SetBackground(AukStyleSheet* This, ULONG color);
int AukStyleSheet_SetTrackBackground(AukStyleSheet* This, ULONG color);
int AukStyleSheet_SetWaveShape(AukStyleSheet* This, ULONG color);
int AukStyleSheet_SetTextColor(AukStyleSheet* This, ULONG color);

#ifdef __cplusplus
}
#endif

#endif /* AUKSTYLESHEET_H */
