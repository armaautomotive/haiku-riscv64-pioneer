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
	if (!support && (argc!=2 || strcmp(argv[1],"--prepare-check-and-release"))) {
		fprintf(stderr,"Usage: %s --check-support | --prepare-check-and-release\nDedicated wired 4KiB host page above 4GiB, non-cacheable configuration and CPU verification; NO PCI/MMIO/DMA.\n",argv[0]); return 2;
	}
	int fd=open("/dev/" TT_PCI_PROBE_DEVICE,O_RDWR); if (fd<0) { perror("open"); return 1; }
	if (support) {
		uint32_t version=0; int status=ioctl(fd,TT_PCI_PROBE_PIONEER_HOST_PAGE_SUPPORT,&version,sizeof(version));
		close(fd); if (status) { perror("host page support"); return 1; }
		printf("Owned host-page support v%u; no allocation/card transaction submitted\n",version); return version==1?0:1;
	}
	tt_host_page_result r={};
	int status=ioctl(fd,TT_PCI_PROBE_PIONEER_HOST_PAGE,&r,sizeof(r)); close(fd);
	if (status) { perror("host page"); return 1; }
	printf("Host page v%u state %u (1 ready) allocated/memory-type-set/map-stable/released %u/%u/%u/%u\n",
		r.version,r.state,r.allocated,r.memory_type_set,r.map_stable,r.released);
	printf("Bytes %u physical address bits %u (>32 required), patterns %u mismatches %u\n",r.bytes,r.address_bits,r.patterns,r.mismatches);
	printf("Allocation/map/cache/release status %d/%d/%d/%d; NO device memory access, DMA or bus mastering\n",
		r.allocation_status,r.map_status,r.cache_status,r.release_status);
	return r.version==1 && r.state==TT_HOST_PAGE_READY && r.allocated && r.memory_type_set
		&& r.map_stable && r.released && r.patterns==2 && !r.mismatches && PioneerHostPageAddress(r.physical,r.bytes)?0:1;
}
