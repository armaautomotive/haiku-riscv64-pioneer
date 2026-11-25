/* SPDX-License-Identifier: MIT */
#include "PioneerDeviceManagerPinProbe.h"
#include <Errors.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

int main(int argc, char** argv)
{
	const bool expectAbsent = argc == 2 && strcmp(argv[1], "--expect-absent") == 0;
	if (argc != 1 && !expectAbsent) {
		fprintf(stderr, "usage: pin_probe_client [--expect-absent]\n");
		return 2;
	}
	int fd = open("/dev/misc/pioneer_dm_pin_probe", O_RDONLY);
	if (fd < 0) { perror("open pin probe"); return 1; }
	pioneer_dm_pin_probe_result r = {};
	int status = ioctl(fd, PIONEER_DM_PIN_PROBE, &r, sizeof(r));
	int closeStatus = close(fd);
	if (status != 0 || closeStatus != 0) { perror("pin probe ioctl/close"); return 1; }
	printf("PIN_PROBE version=%lu pin=%ld manager=%ld invalid=%ld root=%ld callback=%ld count=%lu\n",
		(unsigned long)r.version, (long)r.pinModuleStatus, (long)r.managerModuleStatus,
		(long)r.invalidStatus, (long)r.rootStatus, (long)r.callbackStatus, (unsigned long)r.callbacks);
	if (expectAbsent)
		return r.version == 1 && r.pinModuleStatus == B_ENTRY_NOT_FOUND
			&& r.managerModuleStatus == B_NO_INIT && r.invalidStatus == B_NO_INIT
			&& r.rootStatus == B_NO_INIT && r.callbackStatus == B_NO_INIT && r.callbacks == 0 ? 0 : 1;
	return r.version == 1 && r.pinModuleStatus == B_OK && r.managerModuleStatus == B_OK
		&& r.invalidStatus == B_BAD_VALUE && r.rootStatus == B_OK
		&& r.callbackStatus == B_CANCELED && r.callbacks == 1 ? 0 : 1;
}
