#include <OS.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

extern "C" status_t _kern_set_thread_affinity(thread_id, const void*, size_t);

// Read-only diagnostic: pin this process's thread, never disable CPUs or
// alter disk contents. Exiting discards the diagnostic thread's affinity.
// WARNING: a Pioneer run coincided with scheduler.cpp:503's Core() assertion.
// Do not rerun until that scheduler issue is understood; see tools/pioneer/README.md.
int
main(int argc, char** argv)
{
	if (argc != 2) {
		fprintf(stderr, "usage: %s raw-disk-device\n", argv[0]);
		return 1;
	}
	int fd = open(argv[1], O_RDONLY);
	if (fd < 0) {
		perror("open");
		return 1;
	}
	const int cpus[] = {0, 1, 2, 16, 32, 63, 0};
	for (unsigned i = 0; i < sizeof(cpus) / sizeof(cpus[0]); i++) {
		// The current kernel uses a 64-bit CPUSet; the syscall accepts a
		// larger buffer and copies only its native CPUSet size.
		uint32 mask[32] = {};
		mask[cpus[i] / 32] = (uint32)1 << (cpus[i] % 32);
		status_t status = _kern_set_thread_affinity(0, mask, sizeof(mask));
		if (status != B_OK) {
			fprintf(stderr, "pin CPU %d: %s\n", cpus[i], strerror(status));
			close(fd);
			return 1;
		}
		uint8 data[512];
		ssize_t count = pread(fd, data, sizeof(data), 0);
		if (count != sizeof(data)) {
			fprintf(stderr, "CPU %d: read returned %ld\n", cpus[i], (long)count);
			close(fd);
			return 1;
		}
		uint32 sum = 0;
		for (unsigned j = 0; j < sizeof(data); j += 4) {
			sum += (uint32)data[j] | ((uint32)data[j + 1] << 8)
				| ((uint32)data[j + 2] << 16) | ((uint32)data[j + 3] << 24);
		}
		printf("affinity CPU %d sector-zero checksum %#" B_PRIx32 "\n",
			cpus[i], sum);
		fflush(stdout);
	}
	close(fd);
	return 0;
}
