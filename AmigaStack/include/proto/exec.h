#ifndef PROTO_EXEC_H
#define PROTO_EXEC_H

/*
 * AmigaStack - Exec Library Compatibility Layer
 * Redirects AmigaOS exec.library calls to standard C equivalents
 *
 * Threading model:
 * - CreateNewProc creates a thread with its own pr_MsgPort
 * - Main process sends messages via PutMsg to thread's pr_MsgPort
 * - Thread waits with WaitPort, receives with GetMsg
 * - Thread replies with ReplyMsg to msg->mn_ReplyPort
 * - Main process waits for reply with WaitPort on its own port
 */

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/tasks.h>
#include <dos/dostags.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Memory allocation - redirect to calloc/free */
void* AllocVec(unsigned long size, unsigned long flags);
void FreeVec(void* ptr);

/* Message ports */
struct MsgPort* CreateMsgPort(void);
void DeleteMsgPort(struct MsgPort* port);
void PutMsg(struct MsgPort* port, struct Message* msg);
struct Message* GetMsg(struct MsgPort* port);
void ReplyMsg(struct Message* msg);
void WaitPort(struct MsgPort* port);

/* Task/Process functions */
struct Task* FindTask(const char* name);
void WaitTOF(void);

/* Process creation - TagItem variant for Amiga compatibility */
struct Process* CreateNewProc(const struct TagItem* tags);

/* Process creation - Simple variant for PC (no casting needed) */
struct Process* CreateNewProcSimple(void (*entry)(void), const char* name, int priority);

/* Signal functions (simplified) */
ULONG Wait(ULONG signalSet);
void Signal(struct Task* task, ULONG signalSet);
ULONG SetSignal(ULONG newSignals, ULONG signalSet);

/* Task scheduling (no-op on PC) */
void Forbid(void);
void Permit(void);

#ifdef __cplusplus
}
#endif

#endif /* PROTO_EXEC_H */
