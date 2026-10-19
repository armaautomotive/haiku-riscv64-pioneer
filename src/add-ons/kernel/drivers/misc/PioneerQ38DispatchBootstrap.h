/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_DISPATCH_BOOTSTRAP_H
#define PIONEER_Q38_DISPATCH_BOOTSTRAP_H
#include "PioneerQ38CohortProbe.h"
#include "PioneerQ38DispatchImages.h"
#include "PioneerQ38DispatchProof.h"
#ifdef PIONEER_Q38_REUSE_ACTIVATIONS
#include "PioneerQ38ActivationReuse.h"
#endif
// Experimental54 controller. Native qualification is separate from tests.
// This large object and every request/authority MUST be heap-owned (kernel
// stack16KiB). Persistent running peers are expected,NOT old cohort Unsafe.
struct PioneerQ38DispatchLease {
	PioneerQ38CohortWorker worker[120];
	PioneerQ38DispatchAuthority authority;
	uint32_t bootCounter[120][8]={},peerNonce[119]={},rootNonce=0;
#ifdef PIONEER_Q38_REUSE_ACTIVATIONS
	PioneerQ38ActivationReuse inputReuse;
	uint64_t inputEpoch=1; // set under the driver's lock before every RUN
#endif
	unsigned prepared=0;
	// Diagnostic bookkeeping ONLY. Never a completion/source authority and
	// never an excuse for extra device reads/writes after uncertainty.
	unsigned closePhase=0,closeWorker=0;
	uint32_t closeStatus[3]={};
	uint32_t closeDetail[4]={};
	bool bootAttempted=false,booted=false,unsafe=false,closed=false,rootRunning=false;
};
static inline bool PioneerQ38DispatchUnsafe(const PioneerQ38DispatchLease& lease)
{
	if (lease.unsafe || lease.rootRunning || (lease.bootAttempted && !lease.booted && !lease.closed)) return true;
	for (unsigned w=0;w<lease.prepared;w++) {
		const auto& n=lease.worker[w];
		const auto& store=n.master?*n.master:n.store;
		if (n.image.unsafe || n.noc.recovery_required || store.pending || store.recovery_required
			|| n.image.running!=bool(w && lease.booted && !lease.closed)) return true;
	}
	return false;
}
static inline bool PioneerQ38DispatchQuarantine(PioneerQ38DispatchLease& lease,PioneerResidentBanks& banks)
{
	lease.unsafe=true; banks.Quarantine();
#ifdef PIONEER_Q38_REUSE_ACTIVATIONS
	lease.inputReuse.valid=false;
#endif
	for (unsigned w=0;w<lease.prepared;w++) { lease.worker[w].image.unsafe=true; lease.worker[w].Store().recovery_required=true; }
	return false; // caller must quarantine BOTH host owners too;NO later IO writes
}
template<class IO> bool PioneerQ38DispatchSources(IO& io,const tt_dram_samples_result& dram,
	const PioneerResidentBanks& banks,PioneerQ38CohortWorker& node,uint32_t reset)
{
	return node.Store().ready && !node.Store().pending && !node.Store().recovery_required && !node.image.unsafe
		&& banks.Matches(dram) && io.Q38PanelMappingStable() && io.ResetRead()==reset && io.ResetRead()==reset
		&& PioneerRuntimeSources(io,dram,node.noc,node.Store()) && BlackholeResidentIdle(io);
}
template<class IO> bool PioneerQ38DispatchCodeMatches(IO& io,unsigned worker,PioneerQ38DispatchLease* diagnostic=nullptr)
{
	if (worker>=120) return false;
	uint32_t entry=io.RuntimeCodeRead(0); bool okay=entry==TT_Q38_DISPATCH_BOOT;
	if (!okay && diagnostic && !diagnostic->closeDetail[0]) {
		diagnostic->closeDetail[0]=0x300; diagnostic->closeDetail[1]=0x6000;
		diagnostic->closeDetail[2]=entry; diagnostic->closeDetail[3]=TT_Q38_DISPATCH_BOOT;
	}
	for (uint32_t a=0x70000;a<0x74000;a+=4) if (!worker || PioneerQ38PanelImmutable(a)) {
		uint32_t actual=io.Q38PanelRead(a),expected=a<0x72000?0:PioneerQ38DispatchRoleWord(worker,(a-0x72000)/4);
		if (actual!=expected && diagnostic && !diagnostic->closeDetail[0]) {
			diagnostic->closeDetail[0]=0x300; diagnostic->closeDetail[1]=a;
			diagnostic->closeDetail[2]=actual; diagnostic->closeDetail[3]=expected;
		}
		okay &= actual==expected;
	}
	return okay;
}
template<class IO> bool PioneerQ38DispatchPrepareRole(IO& io,const tt_dram_samples_result& dram,
	PioneerResidentBanks& banks,PioneerQ38DispatchLease& lease,unsigned worker)
{
	if (lease.unsafe || lease.bootAttempted || lease.closed || worker>=120 || lease.prepared!=worker) return false;
	lease.prepared=worker+1; // include a caller-prepared NIU/store lease even if its first role proof fails
	auto& node=lease.worker[worker]; auto& image=node.image;
	if (node.noc.state!=TT_COMPUTE_NOC_PASSED || !node.noc.tlb_restored || node.noc.recovery_required
		|| (worker?node.noc.version!=PioneerWorkerVersion(worker) || !PioneerWorkerRoute(dram,node.noc.source[0],worker):node.noc.version!=1)
		|| image.saved || image.running || !io.SelectRuntimeCore(worker)
		|| !PioneerQ38DispatchSources(io,dram,banks,node,0x47800)) return PioneerQ38DispatchQuarantine(lease,banks);
	uint32_t* saved=io.Q38PanelSavedWords(); if (!saved) return PioneerQ38DispatchQuarantine(lease,banks);
	for (unsigned i=0;i<3;i++) { uint32_t value=io.RuntimeControlRead(i); if (i?value!=0:value&1) return PioneerQ38DispatchQuarantine(lease,banks); }
	saved[0]=io.RuntimeCodeRead(0);
	for (unsigned w=0;w<TT_Q38_PANEL_VIEW_WORDS;w++) saved[w+1]=io.Q38PanelRead(TT_Q38_PANEL_VIEW_BASE+w*4);
	image.saved=true;
	if (!PioneerQ38DispatchSources(io,dram,banks,node,0x47800)) return PioneerQ38DispatchQuarantine(lease,banks);
	image.dirty=true;
	for (uint32_t a=0x70000;a<0x74000;a+=4) io.Q38PanelWrite(a,a<0x72000?0:PioneerQ38DispatchRoleWord(worker,(a-0x72000)/4));
	io.RuntimeCodeWrite(0,TT_Q38_DISPATCH_BOOT);
	if (!PioneerQ38DispatchCodeMatches(io,worker)) return PioneerQ38DispatchQuarantine(lease,banks);
	image.ready=true;
	for (unsigned c=0;c<16;c++) lease.authority.source[worker][c]=c?node.noc.source[0][c]:node.Store().config|0x10000;
	return true;
}
template<class IO> bool PioneerQ38DispatchBootstrap(IO& io,const tt_dram_samples_result& dram,
	PioneerResidentBanks& banks,PioneerQ38DispatchLease& lease)
{
	if (lease.unsafe || lease.closed || lease.prepared!=120) return false;
	if (lease.booted) return true;
	if (lease.bootAttempted) return false;
	lease.bootAttempted=true;
	if (!lease.worker[0].image.ready || lease.worker[0].image.running) return PioneerQ38DispatchQuarantine(lease,banks);
	for (unsigned w=1;w<120;w++) {
		auto& n=lease.worker[w];
		if (!n.image.ready || !io.SelectRuntimeCore(w) || !PioneerQ38DispatchSources(io,dram,banks,n,0x47800)) return PioneerQ38DispatchQuarantine(lease,banks);
		const uint32_t boot[]={TT_Q38_PEER_MARKER,w,PioneerWorkerX(w)|(PioneerWorkerY(w)<<6),n.Store().config|0x10000,0,0,0,0};
		uint32_t zero[16]={},sentinel[16]; for (auto& value:sentinel) value=0xdeadbeef;
		if (!PioneerQ38PanelStage(io,0x74040,boot,8) || !PioneerQ38PanelStage(io,0x74100,zero,16)
			|| !PioneerQ38PanelStage(io,0x74140,sentinel,16)) return PioneerQ38DispatchQuarantine(lease,banks);
		const uint32_t command[]={0,0,0,0,0,boot[2],0,0,0,0};
		if (!BlackholeHostWriteCommand(io,command)) return PioneerQ38DispatchQuarantine(lease,banks);
		for (unsigned c=0;c<8;c++) lease.bootCounter[w][c]=io.ResidentNocRead(PioneerQ38PanelCounter(c));
		n.Store().l1_dirty=true; n.Store().l1_restored=false;
	}
	// Arm ALL before release. From the first uncertain write onward there
	// is no retry/reset/restore/free of ANY participant,even finished peers.
	for (unsigned w=1;w<120;w++) {
		if (!io.SelectRuntimeCore(w)) return PioneerQ38DispatchQuarantine(lease,banks);
		io.RuntimeResetWrite(0x87ffff); io.Pause();
		if (!PioneerQ38DispatchSources(io,dram,banks,lease.worker[w],0x87ffff)) return PioneerQ38DispatchQuarantine(lease,banks);
	}
	for (unsigned w=1;w<120;w++) {
		if (!io.SelectRuntimeCore(w) || io.ResetRead()!=0x87ffff) return PioneerQ38DispatchQuarantine(lease,banks);
		lease.worker[w].image.running=true;
		io.RuntimeResetWrite(0x46800); if (io.ResetRead()!=0x46800) return PioneerQ38DispatchQuarantine(lease,banks);
	}
	bool ready[119]={}; unsigned finished=0; const uint64_t deadline=io.Q38CohortNow()+UINT64_C(30000000);
	while (finished<119) {
		if (io.Q38CohortNow()>=deadline) return PioneerQ38DispatchQuarantine(lease,banks);
		for (unsigned w=1;w<120;w++) if (!ready[w-1]) {
			if (!io.SelectRuntimeCore(w) || !PioneerQ38DispatchSources(io,dram,banks,lease.worker[w],0x46800)) return PioneerQ38DispatchQuarantine(lease,banks);
			uint32_t status[16]; for (unsigned i=0;i<16;i++) status[i]=io.Q38PanelRead(0x74140+i*4);
			if (status[2]==0x46503851 || (status[3]!=TT_Q38_PEER_MARKER && status[3]!=0 && status[3]!=0xdeadbeef)) return PioneerQ38DispatchQuarantine(lease,banks);
			if (status[3]!=TT_Q38_PEER_MARKER) continue;
			for (unsigned i=0;i<16;i++) if (i!=3 && status[i]) return PioneerQ38DispatchQuarantine(lease,banks);
			io.Pause(); uint32_t first=io.RuntimePcRead(); io.Pause(); uint32_t second=io.RuntimePcRead();
			if ((first!=TT_Q38_PEER_WAIT_LOAD && first!=TT_Q38_PEER_WAIT_BRANCH)
				|| (second!=TT_Q38_PEER_WAIT_LOAD && second!=TT_Q38_PEER_WAIT_BRANCH)) continue;
			for (unsigned c=0;c<8;c++) if (io.ResidentNocRead(PioneerQ38PanelCounter(c))!=lease.bootCounter[w][c]) return PioneerQ38DispatchQuarantine(lease,banks);
			ready[w-1]=true; finished++;
		}
		io.Pause();
	}
	if (!io.SelectRuntimeCore(0) || !PioneerQ38DispatchSources(io,dram,banks,lease.worker[0],0x47800)) return PioneerQ38DispatchQuarantine(lease,banks);
	uint32_t bankRoutes[8]={};
	for (unsigned bank=0;bank<7;bank++) { uint32_t address=0,index=0; if (!banks.Page(dram,bank,0,address,bankRoutes[bank],index)) return PioneerQ38DispatchQuarantine(lease,banks); }
	if (!PioneerQ38PanelStage(io,0x74040,bankRoutes,8)) return PioneerQ38DispatchQuarantine(lease,banks);
	for (unsigned w=1;w<120;w++) if (!PioneerQ38PanelStage(io,0x7d800+(w-1)*64,lease.authority.source[w],16)) return PioneerQ38DispatchQuarantine(lease,banks);
	lease.booted=true; return true;
}
#endif
