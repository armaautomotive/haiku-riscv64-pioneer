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
	if (!support && (argc!=2 || strcmp(argv[1],"--audit-only"))) {
		fprintf(stderr,"Usage: %s --check-support | --audit-only\nRead-only PCIe pending/error and firmware HDMA ch0 audit; NO host allocation, DMA or bus mastering.\n",argv[0]); return 2;
	}
	int fd=open("/dev/" TT_PCI_PROBE_DEVICE,O_RDWR); if (fd<0) { perror("open"); return 1; }
	if (support) {
		uint32_t version=0; int status=ioctl(fd,TT_PCI_PROBE_PIONEER_HOST_DMA_AUDIT_SUPPORT,&version,sizeof(version));
		close(fd); if (status) { perror("host DMA audit support"); return 1; }
		printf("Read-only host DMA audit support v%u; no card transaction submitted\n",version); return version==1?0:1;
	}
	tt_pioneer_host_dma_audit_result r={};
	int status=ioctl(fd,TT_PCI_PROBE_PIONEER_HOST_DMA_AUDIT,&r,sizeof(r)); close(fd);
	if (status) { perror("host DMA audit"); return 1; }
	const auto& a=r.audit; const auto& h=r.baseline.path;
	const auto& n=r.baseline.baseline; const auto& b=n.baseline;
	const auto& pci=b.control.control.control.pci;
	printf("Host DMA audit v%u state %u (1 passed); route/compute/DRAM states %u/%u/%u\n",a.version,a.state,h.state,n.noc.state,b.dram.state);
	for (unsigned sample=0;sample<3;sample++) printf("Sample %u: PCIe Device Status card/root %04x/%04x; HDMA ch0 WR enable/status %08x/%08x RD %08x/%08x\n",
		sample,a.pcie_status[sample][0],a.pcie_status[sample][1],a.hdma[sample][0],a.hdma[sample][1],a.hdma[sample][2],a.hdma[sample][3]);
	printf("TLB/PCI restored %u/%u; commands after %04x/%04x; cold recovery %u; NO DMA/bus mastering\n",
		a.tlb_restored,pci.restored,pci.commands_after[0],pci.commands_after[1],a.recovery_required);
	return a.version==1 && a.state==TT_HOST_DMA_AUDIT_PASSED && a.tlb_restored && !a.recovery_required
		&& h.state==TT_HOST_PATH_PASSED && pci.restored && !pci.commands_after[0] && !pci.commands_after[1]?0:1;
}
