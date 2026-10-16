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
		fprintf(stderr,"Usage: %s --check-support | --prepare-check-and-release\nCPU-only private4KiB input/16KiB output allocation, all4 physical pages, no alias, noncacheable patterns and release; NO PCI/MMIO/DMA/core execution.\n",argv[0]); return 2;
	}
	int fd=open("/dev/" TT_PCI_PROBE_DEVICE,O_RDWR); if (fd<0) { perror("open"); return 1; }
	if (support) {
		uint32_t version=0; int status=ioctl(fd,TT_PCI_PROBE_PIONEER_OUTPUT_BUFFER_SUPPORT,&version,sizeof(version));
		close(fd); if (status) { perror("output buffer support"); return 1; }
		printf("CPU-only owned-output-buffer probe support v%u; no allocation/card transaction submitted\n",version); return version==1?0:1;
	}
	tt_output_buffer_probe_result r={};
	int status=ioctl(fd,TT_PCI_PROBE_PIONEER_OUTPUT_BUFFER_PROBE,&r,sizeof(r)); close(fd);
	if (status) { perror("output buffer probe"); return 1; }
	printf("CPU-only output-buffer probe v%u state %u (1 passed); input/output states %u/%u allocated %u/%u\n",
		r.version,r.state,r.input_state,r.output_state,r.input_allocated,r.output_allocated);
	printf("Bytes input/output %u/%u; physical address bits %u/%u; pattern passes %u/%u; mismatches %u/%u\n",
		r.input_bytes,r.output_bytes,r.input_address_bits,r.output_address_bits,r.input_patterns,r.output_patterns,r.input_mismatches,r.output_mismatches);
	printf("Whole/all4-page maps stable %u, input/output released %u/%u, output quarantined %u; NO addresses exposed, PCI/MMIO/DMA or core execution\n",
		r.maps_stable,r.input_released,r.output_released,r.output_quarantined);
	return r.version==1 && r.state==TT_OUTPUT_PROBE_PASSED && r.input_allocated && r.output_allocated
		&& r.input_state==TT_HOST_PAGE_READY && r.output_state==TT_OUTPUT_BUFFER_READY
		&& r.input_bytes==4096 && r.output_bytes==16384 && r.input_address_bits>32 && r.output_address_bits>32
		&& r.input_address_bits<=48 && r.output_address_bits<=48
		&& r.input_patterns==2 && r.output_patterns==4 && !r.input_mismatches && !r.output_mismatches
		&& r.maps_stable && r.input_released && r.output_released && !r.output_quarantined?0:1;
}
