/* SPDX-License-Identifier: MIT */
// Administrative CPU-metadata operations ONLY:no card IO/reset/model calls.
#include <Drivers.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../../src/add-ons/kernel/drivers/misc/PioneerDriverReloadProtocol.h"
static bool Tag(const char* text,uint64_t& value)
{
	if (strlen(text)!=16) return false;
	value=0;
	for (unsigned i=0;i<16;i++) {
		unsigned c=unsigned(text[i]),v=c>='0' && c<='9'?c-'0':c>='a' && c<='f'?c-'a'+10:16;
		if (v==16) return false;
		value=(value<<4)|v;
	}
	return value!=0;
}
static bool Query(int fd,tt_pioneer_driver_reload& r)
{
	r={}; r.version=1;
	return !ioctl(fd,TT_PCI_PROBE_RELOAD_STATUS,&r,sizeof(r)) && r.version==1 && r.driver
		&& r.guard_version==1 && r.build_tag && r.generation && r.open_count && r.quiesced<=1 && r.cold<=1 && r.active_owner<=1;
}
static void Print(const tt_pioneer_driver_reload& r)
{
	printf("P100A_RELOAD driver%u tag%016llx generation%llu quiesced%u cold%u active_owner%u open_count%u guard%u;CPU_METADATA_ONLY\n",
		r.driver,(unsigned long long)r.build_tag,(unsigned long long)r.generation,r.quiesced,r.cold,r.active_owner,r.open_count,r.guard_version);
}
int main(int argc,char** argv)
{
	bool status=argc==2 && !strcmp(argv[1],"--status");
	bool quiesce=argc==3 && !strcmp(argv[1],"--quiesce");
	bool wait=argc==4 && !strcmp(argv[1],"--wait");
	uint64_t expected=0,previous=0;
	if ((!status && !quiesce && !wait) || (!status && !Tag(argv[2],expected)) || (wait && !Tag(argv[3],previous))) return 2;
	for (unsigned attempt=0;attempt<(wait?60U:1U);attempt++) {
		int fd=open("/dev/misc/tenstorrent_pci_probe",O_RDWR|O_CLOEXEC);
		if (fd<0) {
			if (wait && (errno==ENOENT || errno==ENODEV || errno==EBUSY)) { usleep(500000); continue; }
			perror("STOP reload administrative open"); return 1;
		}
		tt_pioneer_driver_reload r={};
		if (!Query(fd,r)) { close(fd); puts("STOP reload metadata protocol missing/invalid;NO reset/retry/automatic rollback"); return 1; }
		if (status) { Print(r); return close(fd)?1:0; }
		if (r.cold || r.active_owner) { Print(r); close(fd); puts("STOP card recovery/active owner;NO reload"); return 1; }
		if (quiesce) {
			if (r.build_tag!=expected || r.open_count!=1) { Print(r); close(fd); return 1; }
			uint64_t generation=r.generation; uint32_t driver=r.driver;
			r={}; r.version=1; r.expected_tag=expected; r.expected_generation=generation;
			bool okay=!ioctl(fd,TT_PCI_PROBE_RELOAD_QUIESCE,&r,sizeof(r)) && r.version==1 && r.driver==driver
				&& r.guard_version==1 && r.build_tag==expected && r.generation==generation && r.quiesced==1
				&& !r.cold && !r.active_owner && r.open_count==1;
			if (okay) Print(r);
			if (close(fd)) okay=false;
			puts(okay?"QUIESCED_CLOSED;NEW_HARDWARE_WORK_BLOCKED_UNTIL_RELOAD":"STOP quiesce refused/unproven;NO reset/retry/rollback");
			return okay?0:1;
		}
		bool okay=r.build_tag==expected && r.generation>previous && !r.quiesced;
		if (close(fd)) return 1;
		if (okay) { Print(r); puts("HOT_RELOAD_VERIFIED;NO_REBOOT_NO_GPU_EXECUTION"); return 0; }
		// Poll ONLY administrative metadata,closing EACH FD before waiting so
		// Haiku can unload the old idle image. Never repeat a GPU command.
		usleep(500000);
	}
	puts("STOP replacement not verified;LEAVE_FILES_AND_CARD_UNCHANGED_NO_AUTOMATIC_ROLLBACK"); return 1;
}
