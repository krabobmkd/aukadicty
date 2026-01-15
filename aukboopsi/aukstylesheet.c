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

/* Internal helper to obtain a pen for a ManagedColor.
 * Converts 0x00RRGGBB format to 32-bit per component for ObtainBestPenA.
 * Sets c->pen and c->allocated appropriately.
 */
static void ObtainPenForRGB(struct ColorMap *cm, ManagedColor *c) {
    ULONG r, g, b;
    LONG pen;

    if (!cm || !c) return;

    /* Extract 8-bit components and expand to 32-bit (0xFF -> 0xFFFFFFFF) */
    r = ((c->rgbcolor >> 16) & 0xFF);
    g = ((c->rgbcolor >> 8) & 0xFF);
    b = (c->rgbcolor & 0xFF);

    /* Expand to full 32-bit range as expected by ObtainBestPenA */
    r = (r << 24) | (r << 16) | (r << 8) | r;
    g = (g << 24) | (g << 16) | (g << 8) | g;
    b = (b << 24) | (b << 16) | (b << 8) | b;

    pen = ObtainBestPenA(cm, r, g, b, NULL);

    if (pen != -1) {
        c->pen = (WORD)pen;
        c->allocated = 1;
    } else {
        c->pen = (WORD)FindColor(cm, r, g, b, 255);
        c->allocated = 0;
    }
}

/* Internal helper to release a pen if it was allocated via ObtainBestPenA */
static void ReleasePenIfValid(struct ColorMap *cm, ManagedColor *c) {
    if (!cm || !c) return;
    if (c->allocated && c->pen >= 0) {
        ReleasePen(cm, c->pen);
    }
    c->pen = -1;
    c->allocated = 0;
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

void AukStyleSheet_Delete(AukObject* This) {
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

void AukStyleSheet_Serialize(AukObject* This, ISerializer* ser, const char* pName) {
    AukStyleSheet* styleSheet = (AukStyleSheet*)This;
    (void)pName;

    if (!styleSheet || !ser) {
        return;
    }

    /* Serialize colors (only rgbcolor field, pen/allocated are runtime) */
    ser->t_uint(ser, "background", &styleSheet->style.background.rgbcolor);
    ser->t_uint(ser, "trackBackground", &styleSheet->style.trackBackground.rgbcolor);
    ser->t_uint(ser, "soundBackground", &styleSheet->style.soundBackground.rgbcolor);
    ser->t_uint(ser, "selectedBackground", &styleSheet->style.selectedBackground.rgbcolor);
    ser->t_uint(ser, "waveformDark", &styleSheet->style.waveformDark.rgbcolor);
    ser->t_uint(ser, "waveformLight", &styleSheet->style.waveformLight.rgbcolor);
    ser->t_uint(ser, "textColor", &styleSheet->style.textColor.rgbcolor);

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
        ObtainPenForRGB(cm, &styleSheet->style.background);
        ObtainPenForRGB(cm, &styleSheet->style.trackBackground);
        ObtainPenForRGB(cm, &styleSheet->style.soundBackground);
        ObtainPenForRGB(cm, &styleSheet->style.selectedBackground);
        ObtainPenForRGB(cm, &styleSheet->style.waveformDark);
        ObtainPenForRGB(cm, &styleSheet->style.waveformLight);
        ObtainPenForRGB(cm, &styleSheet->style.textColor);
        ObtainPenForRGB(cm, &styleSheet->style.white);
        ObtainPenForRGB(cm, &styleSheet->style.black);
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
    ReleasePenIfValid(cm, &styleSheet->style.background);
    ReleasePenIfValid(cm, &styleSheet->style.trackBackground);
    ReleasePenIfValid(cm, &styleSheet->style.soundBackground);
    ReleasePenIfValid(cm, &styleSheet->style.selectedBackground);
    ReleasePenIfValid(cm, &styleSheet->style.waveformDark);
    ReleasePenIfValid(cm, &styleSheet->style.waveformLight);
    ReleasePenIfValid(cm, &styleSheet->style.textColor);
    ReleasePenIfValid(cm, &styleSheet->style.white);
    ReleasePenIfValid(cm, &styleSheet->style.black);

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

    if (This->style.background.rgbcolor != color) {
        This->style.background.rgbcolor = color;
    }
    return 1;
}

int AukStyleSheet_SetTrackBackground(AukStyleSheet* This, ULONG color) {
    if (!This) return 0;

    if (This->style.trackBackground.rgbcolor != color) {
        This->style.trackBackground.rgbcolor = color;
    }
    return 1;
}

int AukStyleSheet_SetSoundBackground(AukStyleSheet* This, ULONG color) {
    if (!This) return 0;

    if (This->style.soundBackground.rgbcolor != color) {
        This->style.soundBackground.rgbcolor = color;
    }
    return 1;
}

int AukStyleSheet_SetSelectedBackground(AukStyleSheet* This, ULONG color) {
    if (!This) return 0;

    if (This->style.selectedBackground.rgbcolor != color) {
        This->style.selectedBackground.rgbcolor = color;
    }
    return 1;
}

int AukStyleSheet_SetWaveformDark(AukStyleSheet* This, ULONG color) {
    if (!This) return 0;

    if (This->style.waveformDark.rgbcolor != color) {
        This->style.waveformDark.rgbcolor = color;
    }
    return 1;
}

int AukStyleSheet_SetWaveformLight(AukStyleSheet* This, ULONG color) {
    if (!This) return 0;

    if (This->style.waveformLight.rgbcolor != color) {
        This->style.waveformLight.rgbcolor = color;
    }
    return 1;
}

int AukStyleSheet_SetTextColor(AukStyleSheet* This, ULONG color) {
    if (!This) return 0;

    if (This->style.textColor.rgbcolor != color) {
        This->style.textColor.rgbcolor = color;
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
        styleSheet->style.background.rgbcolor = 0x00333355;      /* Main background gray */
        styleSheet->style.background.pen = -1;
        styleSheet->style.background.allocated = 0;

        styleSheet->style.trackBackground.rgbcolor = 0x00464656; /* Track empty area - dark gray */
        styleSheet->style.trackBackground.pen = -1;
        styleSheet->style.trackBackground.allocated = 0;

        styleSheet->style.soundBackground.rgbcolor = 0x00757575; /* Sound clip area - slight blue tint */
        styleSheet->style.soundBackground.pen = -1;
        styleSheet->style.soundBackground.allocated = 0;

        styleSheet->style.selectedBackground.rgbcolor = 0x005566AA; /* Selected region - blue highlight */
        styleSheet->style.selectedBackground.pen = -1;
        styleSheet->style.selectedBackground.allocated = 0;

        styleSheet->style.waveformDark.rgbcolor = 0x00214783;    /* Dark blue for waveform min/max */
        styleSheet->style.waveformDark.pen = -1;
        styleSheet->style.waveformDark.allocated = 0;

        styleSheet->style.waveformLight.rgbcolor = 0x004464C0;   /* Lighter blue for waveform RMS */
        styleSheet->style.waveformLight.pen = -1;
        styleSheet->style.waveformLight.allocated = 0;

        styleSheet->style.textColor.rgbcolor = 0x00FFFFFF;       /* White */
        styleSheet->style.textColor.pen = -1;
        styleSheet->style.textColor.allocated = 0;

        styleSheet->style.white.rgbcolor = 0x00FFFFFF;
        styleSheet->style.white.pen = -1;
        styleSheet->style.white.allocated = 0;

        styleSheet->style.black.rgbcolor = 0x00000000;
        styleSheet->style.black.pen = -1;
        styleSheet->style.black.allocated = 0;

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
