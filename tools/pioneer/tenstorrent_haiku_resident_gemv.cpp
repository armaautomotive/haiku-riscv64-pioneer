/* SPDX-License-Identifier: MIT */
#include "../../src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.h"
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>
int main(int argc,char** argv)
{
	bool support=argc==2 && !strcmp(argv[1],"--check-support");
	unsigned inputs=0;
	if (argc==3 && !strcmp(argv[1],"--run-fixed-resident-gemv-and-restore"))
		inputs=!strcmp(argv[2],"1024")?1024:!strcmp(argv[2],"5120")?5120:!strcmp(argv[2],"6144")?6144:0;
	if (!support && !inputs) {
		fprintf(stderr,"Usage: %s --check-support | --run-fixed-resident-gemv-and-restore 1024|5120|6144\nFixed integer/BF16 fixture, 16 outputs, full on-card accumulation; NOT Qwen inference or a CPU comparison.\n",argv[0]); return 2;
	}
	int fd=open("/dev/" TT_PCI_PROBE_DEVICE,O_RDWR); if (fd<0) { perror("open"); return 1; }
	if (support) {
		uint32_t version=0; int status=ioctl(fd,TT_PCI_PROBE_PIONEER_RESIDENT_GEMV_SUPPORT,&version,sizeof(version));
		close(fd); if (status) { perror("resident GEMV support"); return 1; }
		printf("Fixed resident GEMV support v%u; no card transaction submitted\n",version); return version==1?0:1;
	}
	tt_pioneer_resident_gemv_result reply={}; reply.version=1; reply.inputs=inputs;
	struct timespec start,end; clock_gettime(CLOCK_MONOTONIC,&start);
	int status=ioctl(fd,TT_PCI_PROBE_PIONEER_RESIDENT_GEMV,&reply,sizeof(reply));
	clock_gettime(CLOCK_MONOTONIC,&end); close(fd);
	if (status) { perror("resident GEMV"); return 1; }
	const tt_resident_math_result& c=reply.result.compute;
	const tt_tensix_matmul_result& m=c.math;
	const tt_pioneer_mmio_result& p=reply.result.baseline.baseline.control.control.control.pci;
	printf("Resident GEMV 16x%u state %u (1 passed) ABI %u stage %u started/completed/stopped %u/%u/%u mismatch %u cold %u\n",
		inputs,m.state,m.version,c.stage,m.started,m.completed,m.stopped,m.mismatches,c.recovery_required);
	printf("Phase/tiles/marker %x/%u/%x PCs %x/%x polls %u; restored GDDR/NIU/L1/TLB/PCI %u/%u/%u/%u/%u\n",
		m.phase,m.fixtures,m.marker,m.pc[0],m.pc[1],m.polls,c.gddr_restored,c.niu_restored,m.l1_restored,m.tlb_restored,p.restored);
	printf("Device response/flits/accepted/read-sent delta %u/%u/%u/%u; expected %u/%u/%u/%u\n",
		uint32_t(c.after[0]-c.before[0]),uint32_t(c.after[1]-c.before[1]),uint32_t(c.after[2]-c.before[2]),uint32_t(c.after[3]-c.before[3]),
		inputs/16,(inputs/16)*17,inputs/16,inputs/16);
	double seconds=end.tv_sec-start.tv_sec+(end.tv_nsec-start.tv_nsec)*1e-9;
	printf("Whole reversible test %.6fs, INCLUDING upload/readback/rollback; not kernel throughput. Commands %04x/%04x\n",seconds,p.commands_after[0],p.commands_after[1]);
	if (c.recovery_required) fprintf(stderr,"STOP: cold recovery required; do not retry or reload a driver.\n");
	return reply.version==1 && reply.inputs==inputs && c.version==1 && m.version==28 && m.state==TT_MATMUL_PASSED
		&& m.completed && m.stopped && !c.recovery_required && c.gddr_restored && c.niu_restored && m.l1_restored && m.tlb_restored
		&& p.state==TT_PIONEER_MMIO_READ_OK && p.restored && p.commands_after[0]==0 && p.commands_after[1]==0?0:1;
}
