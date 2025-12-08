#ifndef AUKENVELOPEPOINT_H
#define AUKENVELOPEPOINT_H

/*
 * AukEnvelopePoint - Represents a point in the envelope curve
 * Used for B-spline interpolation of track volume/pan
 */

#include "aukobject.h"
#include "aukfixed.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct AukEnvelopePoint AukEnvelopePoint;

/* AukEnvelopePoint structure - inherits from AukObject */
struct AukEnvelopePoint {
    AukObject base;          /* Must be first - inheritance */

    /* Data members */
    AukFixed time;           /* Time position in fixed-point */
    AukFixed value;          /* Value at this point (0.0 to 1.0 in fixed-point) */
};

typedef AukEnvelopePoint* AukEnvelopePointPtr;

/* Constructor/Destructor */
void AukEnvelopePoint_New(AukObjectPtr* firstPtr);
void AukEnvelopePoint_Delete(void* This);
const char* AukEnvelopePoint_GetTypeName(void* This);

/* Initialize AukEnvelopePoint structure */
void AukEnvelopePoint_Init(AukEnvelopePoint* point);

/* Methods */
int AukEnvelopePoint_SetTime(AukEnvelopePoint* point, AukFixed time);
AukFixed AukEnvelopePoint_GetTime(AukEnvelopePoint* point);
int AukEnvelopePoint_SetValue(AukEnvelopePoint* point, AukFixed value);
AukFixed AukEnvelopePoint_GetValue(AukEnvelopePoint* point);

#ifdef __cplusplus
}
#endif

#endif /* AUKENVELOPEPOINT_H */
