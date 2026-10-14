/* Distributed under the MIT License. */
#include "../../src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.h"
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

int main(int argc, char** argv)
{
	if (argc != 2 || strcmp(argv[1], "--read-noc-id-and-restore") != 0) {
		fprintf(stderr, "Usage: %s --read-noc-id-and-restore\n"
			"Private Pioneer only: temporary PCI mapping, three fixed read-only register accesses, full rollback.\n",
			argv[0]);
		return 2;
	}
	int fd = open("/dev/" TT_PCI_PROBE_DEVICE, O_RDWR);
	if (fd < 0) { perror(TT_PCI_PROBE_DEVICE); return 1; }
	tt_pioneer_mmio_result r = {};
	int status = ioctl(fd, TT_PCI_PROBE_PIONEER_NOC_ID, &r, sizeof(r));
	int savedError = errno;
	close(fd);
	if (status != 0) { errno = savedError; perror("PIONEER_NOC_ID"); return 1; }
	printf("NoC ID probe v%" PRIu32 " state %" PRIu32 " stage %" PRIu32 " restored %" PRIu32 "\n",
		r.version, r.state, r.stage, r.restored);
	printf("States: 0 refused, 1 read OK, 2 bad read, 3 map failure, 4 config failure, 5 RESTORE FAILURE\n");
	printf("Profile stage %" PRIu32 " status %" PRId32 "\n", r.profile_stage, r.profile_status);
	printf("Card/root commands %04" PRIx32 "/%04" PRIx32 " -> %04" PRIx32 "/%04" PRIx32 "\n",
		r.commands_before[0], r.commands_before[1], r.commands_after[0], r.commands_after[1]);
	for (unsigned i = 0; i < 6; i++)
		printf("BAR%u before %08" PRIx32 " trial %08" PRIx32 " after %08" PRIx32 "\n",
			i, r.bars_before[i], r.bars_trial[i], r.bars_after[i]);
	for (unsigned i = 0; i < 3; i++)
		printf("Bridge prefetch word %u before %08" PRIx32 " after %08" PRIx32 "\n",
			i, r.bridge_before[i], r.bridge_after[i]);
	printf("NoC ID reads %08" PRIx32 " %08" PRIx32 " %08" PRIx32 "\n",
		r.node_id[0], r.node_id[1], r.node_id[2]);
	printf("DMA, firmware writes, resets and general compute are not part of this test.\n");
	if (r.state == TT_PIONEER_MMIO_RESTORE_FAILED)
		fprintf(stderr, "STOP: cold recovery required; do not run another card test.\n");
	return r.version == 1 && r.state == TT_PIONEER_MMIO_READ_OK && r.restored ? 0 : 1;
}
