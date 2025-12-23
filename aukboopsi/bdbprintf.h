/*
 * Buffered Debug Printf for Amiga C99
 *
 * Header file for buffered debug printing functionality.
 */

#ifndef BDBPRINTF_H
#define BDBPRINTF_H
#include "compilers.h"
#ifdef USE_DEBUG_BDBPRINT
/*
 * bdbprintf - Printf to debug buffer
 *
 * Works like printf() but writes to an internal buffer instead of stdout.
 * If the buffer is full, the message is truncated.
 *
 * Parameters:
 *   format - printf-style format string
 *   ...    - variable arguments matching format specifiers
 *
 * Returns: number of characters written (excluding null terminator)
 */
int bdbprintf(const char *format, ...);

/*
 * flushbdbprint - Print and flush the debug buffer
 *
 * Outputs all buffered content to stdout and clears the buffer.
 */
void flushbdbprint(void);

/*
 * clearbdbprint - Clear the debug buffer without printing
 *
 * Discards all buffered content without printing it.
 */
void clearbdbprint(void);

/*
 * bdbavailable - Get available space in buffer
 *
 * Returns: number of characters that can still be written to buffer
 */
int bdbavailable(void);

/*
 * bdbprintf_new - Debug print for OM_NEW with instance tracking
 *
 * Logs object creation and tracks instance count for leak detection.
 * Calls bdbprintf internally.
 *
 * Parameters:
 *   className - Name of the BOOPSI class being instantiated
 *   instance  - Pointer to the created instance
 *
 * Returns: number of characters written
 */
int bdbprintf_new(const char *className, void *instance);

/*
 * bdbprintf_dispose - Debug print for OM_DISPOSE with instance tracking
 *
 * Logs object disposal and tracks instance count for leak detection.
 * Calls bdbprintf internally.
 *
 * Parameters:
 *   className - Name of the BOOPSI class being disposed
 *   instance  - Pointer to the instance being disposed
 *
 * Returns: number of characters written
 */
int bdbprintf_dispose(const char *className, void *instance);

/*
 * bdbprintf_report_leaks - Report any leaked BOOPSI instances
 *
 * Called automatically via atexit() to report if any objects
 * were created but not disposed (potential memory leaks).
 * Prints total created vs disposed counts.
 */
void bdbprintf_report_leaks(void);

#else
INLINE int bdbprintf(const char *format, ...) { return 0; }
INLINE void flushbdbprint(void) {}
INLINE void clearbdbprint(void) {}
INLINE int bdbavailable(void)  { return 0; }
INLINE int bdbprintf_new(const char *className, void *instance) { return 0; }
INLINE int bdbprintf_dispose(const char *className, void *instance) { return 0; }
INLINE void bdbprintf_report_leaks(void) {}
#endif

#endif /* BDBPRINTF_H */
