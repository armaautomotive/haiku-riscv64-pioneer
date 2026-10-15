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
	bool inspect = argc == 2 && strcmp(argv[1],"--check-fixed-trisc-support") == 0;
	bool backendReset = argc == 2 && (strcmp(argv[1],"--check-reset-trisc-support") == 0 || strcmp(argv[1],"--run-reset-trisc-checksum-and-restore") == 0);
	inspect |= backendReset && strcmp(argv[1],"--check-reset-trisc-support") == 0;
	if (!inspect && !backendReset && (argc != 2 || strcmp(argv[1],"--run-fixed-trisc-checksum-and-restore") != 0)) {
		fprintf(stderr,"Usage: %s --check-fixed-trisc-support | --run-fixed-trisc-checksum-and-restore\n"
			"Private Pioneer: TRISC0 scalar checksum at sole idle tile (1,2).\n"
			"TRISC0 registers/local RAM reset. No Tensix coprocessor, NoC, DMA, ARC or firmware commands.\n",argv[0]);
		fprintf(stderr,"Additional control: --check-reset-trisc-support | --run-reset-trisc-checksum-and-restore\n"
			"The reset control REINITIALIZES the sole tile's backend context, but runs no coprocessor instruction.\n");
		return 2;
	}
	setvbuf(stdout,NULL,_IONBF,0);
	int fd = open("/dev/" TT_PCI_PROBE_DEVICE,O_RDWR);
	if (fd < 0) { perror(TT_PCI_PROBE_DEVICE); return 1; }
	if (inspect) {
		tt_brisc_support s = {};
		int status = ioctl(fd,backendReset ? TT_PCI_PROBE_PIONEER_TRISC_RESET_SUPPORT : TT_PCI_PROBE_PIONEER_TRISC_SUPPORT,&s,sizeof(s));
		int error = errno; close(fd);
		if (status != 0) { errno = error; perror("PIONEER_TRISC_SUPPORT"); return 1; }
		printf("TRISC0 scalar support v%" PRIu32 " code/footprint %" PRIu32 "/%" PRIu32
			" code/results/park %08" PRIx32 "/%08" PRIx32 "/%08" PRIx32 "; metadata only\n",
			s.version,s.code_words,s.footprint_words,s.code_base,s.result_base,s.park_pc);
		return s.version == (backendReset ? 3U : 2U) && s.code_words == 19 && s.footprint_words == 22
			&& s.code_base == 0x70000 && s.result_base == 0x17ffc0 && s.park_pc == 0x70048 ? 0 : 1;
	}
	printf("P100A-TRISC0: scalar control only, no coprocessor/matrix operation\n");
	if (backendReset) printf("Backend reset control: REINITIALIZES the sole idle tile's compute context; no previous Src/Dst preserved\n");
	tt_pioneer_brisc_result r = {};
	int status = ioctl(fd,backendReset ? TT_PCI_PROBE_PIONEER_TRISC_RESET_SMOKE : TT_PCI_PROBE_PIONEER_TRISC_SMOKE,&r,sizeof(r));
	int error = errno; close(fd);
	if (status != 0) { errno = error; perror("PIONEER_TRISC_SMOKE"); return 1; }
	const tt_brisc_smoke_result& b = r.brisc;
	const tt_pioneer_mmio_result& p = r.control.control.control.pci;
	printf("TRISC0 state %" PRIu32 " (1 passed), started/completed/stopped %" PRIu32 "/%" PRIu32 "/%" PRIu32 " polls %" PRIu32 "\n",
		b.state,b.started,b.completed,b.stopped,b.polls);
	printf("Checksum/completion %" PRIu32 "/%08" PRIx32 " expected 528/50494f4e; PC %08" PRIx32 "/%08" PRIx32 "\n",
		b.checksum,b.completion,b.pc[0],b.pc[1]);
	printf("Reset before/running/after %08" PRIx32 "/%08" PRIx32 "/%08" PRIx32 "\n",b.reset_before,b.reset_running,b.reset_after);
	printf("L1/TLB/PCI restored %" PRIu32 "/%" PRIu32 "/%" PRIu32 "; PCI state %" PRIu32 "; cold recovery %" PRIu32 "\n",
		b.l1_restored,b.tlb_restored,p.restored,p.state,b.recovery_required);
	printf("Card/root commands %04" PRIx32 "/%04" PRIx32 "\n",p.commands_after[0],p.commands_after[1]);
	if (b.recovery_required || p.state == TT_PIONEER_MMIO_RESTORE_FAILED)
		fprintf(stderr,"STOP: cold recovery required; do not retry, reload or run another card operation.\n");
	return b.version == (backendReset ? 3U : 2U) && b.state == TT_BRISC_PASSED && b.started && b.completed && b.stopped
		&& b.checksum == 528 && b.completion == 0x50494f4e && b.l1_restored && b.tlb_restored && !b.recovery_required
		&& p.version == 1 && p.state == TT_PIONEER_MMIO_READ_OK && p.restored ? 0 : 1;
}
