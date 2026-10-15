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
	bool inspect = argc == 2 && strcmp(argv[1],"--check-fixed-matmul-support") == 0;
	bool configSmoke = argc == 2 && (strcmp(argv[1],"--check-fixed-tensix-config-support") == 0 || strcmp(argv[1],"--run-fixed-tensix-config-and-reset") == 0);
	inspect |= configSmoke && strcmp(argv[1],"--check-fixed-tensix-config-support") == 0;
	if (!inspect && !configSmoke && (argc != 2 || strcmp(argv[1],"--run-fixed-tensix-matmul-and-reset") != 0)) {
		fprintf(stderr,"Usage: %s --check-fixed-matmul-support | --run-fixed-tensix-matmul-and-reset\n"
			"Private Pioneer: two fixed matrix fixtures on TRISC0, sole idle tile (1,2).\n"
			"REINITIALIZES this tile's compute context; preserves fixed L1 bytes, not prior Src/Dst/register context.\n"
			"Requires 120 idle compute tiles. No device-issued NoC, DMA, ARC or firmware-flash commands.\n",argv[0]);
		fprintf(stderr,"Configuration-only control: --check-fixed-tensix-config-support | --run-fixed-tensix-config-and-reset\n"
			"Same context reset and configuration; bypasses all Dst, data-movement and matrix operations.\n");
		return 2;
	}
	setvbuf(stdout,NULL,_IONBF,0);
	int fd = open("/dev/" TT_PCI_PROBE_DEVICE,O_RDWR);
	if (fd < 0) { perror(TT_PCI_PROBE_DEVICE); return 1; }
	if (inspect) {
		tt_matmul_support s = {};
		int status = ioctl(fd,configSmoke ? TT_PCI_PROBE_PIONEER_TENSIX_CONFIG_SUPPORT : TT_PCI_PROBE_PIONEER_MATMUL_SUPPORT,&s,sizeof(s));
		int error = errno; close(fd);
		if (status != 0) { errno = error; perror("PIONEER_MATMUL_SUPPORT"); return 1; }
		printf("Loaded %s v%" PRIu32 " code/footprint %" PRIu32 "/%" PRIu32
			" entry/code/park/results %08" PRIx32 "/%08" PRIx32 "/%08" PRIx32 "/%08" PRIx32
			" reset/running %08" PRIx32 "/%08" PRIx32 "; metadata only\n",
			configSmoke ? "config-only" : "matmul",s.version,s.code_words,s.footprint_words,s.entry,s.code_base,s.park_pc,s.results_base,s.reset_mask,s.running_mask);
		return s.version == (configSmoke ? 6U : 5U) && s.code_words == 160 && s.footprint_words == 420 && s.entry == 0x6000
			&& s.code_base == 0x70000 && s.park_pc == TT_TENSIX_PARK_PC && s.results_base == 0x17f800
			&& s.reset_mask == 0x87ffff && s.running_mask == 0x46800 ? 0 : 1;
	}
	printf("P100A-MATMUL: one idle tile context will be reinitialized; fixed L1 preserved, no DMA or firmware update\n");
	if (configSmoke) printf("CONFIGURATION-ONLY control: no matrix fixtures or Dst accesses execute; count 2 is only a protocol marker\n");
	tt_pioneer_matmul_result r = {};
	int status = ioctl(fd,configSmoke ? TT_PCI_PROBE_PIONEER_TENSIX_CONFIG : TT_PCI_PROBE_PIONEER_TENSIX_MATMUL,&r,sizeof(r));
	int error = errno; close(fd);
	if (status != 0) { errno = error; perror("PIONEER_TENSIX_MATMUL"); return 1; }
	const tt_tensix_matmul_result& m = r.math;
	const tt_pioneer_mmio_result& p = r.baseline.control.control.control.pci;
	printf("%s state %" PRIu32 " (1 passed), started/completed/stopped %" PRIu32 "/%" PRIu32 "/%" PRIu32
		" context reset %" PRIu32 " polls %" PRIu32 "; routes/cores checked %" PRIu32 "/%" PRIu32 "\n",
		configSmoke ? "Config-only" : "Matmul",m.state,m.started,m.completed,m.stopped,m.context_reset,m.polls,r.baseline.dram.routes_verified,r.baseline.dram.cores_checked);
	printf("Wait-return-PC/%s/marker %08" PRIx32 "/%" PRIu32 "/%08" PRIx32 " mismatches %" PRIu32 "\n",configSmoke ? "protocol-count" : "fixtures",m.phase,m.fixtures,m.marker,m.mismatches);
	printf("Representative output bits %08" PRIx32 "/%08" PRIx32 " (%s); PC %08" PRIx32 "/%08" PRIx32 "\n",
		m.representative[0],m.representative[1],configSmoke ? "expected zero footprint, NOT matrix results" : "expected 41800000/42c00000 = 16.0/96.0",m.pc[0],m.pc[1]);
	printf("Reset before/initialized/running/after %08" PRIx32 "/%08" PRIx32 "/%08" PRIx32 "/%08" PRIx32 "\n",
		m.reset_before,m.reset_initialized,m.reset_running,m.reset_after);
	printf("Entry/FIFO/sync controls %08" PRIx32 "/%08" PRIx32 "/%08" PRIx32 "\n",m.controls[0],m.controls[1],m.controls[2]);
	printf("L1/TLB/PCI restored %" PRIu32 "/%" PRIu32 "/%" PRIu32 "; PCI state %" PRIu32 "; cold recovery %" PRIu32 "\n",
		m.l1_restored,m.tlb_restored,p.restored,p.state,m.recovery_required);
	printf("Card/root commands after %04" PRIx32 "/%04" PRIx32 "; compute context not preserved\n",p.commands_after[0],p.commands_after[1]);
	if (m.recovery_required || p.state == TT_PIONEER_MMIO_RESTORE_FAILED)
		fprintf(stderr,"STOP: cold recovery required; do not retry, reload or run another card operation.\n");
	return m.version == (configSmoke ? 2U : 1U) && m.state == TT_MATMUL_PASSED && m.started && m.completed && m.stopped
		&& m.mismatches == 0 && m.l1_restored && m.tlb_restored && !m.recovery_required
		&& p.version == 1 && p.state == TT_PIONEER_MMIO_READ_OK && p.restored ? 0 : 1;
}
