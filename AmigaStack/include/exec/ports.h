#ifndef EXEC_PORTS_H
#define EXEC_PORTS_H

/*
 * AmigaStack - Exec Ports
 * Message port structures and definitions
 */

#include <exec/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Message structure */
struct Message {
    struct Message* mn_ReplyPort;  /* Reply port pointer */
    UWORD mn_Length;                /* Message length */
};

/* Message port structure */
struct MsgPort {
    struct Message* head;           /* Message queue head */
    struct Message* tail;           /* Message queue tail */
    void* sigbit;                   /* Signal bit (not used in AmigaStack) */
};

#ifdef __cplusplus
}
#endif

#endif /* EXEC_PORTS_H */
