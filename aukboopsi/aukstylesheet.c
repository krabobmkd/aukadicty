#include "aukstylesheet.h"
#include "aukstring.h"
#include "serializer.h"
#include <proto/exec.h>
#include <proto/diskfont.h>
#include <proto/graphics.h>
#include <graphics/text.h>

/*
 * AukStyleSheet implementation
 * Manages UI styling with colors and fonts
 * Fonts are serialized as name+height and opened at runtime
 * Colors are obtained as pens via ObtainBestPenA for efficient rendering
 */

/* Internal helper to obtain a pen for an RGB color.
 * Converts 0x00RRGGBB format to 32-bit per component for ObtainBestPenA.
 * Returns pen number or -1 on failure.
 */
static WORD ObtainPenForRGB(struct ColorMap *cm, ULONG rgb) {
    ULONG r, g, b;
    LONG pen;

    if (!cm) return -1;

    /* Extract 8-bit components and expand to 32-bit (0xFF -> 0xFFFFFFFF) */
    r = ((rgb >> 16) & 0xFF);
    g = ((rgb >> 8) & 0xFF);
    b = (rgb & 0xFF);

    /* Expand to full 32-bit range as expected by ObtainBestPenA */
    r = (r << 24) | (r << 16) | (r << 8) | r;
    g = (g << 24) | (g << 16) | (g << 8) | g;
    b = (b << 24) | (b << 16) | (b << 8) | b;

    pen = ObtainBestPenA(cm, r, g, b, NULL);

    return (WORD)pen;
}

/* Internal helper to release a pen if it was allocated */
static void ReleasePenIfValid(struct ColorMap *cm, WORD *penPtr) {
    if (!cm || !penPtr) return;
    if (*penPtr >= 0) {
        ReleasePen(cm, *penPtr);
        *penPtr = -1;
    }
}

/* Internal helper to open a single font */
static struct TextFont* OpenFontBySpec(const char* name, int height,
       struct TextAttr *font_ta ) {

    struct TextFont* font = NULL;

    if (!name || height <= 0) {
        return NULL;
    }

    font_ta->ta_Name = (STRPTR)name;
    font_ta->ta_YSize = (UWORD)height;
    font_ta->ta_Style = FS_NORMAL;
    font_ta->ta_Flags = FPF_DISKFONT;

    font = OpenDiskFont(font_ta);

    if (!font) {
        /* Fallback: try ROM font */
        font_ta->ta_Flags = FPF_ROMFONT;
        font = OpenFont(font_ta);
    }

    return font;
}

void AukStyleSheet_New(AukStyleSheetPtr* firstPtr) {
    AukStyleSheet* styleSheet;

    if (!firstPtr) {
        return;
    }

    styleSheet = (AukStyleSheet*)AllocVec(sizeof(AukStyleSheet), MEMF_CLEAR);
    if (styleSheet) {
        AukStyleSheet_Init(styleSheet);
        AukObjectPtr_Set((AukObjectPtr*)firstPtr, &styleSheet->base);
    }
}

void AukStyleSheet_Delete(void* This) {
    AukStyleSheet* styleSheet = (AukStyleSheet*)This;
    if (styleSheet) {
        /* Release obtained pens */
        AukStyleSheet_ReleasePens(This);

        /* Close opened fonts */
        AukStyleSheet_CloseFonts(This);

        /* Free font name strings */
        if (styleSheet->fontTinyName) {
            AukString_Free(styleSheet->fontTinyName);
        }
        if (styleSheet->fontNormalName) {
            AukString_Free(styleSheet->fontNormalName);
        }
        if (styleSheet->fontBigName) {
            AukString_Free(styleSheet->fontBigName);
        }

        /* Call base object delete (which will FreeVec) */
        AukObject_Delete(&styleSheet->base);
    }
}

const char* AukStyleSheet_GetTypeName(void* This) {
    (void)This;
    return "AukStyleSheet";
}

void AukStyleSheet_Serialize(void* This, ISerializer* ser, const char* pName) {
    AukStyleSheet* styleSheet = (AukStyleSheet*)This;
    (void)pName;

    if (!styleSheet || !ser) {
        return;
    }

    /* Serialize colors */
    ser->t_uint(ser, "background", &styleSheet->style.background);
    ser->t_uint(ser, "trackBackground", &styleSheet->style.trackBackground);
    ser->t_uint(ser, "soundBackground", &styleSheet->style.soundBackground);
    ser->t_uint(ser, "selectedBackground", &styleSheet->style.selectedBackground);
    ser->t_uint(ser, "waveformDark", &styleSheet->style.waveformDark);
    ser->t_uint(ser, "waveformLight", &styleSheet->style.waveformLight);
    ser->t_uint(ser, "textColor", &styleSheet->style.textColor);

    /* Serialize font specifications (name + height for each font) */
    ser->t_string_mutable(ser, "fontTinyName", &styleSheet->fontTinyName);
    ser->t_int(ser, "fontTinyHeight", &styleSheet->fontTinyHeight);

    ser->t_string_mutable(ser, "fontNormalName", &styleSheet->fontNormalName);
    ser->t_int(ser, "fontNormalHeight", &styleSheet->fontNormalHeight);

    ser->t_string_mutable(ser, "fontBigName", &styleSheet->fontBigName);
    ser->t_int(ser, "fontBigHeight", &styleSheet->fontBigHeight);

    /* Serialize reference font height */
    ser->t_int(ser, "fontHeight", &styleSheet->style.fontHeight);

    /* After reading, open fonts from loaded specifications */
    // not at that moment...
//    if (IS_READING(ser)) {
//        AukStyleSheet_Update(This);
//    }
}

int AukStyleSheet_SetFontTiny(void* This, const char* name, int height) {
    AukStyleSheet* styleSheet = (AukStyleSheet*)This;
    int changed;

    if (!styleSheet || !name || height <= 0) {
        return 0;
    }

    /* Check if value actually changed */
    changed = (styleSheet->fontTinyName == NULL ||
               AukString_Compare(styleSheet->fontTinyName, name) != 0 ||
               styleSheet->fontTinyHeight != height);

    if (!changed) {
        return 1; /* No change, but success */
    }

    /* Free old name if exists */
    if (styleSheet->fontTinyName) {
        AukString_Free(styleSheet->fontTinyName);
    }

    /* Set new specification */
    styleSheet->fontTinyName = AukString_Duplicate(name);
    styleSheet->fontTinyHeight = height;

    return styleSheet->fontTinyName != NULL;
}

int AukStyleSheet_SetFontNormal(void* This, const char* name, int height) {
    AukStyleSheet* styleSheet = (AukStyleSheet*)This;
    int changed;

    if (!styleSheet || !name || height <= 0) {
        return 0;
    }

    /* Check if value actually changed */
    changed = (styleSheet->fontNormalName == NULL ||
               AukString_Compare(styleSheet->fontNormalName, name) != 0 ||
               styleSheet->fontNormalHeight != height);

    if (!changed) {
        return 1; /* No change, but success */
    }

    /* Free old name if exists */
    if (styleSheet->fontNormalName) {
        AukString_Free(styleSheet->fontNormalName);
    }

    /* Set new specification */
    styleSheet->fontNormalName = AukString_Duplicate(name);
    styleSheet->fontNormalHeight = height;

    return styleSheet->fontNormalName != NULL;
}

int AukStyleSheet_SetFontBig(void* This, const char* name, int height) {
    AukStyleSheet* styleSheet = (AukStyleSheet*)This;
    int changed;

    if (!styleSheet || !name || height <= 0) {
        return 0;
    }

    /* Check if value actually changed */
    changed = (styleSheet->fontBigName == NULL ||
               AukString_Compare(styleSheet->fontBigName, name) != 0 ||
               styleSheet->fontBigHeight != height);

    if (!changed) {
        return 1; /* No change, but success */
    }

    /* Free old name if exists */
    if (styleSheet->fontBigName) {
        AukString_Free(styleSheet->fontBigName);
    }

    /* Set new specification */
    styleSheet->fontBigName = AukString_Duplicate(name);
    styleSheet->fontBigHeight = height;

    return styleSheet->fontBigName != NULL;
}

/* AukStyleSheet_ApplyStyle - Obtain pens and open fonts
 *
 * This method synchronizes the color values to screen pens and
 * the font specifications to actual runtime font pointers.
 * Call this after setting colors/fonts, or after deserializing.
 *
 * @param scr The locked screen to obtain pens from
 */
int AukStyleSheet_ApplyStyle(void* This, struct Screen *scr) {
    AukStyleSheet* styleSheet = (AukStyleSheet*)This;
    struct ColorMap *cm;
    int success = 1;

    if (!styleSheet) {
        return 0;
    }

    /* Release any previously obtained pens */
    AukStyleSheet_ReleasePens(This);

    /* Store screen reference and obtain pens */
    styleSheet->screen = scr;
    if (scr) {
        cm = scr->ViewPort.ColorMap;

        /* Obtain pens for all colors */
        styleSheet->style.penBackground = ObtainPenForRGB(cm, styleSheet->style.background);
        styleSheet->style.penTrackBackground = ObtainPenForRGB(cm, styleSheet->style.trackBackground);
        styleSheet->style.penSoundBackground = ObtainPenForRGB(cm, styleSheet->style.soundBackground);
        styleSheet->style.penSelectedBackground = ObtainPenForRGB(cm, styleSheet->style.selectedBackground);
        styleSheet->style.penWaveformDark = ObtainPenForRGB(cm, styleSheet->style.waveformDark);
        styleSheet->style.penWaveformLight = ObtainPenForRGB(cm, styleSheet->style.waveformLight);
        styleSheet->style.penText = ObtainPenForRGB(cm, styleSheet->style.textColor);

        /* Obtain fixed white and black pens */
        styleSheet->style.penWhite = ObtainPenForRGB(cm, 0x00FFFFFF);
        styleSheet->style.penBlack = ObtainPenForRGB(cm, 0x00000000);
    }

    /* Close any existing fonts first */
    AukStyleSheet_CloseFonts(This);

    /* Open fonts from specifications */
    if (styleSheet->fontTinyName && styleSheet->fontTinyHeight > 0) {
        styleSheet->style.fontTiny = OpenFontBySpec(styleSheet->fontTinyName, styleSheet->fontTinyHeight,&styleSheet->style.fontTiny_TA);
        /* Fallback: minimal font installed by OS3.2 */
        if(!styleSheet->style.fontTiny)  styleSheet->style.fontTiny = OpenFontBySpec("SevenAlone.font", 7,&styleSheet->style.fontTiny_TA);
        /* Fallback: historic default minimal font */
        if(!styleSheet->style.fontTiny)  styleSheet->style.fontTiny = OpenFontBySpec("Topaz.font", 8,&styleSheet->style.fontTiny_TA);
        if (!styleSheet->style.fontTiny) success = 0;
    }

    if (styleSheet->fontNormalName && styleSheet->fontNormalHeight > 0) {
        styleSheet->style.fontNormal = OpenFontBySpec(styleSheet->fontNormalName, styleSheet->fontNormalHeight,&styleSheet->style.fontNormal_TA);
        if(!styleSheet->style.fontNormal)  styleSheet->style.fontNormal = OpenFontBySpec("Topaz.font", 9,&styleSheet->style.fontNormal_TA);

        if (!styleSheet->style.fontNormal) success = 0;

        /* Update reference font height */
        if (styleSheet->style.fontNormal) {
            styleSheet->style.fontHeight = styleSheet->style.fontNormal->tf_YSize;
        }
    }

    if (styleSheet->fontBigName && styleSheet->fontBigHeight > 0) {
        styleSheet->style.fontBig = OpenFontBySpec(styleSheet->fontBigName, styleSheet->fontBigHeight, &styleSheet->style.fontBig_TA);
        if (!styleSheet->style.fontBig) success = 0;
    }

    return success;
}

/* AukStyleSheet_ReleasePens - Release all obtained pens */
void AukStyleSheet_ReleasePens(void* This) {
    AukStyleSheet* styleSheet = (AukStyleSheet*)This;
    struct ColorMap *cm;

    if (!styleSheet || !styleSheet->screen) {
        return;
    }

    cm = styleSheet->screen->ViewPort.ColorMap;

    /* Release all pens */
    ReleasePenIfValid(cm, &styleSheet->style.penBackground);
    ReleasePenIfValid(cm, &styleSheet->style.penTrackBackground);
    ReleasePenIfValid(cm, &styleSheet->style.penSoundBackground);
    ReleasePenIfValid(cm, &styleSheet->style.penSelectedBackground);
    ReleasePenIfValid(cm, &styleSheet->style.penWaveformDark);
    ReleasePenIfValid(cm, &styleSheet->style.penWaveformLight);
    ReleasePenIfValid(cm, &styleSheet->style.penText);
    ReleasePenIfValid(cm, &styleSheet->style.penWhite);
    ReleasePenIfValid(cm, &styleSheet->style.penBlack);

    styleSheet->screen = NULL;
}

void AukStyleSheet_CloseFonts(void* This) {
    AukStyleSheet* styleSheet = (AukStyleSheet*)This;

    if (!styleSheet) {
        return;
    }

    if (styleSheet->style.fontTiny) {
        CloseFont(styleSheet->style.fontTiny);
        styleSheet->style.fontTiny = NULL;
    }

    if (styleSheet->style.fontNormal) {
        CloseFont(styleSheet->style.fontNormal);
        styleSheet->style.fontNormal = NULL;
    }

    if (styleSheet->style.fontBig) {
        CloseFont(styleSheet->style.fontBig);
        styleSheet->style.fontBig = NULL;
    }
}

int AukStyleSheet_SetBackground(AukStyleSheet* This, ULONG color) {
    if (!This) return 0;

    if (This->style.background != color) {
        This->style.background = color;
    }
    return 1;
}

int AukStyleSheet_SetTrackBackground(AukStyleSheet* This, ULONG color) {
    if (!This) return 0;

    if (This->style.trackBackground != color) {
        This->style.trackBackground = color;
    }
    return 1;
}

int AukStyleSheet_SetSoundBackground(AukStyleSheet* This, ULONG color) {
    if (!This) return 0;

    if (This->style.soundBackground != color) {
        This->style.soundBackground = color;
    }
    return 1;
}

int AukStyleSheet_SetSelectedBackground(AukStyleSheet* This, ULONG color) {
    if (!This) return 0;

    if (This->style.selectedBackground != color) {
        This->style.selectedBackground = color;
    }
    return 1;
}

int AukStyleSheet_SetWaveformDark(AukStyleSheet* This, ULONG color) {
    if (!This) return 0;

    if (This->style.waveformDark != color) {
        This->style.waveformDark = color;
    }
    return 1;
}

int AukStyleSheet_SetWaveformLight(AukStyleSheet* This, ULONG color) {
    if (!This) return 0;

    if (This->style.waveformLight != color) {
        This->style.waveformLight = color;
    }
    return 1;
}

int AukStyleSheet_SetTextColor(AukStyleSheet* This, ULONG color) {
    if (!This) return 0;

    if (This->style.textColor != color) {
        This->style.textColor = color;
    }
    return 1;
}

void AukStyleSheet_Init(AukStyleSheet* styleSheet) {
    if (styleSheet) {
        /* Initialize base object */
        AukObject_Init(&styleSheet->base);

        /* Override virtual methods */
        styleSheet->base.New = (void (*)(AukObjectPtr*))AukStyleSheet_New;
        styleSheet->base.Delete = AukStyleSheet_Delete;
        styleSheet->base.GetTypeName = (const char* (*)(AukObject*))AukStyleSheet_GetTypeName;
        styleSheet->base.Serialize = AukStyleSheet_Serialize;

        /* Set AukStyleSheet specific methods */
        styleSheet->SetFontTiny = AukStyleSheet_SetFontTiny;
        styleSheet->SetFontNormal = AukStyleSheet_SetFontNormal;
        styleSheet->SetFontBig = AukStyleSheet_SetFontBig;
        styleSheet->ApplyStyle = AukStyleSheet_ApplyStyle;
        styleSheet->ReleasePens = AukStyleSheet_ReleasePens;
        styleSheet->CloseFonts = AukStyleSheet_CloseFonts;

        /* Initialize colors with Audacity-like defaults */
        styleSheet->style.background = 0x00464646;      /* Main background gray */
        styleSheet->style.trackBackground = 0x00303030; /* Track empty area - dark gray */
        styleSheet->style.soundBackground = 0x00454555; /* Sound clip area - slight blue tint */
        styleSheet->style.selectedBackground = 0x005566AA; /* Selected region - blue highlight */
        styleSheet->style.waveformDark = 0x00214783;    /* Dark blue for waveform min/max */
        styleSheet->style.waveformLight = 0x004464C0;   /* Lighter blue for waveform RMS */
        styleSheet->style.textColor = 0x00FFFFFF;       /* White */

        /* Initialize pen indices to -1 (not allocated) */
        styleSheet->style.penBackground = -1;
        styleSheet->style.penTrackBackground = -1;
        styleSheet->style.penSoundBackground = -1;
        styleSheet->style.penSelectedBackground = -1;
        styleSheet->style.penWaveformDark = -1;
        styleSheet->style.penWaveformLight = -1;
        styleSheet->style.penText = -1;
        styleSheet->style.penWhite = -1;
        styleSheet->style.penBlack = -1;

        /* Screen not yet set */
        styleSheet->screen = NULL;

        /* Initialize font pointers to NULL */
        styleSheet->style.fontTiny = NULL;
        styleSheet->style.fontNormal = NULL;
        styleSheet->style.fontBig = NULL;

        /* Initialize font specification strings to NULL */
        styleSheet->fontTinyName = NULL;
        styleSheet->fontNormalName = NULL;
        styleSheet->fontBigName = NULL;

        /* Initialize font heights to defaults */
        styleSheet->fontTinyHeight = 8;
        styleSheet->fontNormalHeight = 11;
        styleSheet->fontBigHeight = 15;

        /* Initialize reference font height */
        styleSheet->style.fontHeight = 11;
    }
}
