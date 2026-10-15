/* SPDX-License-Identifier: MIT */
#ifndef BLACKHOLE_HOST_DMA_AUDIT_H
#define BLACKHOLE_HOST_DMA_AUDIT_H
#include "BlackholeHostPathProbe.h"
enum {
	TT_HOST_DMA_AUDIT_REFUSED=0,TT_HOST_DMA_AUDIT_PASSED=1,
	TT_HOST_DMA_AUDIT_CONFIG_FAILED=2,TT_HOST_DMA_AUDIT_BUSY=3,
	TT_HOST_DMA_AUDIT_CHANGED=4,TT_HOST_DMA_AUDIT_RESTORE_FAILED=5
};
struct tt_host_dma_audit_result {
	uint32_t version,state,tlb_restored,recovery_required;
	uint32_t pcie_status[3][2],hdma[3][4];
	uint32_t tlb_before[2][3],tlb_after[2][3];
};
static inline uint32_t BlackholeHostDbiTlbWord(unsigned word)
{
	// Documented NoC window 62 routes to DBI. This is device-local, not
	// a host pointer. The only target offsets are firmware's HDMA ch0
	// enable/status: 0x380000/80 and 0x380100/180. Never ring a doorbell.
	const uint64_t base=UINT64_C(0xf800000000200000);
	if (word==0) return uint32_t(base>>21);
	if (word==1) return uint32_t((base>>53)&0x7ff)|(2U<<11)|(2U<<23);
	return word==2?0xa40:UINT32_MAX;
}
static inline uint32_t BlackholeHostHdmaOffset(unsigned word)
{
	const uint32_t offsets[]={0,0x80,0x100,0x180};
	return word<4?offsets[word]:UINT32_MAX;
}
template<class IO>
bool BlackholeHostPcieStatus(IO& io,unsigned peer,uint32_t& status)
{
	uint32_t cap=io.HostPciRead(peer,0x34,1);
	uint64_t visited=0; bool found=false;
	for (unsigned hops=0;cap && hops<48;hops++) {
		if (cap<0x40 || cap>0xfc || (cap&3) || (visited&(UINT64_C(1)<<(cap/4)))) return false;
		visited|=UINT64_C(1)<<(cap/4);
		uint32_t header=io.HostPciRead(peer,cap,2);
		if ((header&0xff)==0x10) {
			if (found || cap>0xf4) return false;
			uint32_t version=io.HostPciRead(peer,cap+2,2)&15;
			if (version!=1 && version!=2) return false;
			status=io.HostPciRead(peer,cap+0xa,2); found=true;
		}
		cap=(header>>8)&0xff;
	}
	// Device Status bits 0..3 are PCIe errors; bit 5 is Transactions
	// Pending. Reading is non-destructive; never acknowledge/clear errors.
	return cap==0 && found && (status&0x2f)==0 && status<=0xffff;
}
template<class IO>
bool BlackholeHostDmaAudit(IO& io,tt_dram_samples_result& baseline,
	const tt_compute_noc_result& n,const tt_host_path_result& path,tt_host_dma_audit_result& r)
{
	r={}; r.version=1; r.tlb_restored=1;
	if (path.version!=1 || path.state!=TT_HOST_PATH_PASSED || !path.tlb_restored || path.recovery_required
		|| baseline.version!=1 || baseline.state!=TT_DRAM_SAMPLE_ROUTES_OK || baseline.routes_verified!=7
		|| !baseline.data_restored || !baseline.tlb_restored || baseline.recovery_required
		|| n.version!=1 || n.state!=TT_COMPUTE_NOC_PASSED || !n.tlb_restored || n.recovery_required
		|| !BlackholeHostSourceRoute(n.source[0]) || !BlackholeDramBuildRoutes(baseline)) return false;
	for (unsigned s=0;s<2;s++) for (unsigned w=0;w<3;w++) {
		r.tlb_before[s][w]=io.DramTlbRead(s,w);
		if (w==2 && (r.tlb_before[s][w]&0xffffc000)) return false;
	}
	r.tlb_restored=0;
	do {
		r.state=TT_HOST_DMA_AUDIT_CONFIG_FAILED; baseline.cores_checked=0;
		if (!BlackholeDramAllCoresHeld(io,baseline)
			|| !BlackholeDramConfigure(io,1,0xffa00000,1,2,true)) break;
		for (unsigned w=0;w<3;w++) io.DramTlbWrite(0,w,BlackholeHostDbiTlbWord(w));
		bool mapped=true;
		for (unsigned w=0;w<3;w++) mapped &= io.DramTlbRead(0,w)==BlackholeHostDbiTlbWord(w);
		if (!mapped) break;
		r.state=TT_HOST_DMA_AUDIT_PASSED;
		for (unsigned sample=0;sample<3;sample++) {
			for (unsigned peer=0;peer<2;peer++) {
				if (io.HostPciRead(peer,4,2)!=0x402 || !BlackholeHostPcieStatus(io,peer,r.pcie_status[sample][peer]))
					r.state=TT_HOST_DMA_AUDIT_BUSY;
				if (r.pcie_status[sample][peer]!=r.pcie_status[0][peer]) r.state=TT_HOST_DMA_AUDIT_CHANGED;
			}
			for (unsigned w=0;w<4;w++) {
				r.hdma[sample][w]=io.HostHdmaRead(w);
				// Pinned firmware only uses read/write channel 0. Require both
				// disabled, and reset(0) or stopped(3), not running/aborted/unknown.
				if ((w%2==0 && r.hdma[sample][w]!=0)
					|| (w%2==1 && r.hdma[sample][w]!=0 && r.hdma[sample][w]!=3)) r.state=TT_HOST_DMA_AUDIT_BUSY;
				if (r.hdma[sample][w]!=r.hdma[0][w]) r.state=TT_HOST_DMA_AUDIT_CHANGED;
			}
		}
		bool stable=BlackholeDramSourceStable(io,baseline) && io.ResetRead()==0x47800;
		for (unsigned w=0;w<16;w++) stable &= io.ComputeNocRead(BlackholeDramSourceOffset(w))==n.source[0][w];
		if (!stable) { r.state=TT_HOST_DMA_AUDIT_CHANGED; r.recovery_required=1; }
	} while (false);
	for (unsigned attempt=0;attempt<2 && !r.tlb_restored;attempt++) {
		for (unsigned s=0;s<2;s++) for (unsigned w=0;w<3;w++) io.DramTlbWrite(s,w,r.tlb_before[s][w]);
		r.tlb_restored=1;
		for (unsigned s=0;s<2;s++) for (unsigned w=0;w<3;w++) {
			r.tlb_after[s][w]=io.DramTlbRead(s,w); r.tlb_restored &= r.tlb_after[s][w]==r.tlb_before[s][w];
		}
	}
	if (!r.tlb_restored) { r.state=TT_HOST_DMA_AUDIT_RESTORE_FAILED; r.recovery_required=1; }
	return r.state==TT_HOST_DMA_AUDIT_PASSED && !r.recovery_required;
}
#endif
