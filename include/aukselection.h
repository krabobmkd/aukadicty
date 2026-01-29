#ifndef AUKSELECTION_H
#define AUKSELECTION_H

#include "aukfixed.h"

#ifdef __cplusplus
extern "C" {
#endif

/* AukSound structure - inherits from AukObject */
typedef struct AukSelection {
    int _mode;   // 0 no selection, 1 track span, 1 or more whole tracks
    int _itrack; // when time span apply,  mode 1, else should be -1
    long long _start;
    long long _end;
} AukSelection;

//    int hasSelection;          /* Boolean: 1 if time selection exists, 0 otherwise */
//    AukFixed selectionStart;   /* Start time of selection */
//    AukFixed selectionEnd;     /* End time of selection (exclusive) */
#ifdef __cplusplus
}
#endif

#endif /* AUKSOUND_H */
