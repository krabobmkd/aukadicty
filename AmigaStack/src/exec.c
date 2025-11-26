/*
 * AmigaStack - Exec Library Implementation
 * Maps AmigaOS exec.library functions to standard C equivalents
 */

#include <proto/exec.h>
#include <stdlib.h>
#include <string.h>

#ifdef BOOL
 #undef BOOL
#endif

#ifdef BYTE
 #undef BYTE
#endif

#ifdef WORD
 #undef WORD
#endif

#ifndef HANDLE
  typedef void *HANDLE;
#endif

#ifdef _WIN32
//#include <windows.h>
#include <process.h>
#else
#include <pthread.h>
#include <unistd.h>
#endif

/*
 * AllocVec - Allocate memory
 * Maps to calloc (which clears memory by default)
 */
void* AllocVec(unsigned long size, unsigned long flags) {
    void* ptr;

    if (size == 0) {
        return NULL;
    }

    if (flags & MEMF_CLEAR) {
        /* Use calloc to get cleared memory */
        ptr = calloc(1, size);
    } else {
        /* Use malloc for uncleared memory */
        ptr = malloc(size);
    }

    return ptr;
}

/*
 * FreeVec - Free memory allocated with AllocVec
 * Maps to free
 */
void FreeVec(void* ptr) {
    if (ptr) {
        free(ptr);
    }
}

/*
 * CreateMsgPort - Create a message port
 * Simplified implementation for compatibility
 */
struct MsgPort* CreateMsgPort(void) {
    struct MsgPort* port;

    port = (struct MsgPort*)calloc(1, sizeof(struct MsgPort));
    if (!port) {
        return NULL;
    }

    port->head = NULL;
    port->tail = NULL;
    port->sigbit = NULL;

    return port;
}

/*
 * DeleteMsgPort - Delete a message port
 */
void DeleteMsgPort(struct MsgPort* port) {
    if (port) {
        free(port);
    }
}

/*
 * PutMsg - Send a message to a port
 * Simplified implementation
 */
void PutMsg(struct MsgPort* port, struct Message* msg) {
    if (!port || !msg) {
        return;
    }

    /* In a real implementation, this would queue the message */
    /* For now, we do nothing as the cache process is simplified */
}

/*
 * GetMsg - Get a message from a port
 * Simplified implementation
 */
struct Message* GetMsg(struct MsgPort* port) {
    if (!port) {
        return NULL;
    }

    /* In a real implementation, this would dequeue a message */
    /* For now, always return NULL */
    return NULL;
}

/*
 * ReplyMsg - Reply to a message
 * Simplified implementation
 */
void ReplyMsg(struct Message* msg) {
    /* In a real implementation, this would send the message back */
    /* For now, we do nothing */
}

/*
 * WaitPort - Wait for messages on a port
 * Simplified implementation
 */
void WaitPort(struct MsgPort* port) {
    if (!port) {
        return;
    }

    /* In a real implementation, this would block until a message arrives */
    /* For now, just sleep briefly */
#ifdef _WIN32
    Sleep(10);  /* Sleep 10ms */
#else
    usleep(10000);  /* Sleep 10ms */
#endif
}

/* Thread/process entry wrapper */
struct ProcessData {
    void (*entry)(void);
    char name[256];
};

#ifdef _WIN32
/* Windows thread entry point */
static unsigned int __stdcall process_thread(void* arg) {
    struct ProcessData* data = (struct ProcessData*)arg;
    if (data && data->entry) {
        data->entry();
    }
    free(data);
    return 0;
}
#else
/* POSIX thread entry point */
static void* process_thread(void* arg) {
    struct ProcessData* data = (struct ProcessData*)arg;
    if (data && data->entry) {
        data->entry();
    }
    free(data);
    return NULL;
}
#endif

/*
 * CreateNewProc - Create a new process/thread
 * Maps to CreateThread (Windows) or pthread_create (POSIX)
 */
struct Process* CreateNewProc(const struct TagItem* tags) {
    struct ProcessData* data;
    struct Process* proc;
    void (*entry)(void) = NULL;
    const char* name = "Unknown";
    const struct TagItem* tag;

    if (!tags) {
        return NULL;
    }

    /* Parse tags */
    for (tag = tags; tag->ti_Tag != TAG_DONE; tag++) {
        switch (tag->ti_Tag) {
            case NP_Entry:
                entry = (void (*)(void))tag->ti_Data;
                break;
            case NP_Name:
                name = (const char*)tag->ti_Data;
                break;
            /* Ignore other tags */
        }
    }

    if (!entry) {
        return NULL;
    }

    /* Allocate process data */
    data = (struct ProcessData*)malloc(sizeof(struct ProcessData));
    if (!data) {
        return NULL;
    }

    data->entry = entry;
    strncpy(data->name, name, sizeof(data->name) - 1);
    data->name[sizeof(data->name) - 1] = '\0';

    /* Allocate process structure */
    proc = (struct Process*)calloc(1, sizeof(struct Process));
    if (!proc) {
        free(data);
        return NULL;
    }

#ifdef _WIN32
    /* Create Windows thread */
    HANDLE thread = (HANDLE)_beginthreadex(
        NULL,
        0,
        process_thread,
        data,
        0,
        NULL
    );

    if (thread == 0) {
        free(data);
        free(proc);
        return NULL;
    }

    CloseHandle(thread);  /* Detach thread */
#else
    /* Create POSIX thread */
    pthread_t thread;
    pthread_attr_t attr;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    if (pthread_create(&thread, &attr, process_thread, data) != 0) {
        pthread_attr_destroy(&attr);
        free(data);
        free(proc);
        return NULL;
    }

    pthread_attr_destroy(&attr);
#endif

    return proc;
}
