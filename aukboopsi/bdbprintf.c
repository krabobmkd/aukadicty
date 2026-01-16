/*
 * Buffered Debug Printf for Amiga C99
 *
 * Provides printf-like functionality that buffers output for later printing.
 * Useful for contexts where immediate output is not possible.
 */
#ifdef USE_DEBUG_BDBPRINT
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <proto/dos.h>
#include <proto/exec.h>

#define BDB_BUFFER_SIZE 4096

/* Static buffer for storing debug output */
static char bdb_buffer[BDB_BUFFER_SIZE];
// volatile means will keep one coherent value if multiple processes use it.
// (compiler will not map it to a register.)
static volatile int bdb_position = 0;



extern struct Task	*myTask;
/*
 * bdbprintf - Printf to debug buffer
 *
 * Works like printf() but writes to an internal buffer instead of stdout.
 * If the buffer is full, the message is truncated.
 *
 * Returns: number of characters written (excluding null terminator)
 */
int bdbprintf(const char *format, ...)
{
    va_list args;
    int remaining;
    int written;

    /* Calculate remaining space in buffer (leave room for null terminator) */
    remaining = BDB_BUFFER_SIZE - bdb_position - 1;

    if (remaining <= 0) {
        /* Buffer is full, can't write anything */
        return 0;
    }

    /* Format the string directly into the buffer */
    va_start(args, format);
    written = vsnprintf(bdb_buffer + bdb_position, remaining + 1, format, args);
    va_end(args);

    /* vsnprintf returns the number of characters that would have been written */
    /* We need to clamp this to what actually fit */
    if (written > remaining) {
        written = remaining;
    }

    if (written > 0) {
        bdb_position += written;
    }

    if(myTask) Signal(myTask,SIGBREAKF_CTRL_F);

    return written;
}

/*
 * flushbdbprint - Print and flush the debug buffer
 *
 * Outputs all buffered content to stdout and clears the buffer.
 */
void flushbdbprint(void)
{
    if (bdb_position > 0) {
        /* Ensure null termination */
        bdb_buffer[bdb_position] = '\0';

        /* Print the buffer */
        //printf("%s", bdb_buffer);
		Printf(bdb_buffer); // dos version
        fflush(stdout);

        /* Reset buffer position */
        bdb_position = 0;
        bdb_buffer[0] = '\0';
    }
}

/*
 * clearbdbprint - Clear the debug buffer without printing
 *
 * Discards all buffered content.
 */
void clearbdbprint(void)
{
    bdb_position = 0;
    bdb_buffer[0] = '\0';
}

/*
 * bdbavailable - Get available space in buffer
 *
 * Returns: number of characters that can still be written to buffer
 */
int bdbavailable(void)
{
    return BDB_BUFFER_SIZE - bdb_position - 1;
}

/* Instance tracking for leak detection */
static volatile int instances_created = 0;
static volatile int instances_disposed = 0;


/*
 * bdbprintf_new - Debug print for OM_NEW with instance tracking
 */
int bdbprintf_new(const char *className, void *instance)
{
    int result;

    instances_created++;

    result = bdbprintf("[NEW] %s instance:%lx (total created:%ld)\n",
                       className, (unsigned long)instance, (long)instances_created);

    return result;
}

/*
 * bdbprintf_dispose - Debug print for OM_DISPOSE with instance tracking
 */
int bdbprintf_dispose(const char *className, void *instance)
{
    int result;

    instances_disposed++;

    result = bdbprintf("[DISPOSE] %s instance:%lx (total disposed:%ld)\n",
                       className, (unsigned long)instance, (long)instances_disposed);

    return result;
}

/*
 * bdbprintf_report_leaks - Report any leaked BOOPSI instances
 */
void bdbprintf_report_leaks(void)
{
    long leaked;

    leaked = instances_created - instances_disposed;

    if(leaked != 0)
    {
        bdbprintf("\n*** MEMORY LEAK DETECTED ***\n");
        bdbprintf("  BOOPSI Instances Created:  %ld\n", (long)instances_created);
        bdbprintf("  BOOPSI Instances Disposed: %ld\n", (long)instances_disposed);
        bdbprintf("  LEAKED INSTANCES:          %ld\n", leaked);
        bdbprintf("*** END LEAK REPORT ***\n\n");
        flushbdbprint();
    }
    else if(instances_created > 0)
    {
        bdbprintf("\nBOOPSI Instance Tracking: OK\n");
        bdbprintf("  Total instances created and disposed: %ld\n", (long)instances_created);
        bdbprintf("  No leaks detected.\n\n");
        flushbdbprint();
    }
}

/* Class tracking for MakeClass/FreeClass pairing verification */
static volatile int classes_made = 0;
static volatile int classes_freed = 0;

/*
 * bdbprintf_makeclass - Track MakeClass call
 */
int bdbprintf_makeclass(const char *className, void *classPtr)
{
    int result;

    classes_made++;

//    result = bdbprintf("[MAKECLASS] %s class:%lx (total made:%ld)\n",
//                       className, (unsigned long)classPtr, (long)classes_made);

    return result;
}

/*
 * bdbprintf_freeclass - Track FreeClass call
 */
int bdbprintf_freeclass(const char *className, void *classPtr)
{
    int result;

    classes_freed++;

//    result = bdbprintf("[FREECLASS] %s class:%lx (total freed:%ld)\n",
//                       className, (unsigned long)classPtr, (long)classes_freed);

    return result;
}

/*
 * bdbprintf_report_classes - Report any unfreed classes
 */
void bdbprintf_report_classes(void)
{
    long leaked;

    leaked = classes_made - classes_freed;

    if(leaked != 0)
    {
        bdbprintf("\n*** CLASS LEAK DETECTED ***\n");
        bdbprintf("  Classes Made (MakeClass):  %ld\n", (long)classes_made);
        bdbprintf("  Classes Freed (FreeClass): %ld\n", (long)classes_freed);
        bdbprintf("  LEAKED CLASSES:            %ld\n", leaked);
        bdbprintf("*** END CLASS LEAK REPORT ***\n\n");
        flushbdbprint();
    }
    else if(classes_made > 0)
    {
        bdbprintf("\nClass Tracking (MakeClass/FreeClass): OK\n");
        bdbprintf("  Total classes made and freed: %ld\n", (long)classes_made);
        bdbprintf("  No class leaks detected.\n\n");
        flushbdbprint();
    }
}
#endif
