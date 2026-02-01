/**
 * BoopsiDelay - Delayed message queue for BOOPSI gadget notifications
 */

#include "boopsimessage.h"
#include <intuition/gadgetclass.h>

/* to have the interesting gadgets properties to listen */
#include "TrackListArea/class_tracklistarea.h"
#include "TrackArea/class_trackarea.h"
#include <gadgets/slider.h>
#include <gadgets/scroller.h>

#include "bdbprintf.h"


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


// - - - note having a private "boopsi object class and instance"
// - - - makes it fancy to connect values and receive events.
// Boopsi class pointer to manage our private modelclass.
Class *TargetModelClass = NULL;
// Target Model instance as a Boopsi object.
Object *TargetInstance = NULL;

BoopsiDelayQueue *DelayQueue=NULL;

typedef ULONG (*REHOOKFUNC)();


extern struct Task	*myTask;

/**
 * Initialize the notification queue
 */
void BoopsiDelay_Init(BoopsiDelayQueue *q);


/* The attribs we actually delay
*/
static ULONG delayedAttribs[]={
    GA_Selected,SLIDER_Level,SCROLLER_Top,
    TRACKLIST_ScrollY,TRACKLIST_TimeProjection,TRACKLIST_DomainHeight,
    TRACKAREA_TimeSelectionChange,TRACKAREA_TimeZoomChange,
    TRACKAREA_SoundSlideChange
};
#define nbDelayedAttribs (sizeof(delayedAttribs)/sizeof(ULONG))


// usefull union for dispatchers. Each structs also starts with MethodID.
typedef union MsgUnion
{
  ULONG  MethodID;
  // from classusr.h or gadgetclass.h, all starts with MethodID.
  struct opSet        opSet;
  struct opUpdate     opUpdate;
  struct opGet        opGet;
  struct gpHitTest    gpHitTest;
  struct gpRender     gpRender;
  struct gpInput      gpInput;
  struct gpGoInactive gpGoInactive;
  struct gpLayout     gpLayout;
} *Msgs;


static ULONG ASM SAVEDS TargetModelDispatch(
                    REG(a0,struct IClass *C),
                    REG(a2,Object *obj),
                    REG(a1,union MsgUnion *M))
{
  ULONG retval=0;

  switch(M->MethodID)
  {
    case OM_NEW:
        if((obj=(Object *)DoSuperMethodA(C,(Object *)obj,(Msg)M))!= NULL)
        {
            DelayQueue=(struct BoopsiDelayQueue *)INST_DATA(C, obj);
            memset(DelayQueue,0,sizeof(struct BoopsiDelayQueue)); // absolutely *NOT* sure about this being cleaned, more secure.
            BoopsiDelay_Init(DelayQueue);
            retval = (ULONG)obj;
        }
    break;
    case OM_DISPOSE:
        retval=DoSuperMethodA(C,(Object *)obj,(Msg)M);
      break;
    case OM_NOTIFY:
    case OM_UPDATE:
        {
            struct TagItem *ptag;
            ULONG sender_ID=0;


            /* active this to trace messages sent by gadgets
             {
                ptag = M->opUpdate.opu_AttrList;
                while(ptag->ti_Tag != 0)
                {
                    bdbprintf("n:%08x %08x\n",ptag->ti_Tag,ptag->ti_Data);
                    ptag++;
                }
                bdbprintf("\n");
            }*/

            if((ptag = FindTagItem( GA_ID,M->opUpdate.opu_AttrList ))!=NULL) sender_ID = ptag->ti_Data;

            /* Queue message if sender_ID != 0 */
            if (sender_ID != 0)
            {
                int i;
                BoopsiDelay_BeginMessage(DelayQueue, sender_ID);
                for(i=0;i<nbDelayedAttribs;i++)
                {
                    if ((ptag = FindTagItem(delayedAttribs[i], M->opUpdate.opu_AttrList)) != NULL)
                        BoopsiDelay_AddTag(DelayQueue, delayedAttribs[i], ptag->ti_Data);
                }

                BoopsiDelay_EndMessage(DelayQueue);

                /* Signal main loop to process queue */
                if (myTask) Signal(myTask, SIGBREAKF_CTRL_F);

                retval = 1;
            }
        }
        break;
    default:
        retval=DoSuperMethodA(C,(Object *)obj,(Msg)M);
    break;
  }
  return retval;
}

int initMessageTargetModel(void)
{
    // this is how you create a private transient class:
    // -First param: no name needed for itself.
    // - "modelclass" is super class name, which is the base for all boopsi class.
    // a super class name or pointer must always be provided.
    TargetModelClass = MakeClass(NULL,"modelclass",NULL,sizeof(struct BoopsiDelayQueue),0);
    if(!TargetModelClass) return 0;
    bdbprintf_makeclass("TargetModelClass", TargetModelClass);

    TargetModelClass->cl_Dispatcher.h_Entry = (REHOOKFUNC) &TargetModelDispatch;

    TargetInstance = (Object *)NewObject( TargetModelClass, NULL, TAG_DONE);
    if(!TargetInstance) return 0;

    return 1;
}
void closeMessageTargetModel(void)
{
    if(TargetInstance) DisposeObject(TargetInstance);
    TargetInstance = NULL;

    if(TargetModelClass)
    {
        bdbprintf_freeclass("TargetModelClass", TargetModelClass);
        FreeClass(TargetModelClass);
    }
    TargetModelClass = NULL;
}

// - - - - - - - - - -

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
