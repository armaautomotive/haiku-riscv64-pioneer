/* Distributed under the MIT License. */
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
	if (argc != 2 || strcmp(argv[1], "--read-core-reset-and-restore") != 0) {
		fprintf(stderr, "Usage: %s --read-core-reset-and-restore\n"
			"Private Pioneer only: read enabled physical Tensix (1,2); no reset or data writes.\n", argv[0]);
		return 2;
	}
	int fd = open("/dev/" TT_PCI_PROBE_DEVICE, O_RDWR);
	if (fd < 0) { perror(TT_PCI_PROBE_DEVICE); return 1; }
	tt_pioneer_tensix_result r = {};
	int status = ioctl(fd, TT_PCI_PROBE_PIONEER_TENSIX_RESET, &r, sizeof(r));
	int savedError = errno;
	close(fd);
	if (status != 0) { errno = savedError; perror("PIONEER_TENSIX_RESET"); return 1; }
	const tt_tensix_reset_result& t = r.tensix;
	const tt_pioneer_mmio_result& p = r.control.control.pci;
	printf("Tensix reset state %" PRIu32 " (1 all five RISCs held), NIU %08" PRIx32
		" translation %u; TLB restored %" PRIu32 "; PCI state %" PRIu32 " restored %" PRIu32 "\n",
		t.state, t.niu_config, (t.niu_config >> 14) & 1, t.tlb_restored, p.state, p.restored);
	printf("Reset samples %08" PRIx32 " / %08" PRIx32 " / %08" PRIx32 "; required mask 00047800\n",
		t.reset[0], t.reset[1], t.reset[2]);
	for (unsigned i = 0; i < 3; i++)
		printf("Core TLB word %u before %08" PRIx32 " trial %08" PRIx32 " after %08" PRIx32 "\n",
			i, t.tlb_before[i], t.tlb_trial[i], t.tlb_after[i]);
	printf("Card/root commands after %04" PRIx32 "/%04" PRIx32 "; no core, firmware or DMA writes.\n",
		p.commands_after[0], p.commands_after[1]);
	if (!t.tlb_restored || !r.control.telemetry.tlb_restored
		|| !r.control.control.arc.tlb_restored || p.state == TT_PIONEER_MMIO_RESTORE_FAILED)
		fprintf(stderr, "STOP: cold recovery required; do not run another card test.\n");
	return t.version == 1 && t.state == TT_TENSIX_RESET_HELD && t.tlb_restored
		&& p.state == TT_PIONEER_MMIO_READ_OK && p.restored ? 0 : 1;
}
