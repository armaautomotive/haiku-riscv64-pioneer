/* Distributed under the MIT License. */
#include "../../src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.h"
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>

int main()
{
	int fd = open("/dev/" TT_PCI_PROBE_DEVICE, O_RDWR);
	if (fd < 0) { perror(TT_PCI_PROBE_DEVICE); return 1; }
	tt_pioneer_profile p = {};
	int status = ioctl(fd, TT_PCI_PROBE_PIONEER_PROFILE, &p, sizeof(p));
	close(fd);
	if (status != 0) { perror("PIONEER_PROFILE"); return 1; }
	printf("Read-only Pioneer profile v%" PRIu32 " stage %" PRIu32 " status %" PRId32 " ready %" PRIu32 "\n",
		p.version, p.stage, p.status, p.ready);
	printf("Virtual PCI card %" PRIu32 " root %" PRIu32 "; root command %04" PRIx32 " buses %08" PRIx32 "\n",
		p.card_bus, p.root_bus, p.root_command, p.root_buses);
	printf("Controller %" PRIx64 " config %" PRIx64 "/%" PRIx64 "\n", p.controller, p.config, p.config_length);
	printf("Root memory %08" PRIx32 " prefetch %08" PRIx32 " upper %08" PRIx32 ":%08" PRIx32 "\n",
		p.root_memory, p.root_prefetch, p.root_prefetch_base_high, p.root_prefetch_limit_high);
	for (unsigned i = 0; i < TT_PIONEER_RANGE_COUNT; i++)
		printf("Range %u PCI %" PRIx64 " host %" PRIx64 " size %" PRIx64 " type %" PRIu32 " flags %x\n",
			i, p.ranges[i].pci, p.ranges[i].host, p.ranges[i].size, p.ranges[i].type, p.ranges[i].flags);
	printf("Proposed BAR0 host %" PRIx64 "..%" PRIx64 " BAR2 host %" PRIx64 "..%" PRIx64 "\n",
		p.bar0_host_start, p.bar0_host_end, p.bar2_host_start, p.bar2_host_end);
	printf("No PCI register, MMIO, DMA, or firmware write performed by profile inspection.\n");
	return p.version == 1 && p.status == 0 && p.ready == 1 ? 0 : 1;
}
