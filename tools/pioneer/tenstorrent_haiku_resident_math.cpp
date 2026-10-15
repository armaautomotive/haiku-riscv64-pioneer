/* SPDX-License-Identifier: MIT */
#include "../../src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.h"
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
int main(int argc,char** argv)
{
	bool support=argc==2 && !strcmp(argv[1],"--check-support");
	if (!support && (argc!=2 || strcmp(argv[1],"--run-fixed-resident-matrix-and-restore"))) {
		fprintf(stderr,"Usage: %s --check-support | --run-fixed-resident-matrix-and-restore\nFixed dense BF16 proof, two on-card weight fetches; not Qwen inference or a speed benchmark.\n",argv[0]); return 2;
	}
	int fd=open("/dev/" TT_PCI_PROBE_DEVICE,O_RDWR); if (fd<0) { perror("open"); return 1; }
	if (support) {
		uint32_t version=0; int status=ioctl(fd,TT_PCI_PROBE_PIONEER_RESIDENT_MATH_SUPPORT,&version,sizeof(version));
		close(fd); if (status) { perror("resident matrix support"); return 1; }
		printf("Fixed resident matrix support v%u; no card transaction submitted\n",version); return version==1?0:1;
	}
	tt_pioneer_resident_math_result result={};
	int status=ioctl(fd,TT_PCI_PROBE_PIONEER_RESIDENT_MATH,&result,sizeof(result));
	close(fd); if (status) { perror("resident matrix"); return 1; }
	const tt_resident_math_result& c=result.compute;
	const tt_tensix_matmul_result& m=c.math;
	const tt_pioneer_mmio_result& p=result.baseline.baseline.control.control.control.pci;
	printf("Resident matrix v%u stage %u state %u (1 passed), math ABI %u started/completed/stopped %u/%u/%u mismatch %u cold %u\n",
		c.version,c.stage,m.state,m.version,m.started,m.completed,m.stopped,m.mismatches,c.recovery_required);
	printf("Phase/fixtures/marker %08x/%u/%08x PCs %08x/%08x polls %u reset %x/%x/%x/%x\n",
		m.phase,m.fixtures,m.marker,m.pc[0],m.pc[1],m.polls,m.reset_before,m.reset_initialized,m.reset_running,m.reset_after);
	printf("Restored GDDR/NIU/L1/TLB/PCI %u/%u/%u/%u/%u; compute route state %u; DRAM state %u\n",
		c.gddr_restored,c.niu_restored,m.l1_restored,m.tlb_restored,p.restored,result.baseline.noc.state,result.baseline.baseline.dram.state);
	printf("Device read counters response/flits/accepted/read-sent delta %u/%u/%u/%u (expected 2/34/2/2); host doorbell writes forbidden\n",
		uint32_t(c.after[0]-c.before[0]),uint32_t(c.after[1]-c.before[1]),uint32_t(c.after[2]-c.before[2]),uint32_t(c.after[3]-c.before[3]));
	printf("Commands after %04x/%04x; no host DMA/firmware changes\n",p.commands_after[0],p.commands_after[1]);
	if (c.recovery_required) fprintf(stderr,"STOP: cold recovery required; do not retry or reload a driver.\n");
	return c.version==1 && m.version==27 && m.state==TT_MATMUL_PASSED && m.started && m.completed && m.stopped
		&& !c.recovery_required && c.gddr_restored && c.niu_restored && m.l1_restored && m.tlb_restored
		&& p.state==TT_PIONEER_MMIO_READ_OK && p.restored && p.commands_after[0]==0 && p.commands_after[1]==0?0:1;
}
