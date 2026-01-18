/**
 * BoopsiDelay - Delayed message queue for BOOPSI gadget notifications
 */

#include "boopsidelay.h"
#include <intuition/gadgetclass.h>

void BoopsiDelay_Init(BoopsiDelayQueue *q)
{
    q->writePos = 0;
    q->readPos = 0;
    q->hasMessages = FALSE;
}

BOOL BoopsiDelay_AddTag(BoopsiDelayQueue *q, ULONG tag, ULONG data)
{
    int maxmessage=BOOPSIDELAY_QUEUE_SIZE-4;

    if(tag== TAG_END) maxmessage=BOOPSIDELAY_QUEUE_SIZE; // if no more room, room for TAG_END.
    if (q->writePos >= maxmessage) // I added -16 because final TAG_END must not be missing.
    {
        return FALSE;
    }

    q->queue[q->writePos].ti_Tag = tag;
    q->queue[q->writePos].ti_Data = data;
    q->writePos++;

    return TRUE;
}

BOOL BoopsiDelay_BeginMessage(BoopsiDelayQueue *q, ULONG senderID)
{
    /* Need at least 2 slots: GA_ID + TAG_END */
    if (q->writePos >= BOOPSIDELAY_QUEUE_SIZE - 1)
    {
        return FALSE;
    }

    return BoopsiDelay_AddTag(q, GA_ID, senderID);
}

BOOL BoopsiDelay_EndMessage(BoopsiDelayQueue *q)
{
    if (!BoopsiDelay_AddTag(q, TAG_END, 0))
    {
        return FALSE;
    }

    q->hasMessages = TRUE;
    return TRUE;
}

BOOL BoopsiDelay_HasMessages(BoopsiDelayQueue *q)
{
    return q->hasMessages;
}

struct TagItem *BoopsiDelay_NextMessage(BoopsiDelayQueue *q)
{
    struct TagItem *msg;

    if (!q->hasMessages || q->readPos >= q->writePos)
    {
        /* No more messages - reset queue for reuse */
        q->readPos = 0;
        q->writePos = 0;
        q->hasMessages = FALSE;
        return NULL;
    }

    /* Return pointer to current message start */
    msg = &q->queue[q->readPos];

    /* Advance readPos past this message (find TAG_END) */
    while (q->readPos < q->writePos)
    {
        if (q->queue[q->readPos].ti_Tag == TAG_END)
        {
            q->readPos++;
            break;
        }
        q->readPos++;
    }

    /* Check if more messages remain */
    if (q->readPos >= q->writePos)
    {
        q->readPos = 0;
        q->writePos = 0;
        q->hasMessages = FALSE;
    }

    return msg;
}
