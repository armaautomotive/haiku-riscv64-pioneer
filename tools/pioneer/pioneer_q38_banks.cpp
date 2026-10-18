/* SPDX-License-Identifier: MIT */
// Bounded explicit driver45 storage qualification,NOT a model runner.
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
#include <time.h>
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
static double Seconds()
{
	timespec t={}; if (clock_gettime(CLOCK_MONOTONIC,&t)) return -1;
	return t.tv_sec+t.tv_nsec*1e-9;
}
static uint32_t BulkPattern(unsigned bank,unsigned end,unsigned trial,unsigned word)
{
	return 0x45602700U^(bank*0x17335519U)^(end*0x123abc01U)^(trial*0x37413579U)^(word*0x25435671U);
}
static bool BulkVerified(const tt_q38_bank_bulk& r)
{
	const auto& t=r.result.transfer;
	bool okay=r.result.pages==r.pages && r.result.host_words==r.pages*1024 && r.result.host_ready
		&& r.result.l1_prepared && t.version==1 && t.state==TT_STORE_PASSED && t.requests==6 && t.completed==6
		&& t.wrote_device && t.host_write_issued && t.host_visible && t.readback_complete && !t.mismatches && !t.recovery_required;
	for (unsigned step=0;step<6;step++) for (unsigned counter=0;counter<4;counter++)
		okay &= uint32_t(t.after[step][counter]-t.before[step][counter])==(counter==1?r.pages*64:1);
	return okay;
}
static bool ExerciseBulk(int fd)
{
	// Independent PIO initial sentinels and final readback. Eight pages at
	// each bank endpoint include neighbours outside EVERY uploaded interval.
	tt_q38_bank_page page={}; page.version=TT_Q38_BANK_PROTOCOL;
	for (unsigned bank=0;bank<7;bank++) for (unsigned end=0;end<2;end++) for (unsigned p=0;p<8;p++) {
		page.bank=bank; page.offset=(end?PioneerResidentBanks::Capacity-32768:0)+p*4096;
		for (unsigned w=0;w<1024;w++) page.data[w]=BulkPattern(bank,end,0,p*1024+w);
		if (!Call(fd,TT_PCI_PROBE_Q38_BANK_UPLOAD,&page,sizeof(page)) || page.result.state!=TT_STORE_PASSED
			|| page.result.requests!=3 || page.result.completed!=3 || page.result.recovery_required || page.result.mismatches) return false;
	}
	tt_q38_bank_bulk bulk={}; bulk.version=TT_Q38_BANK_PROTOCOL;
	double elapsed=0; unsigned bytes=0;
	for (unsigned bank=0;bank<7;bank++) for (unsigned end=0;end<2;end++) for (unsigned pages=1;pages<=4;pages++) {
		bulk.bank=bank; bulk.pages=pages;
		bulk.offset=(end?PioneerResidentBanks::Capacity-32768:0)+(end?8-pages:1)*4096;
		for (unsigned w=0;w<4096;w++) bulk.data[w]=BulkPattern(bank,end,pages,w);
		double start=Seconds();
		bool okay=start>=0 && Call(fd,TT_PCI_PROBE_Q38_BANK_BULK_UPLOAD,&bulk,sizeof(bulk)) && BulkVerified(bulk);
		double stop=Seconds(); okay &= stop>=start;
		printf("BULK bank=%u physical=%u end=%u pages=%u six_known=%u seconds=%.6f\n",bank,BlackholeDramBank(bank),end,pages,unsigned(okay),stop-start); fflush(stdout);
		if (!okay) return false;
		elapsed+=stop-start; bytes+=pages*4096;
	}
	for (unsigned bank=0;bank<7;bank++) for (unsigned end=0;end<2;end++) for (unsigned p=0;p<8;p++) {
		page.bank=bank; page.offset=(end?PioneerResidentBanks::Capacity-32768:0)+p*4096;
		memset(page.data,0,sizeof(page.data));
		if (!Call(fd,TT_PCI_PROBE_Q38_BANK_READ,&page,sizeof(page)) || page.result.state!=TT_STORE_PASSED
			|| page.result.requests!=1 || page.result.completed!=1 || page.result.mismatches || page.result.recovery_required) return false;
		bool changed=end?p>=4:(p>=1 && p<5);
		unsigned first=end?4:1;
		for (unsigned w=0;w<1024;w++) if (page.data[w]!=BulkPattern(bank,end,changed?4:0,(changed?p-first:p)*1024+w)) return false;
	}
	printf("NATIVE_SEVEN_BANK_BULK_SAMPLE_PASS bytes=%u seconds=%.6f;112_PIO_PAGES_INCLUDING_NEIGHBOURS;NOT_FULL_MODEL\n",bytes,elapsed);
	return true;
}
int main(int argc,char** argv)
{
	if (argc!=2 || (strcmp(argv[1],"--support") && strcmp(argv[1],"--exercise") && strcmp(argv[1],"--exercise-bulk"))) {
		fprintf(stderr,"Usage:%s --support|--exercise|--exercise-bulk;NOT_INFERENCE\n",argv[0]); return 2;
	}
	bool bulk=!strcmp(argv[1],"--exercise-bulk");
	int fd=open("/dev/" TT_PCI_PROBE_DEVICE,O_RDWR|O_CLOEXEC);
	if (fd<0) { perror("open P100A"); return 1; }
	tt_q38_bank_support support={};
	bool okay=Call(fd,TT_PCI_PROBE_Q38_BANK_SUPPORT,&support,sizeof(support));
	if (!okay || support.driver!=TT_Q38_BANK_DRIVER || support.version!=TT_Q38_BANK_PROTOCOL || support.banks!=7 || support.page_bytes!=4096
		|| support.max_pages!=4 || support.storage_only!=1 || support.bytes_per_bank!=PioneerResidentBanks::Capacity) {
		fprintf(stderr,"STOP:exact experimental45 storage ABI required;NO_BEGIN_OR_GPU_LAUNCH\n"); close(fd); return 1;
	}
	printf("DRIVER%u_STORAGE_ONLY banks=7 max_pages=4 bytes_per_bank=%llu;NOT_MODEL_SUPPORT\n",support.driver,(unsigned long long)support.bytes_per_bank);
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
	if (bulk && okay) okay=ExerciseBulk(fd);
	for (unsigned bank=0;bank<7 && okay && !bulk;bank++) for (unsigned last=0;last<2 && okay;last++) {
		page.bank=bank; page.offset=last?PioneerResidentBanks::Capacity-4096:0;
		for (unsigned w=0;w<1024;w++) page.data[w]=Pattern(bank,last,w);
		okay=Call(fd,TT_PCI_PROBE_Q38_BANK_UPLOAD,&page,sizeof(page))
			&& page.result.state==TT_STORE_PASSED && !page.result.recovery_required
			&& page.result.requests==3 && page.result.completed==3 && !page.result.mismatches;
		printf("UPLOAD logical_bank=%u physical_bank=%u last=%u known_complete=%u\n",bank,BlackholeDramBank(bank),last,unsigned(okay)); fflush(stdout);
	}
	for (unsigned bank=0;bank<7 && okay && !bulk;bank++) for (unsigned last=0;last<2 && okay;last++) {
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
		okay=Call(fd,TT_PCI_PROBE_Q38_BANK_STATUS,&session,sizeof(session)) && session.result.uploaded_pages==(bulk?112U:14U) && !session.result.cold;
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
	puts(bulk?"NATIVE_SEVEN_BANK_BULK_PASS;112_PAGES;CLEAN_END;NOT_FULL_CAPACITY_OR_MODEL_INFERENCE":
		"NATIVE_SEVEN_BANK_STORAGE_SAMPLE_PASS;14_DISTINCT_PAGES;CLEAN_END;NOT_FULL_CAPACITY_OR_MODEL_INFERENCE");
	return 0;
}
