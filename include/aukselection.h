#ifndef AUKSELECTION_H
#define AUKSELECTION_H

#include "aukfixed.h"

#ifdef __cplusplus
extern "C" {
#endif

/* AukSelection: tells what is selected at project level.
    _mode== 0 no selection,
    _mode==1 track span for a single track: (_itrack,_start,_end),
    _mode==2: one or more whole tracks are selected: for each tracks in project, track is selected if AukTrack flags tells it.
 */
typedef struct AukSelection {
    int _mode;   //
    int _itrack; // when time span apply,  mode 1, else should be -1
    long long _start;
    long long _end;
} AukSelection;

#ifdef __cplusplus
}
#endif

#endif /* AUKSOUND_H */
