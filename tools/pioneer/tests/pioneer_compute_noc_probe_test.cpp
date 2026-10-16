/* SPDX-License-Identifier: MIT */
#include <cassert>
#include <cstring>
#include <initializer_list>
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeComputeNocProbe.h"
struct Mock {
	uint32_t source[16]={0x6005,0x0a418820,0x1ac4a166,0x127541ee,0x1e280440,0x1662b5c3,0x147,
		0x0a418820,0x16a4a0e6,0x06a12c20,0x0c72a089,0x0e548c20,2,0,3,0};
	uint32_t tlb[2][3]={{1,2,3},{4,5,6}};
	unsigned writes=0,reads=0,computeReads=0,drop=0,sourceReads=0;
	unsigned targetY=2;
	bool badId=false,busy=false,pending=false,changed=false,pcieChanged=false,restoreFails=false,coreActive=false;
	uint32_t DramTlbRead(unsigned slot,unsigned word) { assert(slot<2 && word<3); return tlb[slot][word]; }
	void DramTlbWrite(unsigned slot,unsigned word,uint32_t value) {
		assert(slot==1 && word<3); writes++;
		if (writes!=drop && !(restoreFails && value==word+4)) tlb[slot][word]=value;
	}
	uint32_t ResetRead() { reads++; return coreActive?0:0x47800; }
	uint32_t SourceRead(unsigned w) { assert(w<16); return source[w]^(pcieChanged?1:0); }
	uint32_t ComputeNocRead(uint32_t offset) {
		assert(BlackholeComputeNocReadOffset(offset));
		for (unsigned w=0;w<3;w++) assert(tlb[1][w]==BlackholeDramFixedTlbWord(0xffa00000,1,targetY,true,w));
		assert(reads==240); computeReads++;
		if (offset==0x44) return badId?0:0x1c611001|(targetY<<6);
		if (offset==0x48) return 0x10000;
		for (unsigned w=0;w<16;w++) if (offset==BlackholeDramSourceOffset(w)) {
			sourceReads++; return source[w]^((changed && sourceReads>16)?1:0);
		}
		for (unsigned i=0;i<4;i++) if (offset==i*0x800+0x40) return busy?1:0;
		return pending?1:0;
	}
	tt_dram_samples_result Baseline() {
		tt_dram_samples_result r={}; r.version=1; r.state=TT_DRAM_SAMPLE_ROUTES_OK;
		r.routes_verified=7; r.data_restored=r.tlb_restored=1;
		memcpy(r.source,source,sizeof(source)); assert(BlackholeDramBuildRoutes(r)); return r;
	}
	void Restored() { for (unsigned w=0;w<3;w++) { assert(tlb[0][w]==w+1); assert(tlb[1][w]==w+4); } }
};
int main()
{
	Mock io; tt_compute_noc_result r; auto baseline=io.Baseline();
	assert(BlackholeComputeNocProbe(io,baseline,r));
	assert(r.version==1 && r.state==TT_COMPUTE_NOC_PASSED && r.tlb_restored && !r.recovery_required);
	assert(baseline.cores_checked==120 && io.computeReads==114); io.Restored();
	unsigned writes=io.writes;
	for (unsigned drop=1;drop<=writes;drop++) {
		io={}; io.drop=drop; baseline=io.Baseline();
		BlackholeComputeNocProbe(io,baseline,r); assert(r.tlb_restored && !r.recovery_required); io.Restored();
	}
	io={}; io.badId=true; baseline=io.Baseline(); assert(!BlackholeComputeNocProbe(io,baseline,r)); io.Restored();
	io={}; io.busy=true; baseline=io.Baseline(); assert(!BlackholeComputeNocProbe(io,baseline,r) && r.state==TT_COMPUTE_NOC_BUSY); io.Restored();
	io={}; io.pending=true; baseline=io.Baseline(); assert(!BlackholeComputeNocProbe(io,baseline,r) && r.state==TT_COMPUTE_NOC_BUSY); io.Restored();
	io={}; io.changed=true; baseline=io.Baseline(); assert(!BlackholeComputeNocProbe(io,baseline,r) && r.state==TT_COMPUTE_NOC_CHANGED); io.Restored();
	io={}; baseline=io.Baseline(); io.pcieChanged=true; assert(!BlackholeComputeNocProbe(io,baseline,r) && r.recovery_required); io.Restored();
	io={}; io.restoreFails=true; baseline=io.Baseline(); assert(!BlackholeComputeNocProbe(io,baseline,r) && r.state==TT_COMPUTE_NOC_RESTORE_FAILED && r.recovery_required);
	io={}; io.coreActive=true; baseline=io.Baseline(); assert(!BlackholeComputeNocProbe(io,baseline,r) && io.computeReads==0); io.Restored();
	io={}; baseline=io.Baseline(); baseline.routes_verified=6;
	assert(!BlackholeComputeNocProbe(io,baseline,r) && io.writes==0 && io.computeReads==0);
	io={}; baseline=io.Baseline(); io.source[3]^=1; // compute-source route differs from PCIe proof
	assert(!BlackholeComputeNocProbe(io,baseline,r)); io.Restored();
	for (unsigned offset=0;offset<0x2000;offset++) {
		if (!BlackholeComputeNocReadOffset(offset)) continue;
		assert((offset&3)==0 && offset!=0x60 && offset!=0x1000 && offset!=0x1c);
	}
	// The second-core probe is still read-only and cannot accept arbitrary
	// coordinates. It has its own response version and identical rollback.
	io={}; io.targetY=3; baseline=io.Baseline();
	assert(BlackholeComputeNocProbe(io,baseline,r,3) && r.version==2 && r.state==TT_COMPUTE_NOC_PASSED && r.tlb_restored);
	assert(baseline.cores_checked==120 && io.computeReads==114 && (r.node[0][0]&0xfff)==0xc1); io.Restored();
	writes=io.writes;
	for (unsigned drop=1;drop<=writes;drop++) {
		io={}; io.targetY=3; io.drop=drop; baseline=io.Baseline();
		BlackholeComputeNocProbe(io,baseline,r,3); assert(r.tlb_restored && !r.recovery_required); io.Restored();
	}
	for (unsigned fault=0;fault<7;fault++) {
		io={}; io.targetY=3; baseline=io.Baseline();
		io.badId=fault==0; io.busy=fault==1; io.pending=fault==2; io.changed=fault==3;
		io.pcieChanged=fault==4; io.restoreFails=fault==5; io.coreActive=fault==6;
		assert(!BlackholeComputeNocProbe(io,baseline,r,3));
		assert(bool(r.recovery_required)==(fault==4 || fault==5));
		if (fault!=5) io.Restored();
	}
	for (unsigned invalid:{0U,1U,4U,11U,63U,~0U}) {
		io={}; baseline=io.Baseline();
		assert(!BlackholeComputeNocProbe(io,baseline,r,invalid) && !io.writes && !io.reads && !io.computeReads && r.tlb_restored);
		assert(!BlackholeComputeNocRouteValid(io.source,invalid));
	}
}
