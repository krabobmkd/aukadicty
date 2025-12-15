#ifndef GADGETS_TIMERULE_H
#define GADGETS_TIMERULE_H
/**
 * Definitions for Gadget TimeRule
 * (This is the public file that can be released when publishing just the .gadget)
 */
#include <exec/types.h>
#include <intuition/gadgetclass.h>
#include <intuition/classes.h>
            #include <inline/macros.h>
#define VERSION_TIMERULE 1
#define TimeRule_SUPERCLASS_ID "gadgetclass"

#ifdef TIMERULE_STATICLINK
    extern int TimeRuleStaticInit();
    extern void TimeRuleStaticClose();
    extern Class *TIMERULE_GetClass();
#else
    // TimeRule_CLASS_ID is the identifier for this class, when shared.
    // NewObject() can use either TIMERULE_GetClass() or TimeRule_CLASS_ID.
    #define TimeRule_CLASS_ID "timerule.gadget"

    // the following is to define function with implicit library call for TIMERULE_GetClass().
    // note it should be in includes generated from a fd files.
    Class * __stdargs TIMERULE_GetClass( void );
    // ... could have other functions here

    #ifndef _NO_INLINE
        # if defined(__GNUC__)
            #include <inline/macros.h>
            #define TIMERULE_GetClass() LP0(0x1e, Class *, TIMERULE_GetClass ,, TimeRuleBase)
            // ... could have other functions here
        # endif
        #if defined(LATTICE) || defined(__SASC) || defined(_DCC)
           #pragma libcall TimeRuleBase TIMERULE_GetClass 1e 00
            // ... could have other functions here
        # endif
        #if defined(__VBCC__)
            Class * __TIMERULE_GetClass(__reg("a6") void *)="\tjsr\t-$1e(a6)";
            #define TIMERULE_GetClass() __TIMERULE_GetClass(TimeRuleBase)
			// ... could have other functions here			
        #endif
    #endif /* _NO_INLINE */

    #ifndef __NOLIBBASE__
      extern struct Library *
        # ifdef __CONSTLIBBASEDECL__
       __CONSTLIBBASEDECL__
        # endif /* __CONSTLIBBASEDECL__ */
      TimeRuleBase;
    #endif /* !__NOLIBBASE__ */

// end if dynamic link
#endif

/**  Attributes defined by the gadget class,
 * all attribs from gadgetclass.h are also valid.
 */
// different classes may not use same base.
//DEVTODO: have another offset for your new class to not collide super class ones and optimize...
#define TIMERULE_Dummy			(TAG_USER+0x04110000)

// abstract coordinate from 0 to 65535 , whatever width is.
#define	TIMERULE_CenterX		(TIMERULE_Dummy+1)
// abstract coordinate from 0 to 65535, whatever height is.
#define	TIMERULE_CenterY		(TIMERULE_Dummy+2)

/** DEVTODO: adds attributes definitions here and
 * manage them in class_timerule_attribs.c
 */
#endif
