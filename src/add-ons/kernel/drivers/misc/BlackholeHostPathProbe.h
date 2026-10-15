/* SPDX-License-Identifier: MIT */
#ifndef BLACKHOLE_HOST_PATH_PROBE_H
#define BLACKHOLE_HOST_PATH_PROBE_H
#include "BlackholeComputeNocProbe.h"
enum {
	TT_HOST_PATH_REFUSED=0,TT_HOST_PATH_PASSED=1,TT_HOST_PATH_CONFIG_FAILED=2,
	TT_HOST_PATH_BAD_ROUTE=3,TT_HOST_PATH_BAD_INBOUND=4,TT_HOST_PATH_BAD_SII=5,
	TT_HOST_PATH_CHANGED=6,TT_HOST_PATH_RESTORE_FAILED=7
};
struct tt_host_path_result {
	uint32_t version,state,tlb_restored,recovery_required,connected_pcie;
	uint32_t inbound[3][2],sii[3][8];
	uint32_t tlb_before[2][3],tlb_after[2][3];
};
static inline bool BlackholeHostSourceRoute(const uint32_t* source)
{
	return BlackholeComputeNocRouteValid(source)
		&& BlackholeDramSourceX(source,19)==2 && BlackholeDramSourceY(source,24)==0
		&& (source[13]&(1U<<19))==0 && (source[14]&(1U<<24))==0;
}
static inline uint32_t BlackholeHostSiiTlbWord(unsigned word)
{
	// Full 64-bit *device-local* SII address, not a host physical pointer.
	// BAR0 TLB local address is 43 bits (address >> 21). PCIe1 is (2,0).
	const uint64_t base=UINT64_C(0xfffffffff0000000);
	if (word==0) return uint32_t(base>>21);
	if (word==1) return uint32_t((base>>53)&0x7ff)|(2U<<11)|(2U<<23);
	return word==2?0xa40:UINT32_MAX; // strict read-only, dynamic VC
}
// Read-only prerequisite only: no host memory allocation, bus mastering,
// initiator/PCIe-SII/inbound writes, core start or DMA transaction.
template<class IO>
bool BlackholeHostPathProbe(IO& io,tt_dram_samples_result& baseline,
	const tt_compute_noc_result& n,uint32_t connected,tt_host_path_result& r)
{
	r={}; r.version=1; r.tlb_restored=1; r.connected_pcie=connected;
	if (baseline.version!=1 || baseline.state!=TT_DRAM_SAMPLE_ROUTES_OK
		|| baseline.routes_verified!=7 || !baseline.data_restored || !baseline.tlb_restored || baseline.recovery_required
		|| n.version!=1 || n.state!=TT_COMPUTE_NOC_PASSED || !n.tlb_restored || n.recovery_required
		|| !BlackholeDramBuildRoutes(baseline)) return false;
	r.state=TT_HOST_PATH_BAD_ROUTE;
	if ((connected&0xfff)!=2 || !BlackholeHostSourceRoute(n.source[0])) return false;
	for (unsigned s=0;s<2;s++) for (unsigned w=0;w<3;w++) {
		r.tlb_before[s][w]=io.DramTlbRead(s,w);
		if (w==2 && (r.tlb_before[s][w]&0xffffc000)!=0) return false;
	}
	r.tlb_restored=0;
	do {
		r.state=TT_HOST_PATH_CONFIG_FAILED; baseline.cores_checked=0;
		if (!BlackholeDramAllCoresHeld(io,baseline)) break;
		for (unsigned w=0;w<3;w++) io.DramTlbWrite(0,w,BlackholeHostSiiTlbWord(w));
		bool configured=true;
		for (unsigned w=0;w<3;w++) configured &= io.DramTlbRead(0,w)==BlackholeHostSiiTlbWord(w);
		if (!configured) break;
		r.state=TT_HOST_PATH_PASSED;
		for (unsigned sample=0;sample<3;sample++) {
			for (unsigned w=0;w<2;w++) r.inbound[sample][w]=io.HostInboundRead(w);
			// Existing Haiku Cadence setup: 48-bit no-BAR-match identity map.
			if (r.inbound[sample][0]!=0x2f || r.inbound[sample][1]!=0) r.state=TT_HOST_PATH_BAD_INBOUND;
			for (unsigned w=0;w<8;w++) r.sii[sample][w]=io.HostSiiRead(w);
			// Pinned 18.5 firmware window 0: memory TLP, iATU bypass,
			// zero address prefix, no relaxed ordering/no-snoop/DBI flag.
			if (r.sii[sample][0]!=0x00400000) r.state=TT_HOST_PATH_BAD_SII;
			for (unsigned w=0;w<2;w++) if (r.inbound[sample][w]!=r.inbound[0][w]) r.state=TT_HOST_PATH_CHANGED;
			for (unsigned w=0;w<8;w++) if (r.sii[sample][w]!=r.sii[0][w]) r.state=TT_HOST_PATH_CHANGED;
		}
		bool stable=BlackholeDramSourceStable(io,baseline) && io.ResetRead()==0x47800;
		for (unsigned w=0;w<16;w++) stable &= io.ComputeNocRead(BlackholeDramSourceOffset(w))==n.source[0][w];
		if (!stable) { r.state=TT_HOST_PATH_CHANGED; r.recovery_required=1; }
	} while (false);
	for (unsigned attempt=0;attempt<2 && !r.tlb_restored;attempt++) {
		for (unsigned s=0;s<2;s++) for (unsigned w=0;w<3;w++) io.DramTlbWrite(s,w,r.tlb_before[s][w]);
		r.tlb_restored=1;
		for (unsigned s=0;s<2;s++) for (unsigned w=0;w<3;w++) {
			r.tlb_after[s][w]=io.DramTlbRead(s,w); r.tlb_restored &= r.tlb_after[s][w]==r.tlb_before[s][w];
		}
	}
	if (!r.tlb_restored) { r.state=TT_HOST_PATH_RESTORE_FAILED; r.recovery_required=1; }
	return r.state==TT_HOST_PATH_PASSED && r.tlb_restored && !r.recovery_required;
}
#endif
