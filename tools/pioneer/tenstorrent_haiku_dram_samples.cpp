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
	bool inspect = argc == 2 && strcmp(argv[1], "--check-fixed-dram-samples-support") == 0;
	bool routes = argc == 2 && strcmp(argv[1], "--read-enabled-dram-routes-and-restore") == 0;
	bool samples = argc == 2 && strcmp(argv[1], "--test-fixed-dram-samples-and-restore") == 0;
	if (!inspect && !routes && !samples) {
		fprintf(stderr, "Usage: %s --check-fixed-dram-samples-support | --read-enabled-dram-routes-and-restore | --test-fixed-dram-samples-and-restore\n"
			"Private Pioneer only. Seven fixed banks, five fixed 64-byte samples per bank.\n"
			"Requires all 120 compute tiles stopped. No arbitrary addresses, core starts, DMA or firmware commands/flash.\n", argv[0]);
		return 2;
	}
	setvbuf(stdout, NULL, _IONBF, 0);
	int fd = open("/dev/" TT_PCI_PROBE_DEVICE, O_RDWR);
	if (fd < 0) { perror(TT_PCI_PROBE_DEVICE); return 1; }
	if (inspect) {
		tt_dram_samples_support support = {};
		int status = ioctl(fd, TT_PCI_PROBE_PIONEER_DRAM_SAMPLES_SUPPORT, &support, sizeof(support));
		int error = errno; close(fd);
		if (status != 0) { errno = error; perror("PIONEER_DRAM_SAMPLES_SUPPORT"); return 1; }
		printf("Loaded DRAM samples v%" PRIu32 " banks/regions/words %" PRIu32 "/%" PRIu32 "/%" PRIu32
			" enabled columns/GDDR %04" PRIx32 "/%02" PRIx32 "; no PCI/MMIO access\n",
			support.version,support.banks,support.regions,support.words,support.enabled_columns,support.enabled_gddr);
		return support.version == 2 && support.banks == 7 && support.regions == 5 && support.words == 16
			&& support.enabled_columns == 0xfff && support.enabled_gddr == 0xf7 ? 0 : 1;
	}
	printf("P100A-DRAM: %s, original payload is kernel-local only\n", samples ? "reversible fixed sample test" : "read-only seven-bank route check");
	tt_pioneer_dram_samples_result r = {};
	int status = ioctl(fd, samples ? TT_PCI_PROBE_PIONEER_DRAM_SAMPLES : TT_PCI_PROBE_PIONEER_DRAM_ALL_ROUTES, &r, sizeof(r));
	int error = errno; close(fd);
	if (status != 0) { errno = error; perror("PIONEER_DRAM_SAMPLES"); return 1; }
	const tt_dram_samples_result& d = r.dram;
	const tt_pioneer_mmio_result& p = r.control.control.control.pci;
	printf("DRAM state %" PRIu32 " (1 routes OK, 2 samples passed); routes/cores checked %" PRIu32 "/%" PRIu32 "\n", d.state,d.routes_verified,d.cores_checked);
	printf("Saved/modified regions %" PRIu32 "/%" PRIu32 " passes %" PRIu32 " mismatches %" PRIu32 "\n", d.saved_regions,d.modified_regions,d.passes,d.mismatches);
	for (unsigned bank = 0; bank < TT_DRAM_BANKS; bank++) {
		printf("Physical bank %" PRIu32 " translated (%" PRIu32 ",%" PRIu32 ")\n", BlackholeDramBank(bank),d.translated[bank][0],d.translated[bank][1]);
		for (unsigned sample = 0; sample < 3; sample++)
			printf("  ID %u: NODE/ENDPOINT/NIU/LABEL %08" PRIx32 "/%08" PRIx32 "/%08" PRIx32 "/%08" PRIx32 "\n", sample,
				d.id[bank][sample][0],d.id[bank][sample][1],d.id[bank][sample][2],d.id[bank][sample][3]);
	}
	printf("Data/TLB/PCI restored %" PRIu32 "/%" PRIu32 "/%" PRIu32 "; PCI state %" PRIu32 "; cold recovery %" PRIu32 "\n",
		d.data_restored,d.tlb_restored,p.restored,p.state,d.recovery_required);
	printf("Card/root commands after %04" PRIx32 "/%04" PRIx32 "; no core starts, DMA or firmware commands\n",p.commands_after[0],p.commands_after[1]);
	if (d.recovery_required || p.state == TT_PIONEER_MMIO_RESTORE_FAILED)
		fprintf(stderr, "STOP: cold recovery required; do not retry, reload or run another card operation.\n");
	bool okay = d.version == 1 && d.state == (samples ? TT_DRAM_SAMPLE_PASSED : TT_DRAM_SAMPLE_ROUTES_OK)
		&& d.routes_verified == TT_DRAM_BANKS && d.data_restored && d.tlb_restored && !d.recovery_required
		&& p.version == 1 && p.state == TT_PIONEER_MMIO_READ_OK && p.restored;
	if (samples) okay &= d.saved_regions == 35 && d.modified_regions == 35 && d.passes == 2 && d.mismatches == 0 && d.cores_checked == 240;
	else okay &= d.saved_regions == 0 && d.modified_regions == 0;
	return okay ? 0 : 1;
}
