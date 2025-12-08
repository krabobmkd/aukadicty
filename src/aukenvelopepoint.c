#include "aukenvelopepoint.h"
#include "serializer.h"
#include <proto/exec.h>

/*
 * AukEnvelopePoint implementation
 * Represents a point in the envelope curve
 */

void AukEnvelopePoint_New(AukObjectPtr* firstPtr) {
    AukEnvelopePoint* point;

    if (!firstPtr) return;

    point = (AukEnvelopePoint*)AllocVec(sizeof(AukEnvelopePoint), MEMF_CLEAR);
    if (point) {
        AukEnvelopePoint_Init(point);
        AukObjectPtr_Set(firstPtr, &point->base);
    }
}

void AukEnvelopePoint_Delete(void* This) {
    AukEnvelopePoint* point = (AukEnvelopePoint*)This;

    if (point) {
        /* Call base object delete */
        AukObject_Delete(This);
    }
}

const char* AukEnvelopePoint_GetTypeName(void* This) {
    (void)This;
    return "AukEnvelopePoint";
}

void AukEnvelopePoint_Serialize(void* This, ISerializer* ser, const char* pName) {
    AukEnvelopePoint* point = (AukEnvelopePoint*)This;
    (void)pName;

    if (!point || !ser) {
        return;
    }

    /* Serialize time and value */
    ser->t_fixed(ser, "time", &point->time);
    ser->t_fixed(ser, "value", &point->value);
}

int AukEnvelopePoint_SetTime(AukEnvelopePoint* point, AukFixed time) {
    int changed;

    if (!point) {
        return 0;
    }

    /* Check if value actually changed */
    changed = (point->time != time);

    if (!changed) {
        return 1; /* No change, but success */
    }

    point->time = time;

    /* Send update notification */
    point->base.SendUpdate(&point->base, NULL);

    return 1;
}

AukFixed AukEnvelopePoint_GetTime(AukEnvelopePoint* point) {
    return point ? point->time : 0;
}

int AukEnvelopePoint_SetValue(AukEnvelopePoint* point, AukFixed value) {
    int changed;

    if (!point) {
        return 0;
    }

    /* Check if value actually changed */
    changed = (point->value != value);

    if (!changed) {
        return 1; /* No change, but success */
    }

    point->value = value;

    /* Send update notification */
    point->base.SendUpdate(&point->base, NULL);

    return 1;
}

AukFixed AukEnvelopePoint_GetValue(AukEnvelopePoint* point) {
    return point ? point->value : AUK_FIXED_ONE;
}

void AukEnvelopePoint_Init(AukEnvelopePoint* point) {
    if (point) {
        /* Initialize base object */
        AukObject_Init(&point->base);

        /* Override virtual methods */
        point->base.New = AukEnvelopePoint_New;
        point->base.Delete = AukEnvelopePoint_Delete;
        point->base.GetTypeName = AukEnvelopePoint_GetTypeName;
        point->base.Serialize = AukEnvelopePoint_Serialize;

        /* Initialize data members to default values */
        point->time = 0;
        point->value = AUK_FIXED_ONE; /* Default to full volume */
    }
}
