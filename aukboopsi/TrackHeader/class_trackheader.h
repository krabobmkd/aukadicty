#ifndef GADGETS_TRACKHEADER_H
#define GADGETS_TRACKHEADER_H
/**
 * Definitions for Gadget TrackHeader
 * (This is the public file that can be released when publishing just the .gadget)
 */
#include <exec/types.h>
#include <intuition/gadgetclass.h>
#include <intuition/classes.h>

#include <inline/macros.h>
#include <gadgets/layout.h>
#define VERSION_TRACKHEADER 1
#define TrackHeader_SUPERCLASS_ID "layout.gadget"


    extern int TrackHeaderStaticInit();
    extern void TrackHeaderStaticClose();
    extern Class *TRACKHEADER_GetClass();
    extern Class *HEADERBUTTON_GetClass();
    extern Class *HEADERSLIDER_GetClass();

/**  Attributes defined by the gadget class,
 * all attribs from gadgetclass.h are also valid.
 */
// different classes may not use same base.
//DEVTODO: have another offset for your new class to not collide super class ones and optimize...
#define TRACKHEADER_Dummy			(TAG_USER+0x04240000)

/* needed by OM_NEW */
#define TRACKHEADER_TrackIndex (TRACKHEADER_Dummy+1)

/* */
#define TRACKHEADER_Name (TRACKHEADER_Dummy+2)

/* */
#define TRACKHEADER_Pan (TRACKHEADER_Dummy+3)

/* */
#define TRACKHEADER_Volume (TRACKHEADER_Dummy+4)

/* silent and solo, ... boolean states */
#define TRACKHEADER_Flags (TRACKHEADER_Dummy+5)

/* at that level, 0 no solo , 1 you are soloed, 2 other is soloed */
#define TRACKHEADER_SoloState (TRACKHEADER_Dummy+6)

/* Pointer to AukStyle for visual styling */
#define	TRACKHEADER_StyleSheet		(TRACKHEADER_Dummy+7)


/** DEVTODO: adds attributes definitions here and
 * manage them in class_trackheader_attribs.c
 */
#endif
