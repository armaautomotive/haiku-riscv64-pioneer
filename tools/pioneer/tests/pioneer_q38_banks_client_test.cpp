/* SPDX-License-Identifier: MIT */
// Execute the REAL native sample client with synthetic syscalls. No card,
// network, model files, or privileged actions. The transport itself has
// independent real-handler/NoC fault tests; this checks the user gate too.
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <array>
#include <map>
#include "cli-stubs/OS.h"
#include "../../../src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38BankProtocol.h"
static std::map<uint64_t,std::array<uint32_t,1024>> memory;
static unsigned opens=0,closes=0,begins=0,ends=0,bulks=0,reads=0,uploads=0;
static unsigned fault=0,field=0,oldDriver=0;
static bool active=false,failed=false;
static uint64_t Key(unsigned bank,uint64_t offset)
{
	assert(bank<7 && !(offset&4095) && offset<PioneerResidentBanks::Capacity);
	return bank*PioneerResidentBanks::Capacity+offset;
}
static int MockOpen(const char* path,int flags)
{
	assert(!strcmp(path,"/dev/" TT_PCI_PROBE_DEVICE) && flags==(O_RDWR|O_CLOEXEC));
	opens++; return opens==1?73:74;
}
static int MockClose(int fd)
{
	assert(fd==73 || fd==74); closes++;
	if (fd==73) active=false; // no implicit END call from user on failure
	return 0;
}
static int MockIoctl(int fd,unsigned long op,void* data,size_t bytes)
{
	assert((fd==73 || fd==74) && !failed);
	if (op==TT_PCI_PROBE_Q38_BANK_SUPPORT) {
		assert(fd==73 && bytes==sizeof(tt_q38_bank_support));
		auto& s=*static_cast<tt_q38_bank_support*>(data);
		s={TT_Q38_BANK_DRIVER,TT_Q38_BANK_PROTOCOL,7,4096,4,1,PioneerResidentBanks::Capacity};
		if (oldDriver) { s.driver=44; s.max_pages=1; }
		return 0;
	}
	if (op==TT_PCI_PROBE_Q38_BANK_BEGIN) {
		assert(bytes==sizeof(tt_q38_bank_session));
		if (active) { errno=B_BUSY; return -1; }
		assert(fd==73); begins++; active=true;
		auto& r=static_cast<tt_q38_bank_session*>(data)->result;
		r={}; r.version=1; r.active=r.host_ready=1; r.state=TT_RESIDENT_SESSION_READY; r.bytes=PioneerResidentBanks::Total;
		return 0;
	}
	assert(active && fd==73);
	if (op==TT_PCI_PROBE_PIONEER_SESSION_STATUS) { errno=B_NOT_ALLOWED; return -1; }
	if (op==TT_PCI_PROBE_Q38_BANK_END) {
		assert(bytes==sizeof(tt_q38_bank_session)); ends++; active=false;
		auto& r=static_cast<tt_q38_bank_session*>(data)->result;
		r={}; r.version=1; r.state=TT_RESIDENT_SESSION_CLOSED; r.pci_restored=r.host_released=1; return 0;
	}
	if (op==TT_PCI_PROBE_Q38_BANK_STATUS) {
		auto& r=static_cast<tt_q38_bank_session*>(data)->result;
		r={}; r.version=1; r.uploaded_pages=unsigned(memory.size()); return 0;
	}
	if (op==TT_PCI_PROBE_Q38_BANK_UPLOAD || op==TT_PCI_PROBE_Q38_BANK_READ) {
		assert(bytes==sizeof(tt_q38_bank_page)); auto& p=*static_cast<tt_q38_bank_page*>(data);
		assert(p.version==1); uint64_t key=Key(p.bank,p.offset);
		bool writing=op==TT_PCI_PROBE_Q38_BANK_UPLOAD;
		p.result={}; p.result.version=1; p.result.state=TT_STORE_PASSED;
		p.result.requests=p.result.completed=writing?3:1;
		if (writing) { uploads++; memcpy(memory[key].data(),p.data,4096); }
		else {
			reads++; auto found=memory.find(key); assert(found!=memory.end()); memcpy(p.data,found->second.data(),4096);
			// A corrupt UNTOUCHED neighbour must also fail the sample.
			if (fault==3 && bulks && p.bank==6 && p.offset==0) { p.data[999]^=1; failed=true; }
		}
		return 0;
	}
	assert(op==TT_PCI_PROBE_Q38_BANK_BULK_UPLOAD && bytes==sizeof(tt_q38_bank_bulk));
	auto& p=*static_cast<tt_q38_bank_bulk*>(data);
	assert(p.version==1 && !p.reserved && PioneerQ38BankBulkRange(p.bank,p.offset,p.pages));
	bulks++;
	for (unsigned page=0;page<p.pages;page++) memcpy(memory[Key(p.bank,p.offset+page*4096)].data(),p.data+page*1024,4096);
	auto& r=p.result.transfer; p.result={}; r.version=1; r.state=TT_STORE_PASSED;
	r.requests=r.completed=6; r.wrote_device=r.host_write_issued=r.host_visible=r.readback_complete=1;
	for (unsigned step=0;step<6;step++) for (unsigned w=0;w<4;w++) {
		r.before[step][w]=UINT32_MAX-2;
		r.after[step][w]=r.before[step][w]+(w==1?64*p.pages:1);
	}
	p.result.pages=p.pages; p.result.host_words=p.pages*1024; p.result.host_ready=p.result.l1_prepared=1;
	if (fault==1) { failed=true; errno=EIO; return -1; }
	if (fault==2) {
		failed=true;
		switch (field) {
			case 0:r.version=2;break; case 1:r.state=TT_STORE_CHANGED;break;
			case 2:r.requests=5;break; case 3:r.completed=5;break; case 4:r.wrote_device=0;break;
			case 5:r.host_write_issued=0;break; case 6:r.host_visible=0;break;
			case 7:r.readback_complete=0;break; case 8:r.mismatches=1;break;
			case 9:r.recovery_required=1;break; case 10:p.result.pages=0;break;
			case 11:p.result.host_words=0;break; case 12:p.result.host_ready=0;break;
			case 13:p.result.l1_prepared=0;break;
			default:r.after[(field-14)/4][(field-14)%4]++;break;
		}
	}
	return 0;
}
#define open MockOpen
#define close MockClose
#define ioctl MockIoctl
#define main PioneerQ38SampleMain
#include "../pioneer_q38_banks.cpp"
#undef main
#undef ioctl
#undef close
#undef open
static int Run(const char* argument)
{
	char name[]="sample"; char* argv[]={name,const_cast<char*>(argument)};
	return PioneerQ38SampleMain(2,argv);
}
static void Reset(unsigned f=0,unsigned w=0)
{
	memory.clear(); opens=closes=begins=ends=bulks=reads=uploads=oldDriver=0;
	active=failed=false; fault=f; field=w;
}
int main()
{
	Reset(); assert(Run("--support")==0 && opens==1 && closes==1 && !begins && !bulks);
	Reset(); oldDriver=1; assert(Run("--exercise-bulk")==1 && !begins && closes==1);
	Reset(); assert(Run("--exercise")==0 && begins==1 && ends==1 && closes==2 && uploads==14 && reads==14 && !bulks);
	Reset(); assert(Run("--exercise-bulk")==0 && begins==1 && ends==1 && closes==2
		&& uploads==112 && bulks==56 && reads==112 && memory.size()==112);
	for (unsigned f=1;f<=2;f++) for (unsigned w=0;w<(f==1?1U:38U);w++) {
		Reset(f,w); assert(Run("--exercise-bulk")==1 && failed && bulks==1 && !ends && closes==2 && !reads);
	}
	Reset(3); assert(Run("--exercise-bulk")==1 && failed && bulks==56 && !ends && closes==2);
	Reset(); assert(Run("--bad-argument")==2 && !opens);
	puts("PASS: REAL seven-bank client,metadata-only/old-driver refusal,all56 bulk shapes+112 independent pages,ALL38 proof-field faults/ioctl error/untouched-neighbour mismatch stop without END/retry;NO hardware");
}
