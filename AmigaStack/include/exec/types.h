#ifndef EXEC_TYPES_H
#define EXEC_TYPES_H

/*
 * AmigaStack - Exec Basic Types
 * Standard AmigaOS type definitions
 */

#ifdef __cplusplus
extern "C" {
#endif

/* Basic integer types */
typedef signed char BYTE;
typedef unsigned char UBYTE;
typedef short WORD;
typedef unsigned short UWORD;
typedef long LONG;
typedef unsigned long ULONG;

/* String and pointer types */
typedef char* STRPTR;
typedef unsigned char* APTR;
/* Boolean type */
typedef long BOOL;

/* Constants */
#ifndef TRUE
#define TRUE  1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef NULL
#define NULL ((void*)0)
#endif

/* Function result codes */
#define RETURN_OK    0
#define RETURN_WARN  5
#define RETURN_ERROR 10
#define RETURN_FAIL  20

#ifdef __cplusplus
}
#endif

#endif /* EXEC_TYPES_H */
