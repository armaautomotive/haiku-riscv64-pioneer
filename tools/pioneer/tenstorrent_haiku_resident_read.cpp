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
	if (!support && (argc!=2 || strcmp(argv[1],"--read-fixed-gddr-to-l1-and-restore"))) {
		fprintf(stderr,"Usage: %s --check-support | --read-fixed-gddr-to-l1-and-restore\nSingle 64-byte on-card read with reversible L1 poison; no core start or host DMA. Not Qwen inference.\n",argv[0]);
		return 2;
	}
	int fd=open("/dev/" TT_PCI_PROBE_DEVICE,O_RDWR);
	if (fd<0) { perror("open"); return 1; }
	if (support) {
		uint32_t version=0;
		int status=ioctl(fd,TT_PCI_PROBE_PIONEER_RESIDENT_READ_SUPPORT,&version,sizeof(version));
		close(fd); if (status) { perror("resident read support"); return 1; }
		printf("Fixed device-read support v%u; no card transaction submitted\n",version); return version==1?0:1;
	}
	tt_pioneer_resident_read_result result={};
	int status=ioctl(fd,TT_PCI_PROBE_PIONEER_RESIDENT_READ,&result,sizeof(result));
	close(fd); if (status) { perror("resident read"); return 1; }
	const tt_resident_read_result& r=result.read;
	const tt_compute_noc_result& n=result.baseline.noc;
	const tt_pioneer_mmio_result& p=result.baseline.baseline.control.control.control.pci;
	printf("Device read v%u state %u (1 passed) phase %u issued/completed %u/%u polls %u mismatch %u cold %u\n",
		r.version,r.state,r.phase,r.issued,r.completed,r.polls,r.mismatches,r.recovery_required);
	printf("Restored L1/NIU/TLB/PCI %u/%u/%u/%u; compute route state %u; DRAM state %u; IRQ config %08x\n",
		r.l1_restored,r.niu_restored,r.tlb_restored,p.restored,n.state,result.baseline.baseline.dram.state,r.irq_config);
	printf("Counters response/flits/accepted/read-sent delta %u/%u/%u/%u\n",
		uint32_t(r.after[0]-r.before[0]),uint32_t(r.after[1]-r.before[1]),uint32_t(r.after[2]-r.before[2]),uint32_t(r.after[3]-r.before[3]));
	printf("Initial initiator words:");
	for (unsigned w=0;w<10;w++) printf(" %08x",r.command_before[w]);
	printf("\nCommands after %04x/%04x; fixed GDDR read only, cores remain held, no host DMA/counter clears/flash\n",p.commands_after[0],p.commands_after[1]);
	if (r.recovery_required) fprintf(stderr,"STOP: cold recovery required; do not retry or reload a driver.\n");
	return r.version==1 && r.state==TT_RESIDENT_READ_PASSED && r.completed && r.l1_restored && r.niu_restored
		&& r.tlb_restored && !r.recovery_required && p.state==TT_PIONEER_MMIO_READ_OK && p.restored
		&& p.commands_after[0]==0 && p.commands_after[1]==0?0:1;
}
