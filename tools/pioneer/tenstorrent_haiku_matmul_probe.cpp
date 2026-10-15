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
	struct Mode {
		const char *check, *run, *name, *description;
		uint32_t support, operation, metadataVersion, resultVersion;
		bool protocolOnly;
	};
	const Mode modes[] = {
		{"--check-fixed-matmul-support","--run-fixed-tensix-matmul-and-reset","Matmul","Two numerical matrix fixtures",
		 TT_PCI_PROBE_PIONEER_MATMUL_SUPPORT,TT_PCI_PROBE_PIONEER_TENSIX_MATMUL,5,1,false},
		{"--check-fixed-tensix-config-support","--run-fixed-tensix-config-and-reset","Config-only","Configuration only; no data-path operations",
		 TT_PCI_PROBE_PIONEER_TENSIX_CONFIG_SUPPORT,TT_PCI_PROBE_PIONEER_TENSIX_CONFIG,6,2,true},
		{"--check-fixed-sfpu-load-support","--run-fixed-sfpu-load-and-reset","SFPU-load-only","SFPLOADI/drain only; no Dst/Src/matrix",
		 TT_PCI_PROBE_PIONEER_SFPU_LOAD_SUPPORT,TT_PCI_PROBE_PIONEER_SFPU_LOAD,8,3,true},
		{"--check-fixed-sfpu-store-support","--run-fixed-sfpu-store-and-reset","SFPU-store-only","SFPSTORE/drain; no ZEROACC/MOVD2/matrix/RISC Dst reads",
		 TT_PCI_PROBE_PIONEER_SFPU_STORE_SUPPORT,TT_PCI_PROBE_PIONEER_SFPU_STORE,9,4,true},
		{"--check-fixed-operand-support","--run-fixed-operand-and-reset","Operand-only","ZEROACC/MOVD2/ownership/drain; no matrix/RISC Dst reads",
		 TT_PCI_PROBE_PIONEER_OPERAND_SUPPORT,TT_PCI_PROBE_PIONEER_OPERAND_SMOKE,10,5,true},
		{"--check-fixed-matrix-exec-support","--run-fixed-matrix-exec-and-reset","Matrix-exec-only","One MVMUL/drain; no RISC Dst reads, no numerical validation",
		 TT_PCI_PROBE_PIONEER_MATRIX_EXEC_SUPPORT,TT_PCI_PROBE_PIONEER_MATRIX_EXEC_SMOKE,11,6,true},
		{"--check-fixed-zeroacc-support","--run-fixed-zeroacc-and-reset","ZEROACC-only","SFPU prefix and ZEROACC/drain; no operand moves/handoff/matrix",
		 TT_PCI_PROBE_PIONEER_ZEROACC_SUPPORT,TT_PCI_PROBE_PIONEER_ZEROACC_SMOKE,12,7,true},
		{"--check-fixed-movd-support","--run-fixed-movd-and-reset","MOVD-only","SFPU/ZEROACC/MOVD2/drain; no handoff/matrix/RISC Dst reads",
		 TT_PCI_PROBE_PIONEER_MOVD_SUPPORT,TT_PCI_PROBE_PIONEER_MOVD_SMOKE,13,8,true},
		{"--check-kernel-operand-support","--run-kernel-operand-and-reset","Kernel-operand-only","Kernel-style clear/valid BEFORE MOVD2; no matrix/RISC Dst reads",
		 TT_PCI_PROBE_PIONEER_KERNEL_OPERAND_SUPPORT,TT_PCI_PROBE_PIONEER_KERNEL_OPERAND,14,10,true},
		{"--check-kernel-matrix-exec-support","--run-kernel-matrix-exec-and-reset","Kernel-matrix-exec-only","Kernel-style handoff and one MVMUL/drain; no numerical readback",
		 TT_PCI_PROBE_PIONEER_KERNEL_MATRIX_EXEC_SUPPORT,TT_PCI_PROBE_PIONEER_KERNEL_MATRIX_EXEC,15,11,true},
		{"--check-kernel-matmul-support","--run-kernel-matmul-and-reset","Kernel-matmul","Kernel-style handoff and two numerical fixtures with actual Dst readback",
		 TT_PCI_PROBE_PIONEER_KERNEL_MATMUL_SUPPORT,TT_PCI_PROBE_PIONEER_KERNEL_MATMUL,16,9,false},
		{"--check-fixed-dst-read-support","--run-fixed-dst-read-and-reset","Dst-read-only","SFPU fill and numerical RISC Dst readback; no ownership/matrix",
		 TT_PCI_PROBE_PIONEER_DST_READ_SUPPORT,TT_PCI_PROBE_PIONEER_DST_READ,17,12,false}
	};
	const Mode* selected = NULL; bool inspect = false;
	if (argc == 2) for (const Mode& mode : modes) {
		if (strcmp(argv[1],mode.check) == 0) { selected = &mode; inspect = true; }
		if (strcmp(argv[1],mode.run) == 0) selected = &mode;
	}
	if (selected == NULL) {
		fprintf(stderr,"Usage: %s MODE\nPrivate Pioneer idle tile (1,2); reinitializes compute context, preserves fixed L1 only.\n"
			"Requires 120 held tiles; no device NoC, DMA, ARC or firmware-flash commands.\n",argv[0]);
		for (const Mode& mode : modes) fprintf(stderr,"  %s | %s\n    %s\n",mode.check,mode.run,mode.description);
		return 2;
	}
	const Mode& mode = *selected;
	setvbuf(stdout,NULL,_IONBF,0);
	int fd = open("/dev/" TT_PCI_PROBE_DEVICE,O_RDWR);
	if (fd < 0) { perror(TT_PCI_PROBE_DEVICE); return 1; }
	if (inspect) {
		tt_matmul_support s = {};
		int status = ioctl(fd,mode.support,&s,sizeof(s));
		int error = errno; close(fd);
		if (status != 0) { errno = error; perror("PIONEER_MATMUL_SUPPORT"); return 1; }
		printf("Loaded %s v%" PRIu32 " code/footprint %" PRIu32 "/%" PRIu32
			" entry/code/park/results %08" PRIx32 "/%08" PRIx32 "/%08" PRIx32 "/%08" PRIx32
			" reset/running %08" PRIx32 "/%08" PRIx32 "; metadata only\n",
			mode.name,s.version,s.code_words,s.footprint_words,s.entry,s.code_base,s.park_pc,s.results_base,s.reset_mask,s.running_mask);
		return s.version == mode.metadataVersion && s.code_words == 160 && s.footprint_words == 420 && s.entry == 0x6000
			&& s.code_base == 0x70000 && s.park_pc == TT_TENSIX_PARK_PC && s.results_base == 0x17f800
			&& s.reset_mask == 0x87ffff && s.running_mask == 0x46800 ? 0 : 1;
	}
	printf("P100A-MATMUL: one idle tile context will be reinitialized; fixed L1 preserved, no DMA or firmware update\n");
	printf("%s: %s\n",mode.name,mode.description);
	if (mode.protocolOnly) printf("Count 2 is only a protocol marker, NOT two numerical matrix results\n");
	tt_pioneer_matmul_result r = {};
	int status = ioctl(fd,mode.operation,&r,sizeof(r));
	int error = errno; close(fd);
	if (status != 0) { errno = error; perror("PIONEER_TENSIX_MATMUL"); return 1; }
	const tt_tensix_matmul_result& m = r.math;
	const tt_pioneer_mmio_result& p = r.baseline.control.control.control.pci;
	printf("%s state %" PRIu32 " (1 passed), started/completed/stopped %" PRIu32 "/%" PRIu32 "/%" PRIu32
		" context reset %" PRIu32 " polls %" PRIu32 "; routes/cores checked %" PRIu32 "/%" PRIu32 "\n",
		mode.name,m.state,m.started,m.completed,m.stopped,m.context_reset,m.polls,r.baseline.dram.routes_verified,r.baseline.dram.cores_checked);
	printf("Wait-return-PC/%s/marker %08" PRIx32 "/%" PRIu32 "/%08" PRIx32 " mismatches %" PRIu32 "\n",mode.protocolOnly ? "protocol-count" : "fixtures",m.phase,m.fixtures,m.marker,m.mismatches);
	printf("Representative output bits %08" PRIx32 "/%08" PRIx32 " (%s); PC %08" PRIx32 "/%08" PRIx32 "\n",
		m.representative[0],m.representative[1],mode.protocolOnly ? "expected zero footprint, NOT matrix results" : mode.resultVersion == 12 ? "expected 3f800000/40000000 = 1.0/2.0, NOT matrix results" : "expected 41800000/42c00000 = 16.0/96.0",m.pc[0],m.pc[1]);
	printf("Reset before/initialized/running/after %08" PRIx32 "/%08" PRIx32 "/%08" PRIx32 "/%08" PRIx32 "\n",
		m.reset_before,m.reset_initialized,m.reset_running,m.reset_after);
	printf("Entry/FIFO/sync controls %08" PRIx32 "/%08" PRIx32 "/%08" PRIx32 "\n",m.controls[0],m.controls[1],m.controls[2]);
	printf("L1/TLB/PCI restored %" PRIu32 "/%" PRIu32 "/%" PRIu32 "; PCI state %" PRIu32 "; cold recovery %" PRIu32 "\n",
		m.l1_restored,m.tlb_restored,p.restored,p.state,m.recovery_required);
	printf("Card/root commands after %04" PRIx32 "/%04" PRIx32 "; compute context not preserved\n",p.commands_after[0],p.commands_after[1]);
	if (m.recovery_required || p.state == TT_PIONEER_MMIO_RESTORE_FAILED)
		fprintf(stderr,"STOP: cold recovery required; do not retry, reload or run another card operation.\n");
	return m.version == mode.resultVersion && m.state == TT_MATMUL_PASSED && m.started && m.completed && m.stopped
		&& m.mismatches == 0 && m.l1_restored && m.tlb_restored && !m.recovery_required
		&& p.version == 1 && p.state == TT_PIONEER_MMIO_READ_OK && p.restored ? 0 : 1;
}
