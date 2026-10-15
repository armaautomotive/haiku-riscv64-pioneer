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
	if (!support && (argc!=2 || strcmp(argv[1],"--read-host-path-and-restore"))) {
		fprintf(stderr,"Usage: %s --check-support | --read-host-path-and-restore\nRead-only host-transfer prerequisites; does NOT enable DMA/bus mastering or prove host-memory transfers.\n",argv[0]); return 2;
	}
	int fd=open("/dev/" TT_PCI_PROBE_DEVICE,O_RDWR); if (fd<0) { perror("open"); return 1; }
	if (support) {
		uint32_t version=0; int status=ioctl(fd,TT_PCI_PROBE_PIONEER_HOST_PATH_SUPPORT,&version,sizeof(version));
		close(fd); if (status) { perror("host path support"); return 1; }
		printf("Read-only host-path support v%u; no card transaction submitted\n",version); return version==1?0:1;
	}
	tt_pioneer_host_path_result result={};
	int status=ioctl(fd,TT_PCI_PROBE_PIONEER_HOST_PATH,&result,sizeof(result)); close(fd);
	if (status) { perror("host path audit"); return 1; }
	const tt_host_path_result& r=result.path;
	const tt_pioneer_mmio_result& p=result.baseline.baseline.control.control.control.pci;
	printf("Host path v%u state %u (1 passed) connected PCIe %08x TLB/PCI restored %u/%u cold %u\n",
		r.version,r.state,r.connected_pcie,r.tlb_restored,p.restored,r.recovery_required);
	for (unsigned s=0;s<3;s++) {
		printf("Sample %u inbound %08x/%08x SII windows",s,r.inbound[s][0],r.inbound[s][1]);
		for (unsigned w=0;w<8;w++) printf(" %08x",r.sii[s][w]);
		puts("");
	}
	printf("Commands after %04x/%04x; no SII/inbound/initiator/payload writes, no core start/DMA/flash\n",p.commands_after[0],p.commands_after[1]);
	if (r.recovery_required) fprintf(stderr,"STOP: cold recovery required; do not retry or reload a driver.\n");
	return r.version==1 && r.state==TT_HOST_PATH_PASSED && r.tlb_restored && !r.recovery_required
		&& p.state==TT_PIONEER_MMIO_READ_OK && p.restored && p.commands_after[0]==0 && p.commands_after[1]==0?0:1;
}
