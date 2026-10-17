/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_COORDINATED_FLEET_H
#define PIONEER_COORDINATED_FLEET_H
#include "PioneerCoordinatorProbe.h"
#include "PioneerRuntimePersistentFleet.h"
#include "PioneerCoordinatedActivation.h"
#include "PioneerCoordinatedOutput.h"
// Separate mode8. Master0 is a held-between-jobs coordinator, not a math
// worker. Peers1..119 may wait in an immutable NoC-free mailbox loop.
// Their last proved idle state remains valid under the exclusive FD/team
// lock until a kernel-owned doorbell. Full live all-peer STOP precedes END.
template<class IO>
bool PioneerCoordinatedMasterBoundary(IO& io,const tt_dram_samples_result& baseline,
	PioneerFleetWorker* fleet,bool attentionReady)
{
	if (!fleet || !fleet[0].Store().ready || !fleet[0].Compute().ready || !fleet[0].preludeDirty) return false;
	for (unsigned w=0;w<TT_PIONEER_WORKERS;w++) {
		auto& f=fleet[w]; auto& s=f.Store(); auto& c=f.Compute();
		if (c.running || c.unsafe || s.pending || s.recovery_required || f.noc.recovery_required
			|| (s.controls_saved && !s.ready) || (c.saved && !c.ready) || (f.preludeSaved && !f.preludeDirty)
			|| (f.persistentActive && (!w || !f.coordinated || !f.persistentPrepared || !f.persistentIdle))) return false;
	}
	if (!PioneerFleetGlobalSources(io,baseline) || !io.SelectRuntimeCore(0)
		|| io.ResetRead()!=0x47800 || io.ResetRead()!=0x47800 || !BlackholeResidentIdle(io)
		|| !PioneerFleetLocalSources(io,fleet[0].noc,fleet[0].Store())) return false;
	for (unsigned s=0;s<2;s++) for (unsigned w=0;w<3;w++)
		if (io.DramTlbRead(s,w)!=BlackholeDramFixedTlbWord(s?0xffa00000:0,1,2,false,w)) return false;
	uint32_t entry=io.RuntimeCodeRead(0);
	return (entry==0x0006b06f || (attentionReady && entry==TT_ATTENTION_TILE_BOOT)) && PioneerFleetGlobalSources(io,baseline);
}
struct tt_coordinated_fleet_result {
	tt_coordinator_probe_result master; // matched = peers validated ON CARD, not CPU polling
	uint32_t inputs,outputs,workers,staged_pages,nonfinite,gathered;
	uint32_t summary[24],dma_words;
};
static inline bool PioneerCoordinatedSummary(const uint32_t* summary,unsigned n,unsigned workers,unsigned nonce,
	const uint32_t* before,const uint32_t* after,bool fused=false)
{
	if (!summary || !workers || workers>119 || !n || (n&31) || n>TT_FLEET_OUTPUTS_MAX || !nonce || nonce>=0x7fffffff
		|| (fused && n>4096) || summary[4]!=workers || summary[5]!=n || summary[6]!=nonce || summary[7]!=(fused?3U:1U)) return false;
	unsigned reads=summary[0],flits=summary[1],writes=summary[2];
	// Hidden projections add ONE gather read per peer. Only the extra
	// mailbox/fault-poll rounds come in pairs. Using10 for a gathered job
	// falsely rejects every odd cohort, despite correct physical counters.
	unsigned minimum=(n<=8192?11:10)*workers+(fused?1:0),writeFlits=2*workers+(fused?n/16:0);
	if (reads<minimum || reads>minimum+4*65536*workers || ((reads-minimum)&1)
		|| writes!=2*workers+(fused?1:0) || summary[3]!=writeFlits || flits!=(n<=8192?reads-workers+n/16+(fused?n/16-1:0):reads)) return false;
	const unsigned expected[]={reads,flits,reads+writes,reads,writes,writeFlits,reads+writes,writes};
	if (before[2]!=before[6] || after[2]!=after[6]) return false;
	for (unsigned i=0;i<8;i++) if (summary[8+i]!=before[i] || summary[16+i]!=after[i]
		|| uint32_t(after[i]-before[i])!=expected[i]) return false;
	return true;
}
static inline bool PioneerCoordinatedProof(const tt_coordinated_fleet_result& r,unsigned k,unsigned n,unsigned limit,unsigned nonce,bool debug=false)
{
	PioneerFleetPlan p; const auto& m=r.master;
	bool fused=!debug && n<=4096;
	return limit && limit<=119 && PioneerFleetPartition(k,n,limit,p) && r.inputs==k && r.outputs==n && r.workers==p.workers
		&& (debug?r.staged_pages==4:(!r.staged_pages || r.staged_pages==(k*2+k/8+4095)/4096))
		&& !r.nonfinite && r.dma_words==((debug?n<=8192:fused)?n:0) && r.gathered==(n<=8192?n:0)
		&& m.version==1 && m.state==TT_RUNTIME_PASSED && m.workers==p.workers && m.matched==p.workers && m.nonce==nonce
		&& m.prepared==1 && m.started==1 && m.completed==1 && m.stopped==1 && !m.recovery_required && m.polls
		&& m.phase==TT_COORDINATOR_DONE && m.count==p.workers && m.marker==TT_COORDINATOR_MARKER
		&& m.pc[0]==TT_COORDINATOR_PARK && m.pc[1]==m.pc[0] && m.summary_verified==1
		&& PioneerCoordinatedSummary(r.summary,n,p.workers,nonce,m.before,m.after,fused);
}
static inline bool PioneerCoordinatedInputMatches(const PioneerCoordinatorLease& lease,unsigned k,const uint32_t* input,const uint32_t* scales)
{
	if (!lease.ready || !lease.fast || !lease.fused || lease.readback || !lease.compact || lease.running || lease.unsafe
		|| !lease.inputValid || lease.inputK!=k || !input || !scales || !PioneerQ8CompactShape(k,32)) return false;
	for (unsigned i=0;i<k/2;i++) if (lease.inputShadow[i]!=input[i]) return false;
	for (unsigned i=0;i<k/32;i++) if (lease.scaleShadow[i]!=scales[i]) return false;
	return true; // exact bits, including signed zero; never hash-only
}
template<class IO>
auto PioneerCoordinatedGatherRead(IO& io,unsigned words,uint32_t* output,int)
	-> decltype(io.CoordinatorPayloadRead(words,output)) { return io.CoordinatorPayloadRead(words,output); }
template<class IO>
bool PioneerCoordinatedGatherRead(IO& io,unsigned words,uint32_t* output,long)
{
	if (!output || !words || words>8192) return false;
	for (unsigned i=0;i<words;i++) output[i]=io.RuntimeBufferRead(4096+i);
	return true;
}
template<class IO>
bool PioneerCoordinatedMatvec(IO& io,const tt_dram_samples_result& baseline,const tt_host_page_result& host,
	PioneerOutputBufferLease& transfer,PioneerFleetWorker* fleet,PioneerBulkUploadLease& bulk,PioneerCoordinatorLease& lease,bool attentionReady,
	uint64_t capacity,uint64_t offset,unsigned k,unsigned n,unsigned limit,const uint32_t* input,
	const uint32_t* scales,uint32_t* output,tt_coordinated_fleet_result& r)
{
	r={}; auto& m=r.master; m.version=1; r.inputs=k; r.outputs=n;
	PioneerFleetPlan p; uint64_t bytes=PioneerQ8CompactWeightBytes(k,n);
	if (!fleet || !input || !scales || !output || !limit || limit>119 || !PioneerFleetPartition(k,n,limit,p)
		|| capacity!=UINT64_C(0x80000000) || !PioneerResidentOffset(capacity,offset) || !bytes
		|| offset>=capacity-TT_RESIDENT_KV_BYTES || bytes>capacity-TT_RESIDENT_KV_BYTES-offset
		|| !lease.ready || !lease.fast || (lease.readback==lease.fused) || (lease.fused && !lease.compact)
		|| lease.unsafe || lease.running || lease.nonce>=0x7ffffffe || !bulk.ready || bulk.unsafe) return false;
	for (unsigned i=0;i<k/2;i++) if (!PioneerInt8OperandValid(uint16_t(input[i])) || !PioneerInt8OperandValid(uint16_t(input[i]>>16))) return false;
	for (unsigned i=0;i<k/32;i++) if ((scales[i]&0x7f800000)==0x7f800000) return false;
	for (unsigned w=1;w<=p.workers;w++) {
		auto& f=fleet[w];
		if (!f.coordinated || !f.persistentPrepared || !f.persistentActive || !f.persistentIdle || f.Compute().running
			|| f.Compute().unsafe || f.Store().pending || f.Store().recovery_required || f.persistentSequence>=0x7ffffffe) return false;
	}
	r.workers=p.workers; m.workers=p.workers; m.state=TT_RUNTIME_CONFIG_FAILED;
	auto& master=fleet[0]; auto& store=master.Store();
	uint32_t previous=0; bool uncertain=false,bmeTouched=false,fused=lease.fused && n<=4096;
	bool inputHit=false;
	do {
		if (!PioneerCoordinatedMasterBoundary(io,baseline,fleet,attentionReady) || !PioneerCoordinatorLocalHeld(io,baseline,master.noc,store)
			|| !BlackholeHostReadMapStable(io,host)) { uncertain=true; break; }
		previous=io.RuntimeCodeRead(0);
		uint32_t* stage=io.RuntimeStageWords(); if (!stage) break;
		inputHit=PioneerCoordinatedInputMatches(lease,k,input,scales);
		bool exact=false;
		if (inputHit) {
			const auto& owned=master.Compute();
			if (!owned.saved || !owned.bulk || !owned.buffers_dirty || owned.buffer_words!=TT_Q8_BULK_SAVED_BUFFER_WORDS
				|| io.RuntimeBufferWords()!=TT_Q8_BULK_SAVED_BUFFER_WORDS) { uncertain=true; break; }
			// This private shadow was sealed ONLY after a complete successful
			// matrix. Every allowed mode8 image/transport leaves the packed
			// consumed packed input unchanged (canonical scale copy is outside
			// that span, or the identical6144 source/destination).
			// Full ownership/source/NoC/PCI/host guards STILL precede every hit.
			exact=PioneerBulkUploadGuard(io,baseline,master.noc,host,store,transfer,0x402);
			if (!exact) { uncertain=true; break; }
		} else {
			lease.inputValid=false; lease.inputK=0;
			tt_bulk_upload_result staged={};
			exact=PioneerCoordinatedActivation(io,baseline,master.noc,host,store,transfer,bulk,master.Compute(),k,input,scales,staged,lease.compact);
			r.staged_pages=staged.pages;
		}
		if (!exact) break;
		for (unsigned i=0;i<1024;i++) stage[i]=0;
		stage[0]=0x31445243; stage[1]=fused?3:1; stage[2]=limit; stage[3]=k; stage[4]=n;
		stage[5]=PioneerResidentAddress(capacity,offset); stage[6]=++lease.nonce; m.nonce=lease.nonce;
		if (fused) { stage[7]=uint32_t(transfer.physical); stage[8]=uint32_t(transfer.physical>>32); stage[9]=16384; }
		for (unsigned w=1;w<=p.workers;w++) stage[64+w-1]=fleet[w].persistentSequence+1;
		bulk.dirty=true;
		// Every word the fixed image reads is verified, including all
		// participating sequences. Unused4KiB padding is not a payload.
		if (!PioneerBulkPayloadStage(io,stage,64+p.workers,0) || !PioneerCoordinatedMasterBoundary(io,baseline,fleet,attentionReady)) break;
		io.RuntimeProfileMark(TT_PROFILE_STAGED);
		store.l1_dirty=true; store.l1_restored=false;
		for (unsigned i=0;i<3;i++) io.RuntimeStatusWrite(i,0);
		for (unsigned i=0;i<3;i++) exact &= io.RuntimeStatusRead(i)==0;
		if (!exact) break;
		io.RuntimeCodeWrite(0,TT_COORDINATOR_BOOT);
		if (io.RuntimeCodeRead(0)!=TT_COORDINATOR_BOOT || !PioneerCoordinatorLocalHeld(io,baseline,master.noc,store)) break;
		m.prepared=1;
		uint32_t executionPci=fused?0x406:0x402;
		if (fused) {
			// Fully qualified exclusive owner; exact finite poison proves
			// every requested host word is replaced, not an old result.
			if (!PioneerBulkUploadGuard(io,baseline,master.noc,host,store,transfer,0x402)) { uncertain=true; break; }
			io.BulkHostFence(); for (unsigned i=0;i<n;i++) io.BulkHostStore(i,0xffffffff); io.BulkHostFence();
			for (unsigned i=0;i<n;i++) exact &= io.BulkHostLoad(i)==0xffffffff;
			io.BulkHostFence(); if (!exact) break;
			bmeTouched=true; io.HostPciWrite(1,4,2,0x406); if (io.HostPciRead(1,4,2)!=0x406) { uncertain=true; break; }
			io.HostPciWrite(0,4,2,0x406);
			if (!PioneerBulkUploadGuard(io,baseline,master.noc,host,store,transfer,0x406)) { uncertain=true; break; }
		}
		io.RuntimeResetWrite(0x87ffff); io.Pause();
		if (io.ResetRead()!=0x87ffff || io.ResetRead()!=0x87ffff || !PioneerCoordinatorSources(io,baseline,master.noc,store,executionPci)
			|| !BlackholeResidentIdle(io)) { uncertain=true; break; }
		PioneerResidentAttentionCounters(io,m.before);
		// No participating peer is considered idle again until the whole
		// immutable on-card proof AND known master stop have succeeded.
		for (unsigned w=1;w<=p.workers;w++) { fleet[w].persistentSequence++; fleet[w].persistentIdle=false; fleet[w].Compute().running=true; }
		lease.running=true; m.started=1; m.state=TT_RUNTIME_COMPLETION_FAILED;
		if (fused) transfer.pending=true;
		io.RuntimeProfileMark(TT_PROFILE_LAUNCHED);
		io.RuntimeResetWrite(0x46800);
		for (unsigned attempt=0;attempt<50000;attempt++) {
			m.polls++;
			if (io.ResetRead()!=0x46800) break;
			m.phase=io.RuntimeStatusRead(0); m.count=io.RuntimeStatusRead(1); m.marker=io.RuntimeStatusRead(2);
			if (m.marker==TT_COORDINATOR_FAULT) break;
			if (m.phase==TT_COORDINATOR_DONE && m.count==p.workers && m.marker==TT_COORDINATOR_MARKER) {
				io.Pause(); m.pc[0]=io.RuntimePcRead(); io.Pause(); m.pc[1]=io.RuntimePcRead();
				if (m.pc[0]==TT_COORDINATOR_PARK && m.pc[1]==m.pc[0] && BlackholeResidentIdle(io)) { m.completed=1; break; }
			}
			io.Pause();
		}
		if (!m.completed || io.ResetRead()!=0x46800 || !PioneerCoordinatorSources(io,baseline,master.noc,store,executionPci)
			|| !BlackholeResidentIdle(io)) { uncertain=true; break; }
		PioneerResidentAttentionCounters(io,m.after);
		for (unsigned i=0;i<24;i++) r.summary[i]=io.BulkL1Read(3072+i);
		// Counter/nonce/shape summary must be proved BEFORE even the
		// coordinator reset; an unaccounted NoC completion is unknown.
		if (!PioneerCoordinatedSummary(r.summary,n,p.workers,lease.nonce,m.before,m.after,fused)) { uncertain=true; break; }
		m.summary_verified=1;
		if (fused) {
			io.HostPciWrite(0,4,2,0x402); io.HostPciWrite(1,4,2,0x402);
			if (!BlackholeHostReadPciQuiet(io,0x402) || !PioneerOutputBufferMapStable(io,transfer,host)
				|| !BlackholeHostReadMapStable(io,host) || !PioneerCoordinatorSources(io,baseline,master.noc,store)
				|| !BlackholeResidentIdle(io)) { uncertain=true; break; }
			bmeTouched=false; transfer.pending=false;
		}
		io.RuntimeResetWrite(0x87ffff); io.Pause();
		if (io.ResetRead()!=0x87ffff || io.ResetRead()!=0x87ffff) { uncertain=true; break; }
		io.RuntimeResetWrite(0x47800); io.Pause();
		if (!PioneerCoordinatorLocalHeld(io,baseline,master.noc,store)) { uncertain=true; break; }
		lease.running=false; m.stopped=1; m.matched=p.workers;
		for (unsigned w=1;w<=p.workers;w++) { fleet[w].persistentIdle=true; fleet[w].Compute().running=false; }
		io.RuntimeProfileMark(TT_PROFILE_COMPLETED);
		if (fused) {
			// Exact original/ordered-readback equality was established by
			// the SAME immutable GPU job; no CPU reference or second launch.
			io.BulkHostFence();
			for (unsigned i=0;i<n;i++) { output[i]=io.BulkHostLoad(i); r.nonfinite+=(output[i]&0x7f800000)==0x7f800000; }
			io.BulkHostFence();
			if (r.nonfinite || !PioneerBulkUploadGuard(io,baseline,master.noc,host,store,transfer,0x402)) { uncertain=true; break; }
			r.dma_words=r.gathered=n;
		} else if (n<=8192 && lease.readback) {
			for (unsigned at=0;at<n;at+=4096) {
				unsigned words=n-at; if (words>4096) words=4096;
				PioneerCoordinatedOutputResult transferred;
				if (!PioneerCoordinatedOutput(io,baseline,master.noc,host,store,transfer,bulk,master.Compute(),lease,
					at,words,output+at,transferred)) { uncertain=true; break; }
				r.dma_words+=words;
			}
			if (uncertain) break;
			r.gathered=n;
		} else if (n<=8192) {
			if (!PioneerCoordinatedGatherRead(io,n,output,0)) { uncertain=true; break; }
			r.gathered=n;
		} else for (unsigned at=0;at<p.workers;at++) {
			if (!io.SelectRuntimeCore(at+1) || !PioneerRuntimePayloadRead(io,4096,p.rows[at],output+p.first[at],0)) { uncertain=true; break; }
		}
		if (uncertain) break;
		for (unsigned i=0;i<n;i++) r.nonfinite+=(output[i]&0x7f800000)==0x7f800000;
		m.state=r.nonfinite?TT_RUNTIME_NONFINITE:TT_RUNTIME_PASSED;
		io.RuntimeProfileMark(TT_PROFILE_PIO);
	} while (false);
	if (bmeTouched) {
		bool off=false;
		for (unsigned attempt=0;attempt<2 && !off;attempt++) {
			io.HostPciWrite(0,4,2,0x402); io.HostPciWrite(1,4,2,0x402); off=BlackholeHostReadPciQuiet(io,0x402);
		}
		if (!off) uncertain=true;
	}
	for (unsigned w=0;w<TT_PIONEER_WORKERS;w++) uncertain |= fleet[w].Compute().running || fleet[w].Compute().unsafe
		|| fleet[w].Store().pending || fleet[w].Store().recovery_required || (fleet[w].persistentActive && !fleet[w].persistentIdle);
	if (!uncertain && !lease.running && PioneerCoordinatorLocalHeld(io,baseline,master.noc,store)) {
		if (previous) {
			io.RuntimeCodeWrite(0,previous);
			if (io.RuntimeCodeRead(0)!=previous || !PioneerCoordinatedMasterBoundary(io,baseline,fleet,attentionReady)) uncertain=true;
		}
	} else uncertain=true;
	if (uncertain) {
		m.recovery_required=1; lease.unsafe=true; store.recovery_required=true; bulk.unsafe=true;
		PioneerPersistentQuarantine(io,fleet);
		PioneerQuarantineOutputBuffer(io,transfer);
	}
	bool proved=!uncertain && PioneerCoordinatedProof(r,k,n,limit,lease.nonce,lease.readback);
	if (proved && lease.compact && !inputHit) {
		// Nothing between jobs may mutate this owned packed input. Neither
		// the shadow alone nor a partial/failed DMA can seal a cache entry.
		for (unsigned i=0;i<k/2;i++) lease.inputShadow[i]=input[i];
		for (unsigned i=0;i<k/32;i++) lease.scaleShadow[i]=scales[i];
		lease.inputK=k; lease.inputValid=true;
	} else if (!proved) { lease.inputValid=false; lease.inputK=0; }
	return proved;
}
#endif
