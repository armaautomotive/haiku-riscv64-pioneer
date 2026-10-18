/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_COHORT_PROBE_H
#define PIONEER_Q38_COHORT_PROBE_H
#include "PioneerQ38CohortProtocol.h"
#include "PioneerQ38CohortImage.h"
#include "PioneerComputeFleet.h"
// OFFLINE candidate policy. No installed driver dispatches this protocol.
// One all-bank owner,one root input upload,read-only peer fetches. No splitK.
struct PioneerQ38CohortWorker {
	tt_compute_noc_result noc={};
	PioneerResidentLease store;
	PioneerResidentLease* master=nullptr;
	PioneerQ38PanelLease image;
	PioneerResidentLease& Store() { return master?*master:store; }
};
struct PioneerQ38CohortLease {
	PioneerQ38CohortWorker worker[TT_Q38_COHORT_WORKERS];
	PioneerQ38CohortExtent extent[TT_Q38_COHORT_WORKERS]; // heap-owned private per-call authority
	unsigned prepared=0; bool unsafe=false;
};
static inline void PioneerQ38CohortQuarantine(PioneerQ38CohortLease& lease,PioneerResidentBanks& banks)
{
	lease.unsafe=true; banks.Quarantine();
	for (unsigned w=0;w<lease.prepared;w++) { lease.worker[w].image.unsafe=true; lease.worker[w].Store().recovery_required=true; }
}
template<class IO> bool PioneerQ38CohortPeerLeaseBegin(IO& io,const tt_dram_samples_result& dram,
	unsigned worker,PioneerQ38CohortWorker& node)
{
	auto& store=node.Store(); const auto& noc=node.noc;
	if (!worker || worker>=120 || store.controls_saved || store.saved || noc.version!=PioneerWorkerVersion(worker)
		|| noc.state!=TT_COMPUTE_NOC_PASSED || !noc.tlb_restored || noc.recovery_required
		|| !PioneerWorkerRoute(dram,noc.source[0],worker) || !BlackholeHostSourceRoute(noc.source[0])) return false;
	store.saved=io.HostReadBackup(2048); if (!store.saved) return false;
	for (unsigned s=0;s<2;s++) for (unsigned w=0;w<3;w++) store.tlb[s][w]=io.DramTlbRead(s,w);
	if ((store.tlb[0][2]&0xffffc000) || (store.tlb[1][2]&0xffffc000)) return false;
	store.controls_saved=true; store.tlb_restored=false; store.config=noc.source[0][0];
	if (!io.SelectRuntimeCore(worker) || io.ResetRead()!=0x47800 || !BlackholeHostReadPciQuiet(io,0x402)
		|| !BlackholeResidentSourceStable(io,noc,store.config) || !BlackholeResidentIdle(io) || (io.ResidentNocRead(0x178)&0xffff)) return false;
	for (unsigned w=0;w<10;w++) if (io.ResidentNocRead(w*4)) return false;
	for (unsigned w=0;w<2048;w++) store.saved[w]=io.ResidentL1Read(w);
	store.configured=true; store.niu_restored=false;
	io.ResidentNocWrite(0x100,store.config|0x10000);
	if (!BlackholeResidentSourceStable(io,noc,store.config|0x10000)) { store.recovery_required=true; return false; }
	store.ready=true; return true;
}
template<class IO> bool PioneerQ38CohortCodeMatches(IO& io)
{
	bool okay=io.RuntimeCodeRead(0)==TT_Q38_COHORT_BOOT;
	for (uint32_t a=0x70000;a<0x74000;a+=4) if (PioneerQ38PanelImmutable(a))
		okay &= io.Q38PanelRead(a)==(a<0x72000?0:PioneerQ38CohortWord((a-0x72000)/4));
	return okay;
}
template<class IO> bool PioneerQ38CohortPrepareWorker(IO& io,const tt_dram_samples_result& dram,
	const PioneerResidentBanks& banks,PioneerQ38CohortWorker& node,unsigned worker)
{
	auto& image=node.image; auto& store=node.Store(); const auto& noc=node.noc;
	if (TT_Q38_PANEL_VIEW_BYTES<0x42000 || worker>=120 || noc.state!=TT_COMPUTE_NOC_PASSED || !noc.tlb_restored
		|| noc.recovery_required || (worker && (noc.version!=PioneerWorkerVersion(worker) || !PioneerWorkerRoute(dram,noc.source[0],worker)))
		|| (!worker && noc.version!=1) || image.unsafe || image.running
		|| !io.SelectRuntimeCore(worker) || !PioneerQ38PanelHeld(io,dram,noc,store,banks)) return false;
	if (image.ready) return PioneerQ38CohortCodeMatches(io);
	uint32_t* saved=io.Q38PanelSavedWords(); if (!saved || image.saved) return false;
	for (unsigned i=0;i<3;i++) { uint32_t value=io.RuntimeControlRead(i); if (i?value!=0:value&1) return false; }
	saved[0]=io.RuntimeCodeRead(0);
	for (unsigned w=0;w<TT_Q38_PANEL_VIEW_WORDS;w++) saved[1+w]=io.Q38PanelRead(TT_Q38_PANEL_VIEW_BASE+w*4);
	image.saved=true;
	if (!PioneerQ38PanelHeld(io,dram,noc,store,banks)) { image.unsafe=true; return false; }
	image.dirty=true;
	for (uint32_t a=0x70000;a<0x74000;a+=4) io.Q38PanelWrite(a,a<0x72000?0:PioneerQ38CohortWord((a-0x72000)/4));
	io.RuntimeCodeWrite(0,TT_Q38_COHORT_BOOT); image.ready=PioneerQ38CohortCodeMatches(io); return image.ready;
}
template<class IO> bool PioneerQ38CohortHeld(IO& io,const tt_dram_samples_result& dram,
	const PioneerResidentBanks& banks,PioneerQ38CohortLease& lease,bool code)
{
	if (lease.unsafe || !lease.prepared || lease.prepared>120 || !banks.Matches(dram)) return false;
	for (unsigned w=0;w<lease.prepared;w++) {
		auto& n=lease.worker[w];
		if (n.image.unsafe || n.image.running || !io.SelectRuntimeCore(w)
			|| !PioneerQ38PanelHeld(io,dram,n.noc,n.Store(),banks) || (code && n.image.ready && !PioneerQ38CohortCodeMatches(io))) return false;
	}
	return true;
}
template<class IO> bool PioneerQ38CohortCommandMatches(IO& io,const PioneerQ38CohortExtent& e,unsigned type,unsigned worker,bool complete)
{
	const uint32_t command[]={e.address+(complete?(e.blocks-1)*PioneerQ38PanelProbeBytes(type):0),0,e.route,0x90000,0,
		PioneerWorkerX(worker)|(PioneerWorkerY(worker)<<6),0,0,PioneerQ38PanelProbeBytes(type),0};
	bool okay=BlackholeResidentIdle(io);
	for (unsigned w=0;w<10;w++) okay &= io.ResidentNocRead(w*4)==command[w];
	return okay;
}
template<class IO> bool PioneerQ38CohortInputsMatch(IO& io,const tt_q38_cohort_probe& job)
{
	bool okay=true;
	for (unsigned w=0;w<job.inputs/2;w++) okay &= io.Q38PanelRead(0xa0000+w*4)==job.activation[w];
	unsigned padded=((job.inputs/PioneerQ38CohortBlockInputs(job.type))*4+63)&~63U;
	for (unsigned w=0;w<padded/4;w++) okay &= io.Q38PanelRead(0x94000+w*4)==job.scales[w];
	return okay;
}
template<class IO> bool PioneerQ38CohortRun(IO& io,const tt_dram_samples_result& dram,PioneerResidentBanks& banks,
	PioneerQ38CohortLease& lease,const uint8_t* initialized,uint32_t initializedPages,tt_q38_cohort_probe& job)
{
	auto* extent=lease.extent;
	if (lease.unsafe || lease.prepared<job.workers || !PioneerQ38CohortAuthorize(banks,dram,initialized,initializedPages,job,extent)) return false;
	for (unsigned w=0;w<job.workers;w++) if (!lease.worker[w].image.ready) return false;
	auto& r=job.result; r={}; r.version=1; r.workers=job.workers; r.state=TT_RUNTIME_CONFIG_FAILED;
	for (auto& row:job.output) for (auto& value:row) value=0;
	bool unknown=false,staged=true;
	do {
		if (!PioneerQ38CohortHeld(io,dram,banks,lease,true)) { unknown=true; break; }
		if (!io.SelectRuntimeCore(0)) { unknown=true; break; }
		unsigned padded=((job.inputs/PioneerQ38CohortBlockInputs(job.type))*4+63)&~63U;
		if (!PioneerQ38PanelStage(io,0xa0000,job.activation,job.inputs/2)
			|| !PioneerQ38PanelStage(io,0x94000,job.scales,padded/4)) break;
		// Stage EVERY participant before any arm/release. Only root receives
		// activation/scales from PCIe;the immutable peers fetch fixed root L1.
		for (unsigned w=0;w<job.workers;w++) {
			auto& node=lease.worker[w]; auto& core=r.core[w]; core.version=1; core.state=TT_RUNTIME_CONFIG_FAILED;
			if (!io.SelectRuntimeCore(w) || !PioneerQ38PanelHeld(io,dram,node.noc,node.Store(),banks)) { unknown=true; break; }
			const uint32_t descriptor[]={TT_Q38_COHORT_MARKER,job.type,PioneerQ38PanelProbeBytes(job.type),0,0,
				job.inputs,extent[w].address,job.part[w].groups,extent[w].route,unsigned(w!=0)};
			if (!PioneerQ38PanelStage(io,0x74000,descriptor,10)) { staged=false; break; }
			for (unsigned row=0;row<job.part[w].groups*16;row++) io.Q38PanelWrite(0x84000+row*4,0x7fc00000);
			for (unsigned row=0;row<job.part[w].groups*16;row++) if (io.Q38PanelRead(0x84000+row*4)!=0x7fc00000) staged=false;
			const uint32_t command[]={extent[w].address,0,extent[w].route,0x90000,0,PioneerWorkerX(w)|(PioneerWorkerY(w)<<6),
				0,0,PioneerQ38PanelProbeBytes(job.type),0};
			if (!staged || !BlackholeHostWriteCommand(io,command) || !PioneerQ38CohortCommandMatches(io,extent[w],job.type,w,false)) { staged=false; break; }
			node.Store().l1_dirty=true; node.Store().l1_restored=false;
			for (unsigned s=0;s<3;s++) io.RuntimeStatusWrite(s,0);
			for (unsigned s=0;s<3;s++) if (io.RuntimeStatusRead(s)) staged=false;
			if (!staged) break;
			core.prepared=1;
		}
		if (unknown || !staged) break;
		if (!PioneerQ38CohortHeld(io,dram,banks,lease,false)) { unknown=true; break; }
		// Arm all,then launch all. Any uncertainty from the first arm onward
		// quarantines the ENTIRE cohort/banks,including known-finished peers.
		for (unsigned w=0;w<job.workers;w++) {
			auto& node=lease.worker[w]; auto& core=r.core[w];
			if (!io.SelectRuntimeCore(w)) { unknown=true; break; }
			core.context_reset=1; io.RuntimeResetWrite(0x87ffff); io.Pause();
			if (io.ResetRead()!=0x87ffff || io.ResetRead()!=0x87ffff || !PioneerQ38PanelSources(io,dram,node.noc,node.Store(),banks)) { unknown=true; break; }
			for (unsigned c=0;c<8;c++) core.before[c]=io.ResidentNocRead(PioneerQ38PanelCounter(c));
		}
		if (unknown) break;
		for (unsigned w=0;w<job.workers;w++) {
			if (!io.SelectRuntimeCore(w) || io.ResetRead()!=0x87ffff) { unknown=true; break; }
			auto& core=r.core[w]; core.started=1; core.state=TT_RUNTIME_COMPLETION_FAILED;
			lease.worker[w].image.running=true; r.started++;
			io.RuntimeResetWrite(0x46800); if (io.ResetRead()!=0x46800) { unknown=true; break; }
		}
		if (unknown) break;
		// Real monotonic global30s deadline,NOT120 multiplied timeouts and
		// NOT dependent on IO::Pause's fleet-specific backoff duration.
		const uint64_t deadline=io.Q38CohortNow()+UINT64_C(30000000);
		while (r.completed<job.workers && !unknown) {
			if (io.Q38CohortNow()>=deadline) { unknown=true; break; }
			for (unsigned w=0;w<job.workers;w++) {
				auto& core=r.core[w]; if (core.completed) continue;
				if (!io.SelectRuntimeCore(w) || io.ResetRead()!=0x46800) { unknown=true; break; }
				core.polls++; core.phase=io.RuntimeStatusRead(0); core.panels=io.RuntimeStatusRead(1); core.marker=io.RuntimeStatusRead(2);
				if (core.marker==TT_Q38_COHORT_FAULT) { unknown=true; break; }
				if (core.phase==TT_Q38_COHORT_DONE && core.panels==job.part[w].groups*(job.inputs/16) && core.marker==TT_Q38_COHORT_MARKER) {
					io.Pause(); core.pc[0]=io.RuntimePcRead(); io.Pause(); core.pc[1]=io.RuntimePcRead();
					if (core.pc[0]==TT_Q38_COHORT_PARK && core.pc[1]==core.pc[0]) { core.completed=1; r.completed++; }
				}
			}
			io.Pause();
		}
		if (unknown) break;
		// Prove ALL completions/code/sources/commands/counters BEFORE any
		// stop or output read. Shared accepted0x210 is NOT a forbidden write.
		for (unsigned w=0;w<job.workers;w++) {
			auto& node=lease.worker[w]; auto& core=r.core[w]; const auto& e=extent[w];
			if (!io.SelectRuntimeCore(w) || io.ResetRead()!=0x46800 || !PioneerQ38PanelSources(io,dram,node.noc,node.Store(),banks)
				|| !PioneerQ38CohortCodeMatches(io) || !PioneerQ38CohortCommandMatches(io,e,job.type,w,true)) { unknown=true; break; }
			for (unsigned c=0;c<8;c++) {
				core.after[c]=io.ResidentNocRead(PioneerQ38PanelCounter(c));
				uint32_t expected=c==1?e.flits:(c==0 || c==2 || c==3 || c==6)?e.reads:0;
				if (uint32_t(core.after[c]-core.before[c])!=expected) { unknown=true; break; }
			}
			if (unknown) break;
		}
		if (unknown) break;
		if (!io.SelectRuntimeCore(0) || !PioneerQ38CohortInputsMatch(io,job)) { unknown=true; break; }
		for (unsigned w=0;w<job.workers;w++) {
			auto& node=lease.worker[w]; auto& core=r.core[w];
			if (!io.SelectRuntimeCore(w)) { unknown=true; break; }
			io.RuntimeResetWrite(0x87ffff); io.Pause();
			if (io.ResetRead()!=0x87ffff || io.ResetRead()!=0x87ffff) { unknown=true; break; }
			io.RuntimeResetWrite(0x47800); io.Pause();
			if (!PioneerQ38PanelHeld(io,dram,node.noc,node.Store(),banks)) { unknown=true; break; }
			core.stopped=1; r.stopped++; node.image.running=false;
		}
		if (unknown) break;
		// A nonfinite result AFTER all workers are proven stopped is a known
		// arithmetic failure,not UNKNOWN. Entire caller output stays unpublished
		// on UNKNOWN,including a failed stop of the LAST participating worker.
		r.state=TT_RUNTIME_PASSED;
		for (unsigned w=0;w<job.workers;w++) {
			if (!io.SelectRuntimeCore(w)) { unknown=true; break; }
			auto& core=r.core[w]; core.state=TT_RUNTIME_PASSED;
			for (unsigned row=0;row<job.part[w].groups*16;row++) {
				job.output[w][row]=io.Q38PanelRead(0x84000+row*4);
				if ((job.output[w][row]&0x7f800000)==0x7f800000) core.state=r.state=TT_RUNTIME_NONFINITE;
			}
		}
	} while(false);
	if (unknown) {
		for (auto& row:job.output) for (auto& value:row) value=0;
		PioneerQ38CohortQuarantine(lease,banks); r.state=TT_RUNTIME_COMPLETION_FAILED; r.recovery_required=1;
		for (unsigned w=0;w<lease.prepared;w++) r.core[w].recovery_required=1;
	}
	return r.state==TT_RUNTIME_PASSED && r.stopped==job.workers && !r.recovery_required;
}
template<class IO> bool PioneerQ38CohortClose(IO& io,const tt_dram_samples_result& dram,
	PioneerResidentBanks& banks,PioneerQ38CohortLease& lease,bool clean)
{
	if (!lease.prepared) return clean && !lease.unsafe;
	if (!clean || !PioneerQ38CohortHeld(io,dram,banks,lease,true)) { PioneerQ38CohortQuarantine(lease,banks); return false; }
	for (unsigned end=lease.prepared;end;end--) {
		unsigned w=end-1; auto& node=lease.worker[w];
		if (!io.SelectRuntimeCore(w) || !PioneerQ38PanelClose(io,dram,node.noc,node.Store(),banks,node.image,true)
			|| (w && !PioneerResidentLeaseEnd(io,node.noc,node.Store()))) { PioneerQ38CohortQuarantine(lease,banks); return false; }
	}
	if (!io.SelectRuntimeCore(0)) { PioneerQ38CohortQuarantine(lease,banks); return false; }
	return true;
}
#endif
