/* SPDX-License-Identifier: MIT */
#include <OS.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

extern status_t _kern_set_thread_affinity(thread_id, const void*, size_t);
extern int32 _kern_get_cpu(void);

struct memory_worker {
	unsigned cpu;
	volatile uint64* memory;
	size_t words, offset;
	bigtime_t deadline;
};

static uint64 pattern(size_t word, unsigned round)
{
	return (word + 1) * UINT64_C(0x9e3779b97f4a7c15)
		^ (round ? UINT64_C(0x5a5a33cc0ff0c33c) : UINT64_C(0xa5a5cc33f00f3cc3));
}

static int32 worker(void* cookie)
{
	struct memory_worker* w = cookie;
	uint64 affinity[16] = {0};
	affinity[w->cpu / 64] = UINT64_C(1) << (w->cpu % 64);
	if (_kern_set_thread_affinity(0, affinity, sizeof(affinity)) != B_OK) return 1;
	for (unsigned round = 0; round < 2; round++) {
		for (size_t i = 0; i < w->words; i++) {
			if ((i & 131071) == 0 && system_time() >= w->deadline) return 2;
			w->memory[i] = pattern(w->offset + i, round);
		}
		if (snooze(10000) != B_OK || _kern_get_cpu() != (int32)w->cpu) return 3;
		for (size_t i = 0; i < w->words; i++) {
			if ((i & 131071) == 0 && system_time() >= w->deadline) return 2;
			if (w->memory[i] != pattern(w->offset + i, round)) return 4;
		}
	}
	return 0;
}

int main(int argc, char** argv)
{
	setvbuf(stdout, NULL, _IONBF, 0);
	if (argc != 2 || strncmp(argv[1], "--mib=", 6) != 0) {
		fprintf(stderr, "Usage: %s --mib=64..4096 (multiple of 64)\n", argv[0]); return 2;
	}
	char* end;
	unsigned long mib = strtoul(argv[1] + 6, &end, 10);
	if (*end || mib < 64 || mib > 4096 || mib % 64) return 2;
	system_info info;
	if (get_system_info(&info) != B_OK || info.cpu_count != 64) return 1;
	uint64 total = (uint64)info.max_pages * B_PAGE_SIZE;
	uint64 unavailable = ((uint64)info.used_pages + info.cached_pages) * B_PAGE_SIZE;
	uint64 bytes = (uint64)mib * 1024 * 1024;
	if (total <= UINT64_C(0xc0000000) || unavailable > total
		|| total - unavailable < bytes + UINT64_C(0x40000000)) {
		fprintf(stderr, "FAIL: require >3 GiB RAM and test size plus 1 GiB free headroom\n"); return 1;
	}
	cpu_info cpus[64];
	if (get_cpu_info(0, 64, cpus) != B_OK) return 1;
	for (unsigned i = 0; i < 64; i++) if (!cpus[i].enabled) return 1;
	volatile uint64* memory = malloc(bytes);
	if (memory == NULL) return 1;
	printf("MEMORY BEGIN: reported %" PRIu64 " bytes; private sample %lu MiB, 64 pinned workers, two full patterns\n", total, mib);
	struct memory_worker workers[64];
	thread_id threads[64];
	bigtime_t deadline = system_time() + 60000000;
	for (unsigned i = 0; i < 64; i++) {
		size_t words = bytes / sizeof(uint64) / 64;
		workers[i] = (struct memory_worker){i, memory + i * words, words, i * words, deadline};
		threads[i] = spawn_thread(worker, "Pioneer private RAM sample", B_NORMAL_PRIORITY, &workers[i]);
		if (threads[i] < B_OK || resume_thread(threads[i]) != B_OK) {
			fprintf(stderr, "FAIL: worker creation; terminating only test team\n"); _exit(1);
		}
	}
	uint64 seen = 0;
	for (unsigned i = 0; i < 64; i++) {
		status_t status;
		status_t waited = wait_for_thread_etc(threads[i], B_ABSOLUTE_TIMEOUT, deadline + 2000000, &status);
		if (waited != B_OK || status != 0) {
			fprintf(stderr, "FAIL: CPU %u wait %ld status %ld; terminating only test team\n", i, (long)waited, (long)(waited == B_OK ? status : -1));
			_exit(1); // Do not release memory while another worker might still use it.
		}
		seen |= UINT64_C(1) << i;
	}
	free((void*)memory);
	printf("PASS: %lu MiB private RAM sample, two full patterns, CPU mask %016" PRIx64 "; not a whole-RAM audit\n", mib, seen);
	return seen == UINT64_MAX ? 0 : 1;
}
