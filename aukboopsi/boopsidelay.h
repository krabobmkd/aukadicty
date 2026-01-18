#ifndef BOOPSIDELAY_H
#define BOOPSIDELAY_H

/**
 * BoopsiDelay - Delayed message queue for BOOPSI gadget notifications
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

/* Maximum number of tag entries in the queue (128 pairs) */
#define BOOPSIDELAY_QUEUE_SIZE 256

/* Queue structure - to be aggregated in App struct */
typedef struct BoopsiDelayQueue
{
    struct TagItem queue[BOOPSIDELAY_QUEUE_SIZE];
    UWORD writePos;     /* Next write position */
    UWORD readPos;      /* Next read position */
    BOOL hasMessages;   /* TRUE if there are pending messages */
} BoopsiDelayQueue;

/**
 * Initialize the notification queue
 */
void BoopsiDelay_Init(BoopsiDelayQueue *q);

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
