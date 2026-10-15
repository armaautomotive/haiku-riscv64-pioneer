/* SPDX-License-Identifier: MIT */
#include <cassert>
#include <cstring>
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeHostPathProbe.h"
struct HostMock {
	uint32_t source[16]={0x6005,0x0a418820,0x1ac4a166,0x127541ee,0x1e280440,0x1662b5c3,0x147,
		0x0a418820,0x16a4a0e6,0x06a12c20,0x0c72a089,0x0e548c20,2,0,3,0};
	uint32_t tlb[2][3]={{1,2,3},{4,5,6}};
	unsigned writes=0,drop=0,siiReads=0,resetReads=0;
	bool badInbound=false,badSii=false,changed=false,sourceChanged=false,active=false,restoreFails=false;
	uint32_t DramTlbRead(unsigned s,unsigned w) { assert(s<2 && w<3); return tlb[s][w]; }
	void DramTlbWrite(unsigned s,unsigned w,uint32_t value) {
		assert(s<2 && w<3);
		if (++writes!=drop && !(restoreFails && value==1+3*s+w)) tlb[s][w]=value;
	}
	uint32_t ResetRead() { resetReads++; return active?0:0x47800; }
	uint32_t SourceRead(unsigned w) { assert(w<16); return source[w]; }
	uint32_t ComputeNocRead(uint32_t offset) {
		for (unsigned w=0;w<3;w++) assert(tlb[1][w]==BlackholeDramFixedTlbWord(0xffa00000,1,2,true,w));
		for (unsigned w=0;w<16;w++) if (offset==BlackholeDramSourceOffset(w)) return source[w]^(sourceChanged?1:0);
		assert(false); return 0;
	}
	uint32_t HostInboundRead(unsigned w) { assert(w<2); return w==0?(badInbound?0:0x2f):0; }
	uint32_t HostSiiRead(unsigned w) {
		assert(w<8 && resetReads>=240);
		assert(tlb[0][0]==0xffffff80 && tlb[0][1]==0x010017ff && tlb[0][2]==0xa40);
		siiReads++;
		const uint32_t expected[]={0x400000,0x400200,0x400100,0x400300,0,0x200,0x100,0x300};
		return expected[w]^((badSii || (changed && siiReads>8))?1:0);
	}
	tt_dram_samples_result Baseline() {
		tt_dram_samples_result b={}; b.version=1; b.state=TT_DRAM_SAMPLE_ROUTES_OK;
		b.routes_verified=7; b.data_restored=b.tlb_restored=1; memcpy(b.source,source,sizeof(source)); return b;
	}
	tt_compute_noc_result Compute() {
		tt_compute_noc_result n={}; n.version=1; n.state=TT_COMPUTE_NOC_PASSED; n.tlb_restored=1;
		memcpy(n.source[0],source,sizeof(source)); return n;
	}
	void Restored() { for (unsigned s=0;s<2;s++) for (unsigned w=0;w<3;w++) assert(tlb[s][w]==1+3*s+w); }
};
int main()
{
	uint64_t decoded=(uint64_t(BlackholeHostSiiTlbWord(1)&0x7ff)<<53)
		|(uint64_t(BlackholeHostSiiTlbWord(0))<<21);
	assert(decoded==UINT64_C(0xfffffffff0000000));
	HostMock io; auto b=io.Baseline(); auto n=io.Compute(); tt_host_path_result r;
	assert(BlackholeHostPathProbe(io,b,n,0x18611002,r) && io.siiReads==24); io.Restored();
	unsigned writes=io.writes;
	for (unsigned drop=1;drop<=writes;drop++) {
		io={}; b=io.Baseline(); n=io.Compute(); io.drop=drop;
		BlackholeHostPathProbe(io,b,n,0x18611002,r); assert(r.tlb_restored && !r.recovery_required); io.Restored();
	}
	for (unsigned fault=0;fault<6;fault++) {
		io={}; b=io.Baseline(); n=io.Compute(); io.badInbound=fault==0; io.badSii=fault==1;
		io.changed=fault==2; io.sourceChanged=fault==3; io.active=fault==4; io.restoreFails=fault==5;
		assert(!BlackholeHostPathProbe(io,b,n,0x18611002,r));
		if (fault!=5) io.Restored();
		if (fault==3 || fault==5) assert(r.recovery_required);
		if (fault==4) assert(io.siiReads==0);
	}
	io={}; b=io.Baseline(); n=io.Compute();
	assert(!BlackholeHostPathProbe(io,b,n,0x1861100b,r) && io.writes==0);
	io={}; b=io.Baseline(); n=io.Compute(); n.source[0][14]|=1U<<24;
	assert(!BlackholeHostPathProbe(io,b,n,0x18611002,r) && io.writes==0);
}
