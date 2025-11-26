#ifndef PROTO_DOS_H
#define PROTO_DOS_H

/*
 * AmigaStack - DOS Library Compatibility Layer
 * Redirects AmigaOS dos.library calls to standard C stdio equivalents
 */

#include <exec/types.h>
#include <dos/dos.h>

#ifdef __cplusplus
extern "C" {
#endif

/* File I/O - redirect to stdio.h equivalents */
BPTR Open(const char* name, long mode);
long Read(BPTR file, void* buffer, long length);
long Write(BPTR file, const void* buffer, long length);
void Close(BPTR file);
long Seek(BPTR file, long position, long mode);

/* Date/time */
void DateStamp(struct DateStamp* ds);

#ifdef __cplusplus
}
#endif

#endif /* PROTO_DOS_H */
