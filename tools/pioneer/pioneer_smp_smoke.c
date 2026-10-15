/* Distributed under the MIT License. */
#include <OS.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern status_t _kern_set_thread_affinity(thread_id, const void*, size_t);
extern int32 _kern_get_cpu(void);

struct worker_result {
	uint32 cpu, checksum, actual;
	status_t error, sleep_error;
	uint32 sleep_interrupts;
};

// PIONEER_SMOKE_SLEEP_BEGIN
static status_t
wait_for_wakeup(uint32* interruptions)
{
	const bigtime_t deadline = system_time() + 1000;
	for (unsigned retry = 0; retry < 32; retry++) {
		status_t status = snooze_until(deadline, B_SYSTEM_TIMEBASE);
		if (status != B_INTERRUPTED) return status;
		(*interruptions)++;
		if (system_time() >= deadline) return B_OK;
	}
	// Fail closed even if a broken clock or a signal storm prevents progress.
	return B_INTERRUPTED;
}
// PIONEER_SMOKE_SLEEP_END

static uint32
expected_checksum(void)
{
	uint32 value = 0x12345678;
	for (unsigned round = 0; round < 32; round++)
		for (unsigned i = 0; i < 10000; i++) value = value * 1664525U + 1013904223U;
	return value;
}

static int32
worker(void* cookie)
{
	struct worker_result* result = cookie;
	uint64 mask[16] = {0};
	mask[result->cpu / 64] = UINT64_C(1) << (result->cpu % 64);
	result->error = _kern_set_thread_affinity(0, mask, sizeof(mask));
	if (result->error != B_OK) return 1;
	volatile uint32 memory[256];
	uint32 value = 0x12345678;
	for (unsigned round = 0; round < 32; round++) {
		result->actual = _kern_get_cpu();
		if (result->actual != result->cpu) return 2;
		for (unsigned i = 0; i < 256; i++) memory[i] = (result->cpu << 24) ^ (round << 16) ^ i;
		for (unsigned i = 0; i < 10000; i++) value = value * 1664525U + 1013904223U;
		result->sleep_error = wait_for_wakeup(&result->sleep_interrupts);
		if (result->sleep_error != B_OK) return 3;
		if (_kern_get_cpu() != (int32)result->cpu) return 4;
		for (unsigned i = 0; i < 256; i++) {
			if (memory[i] != ((result->cpu << 24) ^ (round << 16) ^ i)) return 5;
		}
	}
	result->checksum = value;
	return value == expected_checksum() ? 0 : 6;
}

int main(int argc, char** argv)
{
	/* Keep results visible on /dev/dprintf even if a later worker panics. */
	setvbuf(stdout, NULL, _IONBF, 0);
	if (argc != 2 || strncmp(argv[1], "--expect-cpus=", 14) != 0) {
		fprintf(stderr, "Usage: %s --expect-cpus=2|4|8|16|32|64\n", argv[0]); return 2;
	}
	char* end;
	unsigned long count = strtoul(argv[1] + 14, &end, 10);
	if (*end || count < 2 || count > 64 || (count & (count - 1))) return 2;
	system_info info;
	if (get_system_info(&info) != B_OK || info.cpu_count != count) {
		fprintf(stderr, "CPU-count gate failed; requested %lu\n", count); return 1;
	}
	cpu_info cpus[64];
	if (get_cpu_info(0, count, cpus) != B_OK) return 1;
	for (unsigned i = 0; i < count; i++) {
		if (!cpus[i].enabled) { fprintf(stderr, "CPU %u disabled\n", i); return 1; }
	}
	struct worker_result results[64] = {{0}};
	thread_id threads[64];
	unsigned spawned = 0;
	int failed = 0;
	for (unsigned i = 0; i < count; i++) {
		results[i].cpu = i;
		threads[i] = spawn_thread(worker, "Pioneer pinned CPU smoke", B_NORMAL_PRIORITY, &results[i]);
		if (threads[i] < B_OK) { failed = 1; break; }
		if (resume_thread(threads[i]) != B_OK) { kill_thread(threads[i]); failed = 1; break; }
		spawned++;
	}
	uint64 seen = 0;
	bigtime_t deadline = system_time() + 10000000;
	for (unsigned i = 0; i < spawned; i++) {
		status_t status = -1;
		status_t waitStatus = wait_for_thread_etc(threads[i], B_ABSOLUTE_TIMEOUT, deadline, &status);
		if (waitStatus != B_OK || status != 0) failed = 1;
		else seen |= UINT64_C(1) << i;
		printf("CPU %u actual %" PRIu32 " status %ld wait %ld affinity %ld sleep %ld interruptions %" PRIu32 " checksum %08" PRIx32 "\n",
			i, results[i].actual, (long)status, (long)waitStatus,
			(long)results[i].error, (long)results[i].sleep_error,
			results[i].sleep_interrupts, results[i].checksum);
		if (waitStatus != B_OK) {
			// Only our own test threads. Do not wait forever if the test fails.
			for (unsigned j = i; j < spawned; j++) kill_thread(threads[j]);
			break;
		}
	}
	uint64 expected = count == 64 ? UINT64_MAX : (UINT64_C(1) << count) - 1;
	if (seen != expected) failed = 1;
	printf("%s: %lu enabled CPUs, pinned work/wakeups/private-memory checks, mask %016" PRIx64 "\n",
		failed ? "FAIL" : "PASS", count, seen);
	return failed;
}
