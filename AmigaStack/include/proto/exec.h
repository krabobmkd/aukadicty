#ifndef PROTO_EXEC_H
#define PROTO_EXEC_H

/*
 * AmigaStack - Exec Library Compatibility Layer
 * Redirects AmigaOS exec.library calls to standard C equivalents
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

/* Process creation */
struct Process* CreateNewProc(const struct TagItem* tags);

#ifdef __cplusplus
}
#endif

#endif /* PROTO_EXEC_H */
