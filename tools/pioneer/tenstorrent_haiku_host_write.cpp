/* SPDX-License-Identifier: MIT */
#include "../../src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.h"
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
int main(int argc,char** argv)
{
	bool page=argc==2 && (!strcmp(argv[1],"--check-page-support") || !strcmp(argv[1],"--write-owned-page-4k-and-readback-once"));
	bool support=argc==2 && (!strcmp(argv[1],"--check-support") || !strcmp(argv[1],"--check-page-support"));
	if (!support && (argc!=2 || (strcmp(argv[1],"--write-owned-page-and-readback-once") && strcmp(argv[1],"--write-owned-page-4k-and-readback-once")))) {
		fprintf(stderr,"Usage: %s --check-support | --check-page-support | --write-owned-page-and-readback-once | --write-owned-page-4k-and-readback-once\nSingle fixed 64-byte or 4KiB card-to-owned-host write, explicit host visibility/guards and distinct L1 readback; temporary bus mastering. UNCERTAIN COMPLETION REQUIRES COLD RECOVERY, NOT RETRY/RELOAD.\n",argv[0]); return 2;
	}
	int fd=open("/dev/" TT_PCI_PROBE_DEVICE,O_RDWR); if (fd<0) { perror("open"); return 1; }
	if (support) {
		uint32_t version=0; int status=ioctl(fd,page?TT_PCI_PROBE_PIONEER_HOST_WRITE_PAGE_SUPPORT:TT_PCI_PROBE_PIONEER_HOST_WRITE_SUPPORT,&version,sizeof(version));
		close(fd); if (status) { perror("owned host-write support"); return 1; }
		printf("Owned fixed %u-byte host-write/readback support v%u; no allocation/card transaction submitted\n",page?4096:64,version); return version==1?0:1;
	}
	tt_pioneer_host_write_result result={};
	int status=ioctl(fd,page?TT_PCI_PROBE_PIONEER_HOST_WRITE_PAGE:TT_PCI_PROBE_PIONEER_HOST_WRITE,&result,sizeof(result)); close(fd);
	if (status) { perror("owned host write/readback"); return 1; }
	const auto& w=result.write; const auto& r=w.transfer;
	const auto& audit=result.baseline.audit; const auto& path=result.baseline.baseline.path;
	const auto& base=result.baseline.baseline.baseline; const auto& pci=base.baseline.control.control.control.pci;
	printf("Owned %u-byte host write/readback v%u state %u (1 passed) phase %u; route/DMA-audit/compute/DRAM states %u/%u/%u/%u\n",
		page?4096:64,r.version,r.state,r.phase,path.state,audit.state,base.noc.state,base.baseline.dram.state);
	printf("Write issued/ACK-completed/host-visible %u/%u/%u polls %u/%u; readback issued/completed %u/%u polls %u\n",
		r.issued,w.write_completed,w.host_visible,r.polls,w.host_polls,w.read_issued,w.read_completed,w.read_polls);
	printf("Write ACK/flits/accepted/sent deltas %u/%u/%u/%u; read response/flits/accepted/sent %u/%u/%u/%u\n",
		uint32_t(r.after[0]-r.before[0]),uint32_t(r.after[1]-r.before[1]),uint32_t(r.after[2]-r.before[2]),uint32_t(r.after[3]-r.before[3]),
		uint32_t(w.read_after[0]-w.read_before[0]),uint32_t(w.read_after[1]-w.read_before[1]),uint32_t(w.read_after[2]-w.read_before[2]),uint32_t(w.read_after[3]-w.read_before[3]));
	printf("Payload+source / host-guard mismatches %u/%u; owned host bytes/address bits %u/%u\n",r.mismatches,r.host_mismatches,r.page.bytes,r.page.address_bits);
	printf("BME mask %u; BME-off/L1/NIU/TLB/PCI restored %u/%u/%u/%u/%u; commands after %04x/%04x\n",
		r.bme_enabled,r.bme_restored,r.l1_restored,r.niu_restored,r.tlb_restored,pci.restored,pci.commands_after[0],pci.commands_after[1]);
	printf("Page released/quarantined %u/%u; cold recovery required %u; all cores held\n",r.page.released,r.quarantined,r.recovery_required);
	return r.version==1 && r.state==TT_HOST_READ_PASSED && r.issued && r.completed && w.write_completed && w.host_visible
		&& w.read_issued && w.read_completed && !r.mismatches && !r.host_mismatches && r.bme_enabled==3
		&& r.bme_restored && r.l1_restored && r.niu_restored && r.tlb_restored && pci.restored
		&& !pci.commands_after[0] && !pci.commands_after[1] && r.page.released && !r.quarantined && !r.recovery_required?0:1;
}
