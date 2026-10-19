/* SPDX-License-Identifier: MIT */
// Fixed native acceptance test;NO matrix code/reset/firmware writes.
// A real BANK_BEGIN owns the card's ordinary audited idle session,then
// verifies quiesce refuses that owner before ending it cleanly.
#include <Drivers.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include "../../src/add-ons/kernel/drivers/misc/PioneerDriverReloadProtocol.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38BankProtocol.h"
static bool Metadata(int fd,tt_pioneer_driver_reload& r)
{
	r={}; r.version=1;
	return !ioctl(fd,TT_PCI_PROBE_RELOAD_STATUS,&r,sizeof(r)) && r.version==1 && r.driver==55 && r.guard_version==1
		&& r.generation && r.build_tag && !r.cold && !r.quiesced;
}
static int Stop(int fd)
{
	puts("STOP_BUSY_PROBE;CLOSE_ONLY_NO_END_RETRY_RESET"); close(fd); return 1;
}
int main()
{
	setvbuf(stdout,NULL,_IOLBF,0);
	int fd=open("/dev/misc/tenstorrent_pci_probe",O_RDWR|O_CLOEXEC); if (fd<0) return 1;
	tt_pioneer_driver_reload before={},r={};
	if (!Metadata(fd,before) || before.active_owner || before.open_count!=1) return Stop(fd);
	// Extra independent open refuses the SAME private-generation quiesce.
	int second=open("/dev/misc/tenstorrent_pci_probe",O_RDWR|O_CLOEXEC);
	if (second<0) return Stop(fd);
	r={}; r.version=1; r.expected_tag=before.build_tag; r.expected_generation=before.generation;
	bool refused=ioctl(fd,TT_PCI_PROBE_RELOAD_QUIESCE,&r,sizeof(r))!=0 && errno==EBUSY;
	bool observed=Metadata(fd,r) && !r.active_owner && r.open_count==2 && r.generation==before.generation && r.build_tag==before.build_tag;
	if (close(second) || !refused || !observed) return Stop(fd);
	puts("PASS_NATIVE_EXTRA_FD_QUIESCE_REFUSED;NO_CARD_SESSION_YET");
	tt_q38_bank_session bank={}; bank.version=1;
	if (ioctl(fd,TT_PCI_PROBE_Q38_BANK_BEGIN,&bank,sizeof(bank)) || bank.result.version!=1 || !bank.result.active
		|| bank.result.state!=TT_RESIDENT_SESSION_READY || !bank.result.host_ready || bank.result.cold
		|| bank.result.bytes!=PioneerResidentBanks::Total || bank.result.capabilities) return Stop(fd);
	if (!Metadata(fd,r) || r.active_owner!=1 || r.open_count!=1) return Stop(fd);
	r={}; r.version=1; r.expected_tag=before.build_tag; r.expected_generation=before.generation;
	refused=ioctl(fd,TT_PCI_PROBE_RELOAD_QUIESCE,&r,sizeof(r))!=0 && errno==EBUSY;
	if (!refused || !Metadata(fd,r) || r.active_owner!=1 || r.open_count!=1
		|| r.generation!=before.generation || r.build_tag!=before.build_tag) return Stop(fd);
	puts("PASS_NATIVE_ACTIVE_BANK_OWNER_QUIESCE_REFUSED;NO_GPU_MATRIX_EXECUTION");
	bank={}; bank.version=1;
	if (ioctl(fd,TT_PCI_PROBE_Q38_BANK_END,&bank,sizeof(bank)) || bank.result.state!=TT_RESIDENT_SESSION_CLOSED
		|| bank.result.active || !bank.result.pci_restored || !bank.result.host_released || bank.result.cold) return Stop(fd);
	if (!Metadata(fd,r) || r.active_owner || r.open_count!=1 || r.generation!=before.generation || r.build_tag!=before.build_tag) return Stop(fd);
	if (close(fd)) return 1;
	puts("PASS_NATIVE_RELOAD_BUSY_GATES_CLEAN_END;NO_REBOOT_RESET_OR_MATRIX_TEST"); return 0;
}
