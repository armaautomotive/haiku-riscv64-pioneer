/* SPDX-License-Identifier: MIT */
#include "../../src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.h"
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
int main(int argc,char** argv)
{
	bool page=argc==2 && (!strcmp(argv[1],"--check-page-support") || !strcmp(argv[1],"--read-owned-page-4k-once"));
	bool support=argc==2 && (!strcmp(argv[1],"--check-support") || !strcmp(argv[1],"--check-page-support"));
	if (!support && (argc!=2 || (strcmp(argv[1],"--read-owned-page-once") && strcmp(argv[1],"--read-owned-page-4k-once")))) {
		fprintf(stderr,"Usage: %s --check-support | --check-page-support | --read-owned-page-once | --read-owned-page-4k-once\nSingle fixed 64-byte or 4KiB read from an owned high host page to stopped tile L1; temporarily enables bus mastering. No host writes. UNCERTAIN COMPLETION REQUIRES COLD RECOVERY, NOT RETRY/RELOAD.\n",argv[0]); return 2;
	}
	int fd=open("/dev/" TT_PCI_PROBE_DEVICE,O_RDWR); if (fd<0) { perror("open"); return 1; }
	if (support) {
		uint32_t version=0; int status=ioctl(fd,page?TT_PCI_PROBE_PIONEER_HOST_READ_PAGE_SUPPORT:TT_PCI_PROBE_PIONEER_HOST_READ_SUPPORT,&version,sizeof(version));
		close(fd); if (status) { perror("owned host-read support"); return 1; }
		printf("Owned fixed %u-byte host-read support v%u; no allocation/card transaction submitted\n",page?4096:64,version); return version==1?0:1;
	}
	tt_pioneer_host_read_result result={};
	int status=ioctl(fd,page?TT_PCI_PROBE_PIONEER_HOST_READ_PAGE:TT_PCI_PROBE_PIONEER_HOST_READ,&result,sizeof(result)); close(fd);
	if (status) { perror("owned host-read"); return 1; }
	const auto& r=result.read;
	const auto& audit=result.baseline.audit;
	const auto& path=result.baseline.baseline.path;
	const auto& base=result.baseline.baseline.baseline;
	const auto& pci=base.baseline.control.control.control.pci;
	printf("Owned %u-byte host read v%u state %u (1 passed) phase %u; route/DMA-audit/compute/DRAM states %u/%u/%u/%u\n",
		page?4096:64,r.version,r.state,r.phase,path.state,audit.state,base.noc.state,base.baseline.dram.state);
	printf("Issued/completed %u/%u polls %u; destination/host-guard mismatches %u/%u; host bytes/address bits %u/%u\n",
		r.issued,r.completed,r.polls,r.mismatches,r.host_mismatches,r.page.bytes,r.page.address_bits);
	printf("NoC response/flits/accepted/sent deltas %u/%u/%u/%u\n",uint32_t(r.after[0]-r.before[0]),uint32_t(r.after[1]-r.before[1]),uint32_t(r.after[2]-r.before[2]),uint32_t(r.after[3]-r.before[3]));
	printf("BME enabled mask %u; BME-off/L1/NIU/TLB/PCI restored %u/%u/%u/%u/%u; commands after %04x/%04x\n",
		r.bme_enabled,r.bme_restored,r.l1_restored,r.niu_restored,r.tlb_restored,pci.restored,pci.commands_after[0],pci.commands_after[1]);
	printf("Page released/quarantined %u/%u; cold recovery required %u; no host writes or core starts\n",r.page.released,r.quarantined,r.recovery_required);
	return r.version==1 && r.state==TT_HOST_READ_PASSED && r.issued && r.completed && !r.mismatches && !r.host_mismatches
		&& r.bme_enabled==3 && r.bme_restored && r.l1_restored && r.niu_restored && r.tlb_restored
		&& pci.restored && !pci.commands_after[0] && !pci.commands_after[1] && r.page.released
		&& !r.quarantined && !r.recovery_required?0:1;
}
