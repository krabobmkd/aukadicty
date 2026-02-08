/*
 * AmigaStack - DOS Library Implementation
 * Maps AmigaOS dos.library functions to standard C stdio equivalents
 */

#include <proto/dos.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#ifdef BOOL
 #undef BOOL
#endif

#ifdef BYTE
 #undef BYTE
#endif

#ifdef WORD
 #undef WORD
#endif


#ifdef _WIN32
//#include <windows.h>
#else
#include <sys/time.h>
#endif

/*
 * Open - Open a file
 * Maps to fopen with appropriate mode
 */
BPTR Open(const char* name, long mode) {
    FILE* file;
    const char* fmode;

    if (!name) {
        return NULL;
    }

    /* Map AmigaOS mode to stdio mode */
    switch (mode) {
        case MODE_OLDFILE:
            fmode = "rb";  /* Read binary */
            break;
        case MODE_NEWFILE:
            fmode = "wb";  /* Write binary */
            break;
        default:
            fmode = "rb";  /* Default to read */
            break;
    }

    file = fopen(name, fmode);
    return (BPTR)file;
}

/*
 * Read - Read from a file
 * Maps to fread
 */
long Read(BPTR file, void* buffer, long length) {
    FILE* fp = (FILE*)file;
    size_t bytes_read;

    if (!fp || !buffer || length <= 0) {
        return -1;
    }

    bytes_read = fread(buffer, 1, length, fp);
    return (long)bytes_read;
}

/*
 * Write - Write to a file
 * Maps to fwrite
 */
long Write(BPTR file, const void* buffer, long length) {
    FILE* fp = (FILE*)file;
    size_t bytes_written;

    if (!fp || !buffer || length <= 0) {
        return -1;
    }

    bytes_written = fwrite(buffer, 1, length, fp);
    return (long)bytes_written;
}

/*
 * Close - Close a file
 * Maps to fclose
 */
void Close(BPTR file) {
    FILE* fp = (FILE*)file;

    if (fp) {
        fclose(fp);
    }
}

/*
 * Seek - Seek in a file
 * Maps to fseek with appropriate whence
 */
long Seek(BPTR file, long position, long mode) {
    FILE* fp = (FILE*)file;
    int whence;
    long old_pos;

    if (!fp) {
        return -1;
    }

    /* Get current position before seeking */
    old_pos = ftell(fp);

    /* Map AmigaOS seek mode to stdio whence */
    switch (mode) {
        case OFFSET_BEGINNING:
            whence = SEEK_SET;
            break;
        case OFFSET_CURRENT:
            whence = SEEK_CUR;
            break;
        case OFFSET_END:
            whence = SEEK_END;
            break;
        default:
            whence = SEEK_SET;
            break;
    }

    /* Perform the seek */
    if (fseek(fp, position, whence) != 0) {
        return -1;
    }

    /* Return old position */
    return old_pos;
}

/*
 * DateStamp - Get current date/time
 * Fills DateStamp structure with current time in Amiga format
 *
 * Amiga format:
 * - ds_Days: days since 1978-01-01
 * - ds_Minute: minutes past midnight
 * - ds_Tick: ticks (1/50 sec) past minute
 */
void DateStamp(struct DateStamp* ds) {
    time_t now;
    struct tm* tm_info;
    struct tm base_time;
    time_t base_seconds;
    double diff_seconds;
    long days, minutes, ticks;

    if (!ds) {
        return;
    }

    /* Get current time */
    time(&now);
    tm_info = localtime(&now);

    /* Amiga epoch: 1978-01-01 00:00:00 */
    memset(&base_time, 0, sizeof(base_time));
    base_time.tm_year = 78;  /* 1978 */
    base_time.tm_mon = 0;    /* January */
    base_time.tm_mday = 1;   /* 1st */
    base_seconds = mktime(&base_time);

    /* Calculate difference */
    diff_seconds = difftime(now, base_seconds);

    /* Convert to Amiga format */
    days = (long)(diff_seconds / 86400);  /* 86400 seconds per day */
    minutes = (long)((diff_seconds - (days * 86400)) / 60);
    ticks = (long)(((diff_seconds - (days * 86400) - (minutes * 60)) * 50));

    /* Fill DateStamp structure */
    ds->ds_Days = days;
    ds->ds_Minute = minutes;
    ds->ds_Tick = ticks;
}
