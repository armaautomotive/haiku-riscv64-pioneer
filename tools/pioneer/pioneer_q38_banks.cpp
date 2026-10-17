/* SPDX-License-Identifier: MIT */
// Bounded explicit driver44 storage qualification,NOT a model runner.
// Default --support is metadata only. --exercise owns all banks, writes
// ONLY their first/last 4KiB pages,then independently reads every pattern.
#include <Drivers.h>
#include <OS.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "../../src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38BankProtocol.h"
static uint32_t Pattern(unsigned bank,unsigned last,unsigned word)
{
	return 0xc0382700U^(bank*0x1020101U)^(last*0x538abc21U)^(word*0x1234567U);
}
static bool Call(int fd,uint32_t operation,void* data,size_t bytes)
{
	if (ioctl(fd,operation,data,bytes)==0) return true;
	fprintf(stderr,"STOP:ioctl=%08x errno=%d %s;NO_RETRY\n",operation,errno,strerror(errno)); return false;
}
int main(int argc,char** argv)
{
	if (argc!=2 || (strcmp(argv[1],"--support") && strcmp(argv[1],"--exercise"))) {
		fprintf(stderr,"Usage:%s --support|--exercise;NOT_INFERENCE\n",argv[0]); return 2;
	}
	int fd=open("/dev/" TT_PCI_PROBE_DEVICE,O_RDWR|O_CLOEXEC);
	if (fd<0) { perror("open P100A"); return 1; }
	tt_q38_bank_support support={};
	bool okay=Call(fd,TT_PCI_PROBE_Q38_BANK_SUPPORT,&support,sizeof(support));
	if (!okay || support.driver!=44 || support.version!=1 || support.banks!=7 || support.page_bytes!=4096
		|| support.max_pages!=1 || support.storage_only!=1 || support.bytes_per_bank!=PioneerResidentBanks::Capacity) {
		fprintf(stderr,"STOP:exact experimental44 storage ABI required;NO_BEGIN_OR_GPU_LAUNCH\n"); close(fd); return 1;
	}
	printf("DRIVER44_STORAGE_ONLY banks=7 bytes_per_bank=%llu;NOT_MODEL_SUPPORT\n",(unsigned long long)support.bytes_per_bank);
	if (!strcmp(argv[1],"--support")) return close(fd)?1:0;
	tt_q38_bank_session session={}; session.version=1;
	okay=Call(fd,TT_PCI_PROBE_Q38_BANK_BEGIN,&session,sizeof(session));
	okay=okay && session.result.state==TT_RESIDENT_SESSION_READY && session.result.active
		&& session.result.host_ready && !session.result.cold && !session.result.capabilities
		&& session.result.bytes==PioneerResidentBanks::Total;
	if (!okay) { fprintf(stderr,"STOP:arena begin;NO_RETRY\n"); close(fd); return 1; }
	// Repeated begin must refuse without destroying the original owner.
	tt_q38_bank_session duplicate={}; duplicate.version=1;
	int rc=ioctl(fd,TT_PCI_PROBE_Q38_BANK_BEGIN,&duplicate,sizeof(duplicate));
	if (rc==0 || errno!=B_BUSY) okay=false;
	// The same FD must not route this allocation into ANY legacy runtime.
	tt_resident_session_result legacy={};
	rc=ioctl(fd,TT_PCI_PROBE_PIONEER_SESSION_STATUS,&legacy,sizeof(legacy));
	if (rc==0 || errno!=B_NOT_ALLOWED) okay=false;
	int other=open("/dev/" TT_PCI_PROBE_DEVICE,O_RDWR|O_CLOEXEC);
	if (other<0) okay=false;
	else {
		rc=ioctl(other,TT_PCI_PROBE_Q38_BANK_BEGIN,&duplicate,sizeof(duplicate));
		if (rc==0 || errno!=B_BUSY) okay=false;
		if (close(other)) okay=false;
	}
	tt_q38_bank_page page={}; page.version=1;
	for (unsigned bank=0;bank<7 && okay;bank++) for (unsigned last=0;last<2 && okay;last++) {
		page.bank=bank; page.offset=last?PioneerResidentBanks::Capacity-4096:0;
		for (unsigned w=0;w<1024;w++) page.data[w]=Pattern(bank,last,w);
		okay=Call(fd,TT_PCI_PROBE_Q38_BANK_UPLOAD,&page,sizeof(page))
			&& page.result.state==TT_STORE_PASSED && !page.result.recovery_required
			&& page.result.requests==3 && page.result.completed==3 && !page.result.mismatches;
		printf("UPLOAD logical_bank=%u physical_bank=%u last=%u known_complete=%u\n",bank,BlackholeDramBank(bank),last,unsigned(okay)); fflush(stdout);
	}
	for (unsigned bank=0;bank<7 && okay;bank++) for (unsigned last=0;last<2 && okay;last++) {
		page.bank=bank; page.offset=last?PioneerResidentBanks::Capacity-4096:0;
		memset(page.data,0,sizeof(page.data));
		okay=Call(fd,TT_PCI_PROBE_Q38_BANK_READ,&page,sizeof(page))
			&& page.result.state==TT_STORE_PASSED && !page.result.recovery_required
			&& page.result.requests==1 && page.result.completed==1 && !page.result.mismatches;
		for (unsigned w=0;w<1024 && okay;w++) okay=page.data[w]==Pattern(bank,last,w);
		printf("READBACK logical_bank=%u last=%u exact_pattern=%u\n",bank,last,unsigned(okay)); fflush(stdout);
	}
	if (okay) {
		session={}; session.version=1;
		okay=Call(fd,TT_PCI_PROBE_Q38_BANK_STATUS,&session,sizeof(session)) && session.result.uploaded_pages==14 && !session.result.cold;
	}
	// On uncertainty NEVER retry/end/reload/reset the card. Final FD close
// invokes the kernel's global quarantine path. Full cold recovery is
	// mandatory if the result/cold latch is unknown; stdout is not authority.
	if (!okay) { fprintf(stderr,"STOP:qualification failed;NO_RETRY; inspect cold latch/serial before recovery\n"); close(fd); return 1; }
	session={}; session.version=1;
	okay=Call(fd,TT_PCI_PROBE_Q38_BANK_END,&session,sizeof(session))
		&& session.result.state==TT_RESIDENT_SESSION_CLOSED && !session.result.active
		&& session.result.pci_restored && session.result.host_released && !session.result.cold;
	if (close(fd)) okay=false;
	if (!okay) { fprintf(stderr,"STOP:END not known clean;NO_RETRY_OR_RELOAD\n"); return 1; }
	puts("NATIVE_SEVEN_BANK_STORAGE_SAMPLE_PASS;14_DISTINCT_PAGES;CLEAN_END;NOT_FULL_CAPACITY_OR_MODEL_INFERENCE");
	return 0;
}
