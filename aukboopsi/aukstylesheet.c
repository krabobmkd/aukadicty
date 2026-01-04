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
    ser->t_uint(ser, "background", &styleSheet->background);
    ser->t_uint(ser, "trackBackground", &styleSheet->trackBackground);
    ser->t_uint(ser, "waveShape", &styleSheet->waveShape);
    ser->t_uint(ser, "textColor", &styleSheet->textColor);

    /* Serialize font specifications (name + height for each font) */
    ser->t_string_mutable(ser, "fontTinyName", &styleSheet->fontTinyName);
    ser->t_int(ser, "fontTinyHeight", &styleSheet->fontTinyHeight);

    ser->t_string_mutable(ser, "fontNormalName", &styleSheet->fontNormalName);
    ser->t_int(ser, "fontNormalHeight", &styleSheet->fontNormalHeight);

    ser->t_string_mutable(ser, "fontBigName", &styleSheet->fontBigName);
    ser->t_int(ser, "fontBigHeight", &styleSheet->fontBigHeight);

    /* Serialize reference font height */
    ser->t_int(ser, "fontHeight", &styleSheet->fontHeight);

    /* After reading, open fonts from loaded specifications */
    if (IS_READING(ser)) {
        AukStyleSheet_OpenFonts(This);
    }
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

    /* Close old font if open */
    if (styleSheet->fontTiny) {
        CloseFont(styleSheet->fontTiny);
        styleSheet->fontTiny = NULL;
    }

    /* Set new specification */
    styleSheet->fontTinyName = AukString_Duplicate(name);
    styleSheet->fontTinyHeight = height;

    if (styleSheet->fontTinyName) {
        /* Open new font */
        styleSheet->fontTiny = OpenFontBySpec(name, height);

        /* Send update notification */
        {
            AukMessage msg;
            msg.type = AUK_MSG_MODIFY;
            styleSheet->base.SendUpdate(&styleSheet->base, &msg);
        }
    }

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

    /* Close old font if open */
    if (styleSheet->fontNormal) {
        CloseFont(styleSheet->fontNormal);
        styleSheet->fontNormal = NULL;
    }

    /* Set new specification */
    styleSheet->fontNormalName = AukString_Duplicate(name);
    styleSheet->fontNormalHeight = height;

    if (styleSheet->fontNormalName) {
        /* Open new font */
        styleSheet->fontNormal = OpenFontBySpec(name, height);

        /* Update reference font height */
        if (styleSheet->fontNormal) {
            styleSheet->fontHeight = styleSheet->fontNormal->tf_YSize;
        } else {
            styleSheet->fontHeight = height;
        }

        /* Send update notification */
        {
            AukMessage msg;
            msg.type = AUK_MSG_MODIFY;
            styleSheet->base.SendUpdate(&styleSheet->base, &msg);
        }
    }

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

    /* Close old font if open */
    if (styleSheet->fontBig) {
        CloseFont(styleSheet->fontBig);
        styleSheet->fontBig = NULL;
    }

    /* Set new specification */
    styleSheet->fontBigName = AukString_Duplicate(name);
    styleSheet->fontBigHeight = height;

    if (styleSheet->fontBigName) {
        /* Open new font */
        styleSheet->fontBig = OpenFontBySpec(name, height);

        /* Send update notification */
        {
            AukMessage msg;
            msg.type = AUK_MSG_MODIFY;
            styleSheet->base.SendUpdate(&styleSheet->base, &msg);
        }
    }

    return styleSheet->fontBigName != NULL;
}

int AukStyleSheet_OpenFonts(void* This) {
    AukStyleSheet* styleSheet = (AukStyleSheet*)This;
    int success = 1;

    if (!styleSheet) {
        return 0;
    }

    /* Close any existing fonts first */
    AukStyleSheet_CloseFonts(This);

    /* Open fonts from specifications */
    if (styleSheet->fontTinyName && styleSheet->fontTinyHeight > 0) {
        styleSheet->fontTiny = OpenFontBySpec(styleSheet->fontTinyName, styleSheet->fontTinyHeight);
        if (!styleSheet->fontTiny) success = 0;
    }

    if (styleSheet->fontNormalName && styleSheet->fontNormalHeight > 0) {
        styleSheet->fontNormal = OpenFontBySpec(styleSheet->fontNormalName, styleSheet->fontNormalHeight);
        if (!styleSheet->fontNormal) success = 0;

        /* Update reference font height */
        if (styleSheet->fontNormal) {
            styleSheet->fontHeight = styleSheet->fontNormal->tf_YSize;
        }
    }

    if (styleSheet->fontBigName && styleSheet->fontBigHeight > 0) {
        styleSheet->fontBig = OpenFontBySpec(styleSheet->fontBigName, styleSheet->fontBigHeight);
        if (!styleSheet->fontBig) success = 0;
    }

    return success;
}

void AukStyleSheet_CloseFonts(void* This) {
    AukStyleSheet* styleSheet = (AukStyleSheet*)This;

    if (!styleSheet) {
        return;
    }

    if (styleSheet->fontTiny) {
        CloseFont(styleSheet->fontTiny);
        styleSheet->fontTiny = NULL;
    }

    if (styleSheet->fontNormal) {
        CloseFont(styleSheet->fontNormal);
        styleSheet->fontNormal = NULL;
    }

    if (styleSheet->fontBig) {
        CloseFont(styleSheet->fontBig);
        styleSheet->fontBig = NULL;
    }
}

int AukStyleSheet_SetBackground(AukStyleSheet* This, ULONG color) {
    if (!This) return 0;

    if (This->background != color) {
        This->background = color;
        {
            AukMessage msg;
            msg.type = AUK_MSG_MODIFY;
            This->base.SendUpdate(&This->base, &msg);
        }
    }
    return 1;
}

int AukStyleSheet_SetTrackBackground(AukStyleSheet* This, ULONG color) {
    if (!This) return 0;

    if (This->trackBackground != color) {
        This->trackBackground = color;
        {
            AukMessage msg;
            msg.type = AUK_MSG_MODIFY;
            This->base.SendUpdate(&This->base, &msg);
        }
    }
    return 1;
}

int AukStyleSheet_SetWaveShape(AukStyleSheet* This, ULONG color) {
    if (!This) return 0;

    if (This->waveShape != color) {
        This->waveShape = color;
        {
            AukMessage msg;
            msg.type = AUK_MSG_MODIFY;
            This->base.SendUpdate(&This->base, &msg);
        }
    }
    return 1;
}

int AukStyleSheet_SetTextColor(AukStyleSheet* This, ULONG color) {
    if (!This) return 0;

    if (This->textColor != color) {
        This->textColor = color;
        {
            AukMessage msg;
            msg.type = AUK_MSG_MODIFY;
            This->base.SendUpdate(&This->base, &msg);
        }
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
        styleSheet->OpenFonts = AukStyleSheet_OpenFonts;
        styleSheet->CloseFonts = AukStyleSheet_CloseFonts;

        /* Initialize colors with sensible defaults */
        styleSheet->background = 0x00808080;      /* Gray */
        styleSheet->trackBackground = 0x00404040; /* Dark gray */
        styleSheet->waveShape = 0x0000FF00;       /* Green */
        styleSheet->textColor = 0x00FFFFFF;       /* White */

        /* Initialize font pointers to NULL */
        styleSheet->fontTiny = NULL;
        styleSheet->fontNormal = NULL;
        styleSheet->fontBig = NULL;

        /* Initialize font specification strings to NULL */
        styleSheet->fontTinyName = NULL;
        styleSheet->fontNormalName = NULL;
        styleSheet->fontBigName = NULL;

        /* Initialize font heights to defaults */
        styleSheet->fontTinyHeight = 8;
        styleSheet->fontNormalHeight = 11;
        styleSheet->fontBigHeight = 15;

        /* Initialize reference font height */
        styleSheet->fontHeight = 11;
    }
}
