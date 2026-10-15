/* SPDX-License-Identifier: MIT */
#ifndef BLACKHOLE_HOST_READ_PROBE_H
#define BLACKHOLE_HOST_READ_PROBE_H
#include "BlackholeHostDmaAudit.h"
#include "BlackholeResidentReadProbe.h"
#include "PioneerHostPage.h"
enum {
	TT_HOST_READ_REFUSED=0,TT_HOST_READ_PASSED=1,TT_HOST_READ_PAGE_FAILED=2,
	TT_HOST_READ_CONFIG_FAILED=3,TT_HOST_READ_CHANGED=4,TT_HOST_READ_TIMEOUT=5,
	TT_HOST_READ_MISMATCH=6,TT_HOST_READ_RESTORE_FAILED=7
};
struct tt_host_read_result {
	uint32_t version,state,phase,issued,completed,polls,mismatches,host_mismatches;
	uint32_t bme_enabled,bme_restored,l1_restored,niu_restored,tlb_restored,quarantined,recovery_required;
	uint32_t before[4],after[4],tlb_before[2][3],tlb_after[2][3];
	tt_host_page_result page;
};
template<class IO>
bool BlackholeHostReadPciQuiet(IO& io,uint32_t command)
{
	bool quiet=true;
	for (unsigned peer=0;peer<2;peer++) {
		uint32_t status=0;
		quiet &= io.HostPciRead(peer,4,2)==command && BlackholeHostPcieStatus(io,peer,status);
	}
	return quiet;
}
template<class IO>
bool BlackholeHostReadMapStable(IO& io,const tt_host_page_result& page)
{
	uint64_t physical=0,bytes=0;
	return io.HostPageMap(physical,bytes)==0 && physical==page.physical && bytes==page.bytes
		&& PioneerHostPageAddress(physical,bytes);
}
// One fixed NoC *read*: private host-page offset 128 -> sole stopped tile's
// last 64 L1 bytes. No host writes, firmware commands, DMA-engine doorbells,
// user addresses, arbitrary lengths, core starts or persistent allocation.
template<class IO>
bool BlackholeHostReadProbe(IO& io,tt_dram_samples_result& baseline,
	const tt_compute_noc_result& n,const tt_host_path_result& path,
	const tt_host_dma_audit_result& audit,tt_host_read_result& r)
{
	r={}; r.version=1; r.bme_restored=r.l1_restored=r.niu_restored=r.tlb_restored=1;
	if (audit.version!=1 || audit.state!=TT_HOST_DMA_AUDIT_PASSED || !audit.tlb_restored || audit.recovery_required
		|| path.version!=1 || path.state!=TT_HOST_PATH_PASSED || !path.tlb_restored || path.recovery_required
		|| baseline.version!=1 || baseline.state!=TT_DRAM_SAMPLE_ROUTES_OK || baseline.routes_verified!=7
		|| !baseline.data_restored || !baseline.tlb_restored || baseline.recovery_required
		|| n.version!=1 || n.state!=TT_COMPUTE_NOC_PASSED || !n.tlb_restored || n.recovery_required
		|| !BlackholeHostSourceRoute(n.source[0]) || !BlackholeDramBuildRoutes(baseline)) return false;
	r.state=TT_HOST_READ_PAGE_FAILED;
	bool prepared=PioneerPrepareHostPage(io,r.page);
	bool savedTlb=false,modified=false,configured=false,bmeTouched=false,canRestore=true;
	uint32_t saved[16]={},config=n.source[0][0];
	do {
		if (!prepared) break;
		r.state=TT_HOST_READ_CONFIG_FAILED; r.phase=1;
		for (unsigned s=0;s<2;s++) for (unsigned w=0;w<3;w++) r.tlb_before[s][w]=io.DramTlbRead(s,w);
		if ((r.tlb_before[0][2]&0xffffc000) || (r.tlb_before[1][2]&0xffffc000)) break;
		savedTlb=true; r.tlb_restored=0; baseline.cores_checked=0;
		if (!BlackholeDramAllCoresHeld(io,baseline)
			|| !BlackholeDramConfigure(io,1,0xffa00000,1,2,false)) break;
		if (!BlackholeDramSourceStable(io,baseline) || !BlackholeResidentSourceStable(io,n,config)
			|| !BlackholeResidentIdle(io) || (io.ResidentNocRead(0x178)&0xffff)) break;
		bool pristine=true;
		for (unsigned w=0;w<10;w++) pristine &= io.ResidentNocRead(w*4)==0;
		if (!pristine || !BlackholeHostReadPciQuiet(io,0x402)) break;
		// Recheck the firmware-used engines, SII window and root mapping
		// immediately before use. No register on the PCIe tile is written.
		r.phase=2;
		for (unsigned w=0;w<3;w++) io.DramTlbWrite(0,w,BlackholeHostDbiTlbWord(w));
		bool stable=true;
		for (unsigned w=0;w<3;w++) stable &= io.DramTlbRead(0,w)==BlackholeHostDbiTlbWord(w);
		if (!stable) break;
		for (unsigned w=0;w<4;w++) stable &= io.HostHdmaRead(w)==audit.hdma[0][w];
		for (unsigned w=0;w<3;w++) io.DramTlbWrite(0,w,BlackholeHostSiiTlbWord(w));
		bool mapped=true;
		for (unsigned w=0;w<3;w++) mapped &= io.DramTlbRead(0,w)==BlackholeHostSiiTlbWord(w);
		if (!mapped) break;
		for (unsigned w=0;w<8;w++) stable &= io.HostSiiRead(w)==path.sii[0][w];
		for (unsigned w=0;w<2;w++) stable &= io.HostInboundRead(w)==path.inbound[0][w];
		if (!stable || !BlackholeHostReadMapStable(io,r.page)) { r.state=TT_HOST_READ_CHANGED; break; }
		if (!BlackholeDramConfigure(io,0,0,1,2,false)) break;
		for (unsigned w=0;w<16;w++) saved[w]=io.ResidentL1Read(w);
		modified=true; r.l1_restored=0; r.phase=3;
		for (unsigned w=0;w<16;w++) io.ResidentL1Write(w,~PioneerHostPagePattern(1,32+w));
		bool poisoned=true;
		for (unsigned w=0;w<16;w++) poisoned &= io.ResidentL1Read(w)==~PioneerHostPagePattern(1,32+w);
		if (!poisoned) break;
		configured=true; r.niu_restored=0;
		io.ResidentNocWrite(0x100,config|0x10000);
		if (!BlackholeResidentSourceStable(io,n,config|0x10000)) break;
		uint64_t source=r.page.physical+128;
		const uint32_t command[]={uint32_t(source),uint32_t(source>>32),19|(24<<6),0x17ffc0,0,1|(2<<6),0,0,64,0};
		for (unsigned w=0;w<10;w++) io.ResidentNocWrite(w*4,command[w]);
		bool ready=BlackholeResidentIdle(io);
		for (unsigned w=0;w<10;w++) ready &= io.ResidentNocRead(w*4)==command[w];
		if (!ready || !BlackholeResidentSourceStable(io,n,config|0x10000)
			|| !BlackholeDramSourceStable(io,baseline) || !BlackholeHostReadMapStable(io,r.page)) break;
		for (unsigned w=0;w<TT_HOST_PAGE_WORDS;w++) r.host_mismatches+=io.HostPageRead(w)!=PioneerHostPagePattern(1,w);
		if (r.host_mismatches || !BlackholeHostReadPciQuiet(io,0x402)) break;
		// This narrowly scoped operation is the only policy allowing BME.
		// A failed/posted write is uncertain and must be cleared/read back.
		r.phase=4; bmeTouched=true; r.bme_restored=0;
		io.HostPciWrite(1,4,2,0x406);
		if (io.HostPciRead(1,4,2)!=0x406) break;
		r.bme_enabled|=2;
		io.HostPciWrite(0,4,2,0x406);
		if (io.HostPciRead(0,4,2)!=0x406) break;
		r.bme_enabled|=1;
		if (!BlackholeHostReadPciQuiet(io,0x406) || !BlackholeResidentIdle(io)
			|| !BlackholeResidentSourceStable(io,n,config|0x10000) || !BlackholeDramSourceStable(io,baseline)) break;
		for (unsigned w=0;w<4;w++) r.before[w]=io.ResidentNocRead(0x208+w*4);
		r.phase=5; r.issued=1; r.state=TT_HOST_READ_TIMEOUT;
		io.ResidentNocWrite(0x40,1);
		for (unsigned attempt=0;attempt<256;attempt++) {
			r.polls++;
			uint32_t pending=io.ResidentNocRead(0x40); // ordered BEFORE counters
			for (unsigned w=0;w<4;w++) r.after[w]=io.ResidentNocRead(0x208+w*4);
			if (!pending && uint32_t(r.after[0]-r.before[0])==1 && uint32_t(r.after[1]-r.before[1])==1
				&& uint32_t(r.after[2]-r.before[2])==1 && uint32_t(r.after[3]-r.before[3])==1
				&& BlackholeResidentIdle(io)) { r.completed=1; break; }
			io.Pause();
		}
		if (!r.completed) { canRestore=false; break; }
		r.phase=6;
		if (!BlackholeResidentSourceStable(io,n,config|0x10000) || !BlackholeDramSourceStable(io,baseline)
			|| !BlackholeHostReadPciQuiet(io,0x406) || !BlackholeHostReadMapStable(io,r.page)) {
			r.state=TT_HOST_READ_CHANGED; canRestore=false; break;
		}
		for (unsigned w=0;w<16;w++) r.mismatches+=io.ResidentL1Read(w)!=PioneerHostPagePattern(1,32+w);
		for (unsigned w=0;w<TT_HOST_PAGE_WORDS;w++) r.host_mismatches+=io.HostPageRead(w)!=PioneerHostPagePattern(1,w);
		r.state=r.mismatches || r.host_mismatches?TT_HOST_READ_MISMATCH:TT_HOST_READ_PASSED;
	} while (false);
	if (bmeTouched) {
		for (unsigned attempt=0;attempt<2 && !r.bme_restored;attempt++) {
			io.HostPciWrite(0,4,2,0x402); io.HostPciWrite(1,4,2,0x402);
			r.bme_restored=BlackholeHostReadPciQuiet(io,0x402);
		}
		canRestore &= r.bme_restored!=0;
	}
	// No cancellation, counter clear, reissue, reset or payload rewrite
	// after uncertain completion. Only BME-off and TLB/PCI control rollback.
	if (modified && canRestore) {
		canRestore=BlackholeResidentSourceStable(io,n,configured?config|0x10000:config)
			&& BlackholeDramSourceStable(io,baseline) && BlackholeResidentIdle(io);
		if (canRestore && configured) {
			for (unsigned attempt=0;attempt<2 && !r.niu_restored;attempt++) {
				for (unsigned w=0;w<10;w++) io.ResidentNocWrite(w*4,0);
				io.ResidentNocWrite(0x100,config);
				r.niu_restored=BlackholeResidentSourceStable(io,n,config);
				for (unsigned w=0;w<10;w++) r.niu_restored &= io.ResidentNocRead(w*4)==0;
			}
			canRestore=r.niu_restored!=0;
		}
		if (canRestore) for (unsigned attempt=0;attempt<2 && !r.l1_restored;attempt++) {
			for (unsigned w=0;w<16;w++) io.ResidentL1Write(w,saved[w]);
			r.l1_restored=io.ResetRead()==0x47800;
			for (unsigned w=0;w<16;w++) r.l1_restored &= io.ResidentL1Read(w)==saved[w];
		}
	}
	if (savedTlb) for (unsigned attempt=0;attempt<2 && !r.tlb_restored;attempt++) {
		for (unsigned s=0;s<2;s++) for (unsigned w=0;w<3;w++) io.DramTlbWrite(s,w,r.tlb_before[s][w]);
		r.tlb_restored=1;
		for (unsigned s=0;s<2;s++) for (unsigned w=0;w<3;w++) {
			r.tlb_after[s][w]=io.DramTlbRead(s,w); r.tlb_restored &= r.tlb_after[s][w]==r.tlb_before[s][w];
		}
	}
	if (!canRestore || !r.bme_restored || !r.l1_restored || !r.niu_restored || !r.tlb_restored) {
		r.recovery_required=1;
		if (r.state!=TT_HOST_READ_TIMEOUT && r.state!=TT_HOST_READ_CHANGED) r.state=TT_HOST_READ_RESTORE_FAILED;
	}
	if (r.page.allocated) {
		if (r.recovery_required) { io.QuarantineHostPage(); r.quarantined=1; }
		else {
			r.page.release_status=io.ReleaseHostPage(); r.page.released=r.page.release_status==0;
			if (!r.page.released) { r.page.state=TT_HOST_PAGE_RELEASE_FAILED; r.state=TT_HOST_READ_RESTORE_FAILED; }
		}
	}
	return r.state==TT_HOST_READ_PASSED && !r.recovery_required && r.page.released;
}
#endif
