#ifndef EXEC_PORTS_H
#define EXEC_PORTS_H

/*
 * AmigaStack - Exec Ports
 * Message port structures and definitions
 *
 * On PC: uses pthreads mutex and condition variables for synchronization
 * On Amiga: uses native MsgPort/Message (this header not used)
 */

#include <exec/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declaration */
struct MsgPort;

/* Node structure for linked list (minimal, for compatibility) */
struct Node {
    struct Node* ln_Succ;
    struct Node* ln_Pred;
    UBYTE ln_Type;
    BYTE ln_Pri;
    char* ln_Name;
};

/* Message structure - must be first member of any message struct */
struct Message {
    struct Node mn_Node;            /* Linked list node */
    struct MsgPort* mn_ReplyPort;   /* Reply port pointer */
    UWORD mn_Length;                /* Message length */
};

/* Message port structure */
struct MsgPort {
    struct Node mp_Node;            /* Linked list node */
    UBYTE mp_Flags;                 /* Flags */
    UBYTE mp_SigBit;                /* Signal bit (for compatibility) */
    void* mp_SigTask;               /* Task to signal (for compatibility) */

    /* Message queue */
    struct Message* mp_MsgList_Head;
    struct Message* mp_MsgList_Tail;

    /* Platform-specific synchronization (opaque pointers) */
    void* _mutex;                   /* Mutex for queue protection */
    void* _cond;                    /* Condition variable for waiting */
};

#ifdef __cplusplus
}
#endif

#endif /* EXEC_PORTS_H */
