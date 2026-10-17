/* SPDX-License-Identifier: MIT */
#include <cassert>
#include <cstring>
#include <cstdio>
#include <set>
#include "../../../src/add-ons/kernel/drivers/misc/PioneerComputeFleet.h"
struct FleetMock {
	uint32_t source[16]={0x6005,0x0a418820,0x1ac4a166,0x127541ee,0x1e280440,0x1662b5c3,0x147,
		0x0a418820,0x16a4a0e6,0x06a12c20,0x0c72a089,0x0e548c20,2,0,3,0};
	uint32_t tlb[2][3]={{1,2,3},{4,5,6}};
	unsigned worker=0,writes=0,drop=0,reads=0,samples=0;
	bool badId=false,badRoute=false,busy=false,changed=false,restoreFails=false,sourceChanged=false;
	uint32_t DramTlbRead(unsigned slot,unsigned word) { assert(slot==1 && word<3); return tlb[slot][word]; }
	void DramTlbWrite(unsigned slot,unsigned word,uint32_t value) {
		assert(slot==1 && word<3); writes++;
		if (writes!=drop && !(restoreFails && value==word+4)) tlb[slot][word]=value;
	}
	uint32_t SourceRead(unsigned word) { return source[word]^(sourceChanged?1:0); }
	uint32_t ComputeNocRead(uint32_t offset) {
		assert(BlackholeComputeNocReadOffset(offset)); reads++;
		unsigned x=PioneerWorkerX(worker),y=PioneerWorkerY(worker);
		for (unsigned w=0;w<3;w++) assert(tlb[1][w]==BlackholeDramFixedTlbWord(0xffa00000,x,y,true,w));
		if (offset==0x44) { samples++; return (badId?0:BlackholeDramSourceX(source,x)|(y<<6))|(17<<12)|(12<<19)|(1<<28); }
		if (offset==0x48) return 0x10000+worker;
		for (unsigned w=0;w<16;w++) if (offset==BlackholeDramSourceOffset(w))
			return source[w]^((badRoute && w==13)?1U<<x:0)^((changed && samples>1)?1:0);
		return busy?1:0;
	}
	tt_dram_samples_result Baseline() {
		tt_dram_samples_result r={}; r.version=1; r.state=TT_DRAM_SAMPLE_ROUTES_OK;
		r.routes_verified=7; r.data_restored=r.tlb_restored=1; memcpy(r.source,source,sizeof(source));
		assert(BlackholeDramBuildRoutes(r)); return r;
	}
	void Restored() { for (unsigned w=0;w<3;w++) assert(tlb[1][w]==w+4); }
};
int main() {
	std::set<unsigned> logical,physical;
	for (unsigned worker=0;worker<120;worker++) {
		FleetMock io; io.worker=worker; auto baseline=io.Baseline(); tt_compute_noc_result r;
		assert(PioneerWorkerNocProbe(io,baseline,worker,r)); io.Restored();
		assert(r.version==0x100+worker && r.state==1 && io.reads==114 && !r.recovery_required);
		assert(logical.insert(PioneerWorkerX(worker)|(PioneerWorkerY(worker)<<6)).second);
		assert(physical.insert(r.node[0][0]&0xfff).second);
		for (unsigned drop=1;drop<=io.writes;drop++) {
			FleetMock lost; lost.worker=worker; lost.drop=drop;
			PioneerWorkerNocProbe(lost,baseline,worker,r); lost.Restored(); assert(!r.recovery_required);
		}
		for (unsigned fault=0;fault<6;fault++) {
			FleetMock bad; bad.worker=worker;
			bad.badId=fault==0; bad.badRoute=fault==1; bad.busy=fault==2; bad.changed=fault==3;
			bad.restoreFails=fault==4; bad.sourceChanged=fault==5;
			assert(!PioneerWorkerNocProbe(bad,baseline,worker,r));
			assert(bool(r.recovery_required)==(fault>=4)); if (fault!=4) bad.Restored();
		}
	}
	assert(logical.size()==120 && physical.size()==120);
	for (unsigned worker:{120U,121U,~0U}) {
		FleetMock io; auto baseline=io.Baseline(); tt_compute_noc_result r;
		assert(!PioneerWorkerNocProbe(io,baseline,worker,r) && !io.reads && !io.writes);
	}
	puts("PASS offline fixed120-worker topology, physical IDs, route/idle stability and every lost TLB write; no hardware execution");
}
