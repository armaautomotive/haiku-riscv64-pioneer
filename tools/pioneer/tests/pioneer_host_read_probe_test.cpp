/* SPDX-License-Identifier: MIT */
#define PIONEER_RESIDENT_READ_MOCK_ONLY
#include "pioneer_resident_read_probe_test.cpp"
#define PIONEER_HOST_PAGE_MOCK_ONLY
#include "pioneer_host_page_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeHostReadProbe.h"
struct HostReadMock : Mock,PageMock {
	uint32_t pciCommand[2]={0x402,0x402};
	uint32_t backup[1024]={},destination[1024]={};
	unsigned transferBytes=64;
	unsigned bmeWrites=0;
	bool quarantined=false,clearFails=false,hostCorrupt=false,lateMapChange=false,pciePending=false;
	bool badHdma=false,badSii=false,backupFail=false;
	HostReadMock() { for (unsigned w=0;w<1024;w++) destination[w]=100+w; }
	uint32_t* HostReadBackup(unsigned words) { assert(words==transferBytes/4); return backupFail?nullptr:backup; }
	uint32_t ResidentL1Read(unsigned w) {
		assert(w<transferBytes/4); Window(0,0,1,2,false); return destination[w];
	}
	void ResidentL1Write(unsigned w,uint32_t value) {
		assert(w<transferBytes/4); Window(0,0,1,2,false);
		if (requests) { assert(done); postIssueWrites++; }
		if (++Mock::writes!=Mock::drop && !(restoreFails && value==100+w)) destination[w]=value;
	}
	uint32_t HostPciRead(unsigned peer,uint16_t offset,uint8_t size) {
		assert(peer<2);
		if (offset==4) { assert(size==2); return pciCommand[peer]; }
		if (offset==0x34) { assert(size==1); return 0x70; }
		if (offset==0x70) { assert(size==2); return 0x10; }
		if (offset==0x72) { assert(size==2); return 2; }
		if (offset==0x7a) { assert(size==2); return pciePending && requests?0x20:0; }
		assert(false); return 0;
	}
	void HostPciWrite(unsigned peer,uint16_t offset,uint8_t size,uint32_t value) {
		assert(peer<2 && offset==4 && size==2 && (value==0x402 || value==0x406));
		if (value==0x406) { assert(!requests && PageMock::allocated && !cached && config==0x16005); bmeWrites++; }
		if (++Mock::writes==Mock::drop || (clearFails && bmeWrites && value==0x402)) return;
		pciCommand[peer]=value;
	}
	uint32_t HostHdmaRead(unsigned w) {
		assert(w<4);
		for (unsigned k=0;k<3;k++) assert(tlb[0][k]==BlackholeHostDbiTlbWord(k));
		return w%2?3:badHdma?1:0;
	}
	uint32_t HostSiiRead(unsigned w) {
		assert(w<8);
		for (unsigned k=0;k<3;k++) assert(tlb[0][k]==BlackholeHostSiiTlbWord(k));
		return (w==0?0x400000U:0U)^(badSii?1:0);
	}
	uint32_t HostInboundRead(unsigned w) { assert(w<2); return w==0?0x2f:0; }
	int32_t HostPageMap(uint64_t& address,uint64_t& length) {
		int32_t result=PageMock::HostPageMap(address,length);
		if (lateMapChange && requests) address+=4096;
		return result;
	}
	void QuarantineHostPage() { assert(PageMock::allocated && !releases); quarantined=true; }
	void ResidentNocWrite(uint32_t offset,uint32_t value) {
		if (offset!=0x40) { Mock::ResidentNocWrite(offset,value); return; }
		Window(1,0xffa00000,1,2,false);
		assert(value==1 && !requests && config==0x16005 && pciCommand[0]==0x406 && pciCommand[1]==0x406);
		unsigned sourceOffset=transferBytes==64?128:0;
		assert(command[0]==uint32_t(physical+sourceOffset) && command[1]==uint32_t((physical+sourceOffset)>>32));
		const uint32_t expected[]={19|(24<<6),0x180000-transferBytes,0,1|(2<<6),0,0,transferBytes,0};
		assert(!memcmp(command+2,expected,sizeof(expected)) && !cached && PageMock::allocated);
		if (++Mock::writes==Mock::drop) return;
		requests++;
		if (acceptedOnly) { counters[2]++; counters[3]++; }
		if (!timeout && !acceptedOnly) {
			done=true; counters[0]++; counters[1]+=transferBytes/64+(badFlits?1:0); counters[2]++; counters[3]++;
			for (unsigned w=0;w<transferBytes/4;w++) destination[w]=page[sourceOffset/4+w]^(badData?1:0);
			if (hostCorrupt) page[0]^=1;
		}
	}
	tt_host_path_result Path() {
		tt_host_path_result p={}; p.version=1; p.state=TT_HOST_PATH_PASSED; p.tlb_restored=1;
		p.sii[0][0]=0x400000; p.inbound[0][0]=0x2f; return p;
	}
	tt_host_dma_audit_result Audit() {
		tt_host_dma_audit_result a={}; a.version=1; a.state=TT_HOST_DMA_AUDIT_PASSED; a.tlb_restored=1;
		a.hdma[0][1]=a.hdma[0][3]=3; return a;
	}
	void Safe() {
		Mock::Restored(); assert(pciCommand[0]==0x402 && pciCommand[1]==0x402);
		for (unsigned w=0;w<1024;w++) assert(destination[w]==100+w);
		assert(!quarantined && !PageMock::allocated && releases==1);
	}
};
#ifndef PIONEER_HOST_READ_MOCK_ONLY
int main()
{
	HostReadMock io; auto b=io.Baseline(); auto n=io.Compute(); auto p=io.Path(); auto a=io.Audit(); tt_host_read_result r;
	assert(BlackholeHostReadProbe(io,b,n,p,a,r) && r.issued && r.completed && r.page.released && r.bme_enabled==3);
	assert(!r.recovery_required && !r.quarantined && io.bmeWrites==2 && io.requests==1); io.Safe();
	unsigned writes=io.Mock::writes;
	for (unsigned drop=1;drop<=writes;drop++) {
		io={}; b=io.Baseline(); io.Mock::drop=drop;
		BlackholeHostReadProbe(io,b,n,p,a,r);
		assert(io.requests<=1);
		if (r.recovery_required) assert(r.quarantined && io.quarantined && io.PageMock::allocated && !io.releases);
		else io.Safe();
		if (r.issued && !r.completed) assert(r.recovery_required && !io.postIssueWrites);
	}
	for (unsigned fault=0;fault<8;fault++) {
		io={}; b=io.Baseline();
		io.timeout=fault==0; io.acceptedOnly=fault==1; io.badFlits=fault==2; io.changed=fault==3;
		io.clearFails=fault==4; io.restoreFails=fault==5; io.lateMapChange=fault==6; io.pciePending=fault==7;
		assert(!BlackholeHostReadProbe(io,b,n,p,a,r));
		assert(r.recovery_required && r.quarantined && io.quarantined && io.PageMock::allocated && !io.releases);
		if (fault!=5) assert(!io.postIssueWrites);
	}
	io={}; b=io.Baseline(); io.badData=true;
	assert(!BlackholeHostReadProbe(io,b,n,p,a,r) && r.completed && r.mismatches==16 && !r.recovery_required); io.Safe();
	io={}; b=io.Baseline(); io.hostCorrupt=true;
	assert(!BlackholeHostReadProbe(io,b,n,p,a,r) && r.completed && r.host_mismatches==1 && !r.recovery_required); io.Safe();
	for (unsigned fault=0;fault<5;fault++) {
		io={}; b=io.Baseline(); io.allocFail=fault==0; io.badHdma=fault==1; io.badSii=fault==2;
		io.active=fault==3; io.irq=fault==4;
		assert(!BlackholeHostReadProbe(io,b,n,p,a,r) && !io.requests && !io.bmeWrites && !r.recovery_required);
		if (fault==0) assert(!io.releases); else io.Safe();
	}
	io={}; b=io.Baseline(); io.physical=UINT64_C(0xfffffffff000);
	for (unsigned w=0;w<4;w++) io.counters[w]=UINT32_MAX;
	assert(BlackholeHostReadProbe(io,b,n,p,a,r)); io.Safe();
	io={}; b=io.Baseline(); a.recovery_required=1;
	assert(!BlackholeHostReadProbe(io,b,n,p,a,r) && !io.Mock::writes && !io.PageMock::allocated);
}
#endif
