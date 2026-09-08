#include <OS.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern status_t _kern_get_thread_affinity(thread_id, void*, size_t);
extern status_t _kern_set_thread_affinity(thread_id, const void*, size_t);

int
main(int argc, char** argv)
{
	if (argc == 2 || (argc == 3 && strcmp(argv[2], "--pin") == 0)) {
		int32 cookie = 0;
		uint32 cpu = 0;
		thread_info thread;
		while (get_next_thread_info(atoi(argv[1]), &cookie, &thread) == B_OK) {
			uint64 mask[16] = {0};
			if (argc == 3) {
				if (cpu >= 64)
					return 1;
				mask[0] = (uint64)1 << cpu++;
				if (_kern_set_thread_affinity(thread.thread, mask, sizeof(mask)) != B_OK)
					return 1;
			}
			status_t status = _kern_get_thread_affinity(thread.thread, mask, sizeof(mask));
			printf("thread=%ld state=%d affinity_status=%ld mask=%016llx\n",
				(long)thread.thread, thread.state, (long)status,
				(unsigned long long)mask[0]);
		}
		return 0;
	}
	system_info system;
	if (get_system_info(&system) != B_OK)
		return 1;
	cpu_info* before = calloc(system.cpu_count, sizeof(cpu_info));
	cpu_info* after = calloc(system.cpu_count, sizeof(cpu_info));
	if (before == NULL || after == NULL)
		return 1;
	if (get_cpu_info(0, system.cpu_count, before) != B_OK)
		return 1;
	bigtime_t start = system_time();
	snooze(2000000);
	if (get_cpu_info(0, system.cpu_count, after) != B_OK)
		return 1;
	bigtime_t elapsed = system_time() - start;
	printf("cpu enabled active_percent\n");
	for (uint32 i = 0; i < system.cpu_count; i++)
		printf("%u %d %.2f\n", (unsigned)i, after[i].enabled,
			100.0 * (after[i].active_time - before[i].active_time) / elapsed);
	free(before);
	free(after);
	return 0;
}
