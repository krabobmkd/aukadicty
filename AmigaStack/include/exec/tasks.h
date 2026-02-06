#ifndef EXEC_TASKS_H
#define EXEC_TASKS_H

/*
 * AmigaStack - Exec Tasks and Processes
 *
 * Provides Process structure compatible with Amiga's dos/dosextens.h
 */

#include <exec/types.h>
#include <exec/ports.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Task structure (minimal for compatibility) */
struct Task {
    struct Node tc_Node;
    UBYTE tc_Flags;
    UBYTE tc_State;
    BYTE tc_IDNestCnt;
    BYTE tc_TDNestCnt;
    ULONG tc_SigAlloc;
    ULONG tc_SigWait;
    ULONG tc_SigRecvd;
    ULONG tc_SigExcept;
    void* tc_UserData;              /* User data pointer */
};

/* Process structure - extends Task */
struct Process {
    struct Task pr_Task;            /* Embedded Task structure */
    struct MsgPort pr_MsgPort;      /* Process message port */

    /* Platform-specific thread handle (opaque) */
    void* _thread_handle;
    int _thread_running;            /* Flag: thread is running */
};

/* FindTask - get current task/process */
struct Task* FindTask(const char* name);

#ifdef __cplusplus
}
#endif

#endif /* EXEC_TASKS_H */
