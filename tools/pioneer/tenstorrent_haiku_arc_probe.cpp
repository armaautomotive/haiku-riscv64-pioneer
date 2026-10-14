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
	if (argc != 2 || strcmp(argv[1], "--read-arc-status-and-restore") != 0) {
		fprintf(stderr, "Usage: %s --read-arc-status-and-restore\n"
			"Private Pioneer only: temporary PCI/TLB configuration, fixed read-only ARC status, full rollback.\n", argv[0]);
		return 2;
	}
	int fd = open("/dev/" TT_PCI_PROBE_DEVICE, O_RDWR);
	if (fd < 0) { perror(TT_PCI_PROBE_DEVICE); return 1; }
	tt_pioneer_arc_result r = {};
	int status = ioctl(fd, TT_PCI_PROBE_PIONEER_ARC_SCRATCH, &r, sizeof(r));
	int savedError = errno;
	close(fd);
	if (status != 0) { errno = savedError; perror("PIONEER_ARC_SCRATCH"); return 1; }
	printf("ARC state %" PRIu32 " (1 ready), TLB restored %" PRIu32
		"; PCI state %" PRIu32 " (1 read OK), restored %" PRIu32 "\n",
		r.arc.state, r.arc.tlb_restored, r.pci.state, r.pci.restored);
	printf("Profile stage %" PRIu32 " status %" PRId32 "\n", r.pci.profile_stage, r.pci.profile_status);
	for (unsigned i = 0; i < 3; i++)
		printf("TLB word %u before %08" PRIx32 " trial %08" PRIx32 " after %08" PRIx32 "\n",
			i, r.arc.tlb_before[i], r.arc.tlb_trial[i], r.arc.tlb_after[i]);
	printf("ARC boot %08" PRIx32 " -> %08" PRIx32 " error0 %08" PRIx32 "\n",
		r.arc.boot_before, r.arc.boot_after, r.arc.error_status0);
	printf("Message queue %08" PRIx32 " telemetry data %08" PRIx32 " tags %08" PRIx32 " present %" PRIu32 "\n",
		r.arc.message_queue, r.arc.telemetry_data, r.arc.telemetry_tags, r.arc.telemetry_present);
	printf("Card/root commands after %04" PRIx32 "/%04" PRIx32 "; NoC ID %08" PRIx32 "\n",
		r.pci.commands_after[0], r.pci.commands_after[1], r.pci.node_id[0]);
	printf("No ARC messages, firmware writes, resets, DMA or general memory access.\n");
	if (r.pci.state == TT_PIONEER_MMIO_RESTORE_FAILED || !r.arc.tlb_restored)
		fprintf(stderr, "STOP: cold recovery required; do not run another card test.\n");
	return r.pci.version == 1 && r.arc.version == 1 && r.pci.state == TT_PIONEER_MMIO_READ_OK
		&& r.pci.restored && r.arc.state == TT_ARC_SCRATCH_READY && r.arc.tlb_restored ? 0 : 1;
}
