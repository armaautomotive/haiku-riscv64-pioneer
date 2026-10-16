/* SPDX-License-Identifier: MIT */
// The real native transport, synthetic syscalls. No card/model/network access.
#include <cassert>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "../../../src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.h"
static unsigned sCalls=0,sEnds=0,sCloses=0,sUploads=0,sFault=0,sField=0;
static bool sFailed=false,sMissingCapability=false;
static int MockOpen(const char* path,int flags) { assert(!strcmp(path,"/dev/" TT_PCI_PROBE_DEVICE) && flags==O_RDWR); return 73; }
static int MockClose(int fd) { assert(fd==73); sCloses++; return 0; }
static int MockIoctl(int fd,unsigned long op,void* data,size_t bytes)
{
	assert(fd==73 && !sFailed); sCalls++;
	if (op==TT_PCI_PROBE_PIONEER_SESSION_SUPPORT) {
		auto& r=*(tt_resident_session_result*)data; r.version=1; r.capabilities=TT_PIONEER_RESIDENT_CAPABILITIES;
		if (sMissingCapability) r.capabilities &= ~TT_PIONEER_CAP_BULK_UPLOAD;
		return 0;
	}
	if (op==TT_PCI_PROBE_PIONEER_SESSION_BEGIN) {
		auto& r=*(tt_resident_session_result*)data; r.version=1; r.state=TT_RESIDENT_SESSION_READY; r.active=r.host_ready=1; return 0;
	}
	if (op==TT_PCI_PROBE_PIONEER_SESSION_END) {
		auto& r=*(tt_resident_session_result*)data; r.version=1; r.state=TT_RESIDENT_SESSION_CLOSED; r.pci_restored=r.host_released=1; sEnds++; return 0;
	}
	if (op==TT_PCI_PROBE_PIONEER_STREAM_UPLOAD) {
		assert(sMissingCapability && bytes==sizeof(tt_pioneer_resident_stream));
		auto& p=*(tt_pioneer_resident_stream*)data; assert(p.version==1 && p.offset==4096+sUploads*4096);
		for (unsigned w=0;w<1024;w++) assert(p.data[w]==(0xa1647930U^(sUploads*1024+w)));
		auto& r=p.result; r={}; r.version=1; r.state=TT_STORE_PASSED; r.requests=r.completed=6;
		r.host_write_issued=r.host_visible=r.readback_complete=1; sUploads++; return 0;
	}
	assert(op==TT_PCI_PROBE_PIONEER_BULK_UPLOAD && bytes==sizeof(tt_pioneer_bulk_upload));
	auto& p=*(tt_pioneer_bulk_upload*)data;
	assert(p.version==1 && p.pages>=1 && p.pages<=4 && p.offset==4096 && !p.reserved[0] && !p.reserved[1]);
	for (unsigned w=0;w<p.pages*1024;w++) assert(p.data[w]==(0xa1647930U^w));
	auto& r=p.result.transfer; r={}; r.version=1; r.state=TT_STORE_PASSED; r.requests=r.completed=6;
	r.wrote_device=r.host_write_issued=r.host_visible=r.readback_complete=1;
	for (unsigned step=0;step<6;step++) for (unsigned w=0;w<4;w++) {
		r.before[step][w]=UINT32_MAX-2; r.after[step][w]=r.before[step][w]+(w==1?64*p.pages:1);
	}
	p.result.pages=p.pages; p.result.host_words=p.pages*1024; p.result.host_ready=p.result.l1_prepared=1;
	sUploads++;
	if (sFault==1) { sFailed=true; errno=EIO; return -1; }
	if (sFault==2) {
		sFailed=true;
		switch (sField) {
			case 0:r.version=2;break; case 1:r.state=TT_STORE_CHANGED;break;
			case 2:r.requests=5;break; case 3:r.completed=5;break; case 4:r.wrote_device=0;break;
			case 5:r.host_write_issued=0;break; case 6:r.host_visible=0;break;
			case 7:r.readback_complete=0;break; case 8:r.mismatches=1;break;
			case 9:r.recovery_required=1;break; case 10:p.result.pages=0;break;
			case 11:p.result.host_words=0;break; case 12:p.result.host_ready=0;break;
			case 13:p.result.l1_prepared=0;break;
			default:r.after[(sField-14)/4][(sField-14)%4]++;break;
		}
	}
	return 0;
}
#define open MockOpen
#define close MockClose
#define ioctl MockIoctl
#include "../TenstorrentHaikuResident.h"
#undef ioctl
#undef close
#undef open
static void Reset(unsigned fault=0,unsigned field=0) { sCalls=sEnds=sCloses=sUploads=0; sFault=fault; sField=field; sFailed=sMissingCapability=false; }
static bool Open(PioneerResidentDevice& device,unsigned mode=1) { return device.Open(32768,false,true,true,true,true,true,true,true,mode); }
int main()
{
	uint32_t data[4096]; for (unsigned w=0;w<4096;w++) data[w]=0xa1647930U^w;
	for (unsigned pages=1;pages<=4;pages++) {
		Reset(); { PioneerResidentDevice device; assert(Open(device)); assert(device.UploadPages(4096,data,pages)); assert(device.uploads==pages); assert(device.Finish()); }
		assert(sEnds==1 && sCloses==1 && sUploads==1);
	}
	for (unsigned mode=1;mode<=2;mode++) for (unsigned fault=1;fault<=2;fault++) for (unsigned field=0;field<(fault==1?1U:38U);field++) {
		Reset(fault,field);
		{ PioneerResidentDevice device; assert(Open(device,mode)); assert(!device.UploadPages(4096,data,4)); unsigned calls=sCalls;
			assert(!device.UploadPages(4096,data,4) && !device.Finish() && sCalls==calls && !device.uploads); }
		assert(sFailed && !sEnds && sCloses==1 && sUploads==1);
	}
	Reset(); sMissingCapability=true;
	{ PioneerResidentDevice device; assert(!Open(device)); }
	assert(sCalls==1 && !sUploads && !sEnds && sCloses==1);
	Reset(); sMissingCapability=true;
	{ PioneerResidentDevice device; assert(Open(device,2)); assert(device.UploadPages(4096,data,4) && device.uploads==4 && device.Finish()); }
	assert(sCalls==7 && sUploads==4 && sEnds==1 && sCloses==1);
	Reset(); { PioneerResidentDevice device; assert(!Open(device,3)); } assert(!sCalls && !sCloses);
	Reset(); { PioneerResidentDevice device; assert(Open(device));
		assert(!device.UploadPages(4096,data,0) && !device.UploadPages(4096,data,5) && !device.UploadPages(4096,NULL,4));
		assert(sCalls==2 && !sUploads && device.Finish()); }
	puts("PASS: REAL bulk client against synthetic syscalls, all1..4 page sizes, exact wrapped counters, all38 proof-field failures and ioctl error => close-only/no fallback; NO hardware");
}
