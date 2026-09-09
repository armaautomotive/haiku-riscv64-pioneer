#include <OS.h>
#include <stdatomic.h>
#include <stdio.h>
#include <string.h>

extern status_t _kern_set_thread_affinity(thread_id, const void*, size_t);
extern int32 _kern_get_cpu(void);

static atomic_int ready;
static atomic_llong start_time;
static struct {
    status_t error;
    unsigned long samples[64];
} results[64];

static int32 worker(void* argument)
{
    int index = (int)(addr_t)argument;
    uint64 mask[16] = {0};
    // Seed two workers on CPU 0 and leave CPU 63 without a probe worker.
    mask[0] = (uint64)1 << (index == 63 ? 0 : index);
    results[index].error = _kern_set_thread_affinity(0, mask, sizeof(mask));
    atomic_fetch_add(&ready, 1);
    bigtime_t start;
    while ((start = atomic_load(&start_time)) == 0)
        snooze(1000);
    if (results[index].error != B_OK)
        return 1;
    bool released = false;
    while (system_time() - start < 5000000) {
        if (!released && system_time() - start >= 1000000) {
            memset(mask, 0, sizeof(mask));
            results[index].error = _kern_set_thread_affinity(0, mask, sizeof(mask));
            if (results[index].error != B_OK)
                return 1;
            released = true;
        }
        for (int spin = 0; spin < 4096; spin++)
            __asm__ volatile("" ::: "memory");
        if (released) {
            int32 cpu = _kern_get_cpu();
            if (cpu >= 0 && cpu < 64)
                results[index].samples[cpu]++;
        }
    }
    return 0;
}

int main(void)
{
    system_info info;
    if (get_system_info(&info) != B_OK || info.cpu_count != 64) {
        fprintf(stderr, "This bounded probe requires 64 online CPUs.\n");
        return 1;
    }
    thread_id threads[64];
    for (int i = 0; i < 64; i++) {
        threads[i] = spawn_thread(worker, "scheduler balance probe",
            B_NORMAL_PRIORITY, (void*)(addr_t)i);
        if (threads[i] < B_OK || resume_thread(threads[i]) != B_OK)
            return 1;
    }
    bigtime_t deadline = system_time() + 10000000;
    while (atomic_load(&ready) != 64) {
        if (system_time() > deadline)
            return 1;
        snooze(1000);
    }
    atomic_store(&start_time, system_time());
    bool failed = false;
    puts("worker thread status cpu:samples_after_affinity_release");
    for (int i = 0; i < 64; i++) {
        status_t result = B_ERROR;
        status_t status = wait_for_thread(threads[i], &result);
        if (status != B_OK || result != B_OK || results[i].error != B_OK)
            failed = true;
        printf("%d %ld %ld", i, (long)threads[i], (long)results[i].error);
        for (int cpu = 0; cpu < 64; cpu++) {
            if (results[i].samples[cpu] != 0)
                printf(" %d:%lu", cpu, results[i].samples[cpu]);
        }
        putchar('\n');
    }
    return failed ? 1 : 0;
}
