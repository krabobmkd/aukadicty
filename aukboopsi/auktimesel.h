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
    long long start;
    long long end;
} AukTimeSpan;

/* Time cursor - single 64-bit time position
 * Used for playback cursor, edit cursor, etc.
 * NULL pointer means no cursor visible.
 */
typedef struct AukTimeCursor {
    long long _t;
} AukTimeCursor;


#endif /* AUKTIMESEL_H */
