#ifndef BOOPSIMESSAGE_H
#define BOOPSIMESSAGE_H

/**
 * BoopsiMessage - Delayed message queue for BOOPSI gadget notifications
 * Also manage object that is the target for everyone to pass with ICA_TARGET.
 *
 * This module provides a FIFO queue for collecting OM_NOTIFY messages
 * from gadgets and processing them after the main WM_HANDLEINPUT loop.
 * This avoids deep recursion during graphic updates.
 *
 * Message format in queue:
 *   GA_ID, sender_ID      (marks beginning of a message)
 *   [attr1, value1]       (optional additional attributes)
 *   [attr2, value2]       (...)
 *   TAG_END, 0            (marks end of a message)
 */

#include <exec/types.h>
#include <utility/tagitem.h>
#include <intuition/classusr.h>

int initMessageTargetModel(void);
void closeMessageTargetModel(void);


struct BoopsiDelayQueue;
typedef struct BoopsiDelayQueue BoopsiDelayQueue;

extern BoopsiDelayQueue *DelayQueue;
extern Object *TargetInstance;
/**
 * Add a tag to the queue
 * @return TRUE if added successfully, FALSE if queue is full
 */
BOOL BoopsiDelay_AddTag(BoopsiDelayQueue *q, ULONG tag, ULONG data);

/**
 * Begin a new message in the queue (adds GA_ID entry)
 * @return TRUE if started successfully, FALSE if queue is full
 */
BOOL BoopsiDelay_BeginMessage(BoopsiDelayQueue *q, ULONG senderID);

/**
 * End the current message in the queue (adds TAG_END entry)
 * @return TRUE if ended successfully, FALSE if queue is full
 */
BOOL BoopsiDelay_EndMessage(BoopsiDelayQueue *q);

/**
 * Check if there are pending messages in the queue
 */
BOOL BoopsiDelay_HasMessages(BoopsiDelayQueue *q);

/**
 * Get the next complete message from the queue
 * Returns a pointer to a TagItem array starting with GA_ID and ending with TAG_END.
 * @return Pointer to TagItem array, or NULL if no more messages
 */
struct TagItem *BoopsiDelay_NextMessage(BoopsiDelayQueue *q);

#endif /* BOOPSIDELAY_H */
