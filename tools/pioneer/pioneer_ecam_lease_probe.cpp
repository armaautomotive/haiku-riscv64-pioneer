/* SPDX-License-Identifier: MIT */
#include "PioneerEcamLeaseProbe.h"
#include <Errors.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
int main(int argc, char** argv) {
	const bool expectAbsent = argc == 2 && strcmp(argv[1], "--expect-absent") == 0;
	if (argc != 1 && !expectAbsent) { fprintf(stderr, "usage: ecam_lease_probe_client [--expect-absent]\n"); return 2; }
	int fd = open("/dev/misc/pioneer_ecam_lease_probe", O_RDONLY);
	if (fd < 0) { perror("open ECAM lease probe"); return 1; }
	pioneer_ecam_lease_probe_result r = {};
	int status = ioctl(fd, PIONEER_ECAM_LEASE_PROBE, &r, sizeof(r));
	int closed = close(fd);
	if (status != 0 || closed != 0) { perror("ECAM lease probe ioctl/close"); return 1; }
	printf("ECAM_LEASE_PROBE version=%lu manager=%ld pci=%ld cards=%lu profile=%ld stage=%lu ready=%lu lease_module=%ld lease=%ld callback=%ld calls=%lu recovery_module=%ld recovery=%ld recovery_calls=%lu\n",
		(unsigned long)r.version, (long)r.managerStatus, (long)r.pciStatus, (unsigned long)r.cards,
		(long)r.profileStatus, (unsigned long)r.profileStage, (unsigned long)r.profileReady,
		(long)r.leaseModuleStatus, (long)r.leaseStatus, (long)r.leaseCallbackStatus,
		(unsigned long)r.leaseCallbacks, (long)r.recoveryModuleStatus, (long)r.recoveryStatus,
		(unsigned long)r.recoveryCallbacks);
	if (expectAbsent)
		return r.version == 1 && r.managerStatus == B_OK && r.pciStatus == B_OK && r.cards == 1
			&& r.profileStatus == B_ENTRY_NOT_FOUND && r.profileStage == 6 && r.profileReady == 1
			&& r.leaseModuleStatus == B_ENTRY_NOT_FOUND && r.leaseStatus == B_NO_INIT
			&& r.leaseCallbackStatus == B_NO_INIT && r.leaseCallbacks == 0
			&& r.recoveryModuleStatus == B_NO_INIT && r.recoveryStatus == B_NO_INIT
			&& r.recoveryCallbacks == 0 ? 0 : 1;
	return r.version == 1 && r.managerStatus == B_OK && r.pciStatus == B_OK && r.cards == 1
		&& r.profileStatus == B_OK && r.profileStage == 6 && r.profileReady == 1
		&& r.leaseModuleStatus == B_OK && r.leaseStatus == B_OK && r.leaseCallbackStatus == B_CANCELED
		&& r.leaseCallbacks == 1 && r.recoveryModuleStatus == B_OK
		&& r.recoveryStatus == B_NOT_ALLOWED && r.recoveryCallbacks == 0 ? 0 : 1;
}
