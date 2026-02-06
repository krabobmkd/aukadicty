#ifndef DOS_DOSTAGS_H
#define DOS_DOSTAGS_H

/*
 * AmigaStack - DOS Tags for Process Creation
 */

#include <exec/types.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Tag structure - ti_Data uses pointer-sized integer for 64-bit compatibility */
struct TagItem {
    unsigned long ti_Tag;
    uintptr_t ti_Data;
};

/* Standard tags */
#define TAG_DONE   0
#define TAG_IGNORE 1

/* Process creation tags */
#define NP_Entry      0x80000001  /* Entry point function */
#define NP_Name       0x80000002  /* Process name */
#define NP_Priority   0x80000003  /* Process priority */
#define NP_StackSize  0x80000004  /* Stack size */

/* Type for process entry points */
typedef unsigned long ULONG;

#ifdef __cplusplus
}
#endif

#endif /* DOS_DOSTAGS_H */
