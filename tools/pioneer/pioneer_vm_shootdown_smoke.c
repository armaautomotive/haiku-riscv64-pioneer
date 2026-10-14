/* Distributed under the MIT License. */
#include <OS.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

extern status_t _kern_set_thread_affinity(thread_id, const void*, size_t);
extern int32 _kern_get_cpu(void);

enum { kWorkers = 126, kRounds = 32, kPage = 4096, kStop = UINT32_MAX };
static const uint32 patterns[] = {0x13579bdf, 0x2468ace0};
struct shared {
	volatile const uint32* page;
	uint32 phase, failed;
	uint64 ack[2];
	bigtime_t deadline;
};
struct worker { struct shared* shared; unsigned index, cpu; thread_id id; };

static int32
worker(void* cookie)
{
	struct worker* w = cookie;
	struct shared* s = w->shared;
	uint64 mask[16] = {0};
	mask[w->cpu / 64] = UINT64_C(1) << (w->cpu % 64);
	if (_kern_set_thread_affinity(0, mask, sizeof(mask)) != B_OK) {
		__atomic_store_n(&s->failed, 1, __ATOMIC_RELEASE); return 1;
	}
	uint32 last = kStop - 1;
	unsigned spins = 0;
	for (;;) {
		uint32 phase = __atomic_load_n(&s->phase, __ATOMIC_ACQUIRE);
		if (phase == kStop) return 0;
		if (phase != last) {
			if (phase & 1) {
				uint32 value = *s->page;
				if (value != patterns[(phase / 2) & 1] || _kern_get_cpu() != (int32)w->cpu) {
					__atomic_store_n(&s->failed, 1, __ATOMIC_RELEASE); return 2;
				}
			}
			last = phase;
			__atomic_fetch_or(&s->ack[w->index / 64], UINT64_C(1) << (w->index % 64), __ATOMIC_RELEASE);
		}
		if (++spins % 4096 == 0 && system_time() >= s->deadline) {
			__atomic_store_n(&s->failed, 1, __ATOMIC_RELEASE); return 3;
		}
	}
}

static int
wait_all(struct shared* s)
{
	while (__atomic_load_n(&s->ack[0], __ATOMIC_ACQUIRE) != UINT64_MAX
		|| __atomic_load_n(&s->ack[1], __ATOMIC_ACQUIRE) != ((UINT64_C(1) << 62) - 1)) {
		if (__atomic_load_n(&s->failed, __ATOMIC_ACQUIRE) || system_time() >= s->deadline) return 1;
	}
	return __atomic_load_n(&s->failed, __ATOMIC_ACQUIRE) != 0;
}

static void
publish(struct shared* s, uint32 phase)
{
	__atomic_store_n(&s->ack[0], 0, __ATOMIC_RELAXED);
	__atomic_store_n(&s->ack[1], 0, __ATOMIC_RELAXED);
	__atomic_store_n(&s->phase, phase, __ATOMIC_RELEASE);
}

int main(int argc, char** argv)
{
	setvbuf(stdout, NULL, _IONBF, 0);
	if (argc != 2 || strcmp(argv[1], "--expect-cpus=64") != 0) {
		fprintf(stderr, "Usage: %s --expect-cpus=64\n", argv[0]); return 2;
	}
	system_info info;
	if (get_system_info(&info) != B_OK || info.cpu_count != 64 || sysconf(_SC_PAGESIZE) != kPage) return 1;
	uint64 mask[16] = {1};
	if (_kern_set_thread_affinity(0, mask, sizeof(mask)) != B_OK || _kern_get_cpu() != 0) return 1;
	char path[] = "/boot/home/pioneer-smp-20261014/vm-page.XXXXXX";
	int fd = mkstemp(path);
	if (fd < 0) { perror("mkstemp"); return 1; }
	/* Only our new 8 KiB scratch file; no existing data. Open fd keeps it alive. */
	if (unlink(path) != 0) { perror("unlink scratch"); close(fd); return 1; }
	uint32 data[2 * kPage / sizeof(uint32)];
	for (unsigned i = 0; i < sizeof(data) / sizeof(data[0]); i++) data[i] = patterns[i / (kPage / sizeof(uint32))];
	if (write(fd, data, sizeof(data)) != sizeof(data)) { perror("write scratch"); close(fd); return 1; }
	void* aliases = mmap(NULL, 2 * kPage, PROT_READ, MAP_SHARED, fd, 0);
	void* target = mmap(NULL, kPage, PROT_NONE, MAP_PRIVATE | MAP_ANON, -1, 0);
	int failed = 1;
	unsigned spawned = 0;
	struct worker workers[kWorkers] = {{0}};
	struct shared s = {.page = target, .deadline = system_time() + 20000000};
	if (aliases == MAP_FAILED || target == MAP_FAILED) goto done;
	/* Keep both different file pages resident through independent aliases, so
	 * alternating MAP_FIXED mappings cannot reuse the same physical page. */
	if (*(volatile const uint32*)aliases != patterns[0]
		|| *(volatile const uint32*)((char*)aliases + kPage) != patterns[1]) goto done;
	if (mmap(target, kPage, PROT_READ, MAP_SHARED | MAP_FIXED, fd, 0) != target) goto done;
	printf("VM shootdown BEGIN: 64 CPUs, two workers per CPU 1..63, %u remap rounds\n", kRounds);
	for (unsigned i = 0; i < kWorkers; i++) {
		workers[i] = (struct worker){&s, i, 1 + i / 2, 0};
		workers[i].id = spawn_thread(worker, "Pioneer VM shootdown", B_NORMAL_PRIORITY, &workers[i]);
		if (workers[i].id < B_OK) goto stop;
		if (resume_thread(workers[i].id) != B_OK) { kill_thread(workers[i].id); goto stop; }
		spawned++;
	}
	if (wait_all(&s)) goto stop;
	/* Odd phases read; even phases park without touching the target page.
	 * Workers remain in the same process, exercising same-map context switches. */
	publish(&s, 1);
	if (wait_all(&s)) goto stop;
	for (unsigned round = 1; round <= kRounds; round++) {
		publish(&s, 2 * round);
		if (wait_all(&s)) goto stop;
		/* MAP_FIXED replaces only the one page reserved by this process. No
		 * worker accesses it until the new read phase is release-published. */
		if (mmap(target, kPage, PROT_READ, MAP_SHARED | MAP_FIXED, fd, (round & 1) * kPage) != target) goto stop;
		publish(&s, 2 * round + 1);
		if (wait_all(&s)) goto stop;
		if (round % 8 == 0) printf("VM shootdown round %u/%u verified by all 126 workers\n", round, kRounds);
	}
	failed = 0;
stop:
	__atomic_store_n(&s.phase, kStop, __ATOMIC_RELEASE);
	for (unsigned i = 0; i < spawned; i++) {
		status_t result = -1;
		if (wait_for_thread_etc(workers[i].id, B_ABSOLUTE_TIMEOUT, s.deadline + 2000000, &result) != B_OK) {
			fprintf(stderr, "FAIL: VM worker join deadline; terminating this test team\n"); _exit(1);
		}
		if (result != 0) failed = 1;
	}
done:
	if (target != MAP_FAILED) munmap(target, kPage);
	if (aliases != MAP_FAILED) munmap(aliases, 2 * kPage);
	close(fd);
	printf("%s: VM shootdown, 64 CPUs, 126 same-team workers, %u distinct-page remaps\n", failed ? "FAIL" : "PASS", kRounds);
	return failed;
}
