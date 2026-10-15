/* SPDX-License-Identifier: MIT */
#define PIONEER_HOST_PATH_MOCK_ONLY
#include "pioneer_host_path_probe_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeHostDmaAudit.h"
struct DmaMock : HostMock {
	uint32_t status[2]={},hdma[4]={};
	unsigned hdmaReads=0;
	bool commandBad=false,capBad=false,capLoop=false,capMissing=false,hdmaChanged=false;
	uint32_t HostPciRead(unsigned peer,uint16_t offset,uint8_t size) {
		assert(peer<2);
		if (offset==4) { assert(size==2); return commandBad?0x406:0x402; }
		if (offset==0x34) { assert(size==1); return capMissing?0:capBad?0x41:0x70; }
		if (offset==0x70) { assert(size==2); return capLoop?0x7010:0x10; }
		if (offset==0x72) { assert(size==2); return 2; }
		if (offset==0x7a) { assert(size==2); return status[peer]; }
		assert(false); return 0;
	}
	uint32_t HostHdmaRead(unsigned word) {
		assert(word<4 && resetReads>=240);
		for (unsigned w=0;w<3;w++) assert(tlb[0][w]==BlackholeHostDbiTlbWord(w));
		hdmaReads++; return hdma[word]^((hdmaChanged && hdmaReads>4)?1:0);
	}
	tt_host_path_result Path() { tt_host_path_result p={}; p.version=1; p.state=TT_HOST_PATH_PASSED; p.tlb_restored=1; return p; }
};
int main()
{
	uint64_t decoded=(uint64_t(BlackholeHostDbiTlbWord(1)&0x7ff)<<53)|(uint64_t(BlackholeHostDbiTlbWord(0))<<21);
	assert(decoded==UINT64_C(0xf800000000200000));
	DmaMock io; auto b=io.Baseline(); auto n=io.Compute(); auto p=io.Path(); tt_host_dma_audit_result r;
	assert(BlackholeHostDmaAudit(io,b,n,p,r) && io.hdmaReads==12); io.Restored();
	unsigned writes=io.writes;
	for (unsigned drop=1;drop<=writes;drop++) {
		io={}; b=io.Baseline(); io.drop=drop;
		BlackholeHostDmaAudit(io,b,n,p,r);
		assert(r.tlb_restored && !r.recovery_required); io.Restored();
	}
	for (unsigned peer=0;peer<2;peer++) for (unsigned bit=0;bit<6;bit++) if (bit!=4) {
		io={}; b=io.Baseline(); io.status[peer]=1U<<bit;
		assert(!BlackholeHostDmaAudit(io,b,n,p,r) && r.state==TT_HOST_DMA_AUDIT_BUSY); io.Restored();
	}
	const uint32_t badValues[]={1,2,4,UINT32_MAX};
	for (unsigned word=0;word<4;word++) for (uint32_t value:badValues) {
		io={}; b=io.Baseline(); io.hdma[word]=value;
		assert(!BlackholeHostDmaAudit(io,b,n,p,r) && r.state==TT_HOST_DMA_AUDIT_BUSY); io.Restored();
	}
	io={}; b=io.Baseline(); io.hdma[1]=io.hdma[3]=3;
	assert(BlackholeHostDmaAudit(io,b,n,p,r)); io.Restored();
	for (unsigned fault=0;fault<9;fault++) {
		io={}; b=io.Baseline();
		io.commandBad=fault==0; io.capBad=fault==1; io.capLoop=fault==2; io.capMissing=fault==3;
		io.hdmaChanged=fault==4; io.sourceChanged=fault==5; io.active=fault==6; io.restoreFails=fault==7;
		if (fault==8) b.recovery_required=1;
		assert(!BlackholeHostDmaAudit(io,b,n,p,r));
		if (fault==5 || fault==7) assert(r.recovery_required);
		else { assert(!r.recovery_required); io.Restored(); }
		if (fault==6 || fault==8) assert(!io.hdmaReads);
	}
}
