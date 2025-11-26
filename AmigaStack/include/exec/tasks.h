#ifndef EXEC_TASKS_H
#define EXEC_TASKS_H

/*
 * AmigaStack - Exec Tasks and Processes
 */

#ifdef __cplusplus
extern "C" {
#endif

/* Message structure */
struct Message {
    struct Message* mn_ReplyPort;
    unsigned short mn_Length;
};

/* Message port structure */
struct MsgPort {
    struct Message* head;
    struct Message* tail;
    void* sigbit;
};

/* Process structure */
struct Process {
    int dummy;  /* Placeholder */
};

#ifdef __cplusplus
}
#endif

#endif /* EXEC_TASKS_H */
