#ifndef BOOPSIDISPOSE_H
#define BOOPSIDISPOSE_H

/**
 * BoopsiDispose - Delayed object disposal queue for BOOPSI gadgets
 *
 * This module provides a queue for delaying DisposeObject() calls
 * to a later pass in the main loop. This avoids disposing objects
 * while they are still being referenced during event processing.
 *
 * Usage:
 *   BoopsiDispose_Init(&queue);
 *   ...
 *   BoopsiDispose_Later(&queue, someGadget);  // Queue for later disposal
 *   ...
 *   BoopsiDispose_Flush(&queue);  // Actually dispose all queued objects
 */

#include <exec/types.h>
#include <intuition/classes.h>

/* Maximum number of objects that can be queued for disposal */
#define BOOPSIDISPOSE_QUEUE_SIZE 64

/* Queue structure - to be aggregated in App struct */
typedef struct BoopsiDisposeQueue
{
    Object *queue[BOOPSIDISPOSE_QUEUE_SIZE];
    UWORD count;    /* Number of objects in queue */
} BoopsiDisposeQueue;

/**
 * Initialize the disposal queue
 */
//no need void BoopsiDispose_Init(BoopsiDisposeQueue *q);

/**
 * Add an object to the disposal queue (dispose later)
 * @param q     The disposal queue
 * @param obj   The BOOPSI object to dispose later
 * @return TRUE if added successfully, FALSE if queue is full
 */
BOOL BoopsiDispose_Later(BoopsiDisposeQueue *q, Object *obj);

/**
 * Check if there are objects pending disposal
 */
BOOL BoopsiDispose_HasPending(BoopsiDisposeQueue *q);

/**
 * Get number of pending objects
 */
UWORD BoopsiDispose_Count(BoopsiDisposeQueue *q);

/**
 * Flush the queue: dispose all queued objects and clear the queue
 * Calls DisposeObject() on each queued object
 */
void BoopsiDispose_Flush(BoopsiDisposeQueue *q);

#endif /* BOOPSIDISPOSE_H */
