#include <nspr.h>
#include <stdio.h>
#include <string.h>

static PRInt32 count;

static void worker(void* unused)
{
    (void)unused;
    for (int i = 0; i < 10000; i++)
        PR_AtomicIncrement(&count);
}

int main(void)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    PR_Init(PR_USER_THREAD, PR_PRIORITY_NORMAL, 0);
    char architecture[64];
    if (PR_GetSystemInfo(PR_SI_ARCHITECTURE, architecture, sizeof(architecture)) != PR_SUCCESS
        || strcmp(architecture, "riscv64") != 0)
        return 1;
    PRThread* threads[4];
    for (int i = 0; i < 4; i++) {
        threads[i] = PR_CreateThread(PR_USER_THREAD, worker, NULL,
            PR_PRIORITY_NORMAL, PR_GLOBAL_THREAD, PR_JOINABLE_THREAD, 0);
        if (!threads[i])
            return 2;
    }
    for (int i = 0; i < 4; i++)
        if (PR_JoinThread(threads[i]) != PR_SUCCESS)
            return 3;
    printf("NSPR atomic count: %d (expected 40000)\n", count);
    if (count != 40000)
        return 4;
    puts("NSPR architecture/threads/atomics: PASS");
    PRNetAddr address;
    if (PR_InitializeNetAddr(PR_IpAddrLoopback, 0, &address) != PR_SUCCESS)
        return 5;
    PRFileDesc* socket = PR_NewTCPSocket();
    if (!socket || PR_Bind(socket, &address) != PR_SUCCESS
        || PR_Listen(socket, 1) != PR_SUCCESS || PR_GetSockName(socket, &address) != PR_SUCCESS)
        return 6;
    if (PR_Close(socket) != PR_SUCCESS)
        return 7;
    puts("NSPR loopback socket/bind/listen: PASS");
    if (PR_Cleanup() != PR_SUCCESS)
        return 8;
    puts("NSPR_SMOKE_PASS (not a Firefox browser test)");
    return 0;
}
