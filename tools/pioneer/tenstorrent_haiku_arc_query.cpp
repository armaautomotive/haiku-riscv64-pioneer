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
	bool inspect = argc == 2 && strcmp(argv[1],"--check-fixed-arc-query-support") == 0;
	if (!inspect && (argc != 2 || strcmp(argv[1],"--query-fixed-arc-clock-and-restore") != 0)) {
		fprintf(stderr,"Usage: %s --check-fixed-arc-query-support | --query-fixed-arc-clock-and-restore\n"
			"Private Pioneer: one GET_AICLK query on ARC application queue 3.\n"
			"Queue pointers/counter advance normally; consumed response remains firmware-owned.\n"
			"No clock/voltage change, RISC/reset, DMA or flash command.\n",argv[0]);
		return 2;
	}
	setvbuf(stdout,NULL,_IONBF,0);
	int fd = open("/dev/" TT_PCI_PROBE_DEVICE,O_RDWR);
	if (fd < 0) { perror(TT_PCI_PROBE_DEVICE); return 1; }
	if (inspect) {
		tt_arc_query_support s = {};
		int status = ioctl(fd,TT_PCI_PROBE_PIONEER_ARC_QUERY_SUPPORT,&s,sizeof(s));
		int error = errno; close(fd);
		if (status != 0) { errno = error; perror("PIONEER_ARC_QUERY_SUPPORT"); return 1; }
		printf("Loaded ARC query v%" PRIu32 " message %02" PRIx32 " queue/entries %" PRIu32 "/%" PRIu32 " CSM bytes %" PRIu32 "; metadata only\n",
			s.version,s.message,s.queue,s.entries,s.csm_bytes);
		return s.version == 2 && s.message == 0x34 && s.queue == 3 && s.entries == 4 && s.csm_bytes == 0x30000 ? 0 : 1;
	}
	printf("P100A-ARC-QUERY: read-only GET_AICLK, no clock/reset/DMA/flash command; normal queue counters advance\n");
	tt_pioneer_arc_query_result r = {};
	int status = ioctl(fd,TT_PCI_PROBE_PIONEER_ARC_CLOCK_QUERY,&r,sizeof(r));
	int error = errno; close(fd);
	if (status != 0) { errno = error; perror("PIONEER_ARC_CLOCK_QUERY"); return 1; }
	const tt_arc_queue_result& q = r.query;
	const tt_pioneer_mmio_result& p = r.control.control.pci;
	printf("ARC query state %" PRIu32 " (1 passed), submitted/replied/acknowledged %" PRIu32 "/%" PRIu32 "/%" PRIu32 " polls %" PRIu32 "\n",
		q.state,q.submitted,q.responded,q.acknowledged,q.polls);
	printf("Reply status %08" PRIx32 " clock %" PRIu32 " MHz mode %" PRIu32 " (1 uncontrolled, 2 forced, 3 normal DVFS); MSI-catcher status %08" PRIx32 "\n",
		q.status,q.clock_mhz,q.clock_mode,q.msi_status);
	printf("Descriptor %08" PRIx32 "/%08" PRIx32 "/%08" PRIx32 "/%08" PRIx32 "; application queue %08" PRIx32 "\n",
		q.descriptor[0],q.descriptor[1],q.descriptor[2],q.descriptor[3],q.base);
	printf("Queue request/response pointers %" PRIu32 "/%" PRIu32 " -> %" PRIu32 "/%" PRIu32 "; sequence counter %" PRIu32 " -> %" PRIu32 "\n",
		q.before[0],q.before[1],q.after[0],q.after[1],q.before[6],q.after[6]);
	printf("Request/TLB/PCI restored %" PRIu32 "/%" PRIu32 "/%" PRIu32 "; PCI state %" PRIu32 "; cold recovery %" PRIu32 "\n",
		q.request_restored,q.tlb_restored,p.restored,p.state,q.recovery_required);
	printf("Card/root commands %04" PRIx32 "/%04" PRIx32 "; firmware-owned response bytes not overwritten\n",p.commands_after[0],p.commands_after[1]);
	if (q.recovery_required || p.state == TT_PIONEER_MMIO_RESTORE_FAILED)
		fprintf(stderr,"STOP: cold recovery required; do not retry, reload or run another card operation.\n");
	return q.version == 1 && q.state == TT_ARC_QUERY_PASSED && q.submitted && q.responded && q.acknowledged
		&& q.request_restored && q.tlb_restored && !q.recovery_required
		&& p.version == 1 && p.state == TT_PIONEER_MMIO_READ_OK && p.restored ? 0 : 1;
}
