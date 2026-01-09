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
 */

/* Internal helper to open a single font */
static struct TextFont* OpenFontBySpec(const char* name, int height) {
    struct TextAttr ta;
    struct TextFont* font = NULL;

    if (!name || height <= 0) {
        return NULL;
    }

    ta.ta_Name = (STRPTR)name;
    ta.ta_YSize = (UWORD)height;
    ta.ta_Style = FS_NORMAL;
    ta.ta_Flags = FPF_DISKFONT;

    font = OpenDiskFont(&ta);
    if (!font) {
        /* Fallback: try ROM font */
        ta.ta_Flags = FPF_ROMFONT;
        font = OpenFont(&ta);
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
    ser->t_uint(ser, "waveShape", &styleSheet->style.waveShape);
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

/* AukStyleSheet_Update - Synchronize font specifications to runtime fonts
 *
 * This method synchronizes the serialized font specifications (fontTinyName,
 * fontTinyHeight, etc.) to the actual runtime font pointers in style.fontTiny, etc.
 * Call this after setting font specifications with SetFontXxx() methods, or after
 * deserializing from disk to apply the loaded font specifications.
 */
int AukStyleSheet_ApplyStyle(void* This) {
    AukStyleSheet* styleSheet = (AukStyleSheet*)This;
    int success = 1;

    if (!styleSheet) {
        return 0;
    }
    /*
        Note: there are fonts described in preferences,
        but this is "negociated" with:
         - our prefs.
         - the boopsi settings.
         - what is available on the os.
    */

    /* Close any existing fonts first */
    AukStyleSheet_CloseFonts(This);

    /* Open fonts from specifications */
    if (styleSheet->fontTinyName && styleSheet->fontTinyHeight > 0) {
        styleSheet->style.fontTiny = OpenFontBySpec(styleSheet->fontTinyName, styleSheet->fontTinyHeight);
        // this is a minimal font installed by OS3.2
        if(!styleSheet->style.fontTiny)  styleSheet->style.fontTiny = OpenFontBySpec("SevenAlone.font", 7);
        // this is the historic default minimal font
        if(!styleSheet->style.fontTiny)  styleSheet->style.fontTiny = OpenFontBySpec("Topaz.font", 8);
        if (!styleSheet->style.fontTiny) success = 0;
    }

    if (styleSheet->fontNormalName && styleSheet->fontNormalHeight > 0) {
        /*  */

        styleSheet->style.fontNormal = OpenFontBySpec(styleSheet->fontNormalName, styleSheet->fontNormalHeight);
        if(!styleSheet->style.fontNormal)  styleSheet->style.fontNormal = OpenFontBySpec("Topaz.font", 9);

        if (!styleSheet->style.fontNormal) success = 0;
        // this is the historic default minimal font

        /* Update reference font height */
        if (styleSheet->style.fontNormal) {
            styleSheet->style.fontHeight = styleSheet->style.fontNormal->tf_YSize;
        }
    }

    if (styleSheet->fontBigName && styleSheet->fontBigHeight > 0) {
        styleSheet->style.fontBig = OpenFontBySpec(styleSheet->fontBigName, styleSheet->fontBigHeight);
        if (!styleSheet->style.fontBig) success = 0;
    }

    return success;
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

int AukStyleSheet_SetWaveShape(AukStyleSheet* This, ULONG color) {
    if (!This) return 0;

    if (This->style.waveShape != color) {
        This->style.waveShape = color;
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
        styleSheet->CloseFonts = AukStyleSheet_CloseFonts;

        /* Initialize colors with sensible defaults */
        styleSheet->style.background = 0x00808080;      /* Gray */
        styleSheet->style.trackBackground = 0x00404040; /* Dark gray */
        styleSheet->style.waveShape = 0x0000FF00;       /* Green */
        styleSheet->style.textColor = 0x00FFFFFF;       /* White */

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
