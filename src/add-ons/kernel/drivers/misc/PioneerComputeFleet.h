/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_COMPUTE_FLEET_H
#define PIONEER_COMPUTE_FLEET_H
#include "BlackholeComputeNocProbe.h"

// Private, immutable P100A topology, not a user-supplied coordinate API.
// Preserve worker0/1's historical coordinates. Logical columns are NOT
// physical columns: e.g. logical7 maps to physical11 on this harvested die.
enum { TT_PIONEER_WORKERS=120 };
static inline unsigned PioneerWorkerX(unsigned worker)
{
	const unsigned columns[]={1,2,3,4,5,6,7,10,11,12,13,14};
	return worker<TT_PIONEER_WORKERS?columns[worker/10]:UINT32_MAX;
}
static inline unsigned PioneerWorkerY(unsigned worker)
{
	return worker<TT_PIONEER_WORKERS?2+worker%10:UINT32_MAX;
}
static inline unsigned PioneerWorkerVersion(unsigned worker) { return worker<TT_PIONEER_WORKERS?0x100+worker:0; }
static inline bool PioneerWorkerRoute(const tt_dram_samples_result& baseline,const uint32_t* source,unsigned worker)
{
	if (worker>=TT_PIONEER_WORKERS || !BlackholeDramSourceRoutesBankZero(source)) return false;
	unsigned x=PioneerWorkerX(worker),y=PioneerWorkerY(worker);
	for (unsigned w=1;w<TT_DRAM_SOURCE_WORDS;w++) if (source[w]!=baseline.source[w]) return false;
	return !(source[13]&(1U<<x)) && !(source[14]&(1U<<y))
		&& BlackholeDramSourceY(source,y)==y;
}
// Caller must prove ALL workers held before walking the fleet. This helper
// reads one fixed NIU; it never clears counters, starts cores or sends NoC
// traffic. The physical ID is checked against the pinned translation table.
template<class IO>
bool PioneerWorkerNocProbe(IO& io,const tt_dram_samples_result& baseline,unsigned worker,tt_compute_noc_result& r)
{
	r={}; r.version=PioneerWorkerVersion(worker); r.tlb_restored=1;
	if (!r.version || baseline.version!=1 || baseline.state!=TT_DRAM_SAMPLE_ROUTES_OK
		|| baseline.routes_verified!=7 || !baseline.data_restored || !baseline.tlb_restored || baseline.recovery_required) return false;
	unsigned x=PioneerWorkerX(worker),y=PioneerWorkerY(worker);
	for (unsigned w=0;w<3;w++) r.tlb_before[w]=io.DramTlbRead(1,w);
	if (r.tlb_before[2]&0xffffc000) return false;
	r.tlb_restored=0;
	do {
		r.state=TT_COMPUTE_NOC_CONFIG_FAILED;
		if (!BlackholeDramConfigure(io,1,0xffa00000,x,y,true)) break;
		r.state=TT_COMPUTE_NOC_PASSED;
		for (unsigned sample=0;sample<3;sample++) {
			r.node[sample][0]=io.ComputeNocRead(0x44); r.node[sample][1]=io.ComputeNocRead(0x48);
			unsigned physical=BlackholeDramSourceX(baseline.source,x)|(y<<6);
			if ((r.node[sample][0]&0xfff)!=physical || ((r.node[sample][0]>>12)&127)!=17
				|| ((r.node[sample][0]>>19)&127)!=12 || (r.node[sample][1]&0xffffff00)!=0x10000)
				r.state=TT_COMPUTE_NOC_BAD_ID;
			for (unsigned w=0;w<TT_DRAM_SOURCE_WORDS;w++) r.source[sample][w]=io.ComputeNocRead(BlackholeDramSourceOffset(w));
			if (!PioneerWorkerRoute(baseline,r.source[sample],worker)) r.state=TT_COMPUTE_NOC_BAD_ROUTE;
			for (unsigned i=0;i<4;i++) {
				r.commands[sample][i]=io.ComputeNocRead(i*0x800+0x40);
				if (r.commands[sample][i]) r.state=TT_COMPUTE_NOC_BUSY;
			}
			for (unsigned i=0;i<16;i++) {
				r.outstanding[sample][i]=io.ComputeNocRead(0x240+i*4);
				if (r.outstanding[sample][i]&255) r.state=TT_COMPUTE_NOC_BUSY;
			}
			for (unsigned w=0;w<TT_DRAM_SOURCE_WORDS;w++) if (r.source[sample][w]!=r.source[0][w]) r.state=TT_COMPUTE_NOC_CHANGED;
			for (unsigned w=0;w<2;w++) if (r.node[sample][w]!=r.node[0][w]) r.state=TT_COMPUTE_NOC_CHANGED;
		}
		if (!BlackholeDramSourceStable(io,baseline)) { r.state=TT_COMPUTE_NOC_CHANGED; r.recovery_required=1; }
	} while (false);
	for (unsigned attempt=0;attempt<2 && !r.tlb_restored;attempt++) {
		for (unsigned w=0;w<3;w++) io.DramTlbWrite(1,w,r.tlb_before[w]);
		r.tlb_restored=1;
		for (unsigned w=0;w<3;w++) { r.tlb_after[w]=io.DramTlbRead(1,w); r.tlb_restored &= r.tlb_after[w]==r.tlb_before[w]; }
	}
	if (!r.tlb_restored) { r.state=TT_COMPUTE_NOC_RESTORE_FAILED; r.recovery_required=1; }
	return r.state==TT_COMPUTE_NOC_PASSED && r.tlb_restored && !r.recovery_required;
}
#endif
