/*
 * AmigaStack - Exec Library Implementation
 * Maps AmigaOS exec.library functions to standard C equivalents
 *
 * Implements proper message passing between threads using:
 * - Mutex for queue protection
 * - Condition variable for WaitPort blocking
 */

#include <proto/exec.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Avoid conflicts with Windows types */
#ifdef BOOL
 #undef BOOL
#endif
#ifdef BYTE
 #undef BYTE
#endif
#ifdef WORD
 #undef WORD
#endif

/* Platform-specific includes */
#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    #include <process.h>
#else
    #include <pthread.h>
    #include <unistd.h>
#endif

/* Thread-local storage for current process */
#ifdef _WIN32
    static __declspec(thread) struct Process* tls_current_process = NULL;
#else
    static __thread struct Process* tls_current_process = NULL;
#endif

/*
 * Memory allocation tracking
 */
struct AllocNode {
    void* ptr;
    unsigned long size;
    struct AllocNode* next;
};

static struct AllocNode* alloc_list_head = NULL;
static int atexit_registered = 0;
static unsigned long total_allocs = 0;
static unsigned long total_frees = 0;
static unsigned long total_bytes_allocated = 0;

#ifdef _WIN32
static CRITICAL_SECTION alloc_mutex;
static int alloc_mutex_initialized = 0;
#else
static pthread_mutex_t alloc_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

static void init_alloc_mutex(void) {
#ifdef _WIN32
    if (!alloc_mutex_initialized) {
        InitializeCriticalSection(&alloc_mutex);
        alloc_mutex_initialized = 1;
    }
#endif
}

/* Report allocations at program exit */
static void report_allocations(void) {
    struct AllocNode* node;
    unsigned long leak_count = 0;
    unsigned long leak_bytes = 0;

    printf("\n========== AllocVec/FreeVec Memory Report ==========\n");
    printf("Total allocations: %lu\n", total_allocs);
    printf("Total frees: %lu\n", total_frees);
    printf("Total bytes allocated: %lu\n", total_bytes_allocated);
    printf("\n");

    node = alloc_list_head;
    while (node) {
        leak_count++;
        leak_bytes += node->size;
        printf("LEAK: %lu bytes at address %p\n", node->size, node->ptr);
        node = node->next;
    }

    if (leak_count == 0) {
        printf("No memory leaks detected - all allocations were freed!\n");
    } else {
        printf("\nWARNING: %lu allocation(s) not freed (%lu bytes leaked)\n",
               leak_count, leak_bytes);
    }
    printf("====================================================\n");
}

/* Add allocation to tracking list */
static void track_allocation(void* ptr, unsigned long size) {
    struct AllocNode* node;

    init_alloc_mutex();

    if (!atexit_registered) {
        atexit(report_allocations);
        atexit_registered = 1;
    }

    node = (struct AllocNode*)malloc(sizeof(struct AllocNode));
    if (!node) {
        return;
    }

    node->ptr = ptr;
    node->size = size;

#ifdef _WIN32
    EnterCriticalSection(&alloc_mutex);
#else
    pthread_mutex_lock(&alloc_mutex);
#endif

    node->next = alloc_list_head;
    alloc_list_head = node;
    total_allocs++;
    total_bytes_allocated += size;

#ifdef _WIN32
    LeaveCriticalSection(&alloc_mutex);
#else
    pthread_mutex_unlock(&alloc_mutex);
#endif
}

/* Remove allocation from tracking list */
static void untrack_allocation(void* ptr) {
    struct AllocNode* node;
    struct AllocNode* prev = NULL;

    init_alloc_mutex();

#ifdef _WIN32
    EnterCriticalSection(&alloc_mutex);
#else
    pthread_mutex_lock(&alloc_mutex);
#endif

    node = alloc_list_head;
    while (node) {
        if (node->ptr == ptr) {
            if (prev) {
                prev->next = node->next;
            } else {
                alloc_list_head = node->next;
            }
            total_frees++;
#ifdef _WIN32
            LeaveCriticalSection(&alloc_mutex);
#else
            pthread_mutex_unlock(&alloc_mutex);
#endif
            free(node);
            return;
        }
        prev = node;
        node = node->next;
    }

#ifdef _WIN32
    LeaveCriticalSection(&alloc_mutex);
#else
    pthread_mutex_unlock(&alloc_mutex);
#endif

    printf("WARNING: FreeVec called on untracked pointer %p\n", ptr);
}

/*
 * AllocVec - Allocate memory
 */
void* AllocVec(unsigned long size, unsigned long flags) {
    void* ptr;

    if (size == 0) {
        return NULL;
    }

    if (flags & MEMF_CLEAR) {
        ptr = calloc(1, size);
    } else {
        ptr = malloc(size);
    }

    if (ptr) {
        track_allocation(ptr, size);
    }

    return ptr;
}

/*
 * FreeVec - Free memory allocated with AllocVec
 */
void FreeVec(void* ptr) {
    if (ptr) {
        untrack_allocation(ptr);
        free(ptr);
    }
}

/* ============================================================
 * Message Port Implementation
 * ============================================================ */

/*
 * Initialize a message port's synchronization primitives
 */
static int msgport_init_sync(struct MsgPort* port) {
#ifdef _WIN32
    CRITICAL_SECTION* mutex = (CRITICAL_SECTION*)malloc(sizeof(CRITICAL_SECTION));
    CONDITION_VARIABLE* cond = (CONDITION_VARIABLE*)malloc(sizeof(CONDITION_VARIABLE));

    if (!mutex || !cond) {
        free(mutex);
        free(cond);
        return 0;
    }

    InitializeCriticalSection(mutex);
    InitializeConditionVariable(cond);

    port->_mutex = mutex;
    port->_cond = cond;
#else
    pthread_mutex_t* mutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
    pthread_cond_t* cond = (pthread_cond_t*)malloc(sizeof(pthread_cond_t));

    if (!mutex || !cond) {
        free(mutex);
        free(cond);
        return 0;
    }

    pthread_mutex_init(mutex, NULL);
    pthread_cond_init(cond, NULL);

    port->_mutex = mutex;
    port->_cond = cond;
#endif
    return 1;
}

/*
 * Destroy a message port's synchronization primitives
 */
static void msgport_destroy_sync(struct MsgPort* port) {
#ifdef _WIN32
    if (port->_mutex) {
        DeleteCriticalSection((CRITICAL_SECTION*)port->_mutex);
        free(port->_mutex);
    }
    if (port->_cond) {
        free(port->_cond);
    }
#else
    if (port->_mutex) {
        pthread_mutex_destroy((pthread_mutex_t*)port->_mutex);
        free(port->_mutex);
    }
    if (port->_cond) {
        pthread_cond_destroy((pthread_cond_t*)port->_cond);
        free(port->_cond);
    }
#endif
    port->_mutex = NULL;
    port->_cond = NULL;
}

/*
 * CreateMsgPort - Create a message port with synchronization
 */
struct MsgPort* CreateMsgPort(void) {
    struct MsgPort* port;

    port = (struct MsgPort*)calloc(1, sizeof(struct MsgPort));
    if (!port) {
        return NULL;
    }

    port->mp_MsgList_Head = NULL;
    port->mp_MsgList_Tail = NULL;

    if (!msgport_init_sync(port)) {
        free(port);
        return NULL;
    }

    return port;
}

/*
 * DeleteMsgPort - Delete a message port
 */
void DeleteMsgPort(struct MsgPort* port) {
    if (port) {
        msgport_destroy_sync(port);
        free(port);
    }
}

/*
 * PutMsg - Send a message to a port (thread-safe)
 */
void PutMsg(struct MsgPort* port, struct Message* msg) {
    if (!port || !msg) {
        return;
    }

#ifdef _WIN32
    EnterCriticalSection((CRITICAL_SECTION*)port->_mutex);
#else
    pthread_mutex_lock((pthread_mutex_t*)port->_mutex);
#endif

    /* Add message to tail of queue */
    msg->mn_Node.ln_Succ = NULL;

    if (port->mp_MsgList_Tail) {
        port->mp_MsgList_Tail->mn_Node.ln_Succ = (struct Node*)msg;
    } else {
        port->mp_MsgList_Head = msg;
    }
    port->mp_MsgList_Tail = msg;

    /* Signal waiting thread */
#ifdef _WIN32
    WakeConditionVariable((CONDITION_VARIABLE*)port->_cond);
    LeaveCriticalSection((CRITICAL_SECTION*)port->_mutex);
#else
    pthread_cond_signal((pthread_cond_t*)port->_cond);
    pthread_mutex_unlock((pthread_mutex_t*)port->_mutex);
#endif
}

/*
 * GetMsg - Get a message from a port (non-blocking, thread-safe)
 */
struct Message* GetMsg(struct MsgPort* port) {
    struct Message* msg;

    if (!port) {
        return NULL;
    }

#ifdef _WIN32
    EnterCriticalSection((CRITICAL_SECTION*)port->_mutex);
#else
    pthread_mutex_lock((pthread_mutex_t*)port->_mutex);
#endif

    msg = port->mp_MsgList_Head;
    if (msg) {
        port->mp_MsgList_Head = (struct Message*)msg->mn_Node.ln_Succ;
        if (!port->mp_MsgList_Head) {
            port->mp_MsgList_Tail = NULL;
        }
        msg->mn_Node.ln_Succ = NULL;
    }

#ifdef _WIN32
    LeaveCriticalSection((CRITICAL_SECTION*)port->_mutex);
#else
    pthread_mutex_unlock((pthread_mutex_t*)port->_mutex);
#endif

    return msg;
}

/*
 * ReplyMsg - Reply to a message by sending it to its reply port
 */
void ReplyMsg(struct Message* msg) {
    if (!msg || !msg->mn_ReplyPort) {
        return;
    }
    PutMsg(msg->mn_ReplyPort, msg);
}

/*
 * WaitPort - Wait for messages on a port (blocking, thread-safe)
 */
void WaitPort(struct MsgPort* port) {
    if (!port) {
        return;
    }

#ifdef _WIN32
    EnterCriticalSection((CRITICAL_SECTION*)port->_mutex);

    /* Wait until there's a message */
    while (!port->mp_MsgList_Head) {
        SleepConditionVariableCS(
            (CONDITION_VARIABLE*)port->_cond,
            (CRITICAL_SECTION*)port->_mutex,
            INFINITE
        );
    }

    LeaveCriticalSection((CRITICAL_SECTION*)port->_mutex);
#else
    pthread_mutex_lock((pthread_mutex_t*)port->_mutex);

    /* Wait until there's a message */
    while (!port->mp_MsgList_Head) {
        pthread_cond_wait(
            (pthread_cond_t*)port->_cond,
            (pthread_mutex_t*)port->_mutex
        );
    }

    pthread_mutex_unlock((pthread_mutex_t*)port->_mutex);
#endif
}

/* ============================================================
 * Process/Thread Implementation
 * ============================================================ */

/*
 * FindTask - Get current task/process
 * If name is NULL, returns current process
 */
struct Task* FindTask(const char* name) {
    if (name != NULL) {
        /* Finding by name not supported */
        return NULL;
    }
    return (struct Task*)tls_current_process;
}

/*
 * WaitTOF - Wait for vertical blank (stub)
 */
void WaitTOF(void) {
#ifdef _WIN32
    Sleep(16);  /* ~60Hz */
#else
    usleep(16000);
#endif
}

/*
 * Wait - Wait for signals (simplified)
 */
ULONG Wait(ULONG signalSet) {
    /* Simplified: just sleep briefly */
#ifdef _WIN32
    Sleep(1);
#else
    usleep(1000);
#endif
    return signalSet;
}

/*
 * Signal - Send signals to a task (simplified)
 */
void Signal(struct Task* task, ULONG signalSet) {
    /* Simplified: do nothing */
    (void)task;
    (void)signalSet;
}

/*
 * SetSignal - Set/get signals (simplified)
 */
ULONG SetSignal(ULONG newSignals, ULONG signalSet) {
    (void)newSignals;
    (void)signalSet;
    return 0;
}

/* Thread entry wrapper data */
struct ProcessData {
    void (*entry)(void);
    char name[256];
    struct Process* process;
};

#ifdef _WIN32
/* Windows thread entry point */
static unsigned int __stdcall process_thread(void* arg) {
    struct ProcessData* data = (struct ProcessData*)arg;

    if (data) {
        /* Set thread-local current process */
        tls_current_process = data->process;
        data->process->_thread_running = 1;

        if (data->entry) {
            data->entry();
        }

        data->process->_thread_running = 0;
        free(data);
    }
    return 0;
}
#else
/* POSIX thread entry point */
static void* process_thread(void* arg) {
    struct ProcessData* data = (struct ProcessData*)arg;

    if (data) {
        /* Set thread-local current process */
        tls_current_process = data->process;
        data->process->_thread_running = 1;

        if (data->entry) {
            data->entry();
        }

        data->process->_thread_running = 0;
        free(data);
    }
    return NULL;
}
#endif

/*
 * CreateNewProcSimple - Create a new process/thread with direct parameters
 *
 * This is the recommended API for PC usage - no TagItem casting needed.
 *
 * Parameters:
 * - entry: Thread entry function (void (*)(void))
 * - name: Process name
 * - priority: Priority (ignored on PC)
 */
struct Process* CreateNewProcSimple(void (*entry)(void), const char* name, int priority) {
    struct ProcessData* data;
    struct Process* proc;

    (void)priority;  /* Unused on PC */

    if (!entry) {
        return NULL;
    }
    if (!name) {
        name = "Unknown";
    }

    /* Allocate process structure */
    proc = (struct Process*)calloc(1, sizeof(struct Process));
    if (!proc) {
        return NULL;
    }

    /* Initialize process message port */
    if (!msgport_init_sync(&proc->pr_MsgPort)) {
        free(proc);
        return NULL;
    }

    /* Allocate thread data */
    data = (struct ProcessData*)malloc(sizeof(struct ProcessData));
    if (!data) {
        msgport_destroy_sync(&proc->pr_MsgPort);
        free(proc);
        return NULL;
    }

    data->entry = entry;
    data->process = proc;
    strncpy(data->name, name, sizeof(data->name) - 1);
    data->name[sizeof(data->name) - 1] = '\0';

#ifdef _WIN32
    {
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
            msgport_destroy_sync(&proc->pr_MsgPort);
            free(proc);
            return NULL;
        }

        proc->_thread_handle = thread;
    }
#else
    {
        pthread_t* thread = (pthread_t*)malloc(sizeof(pthread_t));
        if (!thread) {
            free(data);
            msgport_destroy_sync(&proc->pr_MsgPort);
            free(proc);
            return NULL;
        }

        if (pthread_create(thread, NULL, process_thread, data) != 0) {
            free(thread);
            free(data);
            msgport_destroy_sync(&proc->pr_MsgPort);
            free(proc);
            return NULL;
        }

        proc->_thread_handle = thread;
    }
#endif

    return proc;
}

/*
 * CreateNewProc - Create a new process/thread using TagItem array
 *
 * For Amiga compatibility. Uses CreateNewProcSimple internally.
 * Note: On 64-bit PC, ti_Data must use uintptr_t for pointer values.
 */
struct Process* CreateNewProc(const struct TagItem* tags) {
    void (*entry)(void) = NULL;
    const char* name = NULL;
    int priority = 0;
    const struct TagItem* tag;

    if (!tags) {
        return NULL;
    }

    /* Parse tags */
    for (tag = tags; tag->ti_Tag != TAG_DONE; tag++) {
        switch (tag->ti_Tag) {
            case NP_Entry:
                entry = (void (*)(void))(tag->ti_Data);
                break;
            case NP_Name:
                name = (const char*)(tag->ti_Data);
                break;
            case NP_Priority:
                priority = (int)(tag->ti_Data);
                break;
        }
    }

    return CreateNewProcSimple(entry, name, priority);
}
