/**
 * BoopsiDispose - Delayed object disposal queue for BOOPSI gadgets
 */

#include "boopsidispose.h"
#include <proto/intuition.h>

/* no need
void BoopsiDispose_Init(BoopsiDisposeQueue *q)
{
    UWORD i;
    q->count = 0;
    for (i = 0; i < BOOPSIDISPOSE_QUEUE_SIZE; i++)
    {
        q->queue[i] = NULL;
    }
}*/

BOOL BoopsiDispose_Later(BoopsiDisposeQueue *q, Object *obj)
{
    if (!obj)
    {
        return FALSE;
    }

    if (q->count >= BOOPSIDISPOSE_QUEUE_SIZE)
    {
        return FALSE;
    }

    q->queue[q->count] = obj;
    q->count++;

    return TRUE;
}

BOOL BoopsiDispose_HasPending(BoopsiDisposeQueue *q)
{
    return (q->count > 0);
}

UWORD BoopsiDispose_Count(BoopsiDisposeQueue *q)
{
    return q->count;
}

void BoopsiDispose_Flush(BoopsiDisposeQueue *q)
{
    UWORD i;

    for (i = 0; i < q->count; i++)
    {
        if (q->queue[i])
        {
            DisposeObject(q->queue[i]);
            q->queue[i] = NULL;
        }
    }

    q->count = 0;
}
