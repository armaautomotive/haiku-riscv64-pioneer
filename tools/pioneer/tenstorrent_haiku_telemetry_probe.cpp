/* Distributed under the MIT License. */
#include "../../src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.h"
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

static void PrintTemperature(uint32_t raw)
{
	if (raw == UINT32_MAX) { printf(" unavailable"); return; }
	// Public UMD Blackhole ASIC/board temperature is signed 16.16 Celsius.
	int64_t scaled = (int64_t)(int32_t)raw * 100;
	int64_t centi = (scaled + (scaled >= 0 ? 32768 : -32768)) / 65536;
	uint64_t absolute = centi < 0 ? -centi : centi;
	printf(" %s%" PRIu64 ".%02" PRIu64 " C", centi < 0 ? "-" : "", absolute / 100, absolute % 100);
}

int main(int argc, char** argv)
{
	bool operating = argc == 2 && strcmp(argv[1],"--read-operating-telemetry-and-restore") == 0;
	if (!operating && (argc != 2 || strcmp(argv[1], "--read-telemetry-and-restore") != 0)) {
		fprintf(stderr, "Usage: %s --read-telemetry-and-restore | --read-operating-telemetry-and-restore\n"
			"Private Pioneer only: bounded whitelisted firmware/health values, exact PCI/TLB rollback.\n", argv[0]);
		return 2;
	}
	int fd = open("/dev/" TT_PCI_PROBE_DEVICE, O_RDWR);
	if (fd < 0) { perror(TT_PCI_PROBE_DEVICE); return 1; }
	tt_pioneer_telemetry_result r = {};
	int status = ioctl(fd, operating ? TT_PCI_PROBE_PIONEER_OPERATING_TELEMETRY : TT_PCI_PROBE_PIONEER_TELEMETRY, &r, sizeof(r));
	int savedError = errno;
	close(fd);
	if (status != 0) { errno = savedError; perror("PIONEER_TELEMETRY"); return 1; }
	const tt_arc_telemetry_result& t = r.telemetry;
	printf("Telemetry state %" PRIu32 " (1 OK), TLB restored %" PRIu32
		"; ARC state %" PRIu32 " restored %" PRIu32 "; PCI state %" PRIu32 " restored %" PRIu32 "\n",
		t.state, t.tlb_restored, r.control.arc.state, r.control.arc.tlb_restored,
		r.control.pci.state, r.control.pci.restored);
	printf("Table version %08" PRIx32 " entries %" PRIu32 " mask %08" PRIx32
		" tags %08" PRIx32 " data %08" PRIx32 "\n",
		t.table_version, t.entry_count, t.present_mask, t.tags_address, t.data_address);
	for (unsigned i = 0; i < 3; i++)
		printf("CSM TLB word %u before %08" PRIx32 " trial %08" PRIx32 " after %08" PRIx32 "\n",
			i, t.tlb_before[i], t.tlb_trial[i], t.tlb_after[i]);
	const char* labels[] = {"Firmware bundle", "CM firmware", "ASIC temperature", "Board temperature",
		"Max GDDR temperature", "Fan RPM", "Enabled GDDR", "GDDR status", "GDDR uncorrected errors",
		"Enabled Tensix columns", "Firmware heartbeat", "Thermal shutdown limit"};
	const char* operatingLabels[] = {"AI clock (MHz)", "AXI clock (MHz)", "ARC clock (MHz)", "Vcore (mV)",
		"Board power limit (W)", "Input power (W)", "Thermal trip count", "AI clock limit max", "Firmware bundle",
		"CM firmware", "Firmware heartbeat", "Thermal shutdown limit"};
	for (unsigned i = 0; i < TT_ARC_TELEMETRY_COUNT; i++) {
		printf("Tag %u %-25s: ", BlackholeTelemetryTag(i,operating), operating ? operatingLabels[i] : labels[i]);
		if ((t.present_mask & (1U << i)) == 0) { printf("not published\n"); continue; }
		uint32_t raw = t.values[i];
		printf("%08" PRIx32, raw);
		if ((!operating && i == 0) || (operating && i == 8)) printf(" %u.%u.%u (suffix 0x%02x)", raw >> 24, (raw >> 16) & 255, (raw >> 8) & 255, raw & 255);
		if (!operating && (i == 2 || i == 3)) PrintTemperature(raw);
		if (!operating && i == 5 && raw != UINT32_MAX) printf(" %" PRIu32 " RPM", raw);
		if (operating && i <= 2 && raw != UINT32_MAX) printf(" %" PRIu32 " MHz",raw & 65535);
		printf("\n");
	}
	printf("Card/root commands after %04" PRIx32 "/%04" PRIx32 "; no serial IDs, firmware commands/writes or DMA.\n",
		r.control.pci.commands_after[0], r.control.pci.commands_after[1]);
	if (r.control.pci.state == TT_PIONEER_MMIO_RESTORE_FAILED || !t.tlb_restored || !r.control.arc.tlb_restored)
		fprintf(stderr, "STOP: cold recovery required; do not run another card test.\n");
	return t.version == (operating ? 2U : 1U) && t.state == TT_ARC_TELEMETRY_OK && t.tlb_restored
		&& r.control.arc.tlb_restored && r.control.pci.state == TT_PIONEER_MMIO_READ_OK
		&& r.control.pci.restored ? 0 : 1;
}
