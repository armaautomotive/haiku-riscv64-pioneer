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
	if (!support && (argc!=2 || strcmp(argv[1],"--read-compute-noc-and-restore"))) {
		fprintf(stderr,"Usage: %s --check-support | --read-compute-noc-and-restore\nRead-only source/command-state prerequisite, sole tile (1,2); not Qwen inference.\n",argv[0]); return 2;
	}
	int fd=open("/dev/" TT_PCI_PROBE_DEVICE,O_RDWR); if (fd<0) { perror("open"); return 1; }
	if (support) {
		uint32_t version=0;
		int status=ioctl(fd,TT_PCI_PROBE_PIONEER_COMPUTE_NOC_SUPPORT,&version,sizeof(version));
		close(fd); if (status) { perror("compute NoC support"); return 1; }
		printf("Compute NoC read-only support v%u; no card transaction submitted\n",version); return version==1?0:1;
	}
	tt_pioneer_compute_noc_result result={};
	int status=ioctl(fd,TT_PCI_PROBE_PIONEER_COMPUTE_NOC,&result,sizeof(result));
	close(fd); if (status) { perror("compute NoC probe"); return 1; }
	const tt_compute_noc_result& n=result.noc;
	const tt_pioneer_mmio_result& p=result.baseline.control.control.control.pci;
	printf("Compute NoC state %u (1 passed); routes %u cores %u; TLB/PCI restored %u/%u; cold %u\n",
		n.state,result.baseline.dram.routes_verified,result.baseline.dram.cores_checked,n.tlb_restored,p.restored,n.recovery_required);
	for (unsigned sample=0;sample<3;sample++) {
		printf("Sample %u node/endpoint %08x/%08x NIU %08x FIFO %s commands %x/%x/%x/%x outstanding",
			sample,n.node[sample][0],n.node[sample][1],n.source[sample][0],
			(n.source[sample][0]&0x10000)?"enabled":"disabled",
			n.commands[sample][0],n.commands[sample][1],n.commands[sample][2],n.commands[sample][3]);
		for (unsigned i=0;i<16;i++)
			printf(" %x",n.outstanding[sample][i]);
		puts("");
	}
	for (unsigned w=0;w<TT_DRAM_SOURCE_WORDS;w++) printf("Source[%u] %08x\n",w,n.source[0][w]);
	printf("Commands after %04x/%04x; no NIU configuration, request, counter clear, core start, payload or DMA writes\n",p.commands_after[0],p.commands_after[1]);
	return n.version==1 && n.state==TT_COMPUTE_NOC_PASSED && n.tlb_restored && !n.recovery_required
		&& p.state==TT_PIONEER_MMIO_READ_OK && p.restored && p.commands_after[0]==0 && p.commands_after[1]==0?0:1;
}
