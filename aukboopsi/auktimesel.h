#ifndef AUKTIMESEL_H
#define AUKTIMESEL_H

#include <exec/types.h>

/*
    Time selection structures for Aukadicty
    Used by TimeRule and TrackArea BOOPSI classes

    Time values are 64-bit fixed-point (32.32 format) representing seconds.
    Signed to allow negative time positions if needed.
*/

/* Time selection span - start and end times
 * Both values are 64-bit signed fixed-point (32.32) seconds.
 * NULL pointer or both values == 0 means no selection.
 */
typedef struct AukTimeSpan {
    LONG startHi;       /* Start time high 32 bits (integer seconds, signed) */
    ULONG startLo;      /* Start time low 32 bits (fractional seconds) */
    LONG endHi;         /* End time high 32 bits (integer seconds, signed) */
    ULONG endLo;        /* End time low 32 bits (fractional seconds) */
} AukTimeSpan;

/* Time cursor - single 64-bit time position
 * Used for playback cursor, edit cursor, etc.
 * NULL pointer means no cursor visible.
 */
typedef struct AukTimeCursor {
    LONG hi;            /* High 32 bits (integer seconds, signed) */
    ULONG lo;           /* Low 32 bits (fractional seconds) */
} AukTimeCursor;

/* Helper macros for AukTimeSpan */
#define AUKTIMESPAN_IS_EMPTY(ts) \
    ((ts) == NULL || ((ts)->startHi == 0 && (ts)->startLo == 0 && \
                      (ts)->endHi == 0 && (ts)->endLo == 0))

#define AUKTIMECURSOR_IS_NULL(tc) ((tc) == NULL)

#endif /* AUKTIMESEL_H */
