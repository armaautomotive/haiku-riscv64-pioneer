/* Distributed under the MIT License. */
#include "../../src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.h"
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

int
main(int argc, char** argv)
{
	if (argc != 2 || strcmp(argv[1], "--test-address-and-restore") != 0) {
		fprintf(stderr, "Usage: %s --test-address-and-restore\n"
			"Root-only PCI configuration test; never enables memory or DMA.\n",
			argv[0]);
		return 2;
	}
	int fd = open("/dev/" TT_PCI_PROBE_DEVICE, O_RDWR);
	if (fd < 0) {
		perror(TT_PCI_PROBE_DEVICE);
		return 1;
	}
	tt_bar4_probe_result result = {};
	int status = ioctl(fd, TT_PCI_PROBE_BAR4_ADDRESS, &result, sizeof(result));
	close(fd);
	if (status != 0) {
		perror("TT_PCI_PROBE_BAR4_ADDRESS");
		return 1;
	}
	if (result.version != 1)
		return 1;
	printf("BAR4 address probe state %" PRIu32 " (1 = restored, 0 = refused, 2 = FAILED, 3 = interrupted)\n",
		result.state);
	printf("Command %04" PRIx32 " -> %04" PRIx32 "\n",
		result.command_before, result.command_after);
	for (unsigned i = 0; i < 6; i++)
		printf("BAR%u before %08" PRIx32 " after %08" PRIx32 "\n",
			i, result.before[i], result.after[i]);
	printf("BAR sizes: 0x%" PRIx64 " 0x%" PRIx64 " 0x%" PRIx64 "\n",
		result.size[0], result.size[1], result.size[2]);
	printf("Trial BAR4 raw: %08" PRIx32 ":%08" PRIx32 "\n",
		result.trial[1], result.trial[0]);
	printf("Address retention is NOT allocation, CPU mapping, or a working accelerator.\n");
	return result.state == TT_BAR4_RESTORED ? 0 : 1;
}
