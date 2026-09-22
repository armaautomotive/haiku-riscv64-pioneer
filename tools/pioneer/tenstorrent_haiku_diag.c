/*
 * Copyright 2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Build on Haiku: cc -o tenstorrent_haiku_diag tenstorrent_haiku_diag.c
 */

#include "../../src/add-ons/kernel/drivers/misc/tenstorrent_pci_diag.h"

#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>


int
main(void)
{
	const char* path = "/dev/" TT_PCI_DIAG_DEVICE;
	int fd = open(path, O_RDONLY);
	if (fd < 0) {
		perror(path);
		return 1;
	}

	tt_pci_diag_snapshot snapshot = {0};
	if (ioctl(fd, TT_PCI_DIAG_GET_SNAPSHOT, &snapshot, sizeof(snapshot)) != 0) {
		perror("TT_PCI_DIAG_GET_SNAPSHOT");
		close(fd);
		return 1;
	}
	close(fd);

	if (snapshot.version != TT_PCI_DIAG_VERSION) {
		fprintf(stderr, "Unsupported snapshot version: %" PRIu32 "\n",
			snapshot.version);
		return 1;
	}

	printf("PCI %02x:%02x.%u  %04x:%04x  subsystem %04x:%04x  revision %02x\n",
		snapshot.bus, snapshot.device, snapshot.function,
		snapshot.vendor_id, snapshot.device_id,
		snapshot.subsystem_vendor_id, snapshot.subsystem_id,
		snapshot.revision);
	printf("Command %04x (memory decode %s, bus mastering %s), status %04x\n",
		snapshot.command,
		(snapshot.command & 0x2) != 0 ? "on" : "off",
		(snapshot.command & 0x4) != 0 ? "on" : "off",
		snapshot.status);
	for (int i = 0; i < 6; i++)
		printf("BAR%d config: %08" PRIx32 "\n", i, snapshot.bar[i]);
	printf("Raw BAR values do not establish allocation, size, or CPU mapping.\n");
	return 0;
}
