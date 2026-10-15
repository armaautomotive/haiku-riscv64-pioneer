/* SPDX-License-Identifier: MIT */
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
	bool inspect = argc == 2 && strcmp(argv[1], "--check-fixed-brisc-support") == 0;
	if (!inspect && (argc != 2 || strcmp(argv[1], "--run-fixed-brisc-checksum-and-restore") != 0)) {
		fprintf(stderr, "Usage: %s --check-fixed-brisc-support | --run-fixed-brisc-checksum-and-restore\n"
			"Private Pioneer only: fixed BRISC (1,2), bounded checksum/start/stop, L1/PCI/TLB rollback.\n"
			"BRISC registers/local RAM may reset; no other RISC starts, DMA, firmware messages or flash.\n", argv[0]);
		return 2;
	}
	setvbuf(stdout, NULL, _IONBF, 0);
	printf("P100A-BRISC: opening guarded fixed-checksum operation\n");
	int fd = open("/dev/" TT_PCI_PROBE_DEVICE, O_RDWR);
	if (fd < 0) { perror(TT_PCI_PROBE_DEVICE); return 1; }
	if (inspect) {
		tt_brisc_support support = {};
		int status = ioctl(fd, TT_PCI_PROBE_PIONEER_BRISC_SUPPORT, &support, sizeof(support));
		int savedError = errno; close(fd);
		if (status != 0) { errno = savedError; perror("PIONEER_BRISC_SUPPORT"); return 1; }
		printf("Loaded BRISC support v%" PRIu32 " code/footprint words %" PRIu32 "/%" PRIu32
			" code/result/park %08" PRIx32 "/%08" PRIx32 "/%08" PRIx32 "; no PCI/MMIO access or execution\n",
			support.version, support.code_words, support.footprint_words,
			support.code_base, support.result_base, support.park_pc);
		return support.version == 1 && support.code_words == 19 && support.footprint_words == 22
			&& support.code_base == 0x70000 && support.result_base == 0x17ffc0 && support.park_pc == 0x70048 ? 0 : 1;
	}
	tt_pioneer_brisc_result r = {};
	int status = ioctl(fd, TT_PCI_PROBE_PIONEER_BRISC_SMOKE, &r, sizeof(r));
	int savedError = errno; close(fd);
	if (status != 0) { errno = savedError; perror("PIONEER_BRISC_SMOKE"); return 1; }
	const tt_brisc_smoke_result& b = r.brisc;
	const tt_pioneer_mmio_result& p = r.control.control.control.pci;
	printf("BRISC state %" PRIu32 " (1 passed), started/completed/stopped %" PRIu32 "/%" PRIu32 "/%" PRIu32 " polls %" PRIu32 "\n",
		b.state, b.started, b.completed, b.stopped, b.polls);
	printf("Checksum %" PRIu32 " (expected 528), completion %08" PRIx32 " (expected 50494f4e), PC %08" PRIx32 "/%08" PRIx32 "\n",
		b.checksum, b.completion, b.pc[0], b.pc[1]);
	printf("Reset before/running/after %08" PRIx32 "/%08" PRIx32 "/%08" PRIx32 "\n",
		b.reset_before, b.reset_running, b.reset_after);
	printf("L1/TLB/PCI restored %" PRIu32 "/%" PRIu32 "/%" PRIu32 "; PCI state %" PRIu32 "; cold recovery %" PRIu32 "\n",
		b.l1_restored, b.tlb_restored, p.restored, p.state, b.recovery_required);
	printf("Card/root commands after %04" PRIx32 "/%04" PRIx32 "; no original memory disclosed, DMA or flash\n",
		p.commands_after[0], p.commands_after[1]);
	if (b.recovery_required || p.state == TT_PIONEER_MMIO_RESTORE_FAILED)
		fprintf(stderr, "STOP: cold recovery required. Do not retry, reload a driver, or run another card operation.\n");
	return b.version == 1 && b.state == TT_BRISC_PASSED && b.started && b.completed && b.stopped
		&& b.checksum == 528 && b.completion == 0x50494f4e && b.l1_restored && b.tlb_restored
		&& !b.recovery_required && p.version == 1 && p.state == TT_PIONEER_MMIO_READ_OK && p.restored ? 0 : 1;
}
