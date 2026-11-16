/* SPDX-License-Identifier: MIT */
// Driver identity/capability probe. Only metadata ioctls; no model/device work.
#include <Drivers.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>

static const uint32_t kReloadStatus = B_DEVICE_OP_CODES_END + 0x61e0;
static const uint32_t kFullOffloadSupport = B_DEVICE_OP_CODES_END + 0x8800;
struct Reload {
	uint32_t version, reserved[3];
	uint64_t expected_tag, expected_generation;
	uint32_t driver, quiesced, cold, active_owner, open_count, guard_version;
	uint64_t build_tag, generation;
};
struct Support {
	uint32_t driver, version, prompt, output, positions, vocabulary, compute, reserved;
};
static_assert(sizeof(Reload) == 72, "reload metadata ABI");
static_assert(sizeof(Support) == 32, "full-offload capability ABI");

static double Seconds()
{
	timespec now = {};
	if (clock_gettime(CLOCK_MONOTONIC, &now) != 0)
		return -1;
	return double(now.tv_sec) + double(now.tv_nsec) / 1e9;
}

int main()
{
	// This driver's open hook requires O_RDWR even for these read-only ioctls.
	const int fd = open("/dev/misc/tenstorrent_pci_probe", O_RDWR | O_CLOEXEC);
	if (fd < 0) {
		fprintf(stderr, "OPEN_FAIL errno=%d %s\n", errno, strerror(errno));
		return 2;
	}
	Reload reload = {};
	reload.version = 1;
	double start = Seconds();
	errno = 0;
	const int reloadStatus = ioctl(fd, kReloadStatus, &reload, sizeof(reload));
	const int reloadErrno = errno;
	printf("RELOAD status=%d errno=%d elapsed=%.6f driver=%u tag=%016llx generation=%llu guard=%u quiesced=%u cold=%u owner=%u opens=%u\n",
		reloadStatus, reloadErrno, Seconds() - start, reload.driver,
		(unsigned long long)reload.build_tag, (unsigned long long)reload.generation,
		reload.guard_version, reload.quiesced, reload.cold, reload.active_owner,
		reload.open_count);
	Support support = {};
	start = Seconds();
	errno = 0;
	const int supportStatus = ioctl(fd, kFullOffloadSupport, &support, sizeof(support));
	const int supportErrno = errno;
	printf("SUPPORT status=%d errno=%d elapsed=%.6f driver=%u version=%u prompt=%u output=%u positions=%u vocabulary=%u compute=%u reserved=%u\n",
		supportStatus, supportErrno, Seconds() - start, support.driver,
		support.version, support.prompt, support.output, support.positions,
		support.vocabulary, support.compute, support.reserved);
	const int closeStatus = close(fd);
	printf("CLOSE status=%d\n", closeStatus);
	return reloadStatus == 0 && closeStatus == 0 ? 0 : 1;
}
