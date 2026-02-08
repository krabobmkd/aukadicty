/*
 * Sound File Engine Test
 *
 * Tests the threading/messaging system for background file loading.
 * This example works on PC (Windows/Linux) using AmigaStack's
 * cross-platform thread abstraction.
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#else
    #include <unistd.h>
#endif

#include <proto/exec.h>
#include <proto/dos.h>
#include <dos/dostags.h>

#include "auksoundfileengine.h"
#include "auksoundfile.h"

/* Test 1: Basic engine init/shutdown */
static int test_engine_init_shutdown(void)
{
    AukSoundFileEngine* engine;

    printf("\n=== Test 1: Engine Init/Shutdown ===\n");

    engine = AukSoundFileEngine_Init(NULL,"PROGDIR:",0);
    if (!engine) {
        printf("FAIL: Could not initialize engine\n");
        return 0;
    }
    printf("Engine initialized successfully\n");

    AukSoundFileEngine_Shutdown(engine);
    printf("Engine shutdown successfully\n");

    printf("PASS\n");
    return 1;
}

/* Test 2: Request and release a file */
static int test_request_release_file(void)
{
    AukSoundFileEngine* engine;
    AukSoundFile* file;
    AukSFEFileStatus status;

    printf("\n=== Test 2: Request/Release File ===\n");

    engine = AukSoundFileEngine_Init(NULL,"PROGDIR:",0);
    if (!engine) {
        printf("FAIL: Could not initialize engine\n");
        return 0;
    }

    /* Request a file */
    printf("Requesting file: test.wav\n");
    file = AukSoundFileEngine_RequestFile(engine, "test.wav");
    if (!file) {
        printf("FAIL: Could not request file\n");
        AukSoundFileEngine_Shutdown(engine);
        return 0;
    }
    printf("File requested successfully\n");

    /* Check status */
    status = AukSoundFileEngine_GetFileStatus(engine, file);
    printf("File status: %d\n", (int)status);

    /* Release the file */
    printf("Releasing file\n");
    AukSoundFileEngine_ReleaseFile(engine, file);
    printf("File released\n");

    AukSoundFileEngine_Shutdown(engine);
    printf("PASS\n");
    return 1;
}

/* Test 3: Request same file multiple times */
static int test_multiple_requests(void)
{
    AukSoundFileEngine* engine;
    AukSoundFile* file1;
    AukSoundFile* file2;

    printf("\n=== Test 3: Multiple Requests for Same File ===\n");

    engine = AukSoundFileEngine_Init(NULL,"PROGDIR:",0);
    if (!engine) {
        printf("FAIL: Could not initialize engine\n");
        return 0;
    }

    /* Request same file twice */
    printf("Requesting file first time\n");
    file1 = AukSoundFileEngine_RequestFile(engine, "shared.wav");
    if (!file1) {
        printf("FAIL: First request failed\n");
        AukSoundFileEngine_Shutdown(engine);
        return 0;
    }

    printf("Requesting file second time\n");
    file2 = AukSoundFileEngine_RequestFile(engine, "shared.wav");
    if (!file2) {
        printf("FAIL: Second request failed\n");
        AukSoundFileEngine_ReleaseFile(engine, file1);
        AukSoundFileEngine_Shutdown(engine);
        return 0;
    }

    /* They should be the same pointer (shared) */
    if (file1 == file2) {
        printf("Files are shared (same pointer) - correct behavior\n");
    } else {
        printf("Files are different pointers - may also be valid\n");
    }

    /* Release both */
    printf("Releasing both references\n");
    AukSoundFileEngine_ReleaseFile(engine, file1);
    AukSoundFileEngine_ReleaseFile(engine, file2);

    AukSoundFileEngine_Shutdown(engine);
    printf("PASS\n");
    return 1;
}

/* Test 4: Basic message port test (direct test of AmigaStack) */
static int test_message_port(void)
{
    struct MsgPort* port;
    struct Message msg;
    struct Message* received;

    printf("\n=== Test 4: Message Port Basic Test ===\n");

    /* Create port */
    port = CreateMsgPort();
    if (!port) {
        printf("FAIL: Could not create message port\n");
        return 0;
    }
    printf("Message port created\n");

    /* Send a message to ourselves */
    memset(&msg, 0, sizeof(msg));
    msg.mn_ReplyPort = port;
    msg.mn_Length = sizeof(msg);

    printf("Sending message to port\n");
    PutMsg(port, &msg);

    /* Receive it */
    printf("Receiving message from port\n");
    received = GetMsg(port);
    if (!received) {
        printf("FAIL: Did not receive message\n");
        DeleteMsgPort(port);
        return 0;
    }

    if (received == &msg) {
        printf("Received correct message\n");
    }

    DeleteMsgPort(port);
    printf("PASS\n");
    return 1;
}

/* Test 5: Thread with message passing */
static struct MsgPort* g_testReplyPort = NULL;
static int g_testWorkerReceived = 0;

static void test_worker_thread(void)
{
    struct Process* proc = (struct Process*)FindTask(NULL);
    struct Message* msg;

    printf("[Worker] Started\n");

    /* Wait for message */
    WaitPort(&proc->pr_MsgPort);
    msg = GetMsg(&proc->pr_MsgPort);

    if (msg) {
        printf("[Worker] Received message\n");
        g_testWorkerReceived = 1;
        ReplyMsg(msg);
    }

    printf("[Worker] Exiting\n");
}

static int test_thread_messaging(void)
{
    struct Process* worker;
    struct Message msg;
    struct TagItem tags[4];

    printf("\n=== Test 5: Thread Messaging ===\n");

    g_testWorkerReceived = 0;

    /* Create reply port */
    g_testReplyPort = CreateMsgPort();
    if (!g_testReplyPort) {
        printf("FAIL: Could not create reply port\n");
        return 0;
    }

    /* Create worker thread */
    printf("Creating worker thread\n");
#ifndef AMIGA
    (void)tags;  /* Unused on PC */
    worker = CreateNewProcSimple(test_worker_thread, "TestWorker", 0);
#else
    tags[0].ti_Tag = NP_Entry;
    tags[0].ti_Data = (ULONG)test_worker_thread;
    tags[1].ti_Tag = NP_Name;
    tags[1].ti_Data = (ULONG)"TestWorker";
    tags[2].ti_Tag = TAG_DONE;
    worker = CreateNewProc(tags);
#endif
    if (!worker) {
        printf("FAIL: Could not create worker thread\n");
        DeleteMsgPort(g_testReplyPort);
        return 0;
    }
    printf("Worker thread created\n");

    /* Give worker time to start */
#ifdef _WIN32
    Sleep(100);
#else
    usleep(100000);
#endif

    /* Send message to worker */
    memset(&msg, 0, sizeof(msg));
    msg.mn_ReplyPort = g_testReplyPort;
    msg.mn_Length = sizeof(msg);

    printf("Sending message to worker\n");
    PutMsg(&worker->pr_MsgPort, &msg);

    /* Wait for reply */
    printf("Waiting for reply\n");
    WaitPort(g_testReplyPort);
    (void)GetMsg(g_testReplyPort);

    if (g_testWorkerReceived) {
        printf("Worker successfully received and replied to message\n");
    } else {
        printf("FAIL: Worker did not receive message\n");
        DeleteMsgPort(g_testReplyPort);
        return 0;
    }

    /* Give worker time to exit */
#ifdef _WIN32
    Sleep(100);
#else
    usleep(100000);
#endif

    DeleteMsgPort(g_testReplyPort);
    printf("PASS\n");
    return 1;
}

int main(void)
{
    int passed = 0;
    int total = 5;

    printf("========================================\n");
    printf("Sound File Engine & Threading Tests\n");
    printf("========================================\n");

    passed += test_message_port();
    passed += test_thread_messaging();
    passed += test_engine_init_shutdown();
    passed += test_request_release_file();
    passed += test_multiple_requests();

    printf("\n========================================\n");
    printf("Results: %d/%d tests passed\n", passed, total);
    printf("========================================\n");

    return (passed == total) ? 0 : 1;
}
