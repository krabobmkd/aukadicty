#ifndef DOS_DOS_H
#define DOS_DOS_H

/*
 * AmigaStack - DOS Types
 */

#ifdef __cplusplus
extern "C" {
#endif

/* File handle type - maps to FILE* internally */
typedef void* BPTR;

/* File open modes */
#define MODE_OLDFILE 1005   /* Open existing file for reading */
#define MODE_NEWFILE 1006   /* Create new file for writing */

/* Seek modes */
#define OFFSET_BEGINNING -1  /* Seek from beginning */
#define OFFSET_CURRENT    0  /* Seek from current position */
#define OFFSET_END        1  /* Seek from end */

/* DateStamp structure - Amiga time format */
struct DateStamp {
    long ds_Days;    /* Days since 1978-01-01 */
    long ds_Minute;  /* Minutes past midnight */
    long ds_Tick;    /* Ticks (1/50 sec) past minute */
};

#ifdef __cplusplus
}
#endif

#endif /* DOS_DOS_H */
