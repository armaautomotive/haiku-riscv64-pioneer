#include <OS.h>
#include <stdio.h>
#include <string.h>

extern status_t _kern_set_thread_affinity(thread_id, const void*, size_t);
extern int32 _kern_get_cpu(void);

static int32
worker(void* unused)
{
	(void)unused;
	system_info info;
	if (get_system_info(&info) != B_OK || info.cpu_count < 2)
		return 1;
	for (int i = 0; i < 100; i++) {
		uint32 cpu = i % 2 == 0 ? 0 : info.cpu_count - 1;
		uint64 mask[16] = {0};
		mask[cpu / 64] = (uint64)1 << (cpu % 64);
		status_t status = _kern_set_thread_affinity(0, mask, sizeof(mask));
		if (status != B_OK) {
			fprintf(stderr, "iteration %d: affinity failed: %s\n", i, strerror(status));
			return 1;
		}
		if (_kern_get_cpu() != (int32)cpu) {
			fprintf(stderr, "iteration %d: wrong CPU after affinity change\n", i);
			return 1;
		}
		snooze(1000);
		if (_kern_get_cpu() != (int32)cpu) {
			fprintf(stderr, "iteration %d: wrong CPU after wakeup\n", i);
			return 1;
		}
	}
	return 0;
}

int
main(void)
{
	thread_id thread = spawn_thread(worker, "affinity smoke", B_NORMAL_PRIORITY, NULL);
	if (thread < B_OK)
		return 1;
	uint64 mask[16] = {1};
	status_t status = _kern_set_thread_affinity(thread, mask, sizeof(mask));
	if (status != B_OK) {
		fprintf(stderr, "suspended-thread affinity failed: %s\n", strerror(status));
		kill_thread(thread);
		return 1;
	}
	if (resume_thread(thread) != B_OK)
		return 1;
	status_t result;
	if (wait_for_thread(thread, &result) != B_OK || result != 0)
		return 1;
	puts("PASS: suspended-thread affinity and 100 CPU migrations with wakeups");
	return 0;
}
