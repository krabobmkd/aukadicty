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

/* Serialization keys for each color role */
static const char* colorSerKeys[AUK_COLOR_COUNT] = {
    "bg",           /* AUK_COLOR_BACKGROUND */
    "trBg",         /* AUK_COLOR_TRACK_BACKGROUND */
    "sndBg",        /* AUK_COLOR_SOUND_BACKGROUND */
    "selBg",        /* AUK_COLOR_SELECTED_BACKGROUND */
    "selSd",        /* AUK_COLOR_SELECTED_SOUND_BG */
    "trackhl",      /* AUK_COLOR_TRACK_HIGHLIGHT */
    "trackhl2",     /* AUK_COLOR_TRACK_HIGHLIGHT2 */
    "wvDark",       /* AUK_COLOR_WAVEFORM_DARK */
    "wvLight",      /* AUK_COLOR_WAVEFORM_LIGHT */
    "thbg",         /* AUK_COLOR_TRACK_HEADER_BG */
    "txtCol",       /* AUK_COLOR_TEXT */
    "white",        /* AUK_COLOR_WHITE */
    "black"         /* AUK_COLOR_BLACK */
};

void AukStyleSheet_Serialize(AukObject* This, ISerializer* ser, const char* pName) {
    AukStyleSheet* styleSheet = (AukStyleSheet*)This;
    int i;
    (void)pName;

    if (!styleSheet || !ser) {
        return;
    }

    /* Serialize colors using loop (only rgbcolor field, pen/allocated are runtime) */
    for (i = 0; i < AUK_COLOR_COUNT; i++) {
        ser->t_uint(ser, colorSerKeys[i], (ULONG*)&styleSheet->style.pens[i].rgbcolor);
    }

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
    int i;

    if (!styleSheet) {
        return 0;
    }

    /* Release any previously obtained pens */
    AukStyleSheet_ReleasePens(This);

    /* Store screen reference and obtain pens */
    styleSheet->screen = scr;
    if (scr) {
        cm = scr->ViewPort.ColorMap;

        /* Obtain pens for all colors using loop */
        for (i = 0; i < AUK_COLOR_COUNT; i++) {
            ObtainPenForRGB(cm, &styleSheet->style.pens[i]);
        }
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
    int i;

    if (!styleSheet || !styleSheet->screen) {
        return;
    }

    cm = styleSheet->screen->ViewPort.ColorMap;

    /* Release all pens using loop */
    for (i = 0; i < AUK_COLOR_COUNT; i++) {
        ReleasePenIfValid(cm, &styleSheet->style.pens[i]);
    }

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

    if (This->style.pens[AUK_COLOR_BACKGROUND].rgbcolor != color) {
        This->style.pens[AUK_COLOR_BACKGROUND].rgbcolor = color;
    }
    return 1;
}

int AukStyleSheet_SetTrackBackground(AukStyleSheet* This, ULONG color) {
    if (!This) return 0;

    if (This->style.pens[AUK_COLOR_TRACK_BACKGROUND].rgbcolor != color) {
        This->style.pens[AUK_COLOR_TRACK_BACKGROUND].rgbcolor = color;
    }
    return 1;
}

int AukStyleSheet_SetSoundBackground(AukStyleSheet* This, ULONG color) {
    if (!This) return 0;

    if (This->style.pens[AUK_COLOR_SOUND_BACKGROUND].rgbcolor != color) {
        This->style.pens[AUK_COLOR_SOUND_BACKGROUND].rgbcolor = color;
    }
    return 1;
}

int AukStyleSheet_SetSelectedBackground(AukStyleSheet* This, ULONG color) {
    if (!This) return 0;

    if (This->style.pens[AUK_COLOR_SELECTED_BACKGROUND].rgbcolor != color) {
        This->style.pens[AUK_COLOR_SELECTED_BACKGROUND].rgbcolor = color;
    }
    return 1;
}

int AukStyleSheet_SetWaveformDark(AukStyleSheet* This, ULONG color) {
    if (!This) return 0;

    if (This->style.pens[AUK_COLOR_WAVEFORM_DARK].rgbcolor != color) {
        This->style.pens[AUK_COLOR_WAVEFORM_DARK].rgbcolor = color;
    }
    return 1;
}

int AukStyleSheet_SetWaveformLight(AukStyleSheet* This, ULONG color) {
    if (!This) return 0;

    if (This->style.pens[AUK_COLOR_WAVEFORM_LIGHT].rgbcolor != color) {
        This->style.pens[AUK_COLOR_WAVEFORM_LIGHT].rgbcolor = color;
    }
    return 1;
}

int AukStyleSheet_SetTextColor(AukStyleSheet* This, ULONG color) {
    if (!This) return 0;

    if (This->style.pens[AUK_COLOR_TEXT].rgbcolor != color) {
        This->style.pens[AUK_COLOR_TEXT].rgbcolor = color;
    }
    return 1;
}

/* Default color values for each role (Audacity-like palette) */
static const ULONG defaultColors[AUK_COLOR_COUNT] = {
    0x00333355,  /* AUK_COLOR_BACKGROUND - Main background gray */
    0x00464656,  /* AUK_COLOR_TRACK_BACKGROUND - Track empty area dark gray */
    0x00757575,  /* AUK_COLOR_SOUND_BACKGROUND - Sound clip area */
    0x005566AA,  /* AUK_COLOR_SELECTED_BACKGROUND - Selected region blue */
    0x007575BB,  /* AUK_COLOR_SELECTED_SOUND_BG - Selected sound clip */
    0x00FFDD00,  /* AUK_COLOR_TRACK_HIGHLIGHT - Track selection highlight */
    0x00EE8800,  /* AUK_COLOR_TRACK_HIGHLIGHT2 - Track highlight secondary */
    0x00214783,  /* AUK_COLOR_WAVEFORM_DARK - Waveform min/max dark blue */
    0x004464C0,  /* AUK_COLOR_WAVEFORM_LIGHT - Waveform RMS lighter blue */
    0x008888FF,  /* AUK_COLOR_TRACK_HEADER_BG - Track header background */
    0x00FFFFFF,  /* AUK_COLOR_TEXT - Text color white */
    0x00FFFFFF,  /* AUK_COLOR_WHITE - Always white */
    0x00000000   /* AUK_COLOR_BLACK - Always black */
};

void AukStyleSheet_Init(AukStyleSheet* styleSheet) {
    int i;

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

        /* Initialize all colors with defaults using loop */
        for (i = 0; i < AUK_COLOR_COUNT; i++) {
            styleSheet->style.pens[i].rgbcolor = defaultColors[i];
            styleSheet->style.pens[i].pen = 1;  /* Default pen 1, always valid */
            styleSheet->style.pens[i].allocated = 0;
        }

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

        styleSheet->style.borderSelectionWidth = 4;
    }
}
