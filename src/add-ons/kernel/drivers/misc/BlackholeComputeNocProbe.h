/* SPDX-License-Identifier: MIT */
#ifndef BLACKHOLE_COMPUTE_NOC_PROBE_H
#define BLACKHOLE_COMPUTE_NOC_PROBE_H
#include "BlackholeDramSamplesProbe.h"

// Read-only prerequisite for device-resident weights. A PCIe NIU route proof
// does NOT prove the compute tile uses the same translation/command state.
// Fixed physical Tensix (1,2), NoC0. No initiator/config/counter writes, no
// code upload, core release, GDDR payload access, bus mastering or host DMA.
enum {
	TT_COMPUTE_NOC_REFUSED=0, TT_COMPUTE_NOC_PASSED=1,
	TT_COMPUTE_NOC_CONFIG_FAILED=2, TT_COMPUTE_NOC_BAD_ID=3,
	TT_COMPUTE_NOC_BAD_ROUTE=4, TT_COMPUTE_NOC_BUSY=5,
	TT_COMPUTE_NOC_CHANGED=6, TT_COMPUTE_NOC_RESTORE_FAILED=7
};
struct tt_compute_noc_result {
	uint32_t version, state, tlb_restored, recovery_required;
	uint32_t node[3][2], source[3][TT_DRAM_SOURCE_WORDS];
	uint32_t commands[3][4], outstanding[3][16];
	uint32_t tlb_before[3], tlb_after[3];
};
static inline bool BlackholeComputeNocReadOffset(uint32_t offset)
{
	if (offset==0x44 || offset==0x48) return true;
	for (unsigned i=0;i<TT_DRAM_SOURCE_WORDS;i++)
		if (offset==BlackholeDramSourceOffset(i)) return true;
	for (unsigned i=0;i<4;i++) if (offset==i*0x800+0x40) return true;
	return offset>=0x240 && offset<0x280 && (offset&3)==0;
}
static inline bool BlackholeComputeNocRouteValid(const uint32_t* source)
{
	return BlackholeDramSourceRoutesBankZero(source)
		&& BlackholeDramSourceX(source,1)==1 && BlackholeDramSourceY(source,2)==2
		&& (source[13]&(1U<<1))==0 && (source[14]&(1U<<2))==0;
}
template<class IO>
bool BlackholeComputeNocProbe(IO& io,tt_dram_samples_result& baseline,
	tt_compute_noc_result& r)
{
	r={}; r.version=1; r.tlb_restored=1;
	if (baseline.version!=1 || baseline.state!=TT_DRAM_SAMPLE_ROUTES_OK
		|| baseline.routes_verified!=7 || !baseline.data_restored || !baseline.tlb_restored
		|| baseline.recovery_required || !BlackholeDramBuildRoutes(baseline)) return false;
	for (unsigned w=0;w<3;w++) r.tlb_before[w]=io.DramTlbRead(1,w);
	if ((r.tlb_before[2]&0xffffc000)!=0) return false;
	r.tlb_restored=0;
	do {
		if (!BlackholeDramAllCoresHeld(io,baseline)) break;
		r.state=TT_COMPUTE_NOC_CONFIG_FAILED;
		if (!BlackholeDramConfigure(io,1,0xffa00000,1,2,true)) break;
		r.state=TT_COMPUTE_NOC_PASSED;
		for (unsigned sample=0;sample<3;sample++) {
			r.node[sample][0]=io.ComputeNocRead(0x44);
			r.node[sample][1]=io.ComputeNocRead(0x48);
			if ((r.node[sample][0]&0xfff)!=0x81
				|| ((r.node[sample][0]>>12)&127)!=17
				|| ((r.node[sample][0]>>19)&127)!=12
				|| (r.node[sample][1]&0xffffff00)!=0x10000)
				r.state=TT_COMPUTE_NOC_BAD_ID;
			for (unsigned w=0;w<TT_DRAM_SOURCE_WORDS;w++)
				r.source[sample][w]=io.ComputeNocRead(BlackholeDramSourceOffset(w));
			if (!BlackholeComputeNocRouteValid(r.source[sample])) r.state=TT_COMPUTE_NOC_BAD_ROUTE;
			for (unsigned i=0;i<4;i++) {
				r.commands[sample][i]=io.ComputeNocRead(i*0x800+0x40);
				if (r.commands[sample][i]!=0) r.state=TT_COMPUTE_NOC_BUSY;
			}
			for (unsigned i=0;i<16;i++) {
				r.outstanding[sample][i]=io.ComputeNocRead(0x240+i*4);
				if ((r.outstanding[sample][i]&0xff)!=0) r.state=TT_COMPUTE_NOC_BUSY;
			}
			for (unsigned w=0;w<TT_DRAM_SOURCE_WORDS;w++)
				if (r.source[sample][w]!=r.source[0][w]) r.state=TT_COMPUTE_NOC_CHANGED;
			for (unsigned w=0;w<2;w++)
				if (r.node[sample][w]!=r.node[0][w]) r.state=TT_COMPUTE_NOC_CHANGED;
		}
		if (!BlackholeDramSourceStable(io,baseline)) {
			r.state=TT_COMPUTE_NOC_CHANGED; r.recovery_required=1;
		}
	} while (false);
	for (unsigned attempt=0;attempt<2 && !r.tlb_restored;attempt++) {
		for (unsigned w=0;w<3;w++) io.DramTlbWrite(1,w,r.tlb_before[w]);
		r.tlb_restored=1;
		for (unsigned w=0;w<3;w++) {
			r.tlb_after[w]=io.DramTlbRead(1,w);
			r.tlb_restored &= r.tlb_after[w]==r.tlb_before[w];
		}
	}
	if (!r.tlb_restored) { r.state=TT_COMPUTE_NOC_RESTORE_FAILED; r.recovery_required=1; }
	return r.state==TT_COMPUTE_NOC_PASSED && r.tlb_restored && !r.recovery_required;
}
#endif
