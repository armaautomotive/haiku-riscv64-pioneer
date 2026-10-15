/* SPDX-License-Identifier: MIT */
#include "../../src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.h"
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

int main(int argc, char** argv)
{
	bool inspect = argc == 2 && strcmp(argv[1], "--check-fixed-dram-support") == 0;
	if (!inspect && (argc != 2 || strcmp(argv[1], "--read-fixed-dram-route-and-restore") != 0)) {
		fprintf(stderr, "Usage: %s --check-fixed-dram-support | --read-fixed-dram-route-and-restore\n"
			"Private Pioneer: fixed enabled bank-0 NIU identification only; no GDDR payload writes, core start, DMA or flash.\n", argv[0]);
		return 2;
	}
	setvbuf(stdout, NULL, _IONBF, 0);
	int fd = open("/dev/" TT_PCI_PROBE_DEVICE, O_RDWR);
	if (fd < 0) { perror(TT_PCI_PROBE_DEVICE); return 1; }
	if (inspect) {
		tt_dram_route_support support = {};
		int status = ioctl(fd, TT_PCI_PROBE_PIONEER_DRAM_SUPPORT, &support, sizeof(support));
		int error = errno; close(fd);
		if (status != 0) { errno = error; perror("PIONEER_DRAM_SUPPORT"); return 1; }
		printf("Loaded DRAM route v%" PRIu32 " bank %" PRIu32 " source/ID words %" PRIu32 "/%" PRIu32
			" translated (%" PRIu32 ",%" PRIu32 "); no PCI/MMIO access\n",
			support.version, support.bank, support.source_words, support.id_words, support.translated_x, support.translated_y);
		return support.version == 2 && support.bank == 0 && support.source_words == TT_DRAM_SOURCE_WORDS
			&& support.id_words == TT_DRAM_ID_WORDS && support.translated_x == 18 && support.translated_y == 12 ? 0 : 1;
	}
	printf("P100A-DRAM: fixed bank-0 read-only route transaction\n");
	tt_pioneer_dram_route_result r = {};
	int status = ioctl(fd, TT_PCI_PROBE_PIONEER_DRAM_ROUTE, &r, sizeof(r));
	int error = errno; close(fd);
	if (status != 0) { errno = error; perror("PIONEER_DRAM_ROUTE"); return 1; }
	const tt_dram_route_result& d = r.dram;
	const tt_pioneer_mmio_result& p = r.control.control.control.pci;
	printf("DRAM route state %" PRIu32 " (1 passed); TLB/PCI restored %" PRIu32 "/%" PRIu32 "; PCI state %" PRIu32 "; cold recovery %" PRIu32 "\n",
		d.state, d.tlb_restored, p.restored, p.state, d.recovery_required);
	for (unsigned i = 0; i < TT_DRAM_SOURCE_WORDS; i++)
		printf("Source NIU +%03" PRIx32 " before/after %08" PRIx32 "/%08" PRIx32 "\n",
			BlackholeDramSourceOffset(i), d.source[i], d.source_after[i]);
	for (unsigned sample = 0; sample < 3; sample++)
		printf("Bank 0 sample %u: NODE/ENDPOINT/NIU/LOGICAL %08" PRIx32 "/%08" PRIx32 "/%08" PRIx32 "/%08" PRIx32 "\n",
			sample, d.id[sample][0], d.id[sample][1], d.id[sample][2], d.id[sample][3]);
	printf("Card/root commands after %04" PRIx32 "/%04" PRIx32 "; no payload, core, DMA or firmware writes\n",
		p.commands_after[0], p.commands_after[1]);
	if (d.recovery_required || p.state == TT_PIONEER_MMIO_RESTORE_FAILED)
		fprintf(stderr, "STOP: cold recovery required; do not retry, reload or run another card operation.\n");
	return d.version == 1 && d.state == TT_DRAM_ROUTE_PASSED && d.tlb_restored && !d.recovery_required
		&& p.version == 1 && p.state == TT_PIONEER_MMIO_READ_OK && p.restored ? 0 : 1;
}
