#ifndef GADGETS_VOLUMERULE_H
#define GADGETS_VOLUMERULE_H
/**
 * Definitions for Gadget VolumeRule
 * (This is the public file that can be released when publishing just the .gadget)
 *
 * VolumeRule draws a vertical audio volume scale with marks at
 * -1, -0.5, 0, 0.5, 1.0 values, with horizontal lines.
 * The gadget height represents the volume range [-1, 1].
 */
#include <exec/types.h>
#include <intuition/gadgetclass.h>
#include <intuition/classes.h>

#define VERSION_VOLUMERULE 1
/* VolumeRule inherits directly from gadgetclass */
#define VolumeRule_SUPERCLASS_ID "gadgetclass"

#ifdef VOLUMERULE_STATICLINK
    extern int VolumeRuleStaticInit();
    extern void VolumeRuleStaticClose();
    extern Class *VOLUMERULE_GetClass();
#else
    #define VolumeRule_CLASS_ID "volumerule.gadget"

    Class * __stdargs VOLUMERULE_GetClass( void );

    #ifndef _NO_INLINE
        # if defined(__GNUC__)
            #include <inline/macros.h>
            #define VOLUMERULE_GetClass() LP0(0x1e, Class *, VOLUMERULE_GetClass ,, VolumeRuleBase)
        # endif
        #if defined(LATTICE) || defined(__SASC) || defined(_DCC)
           #pragma libcall VolumeRuleBase VOLUMERULE_GetClass 1e 00
        # endif
        #if defined(__VBCC__)
            Class * __VOLUMERULE_GetClass(__reg("a6") void *)="\tjsr\t-$1e(a6)";
            #define VOLUMERULE_GetClass() __VOLUMERULE_GetClass(VolumeRuleBase)
        #endif
    #endif /* _NO_INLINE */

    #ifndef __NOLIBBASE__
      extern struct Library *
        # ifdef __CONSTLIBBASEDECL__
       __CONSTLIBBASEDECL__
        # endif /* __CONSTLIBBASEDECL__ */
      VolumeRuleBase;
    #endif /* !__NOLIBBASE__ */

#endif

/**  Attributes defined by the gadget class,
 * all attribs from gadgetclass.h are also valid.
 */
#define VOLUMERULE_Dummy        (TAG_USER+0x04520000)

/* Pointer to AukStyle for visual styling (fonts, colors) */
#define VOLUMERULE_StyleSheet   (VOLUMERULE_Dummy+1)

/* Force full redraw */
#define VOLUMERULE_Refresh      (VOLUMERULE_Dummy+2)

#endif
