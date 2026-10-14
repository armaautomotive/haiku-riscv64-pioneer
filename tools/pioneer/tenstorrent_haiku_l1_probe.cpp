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
	if (argc != 2 || strcmp(argv[1], "--test-stopped-core-l1-and-restore") != 0) {
		fprintf(stderr, "Usage: %s --test-stopped-core-l1-and-restore\n"
			"Private Pioneer only: two fixed patterns in 64 L1 bytes, saved/restored; no core start/DMA/flash.\n", argv[0]);
		return 2;
	}
	int fd = open("/dev/" TT_PCI_PROBE_DEVICE, O_RDWR);
	if (fd < 0) { perror(TT_PCI_PROBE_DEVICE); return 1; }
	tt_pioneer_l1_result r = {};
	int status = ioctl(fd, TT_PCI_PROBE_PIONEER_L1_SCRATCH, &r, sizeof(r));
	int savedError = errno;
	close(fd);
	if (status != 0) { errno = savedError; perror("PIONEER_L1_SCRATCH"); return 1; }
	const tt_l1_scratch_result& l = r.l1;
	const tt_pioneer_mmio_result& p = r.control.control.control.pci;
	printf("L1 state %" PRIu32 " (1 passed), pattern passes %" PRIu32 " mismatch mask %08" PRIx32 "\n",
		l.state, l.passes, l.mismatch_mask);
	printf("Data/TLB/PCI restored %" PRIu32 "/%" PRIu32 "/%" PRIu32
		"; reset before/after %08" PRIx32 "/%08" PRIx32 "\n",
		l.data_restored, l.tlb_restored, p.restored, l.reset_before, l.reset_after);
	for (unsigned s = 0; s < 2; s++) {
		for (unsigned w = 0; w < 3; w++)
			printf("TLB %u word %u before %08" PRIx32 " trial %08" PRIx32 " after %08" PRIx32 "\n",
				200 + s, w, l.tlb_before[s][w], l.tlb_trial[s][w], l.tlb_after[s][w]);
	}
	printf("Card/root commands after %04" PRIx32 "/%04" PRIx32 "; no original contents disclosed, core start, DMA or flash.\n",
		p.commands_after[0], p.commands_after[1]);
	if (!l.data_restored || !l.tlb_restored || p.state == TT_PIONEER_MMIO_RESTORE_FAILED)
		fprintf(stderr, "STOP: cold recovery required; do not run another card test.\n");
	return l.version == 1 && l.state == TT_L1_PASSED && l.data_restored && l.tlb_restored
		&& l.passes == 2 && l.mismatch_mask == 0 && p.state == TT_PIONEER_MMIO_READ_OK && p.restored ? 0 : 1;
}
