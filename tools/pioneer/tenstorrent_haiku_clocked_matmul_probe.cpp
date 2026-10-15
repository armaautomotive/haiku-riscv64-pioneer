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
	bool inspect = argc == 2 && strcmp(argv[1],"--check-fixed-clocked-matmul-support") == 0;
	if (!inspect && (argc != 2 || strcmp(argv[1],"--run-fixed-clocked-matmul-and-reset") != 0)) {
		fprintf(stderr,"Usage: %s --check-fixed-clocked-matmul-support | --run-fixed-clocked-matmul-and-reset\n"
			"Private Pioneer: sole idle tile (1,2), normal firmware busy/idle DVFS, fixed matrix fixtures.\n"
			"Reinitializes this tile context; preserves fixed L1. No forced voltage/frequency, DMA or flash.\n",argv[0]);
		return 2;
	}
	setvbuf(stdout,NULL,_IONBF,0);
	int fd = open("/dev/" TT_PCI_PROBE_DEVICE,O_RDWR);
	if (fd < 0) { perror(TT_PCI_PROBE_DEVICE); return 1; }
	if (inspect) {
		tt_matmul_support s = {};
		int status = ioctl(fd,TT_PCI_PROBE_PIONEER_CLOCKED_MATMUL_SUPPORT,&s,sizeof(s));
		int error = errno; close(fd);
		if (status != 0) { errno = error; perror("CLOCKED_MATMUL_SUPPORT"); return 1; }
		printf("Clocked matmul v%" PRIu32 " code/footprint %" PRIu32 "/%" PRIu32
			" entry/code/park/results %08" PRIx32 "/%08" PRIx32 "/%08" PRIx32 "/%08" PRIx32
			" reset/running %08" PRIx32 "/%08" PRIx32 "; metadata only\n",
			s.version,s.code_words,s.footprint_words,s.entry,s.code_base,s.park_pc,s.results_base,s.reset_mask,s.running_mask);
		return s.version == 7 && s.code_words == 160 && s.footprint_words == 420 && s.entry == 0x6000
			&& s.code_base == 0x70000 && s.park_pc == TT_TENSIX_PARK_PC && s.results_base == 0x17f800
			&& s.reset_mask == 0x87ffff && s.running_mask == 0x46800 ? 0 : 1;
	}
	printf("P100A-CLOCKED-MATMUL: normal firmware busy/idle clock guard; one tile context reset; no DMA or flash\n");
	tt_pioneer_clocked_matmul_result r = {};
	int status = ioctl(fd,TT_PCI_PROBE_PIONEER_CLOCKED_MATMUL,&r,sizeof(r));
	int error = errno; close(fd);
	if (status != 0) { errno = error; perror("CLOCKED_MATMUL"); return 1; }
	const tt_tensix_matmul_result& m = r.matmul.math;
	const tt_pioneer_mmio_result& p = r.matmul.baseline.control.control.control.pci;
	const tt_arc_clock_guard_result& c = r.clock;
	const tt_arc_telemetry_result& health = r.matmul.baseline.control.control.telemetry;
	const tt_tensix_reset_result& held = r.matmul.baseline.control.tensix;
	printf("Baseline ARC/telemetry/reset/DRAM states %" PRIu32 "/%" PRIu32 "/%" PRIu32 "/%" PRIu32
		" reset samples %08" PRIx32 "/%08" PRIx32 "/%08" PRIx32 " NIU/source %08" PRIx32 "/%08" PRIx32 "\n",
		r.matmul.baseline.control.control.control.arc.state,health.state,held.state,r.matmul.baseline.dram.state,
		held.reset[0],held.reset[1],held.reset[2],held.niu_config,r.matmul.baseline.dram.source[0]);
	printf("Health mask %08" PRIx32 " FW/GDDR/training/errors/columns %08" PRIx32 "/%08" PRIx32 "/%08" PRIx32 "/%08" PRIx32 "/%08" PRIx32 "\n",
		health.present_mask,health.values[0],health.values[6],health.values[7],health.values[8],health.values[9]);
	printf("Matmul state %" PRIu32 " (1 passed), started/completed/stopped %" PRIu32 "/%" PRIu32 "/%" PRIu32
		" context reset %" PRIu32 " polls %" PRIu32 "; routes/cores %" PRIu32 "/%" PRIu32 "\n",
		m.state,m.started,m.completed,m.stopped,m.context_reset,m.polls,r.matmul.baseline.dram.routes_verified,r.matmul.baseline.dram.cores_checked);
	printf("Wait-return-PC/fixtures/marker %08" PRIx32 "/%" PRIu32 "/%08" PRIx32 " mismatches %" PRIu32 "\n",
		m.phase,m.fixtures,m.marker,m.mismatches);
	printf("Output bits %08" PRIx32 "/%08" PRIx32 " (expected 41800000/42c00000); PC %08" PRIx32 "/%08" PRIx32 "\n",
		m.representative[0],m.representative[1],m.pc[0],m.pc[1]);
	printf("Reset before/initialized/running/after %08" PRIx32 "/%08" PRIx32 "/%08" PRIx32 "/%08" PRIx32 "\n",
		m.reset_before,m.reset_initialized,m.reset_running,m.reset_after);
	printf("Clock state %" PRIu32 " MHz %" PRIu32 "/%" PRIu32 "/%" PRIu32 " modes %" PRIu32 "/%" PRIu32 "/%" PRIu32
		" busy-request/ack/idle-ack %" PRIu32 "/%" PRIu32 "/%" PRIu32 " messages %" PRIu32 " last %02" PRIx32 "\n",
		c.state,c.before_mhz,c.busy_mhz,c.after_mhz,c.before_mode,c.busy_mode,c.after_mode,
		c.busy_requested,c.busy_acknowledged,c.idle_acknowledged,c.messages,c.last_message);
	printf("Clock/L1/math-TLB/queue-request/queue-TLB/PCI restored %" PRIu32 "/%" PRIu32 "/%" PRIu32 "/%" PRIu32 "/%" PRIu32 "/%" PRIu32
		" PCI state %" PRIu32 " cold recovery %" PRIu32 "/%" PRIu32 " commands %04" PRIx32 "/%04" PRIx32 "\n",
		c.restored,m.l1_restored,m.tlb_restored,c.last.request_restored,c.last.tlb_restored,p.restored,p.state,
		m.recovery_required,c.recovery_required,p.commands_after[0],p.commands_after[1]);
	if (m.recovery_required || c.recovery_required || p.state == TT_PIONEER_MMIO_RESTORE_FAILED)
		fprintf(stderr,"STOP: cold recovery required; do not retry, reload or run another card operation.\n");
	return m.version == 1 && m.state == TT_MATMUL_PASSED && m.started && m.completed && m.stopped
		&& m.mismatches == 0 && m.l1_restored && m.tlb_restored && !m.recovery_required
		&& c.version == 1 && c.state == TT_ARC_CLOCK_RESTORED && c.restored && !c.recovery_required
		&& p.version == 1 && p.state == TT_PIONEER_MMIO_READ_OK && p.restored ? 0 : 1;
}
