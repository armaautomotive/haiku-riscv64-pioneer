/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_RUNTIME_FLEET_H
#define PIONEER_RUNTIME_FLEET_H
#include "PioneerComputeFleet.h"
#include "PioneerRuntimeMatvec.h"
#include "BlackholeFleetStartImage.h"
#include "PioneerPayloadBlock.h"

enum { TT_FLEET_OUTPUTS_MAX=262144,TT_FLEET_START_WORDS=1024 };
struct PioneerFleetPlan {
	unsigned workers=0,rows[TT_PIONEER_WORKERS]={},first[TT_PIONEER_WORKERS]={};
};
static inline bool PioneerFleetPartition(unsigned inputs,unsigned outputs,unsigned limit,PioneerFleetPlan& p)
{
	p={};
	if (!limit || limit>TT_PIONEER_WORKERS || !PioneerQ8CompactTensorShape(inputs,outputs)) return false;
	p.workers=outputs/32<limit?outputs/32:limit;
	unsigned blocks=outputs/32,base=blocks/p.workers,extra=blocks%p.workers,first=0;
	for (unsigned w=0;w<p.workers;w++) {
		p.first[w]=first; p.rows[w]=32*(base+(w<extra)); first+=p.rows[w];
		if (!PioneerQ8CompactShape(inputs,p.rows[w])) { p={}; return false; }
	}
	return first==outputs;
}
struct PioneerFleetWorker {
	tt_compute_noc_result noc={};
	PioneerResidentLease store;
	PioneerRuntimeMatvecLease compute;
	PioneerResidentLease* masterStore=nullptr;
	PioneerRuntimeMatvecLease* masterCompute=nullptr;
	PioneerResidentLease& Store() { return masterStore?*masterStore:store; }
	PioneerRuntimeMatvecLease& Compute() { return masterCompute?*masterCompute:compute; }
	bool preludeSaved=false,preludeDirty=false;
	// Separate from an executing math job: a persistent RISC may be
	// provably idle in its local mailbox loop without being held in reset.
	bool persistentPrepared=false,persistentActive=false,persistentIdle=false;
	bool inlineMath=false;
	bool coordinatedInline=false; // explicit whole-decoder composition,never a live upgrade
	bool partialMath=false; // separate native-rounding image,never implicit
	bool phasePipeline=false; // exact four phases with FPU-owned fidelity increments
	bool descriptorTable=false,descriptorTablePrepared=false; // immutable fixed-model schedule,not a live upgrade
	bool accumPipeline=false; // exact SFPU Dst timing and once/job AddrMod1 setup
	bool roundedProduct=false; // separate RN multiply/add image,never implicit or a live upgrade
	bool fusedGather=false; // separate immutable proof-prefix publisher cohort
	bool smallRows=false; // private hidden-projection16-row chunks,whole tensors unchanged
	bool coordinated=false; // separate source-snapshot/marker/PC contract
	bool attentionDispatch=false; // separately owned fixed-group image
	bool gluDispatch=false;
	uint32_t persistentSequence=0;
};
struct tt_runtime_fleet_result {
	uint32_t version,state,workers,started,completed,stopped,recovery_required,staged_pages;
	tt_runtime_matvec_result core[TT_PIONEER_WORKERS];
};
// PCIe-source/PCI state is shared by the fleet, not120 independent states.
// Check it at phase boundaries; check every participating NIU individually.
// These helpers authorize no host DMA and never replace a completion proof.
template<class IO>
bool PioneerFleetGlobalSources(IO& io,const tt_dram_samples_result& baseline) {
	return BlackholeDramSourceStable(io,baseline) && BlackholeHostReadPciQuiet(io,0x402);
}
template<class IO>
bool PioneerFleetLocalSources(IO& io,const tt_compute_noc_result& noc,const PioneerResidentLease& store) {
	bool stable=store.ready && !store.pending && !store.recovery_required;
	for (unsigned w=0;w<16;w++) stable &= io.ResidentNocRead(BlackholeDramSourceOffset(w))==(w?noc.source[0][w]:store.config|0x10000);
	return stable;
}
// A master-only entry switch/device-local KV job cannot affect a stopped
// peer. Under the exclusive FD/team lock, each peer's last independent
// completion/hold proof remains valid until this driver next touches it.
// Audit all ownership/unsafe/pending metadata, then LIVE-audit the master
// and shared PCIe source. Matrix jobs still LIVE-audit every participant;
// END still LIVE-audits the entire prepared fleet before restoration.
// No reset, code/data write, DMA, peer launch or ownership release here.
template<class IO>
bool PioneerFleetMasterBoundary(IO& io,const tt_dram_samples_result& baseline,
	PioneerFleetWorker* fleet,bool attentionReady)
{
	if (!fleet || !fleet[0].Store().ready || !fleet[0].Compute().ready || !fleet[0].preludeDirty) return false;
	for (unsigned w=0;w<TT_PIONEER_WORKERS;w++) {
		auto& f=fleet[w]; auto& s=f.Store(); auto& c=f.Compute();
		if (f.persistentActive || c.running || c.unsafe || s.pending || s.recovery_required || f.noc.recovery_required
			|| (s.controls_saved && !s.ready) || (c.saved && !c.ready) || (f.preludeSaved && !f.preludeDirty)) return false;
	}
	if (!PioneerFleetGlobalSources(io,baseline) || !io.SelectRuntimeCore(0)
		|| io.ResetRead()!=0x47800 || io.ResetRead()!=0x47800 || !BlackholeResidentIdle(io)
		|| !PioneerFleetLocalSources(io,fleet[0].noc,fleet[0].Store())) return false;
	uint32_t entry=io.RuntimeCodeRead(0);
	return (entry==0x0006b06f || (attentionReady && entry==0x0006c06f)) && PioneerFleetGlobalSources(io,baseline);
}
static inline bool PioneerFleetCoreProof(const tt_runtime_matvec_result& r,unsigned inputs,unsigned rows,unsigned worker)
{
	unsigned tiles=(inputs/16)*(rows/16),reads=tiles/32,peer=worker!=0;
	return r.version==1 && r.state==TT_RUNTIME_PASSED && r.prepared && r.context_reset && r.started && r.completed && r.stopped
		&& !r.recovery_required && !r.nonfinite && r.phase==TT_RUNTIME_Q8_FINAL_PHASE && r.tiles==tiles && r.marker==0x4d415431
		&& r.pc[0]==TT_RUNTIME_Q8_PARK_PC && r.pc[1]==r.pc[0]
		&& uint32_t(r.after[0]-r.before[0])==reads+peer
		&& uint32_t(r.after[1]-r.before[1])==reads*TT_Q8_BULK_FLITS+256*peer
		&& uint32_t(r.after[2]-r.before[2])==reads+peer && uint32_t(r.after[3]-r.before[3])==reads+peer;
}
static inline bool PioneerFleetProof(const tt_runtime_fleet_result& r,unsigned inputs,unsigned outputs,unsigned limit)
{
	PioneerFleetPlan p;
	if (!PioneerFleetPartition(inputs,outputs,limit,p) || r.version!=1 || r.state!=TT_RUNTIME_PASSED || r.workers!=p.workers
		|| r.started!=p.workers || r.completed!=p.workers || r.stopped!=p.workers || r.recovery_required
		|| r.staged_pages!=(inputs*2+4095)/4096+1) return false;
	for (unsigned w=0;w<p.workers;w++) if (!PioneerFleetCoreProof(r.core[w],inputs,p.rows[w],w)) return false;
	return true;
}
template<class IO>
bool PioneerFleetLeaseBegin(IO& io,const tt_dram_samples_result& baseline,unsigned worker,PioneerFleetWorker& f)
{
	auto& lease=f.Store(); const auto& noc=f.noc;
	if (!worker || worker>=120 || lease.controls_saved || lease.saved || noc.version!=PioneerWorkerVersion(worker)
		|| noc.state!=TT_COMPUTE_NOC_PASSED || !noc.tlb_restored || noc.recovery_required
		|| !PioneerWorkerRoute(baseline,noc.source[0],worker) || !BlackholeHostSourceRoute(noc.source[0])) return false;
	lease.saved=io.HostReadBackup(2048); if (!lease.saved) return false;
	for (unsigned s=0;s<2;s++) for (unsigned w=0;w<3;w++) lease.tlb[s][w]=io.DramTlbRead(s,w);
	if ((lease.tlb[0][2]&0xffffc000) || (lease.tlb[1][2]&0xffffc000)) return false;
	lease.controls_saved=true; lease.tlb_restored=false; lease.config=noc.source[0][0];
	if (!io.SelectRuntimeCore(worker) || io.ResetRead()!=0x47800 || !BlackholeHostReadPciQuiet(io,0x402)
		|| !BlackholeResidentSourceStable(io,noc,lease.config) || !BlackholeResidentIdle(io) || (io.ResidentNocRead(0x178)&0xffff)) return false;
	for (unsigned w=0;w<10;w++) if (io.ResidentNocRead(w*4)) return false;
	for (unsigned w=0;w<2048;w++) lease.saved[w]=io.ResidentL1Read(w);
	lease.configured=true; lease.niu_restored=false;
	io.ResidentNocWrite(0x100,lease.config|0x10000);
	if (!BlackholeResidentSourceStable(io,noc,lease.config|0x10000)) { lease.recovery_required=true; return false; }
	lease.ready=true; return true;
}
template<class IO>
bool PioneerFleetPreludePrepare(IO& io,PioneerFleetWorker& f)
{
	if (f.preludeSaved) return !f.Compute().unsafe && f.preludeDirty;
	uint32_t* saved=io.FleetSavedWords();
	if (!saved || !f.Compute().ready || io.ResetRead()!=0x47800 || !BlackholeResidentIdle(io)) return false;
	for (unsigned w=0;w<1024;w++) saved[w]=io.FleetCodeRead(w);
	f.preludeSaved=true; f.preludeDirty=true;
	for (unsigned w=0;w<1024;w++) io.FleetCodeWrite(w,BlackholeFleetStartCodeWord(w));
	bool okay=true;
	for (unsigned w=0;w<1024;w++) okay &= io.FleetCodeRead(w)==BlackholeFleetStartCodeWord(w);
	io.RuntimeCodeWrite(0,0x0006b06f); okay &= io.RuntimeCodeRead(0)==0x0006b06f;
	return okay;
}
template<class IO>
bool PioneerFleetPreludeClose(IO& io,PioneerFleetWorker& f,bool safe)
{
	if (!f.preludeSaved) return safe;
	if (!safe || f.Compute().unsafe || f.Compute().running || f.Store().pending || f.Store().recovery_required
		|| io.ResetRead()!=0x47800 || !BlackholeResidentIdle(io)) { f.Compute().unsafe=true; return false; }
	uint32_t* saved=io.FleetSavedWords(); if (!saved) { f.Compute().unsafe=true; return false; }
	for (unsigned attempt=0;attempt<2 && f.preludeDirty;attempt++) {
		for (unsigned w=0;w<1024;w++) io.FleetCodeWrite(w,saved[w]);
		bool restored=true;
		for (unsigned w=0;w<1024;w++) restored &= io.FleetCodeRead(w)==saved[w];
		f.preludeDirty=!restored;
	}
	f.Compute().unsafe |= f.preludeDirty; return !f.Compute().unsafe;
}
// One immutable learned matrix, one shared activation upload, up to120
// independent row workers. No split-K, CPU sums or approximated products.
// All workers prove completion before ANY result is published or peer reset.
template<class IO>
bool PioneerFleetMatvec(IO& io,const tt_dram_samples_result& baseline,const tt_host_page_result& host,
	PioneerFleetWorker* fleet,uint64_t capacity,uint64_t offset,unsigned inputs,unsigned outputs,unsigned limit,
	const uint32_t* input,const uint32_t* scales,uint32_t* output,tt_runtime_fleet_result& result)
{
	result={}; result.version=1;
	PioneerFleetPlan p;
	uint64_t bytes=PioneerQ8CompactWeightBytes(inputs,outputs);
	if (!fleet || !input || !scales || !output || !PioneerFleetPartition(inputs,outputs,limit,p)
		|| !PioneerResidentOffset(capacity,offset) || !bytes || bytes>capacity-offset) return false;
	for (unsigned i=0;i<inputs/2;i++) if (!PioneerInt8OperandValid(uint16_t(input[i])) || !PioneerInt8OperandValid(uint16_t(input[i]>>16))) return false;
	for (unsigned i=0;i<inputs/32;i++) if ((scales[i]&0x7f800000)==0x7f800000) return false;
	for (unsigned w=0;w<p.workers;w++) if (!fleet[w].Compute().ready || !fleet[w].preludeDirty || fleet[w].Compute().unsafe
		|| fleet[w].Compute().running || !fleet[w].Store().ready || fleet[w].Store().pending || fleet[w].Store().recovery_required) return false;
	result.workers=p.workers; result.state=TT_RUNTIME_CONFIG_FAILED;
	bool uncertain=false,passed=false;
	unsigned armed=0;
	do {
		if (!PioneerFleetGlobalSources(io,baseline)) { uncertain=true; break; }
		// Guard the ENTIRE participating set before touching the shared input.
		for (unsigned w=0;w<p.workers;w++) if (!io.SelectRuntimeCore(w) || io.ResetRead()!=0x47800 || io.ResetRead()!=0x47800
			|| !PioneerFleetLocalSources(io,fleet[w].noc,fleet[w].Store()) || !BlackholeResidentIdle(io)
			|| !BlackholeHostReadMapStable(io,host)) { uncertain=true; break; }
		if (uncertain || !PioneerFleetGlobalSources(io,baseline) || !io.SelectRuntimeCore(0)) { uncertain=true; break; }
		uint32_t* stage=io.RuntimeStageWords(); if (!stage) break;
		fleet[0].Compute().buffers_dirty=true;
		auto& master=result.core[0]; master.version=1;
		bool okay=true;
		for (unsigned page=0;page<(inputs*2+4095)/4096 && okay;page++) {
			for (unsigned i=0;i<1024;i++) stage[i]=page*1024+i<inputs/2?input[page*1024+i]:0;
			okay=PioneerRuntimeStage(io,baseline,fleet[0].noc,host,fleet[0].Store(),false,page,stage,master);
		}
		if (okay) {
			for (unsigned i=0;i<1024;i++) stage[i]=i<inputs/32?scales[i]:0;
			okay=PioneerRuntimeStage(io,baseline,fleet[0].noc,host,fleet[0].Store(),false,3,stage,master,true);
		}
		result.staged_pages=master.staged_pages;
		if (!okay) break;
		if (!PioneerFleetGlobalSources(io,baseline)) { uncertain=true; break; }
		io.RuntimeProfileMark(TT_PROFILE_STAGED);
		for (unsigned w=0;w<p.workers;w++) {
			if (!io.SelectRuntimeCore(w)) { uncertain=true; break; }
			auto& f=fleet[w]; auto& r=result.core[w]; r.version=1; r.prepared=1;
			f.Compute().buffers_dirty=true; f.Store().l1_dirty=true; f.Store().l1_restored=false;
			uint32_t address=PioneerResidentAddress(capacity,offset+PioneerQ8CompactWeightBytes(inputs,p.first[w]));
			// first0 has zero bytes, so use the original offset explicitly.
			if (!w) address=PioneerResidentAddress(capacity,offset);
			const uint32_t descriptor[]={address,inputs/16,p.rows[w]/16,uint32_t(w!=0),PioneerWorkerX(w)|(PioneerWorkerY(w)<<6)};
			for (unsigned i=0;i<5;i++) io.RuntimeCodeWrite(1+0x500/4+i,descriptor[i]);
			for (unsigned i=0;i<5;i++) okay &= io.RuntimeCodeRead(1+0x500/4+i)==descriptor[i];
			for (unsigned i=0;i<3;i++) io.RuntimeStatusWrite(i,0);
			const uint32_t command[]={address,0,18|(12<<6),TT_Q8_BULK_L1,0,descriptor[4],0,0,TT_Q8_BULK_BYTES,0};
			if (!okay || !BlackholeHostWriteCommand(io,command) || io.ResetRead()!=0x47800
				|| !PioneerFleetLocalSources(io,f.noc,f.Store())) break;
			r.context_reset=1; io.RuntimeResetWrite(0x87ffff); io.Pause();
			if (io.ResetRead()!=0x87ffff || io.ResetRead()!=0x87ffff || !PioneerFleetLocalSources(io,f.noc,f.Store()) || !BlackholeResidentIdle(io)) { uncertain=true; break; }
			for (unsigned i=0;i<4;i++) r.before[i]=io.ResidentNocRead(0x208+i*4);
			armed++;
		}
		if (uncertain || armed!=p.workers) break;
		if (!PioneerFleetGlobalSources(io,baseline)) { uncertain=true; break; }
		io.RuntimeProfileMark(TT_PROFILE_LAUNCHED);
		for (unsigned w=0;w<p.workers;w++) {
			if (!io.SelectRuntimeCore(w) || io.ResetRead()!=0x87ffff || !BlackholeResidentIdle(io)
				|| !PioneerFleetLocalSources(io,fleet[w].noc,fleet[w].Store())) { uncertain=true; break; }
			auto& r=result.core[w]; r.started=1; fleet[w].Compute().running=true; result.started++;
			io.RuntimeResetWrite(0x46800);
			if (io.ResetRead()!=0x46800) { uncertain=true; break; }
		}
		if (uncertain) break;
		if (!PioneerFleetGlobalSources(io,baseline)) { uncertain=true; break; }
		for (unsigned attempt=0;attempt<50000 && result.completed<p.workers;attempt++) {
			for (unsigned w=0;w<p.workers;w++) {
				auto& r=result.core[w]; if (r.completed) continue;
				if (!io.SelectRuntimeCore(w) || io.ResetRead()!=0x46800) { uncertain=true; break; }
				r.polls++; r.phase=io.RuntimeStatusRead(0); r.tiles=io.RuntimeStatusRead(1); r.marker=io.RuntimeStatusRead(2);
				if (r.marker==0x4e4f4346) { uncertain=true; break; }
				if (r.phase!=TT_RUNTIME_Q8_FINAL_PHASE || r.tiles!=(inputs/16)*(p.rows[w]/16) || r.marker!=0x4d415431) continue;
				io.Pause(); r.pc[0]=io.RuntimePcRead(); io.Pause(); r.pc[1]=io.RuntimePcRead();
				for (unsigned i=0;i<4;i++) r.after[i]=io.ResidentNocRead(0x208+i*4);
				unsigned reads=r.tiles/32,peer=w!=0;
				if (r.pc[0]==TT_RUNTIME_Q8_PARK_PC && r.pc[1]==r.pc[0] && BlackholeResidentIdle(io)
					&& uint32_t(r.after[0]-r.before[0])==reads+peer && uint32_t(r.after[1]-r.before[1])==reads*TT_Q8_BULK_FLITS+256*peer
					&& uint32_t(r.after[2]-r.before[2])==reads+peer && uint32_t(r.after[3]-r.before[3])==reads+peer) {
					r.completed=1; result.completed++;
				}
			}
			if (uncertain) break;
			if (result.completed<p.workers) io.Pause();
		}
		if (uncertain || result.completed!=p.workers) break;
		if (!PioneerFleetGlobalSources(io,baseline)) { uncertain=true; break; }
		for (unsigned w=0;w<p.workers;w++) if (!io.SelectRuntimeCore(w) || io.ResetRead()!=0x46800
			|| !BlackholeResidentIdle(io) || !PioneerFleetLocalSources(io,fleet[w].noc,fleet[w].Store())) { uncertain=true; break; }
		if (uncertain) break;
		io.RuntimeProfileMark(TT_PROFILE_COMPLETED);
		for (unsigned w=0;w<p.workers;w++) {
			if (!io.SelectRuntimeCore(w)) { uncertain=true; break; }
			auto& r=result.core[w];
			if (!PioneerRuntimePayloadRead(io,4096,p.rows[w],output+p.first[w],0)) { uncertain=true; break; }
			for (unsigned i=0;i<p.rows[w];i++) {
				uint32_t value=output[p.first[w]+i];
				r.nonfinite+=(value&0x7f800000)==0x7f800000;
			}
		}
		if (uncertain) break;
		io.RuntimeProfileMark(TT_PROFILE_PIO);
		if (!PioneerFleetGlobalSources(io,baseline)) { uncertain=true; break; }
		for (unsigned w=0;w<p.workers;w++) {
			if (!io.SelectRuntimeCore(w) || io.ResetRead()!=0x46800 || !BlackholeResidentIdle(io)
				|| !PioneerFleetLocalSources(io,fleet[w].noc,fleet[w].Store())) { uncertain=true; break; }
			io.RuntimeResetWrite(0x87ffff); io.Pause();
			if (io.ResetRead()!=0x87ffff || io.ResetRead()!=0x87ffff) { uncertain=true; break; }
			io.RuntimeResetWrite(0x47800); io.Pause();
			if (io.ResetRead()!=0x47800 || io.ResetRead()!=0x47800) { uncertain=true; break; }
			auto& r=result.core[w]; r.stopped=1; result.stopped++; fleet[w].Compute().running=false;
			r.state=r.nonfinite?TT_RUNTIME_NONFINITE:TT_RUNTIME_PASSED;
		}
		if (uncertain) break;
		if (!PioneerFleetGlobalSources(io,baseline)) { uncertain=true; break; }
		io.RuntimeProfileMark(TT_PROFILE_STOPPED);
		result.state=TT_RUNTIME_PASSED;
		passed=PioneerFleetProof(result,inputs,outputs,limit);
		if (!passed) result.state=TT_RUNTIME_NONFINITE;
	} while (false);
	for (unsigned w=0;w<p.workers;w++) uncertain |= fleet[w].Store().pending || fleet[w].Store().recovery_required
		|| fleet[w].Compute().unsafe || (result.core[w].started && (!result.core[w].completed || !result.core[w].stopped));
	if (!uncertain && !result.started) {
		// Only never-released armed workers may be rolled back. A partial
		// launch/completion failure quarantines the ENTIRE participating set.
		if (!PioneerFleetGlobalSources(io,baseline)) uncertain=true;
		for (unsigned w=0;w<p.workers && !uncertain;w++) {
			if (!io.SelectRuntimeCore(w) || !PioneerFleetLocalSources(io,fleet[w].noc,fleet[w].Store())
				|| !BlackholeResidentIdle(io)) { uncertain=true; break; }
			uint32_t reset=io.ResetRead();
			if (reset!=0x47800 && reset!=0x87ffff) { uncertain=true; break; }
		}
		if (!PioneerFleetGlobalSources(io,baseline)) uncertain=true;
		for (unsigned w=0;w<armed && !uncertain;w++) {
			if (!io.SelectRuntimeCore(w)) { uncertain=true; break; }
			io.RuntimeResetWrite(0x47800); io.Pause();
			if (io.ResetRead()!=0x47800 || io.ResetRead()!=0x47800) uncertain=true;
		}
	}
	if (uncertain) {
		result.recovery_required=1;
		for (unsigned w=0;w<p.workers;w++) { fleet[w].Compute().unsafe=true; fleet[w].Store().recovery_required=true; result.core[w].recovery_required=1; }
		io.QuarantineHostPage();
	}
	return passed && !uncertain;
}
#endif
