/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_DECODER_CONTROLLER_H
#define PIONEER_DECODER_CONTROLLER_H
// Experimental whole-decoder controller;requires separate native qualification.
#include "PioneerCoordinatedFleet.h"
#include "PioneerDecoderLayout.h"
#include "BlackholeDecoderCandidateImage.h"
#include "BlackholeParallelDecoderCandidateImage.h"
#include "BlackholeProfileDecoderCandidateImage.h"
#include "PioneerDecoderStageProfile.h"
#include <stddef.h>
struct PioneerDecoderLease {
	bool saved=false,dirty=false,ready=false,running=false,unsafe=false,parallel=false;
	PioneerAttentionTileLease* peers=nullptr;
	bool profiling=false;
	tt_decoder_stage_profile stages={};
};
static inline uint32_t PioneerDecoderCodeWord(const PioneerDecoderLease& code,unsigned word)
{
	return code.profiling?BlackholeProfileDecoderCodeWord(word):code.parallel?BlackholeParallelDecoderCodeWord(word):BlackholeDecoderCodeWord(word);
}
template<class IO>
bool PioneerDecoderPeerOwners(IO& io,PioneerFleetWorker* fleet,const PioneerDecoderLease& code)
{
	if (!code.parallel) return !code.peers;
	if (!fleet || !code.peers || !io.DecoderPeerMappingsStable()) return false;
	for (unsigned i=0;i<8;i++) { const auto& p=code.peers[i];
		if (!p.saved || !p.ready || !p.code_dirty || !p.coordinatedGroup || p.resident || p.unsafe
			|| p.running!=code.running || !fleet[i+1].attentionDispatch) return false;
	}
	return true;
}
struct tt_decoder_candidate_result {
	tt_coordinator_probe_result master;
	uint32_t tokens,sequences,words,nonfinite,summary[24];
};
static inline bool PioneerDecoderProof(const tt_decoder_candidate_result& r,unsigned tokens,unsigned nonce,bool parallel=false)
{
	const auto& m=r.master;
	return r.tokens==tokens && r.sequences==119 && r.words==151936 && !r.nonfinite && m.version==1 && m.state==TT_RUNTIME_PASSED
		&& m.workers==119 && m.matched==119 && m.nonce==nonce && m.prepared==1 && m.started==1 && m.completed==1
		&& m.stopped==1 && !m.recovery_required && m.polls && m.phase==TT_DECODER_DONE && m.count==119
		&& m.marker==TT_COORDINATOR_MARKER && m.pc[0]==TT_DECODER_PARK && m.pc[1]==m.pc[0] && m.summary_verified==1
		&& PioneerDecoderSummary(r.summary,tokens,nonce,m.before,m.after,parallel);
}
template<class IO>
bool PioneerDecoderPrepare(IO& io,const tt_dram_samples_result& baseline,PioneerFleetWorker* fleet,
	const PioneerCoordinatorLease& stack,const PioneerLayerLease& previous,const PioneerAttentionTileLease& attention,PioneerDecoderLease& code,
	bool parallel=false,PioneerAttentionTileLease* peers=nullptr,bool profiling=false)
{
	if ((profiling && !parallel) || !fleet || !stack.ready || !stack.saved || !stack.fast || !stack.fused || !stack.compact || stack.readback || stack.running || stack.unsafe
		|| previous.ready || previous.dirty || previous.running || previous.unsafe || !attention.ready || !attention.saved || !attention.resident
		|| attention.running || attention.unsafe || !io.DecoderMappingStable() || !io.DecoderAttentionMappingStable()
		|| !PioneerCoordinatedMasterBoundary(io,baseline,fleet,true)) return false;
	if (code.ready) return !code.running && !code.unsafe && code.parallel==parallel && code.peers==peers && code.profiling==profiling && PioneerDecoderPeerOwners(io,fleet,code);
	if (code.saved || code.unsafe || !io.DecoderSavedWords()) return false;
	code.parallel=parallel; code.peers=peers; code.profiling=profiling;
	if (!PioneerDecoderPeerOwners(io,fleet,code)) return false;
	for (unsigned i=0;i<8192;i++) io.DecoderSavedWords()[i]=io.LayerCodeRead(i);
	code.saved=true;
	if (!PioneerCoordinatedMasterBoundary(io,baseline,fleet,true) || !io.DecoderMappingStable() || !PioneerDecoderPeerOwners(io,fleet,code)) { code.unsafe=true; return false; }
	code.dirty=true;
	for (unsigned i=0;i<8192;i++) io.LayerCodeWrite(i,PioneerDecoderCodeWord(code,i));
	bool exact=true; for (unsigned i=0;i<8192;i++) exact &= io.LayerCodeRead(i)==PioneerDecoderCodeWord(code,i);
	if (exact && (!PioneerCoordinatedMasterBoundary(io,baseline,fleet,true) || !io.DecoderMappingStable() || !io.DecoderAttentionMappingStable()
		|| !PioneerDecoderPeerOwners(io,fleet,code))) {
		code.unsafe=true; fleet[0].Store().recovery_required=true; io.QuarantineHostPage(); return false;
	}
	code.ready=exact; return exact;
}
template<class IO>
bool PioneerDecoderClose(IO& io,const tt_dram_samples_result& baseline,const tt_compute_noc_result& noc,
	PioneerResidentLease& store,PioneerDecoderLease& code,bool allPeersStopped)
{
	if (!code.saved) return allPeersStopped && !code.unsafe;
	bool safe=allPeersStopped && !code.running && !code.unsafe && io.DecoderSavedWords() && io.DecoderMappingStable()
		&& PioneerCoordinatorLocalHeld(io,baseline,noc,store);
	if (safe && code.dirty) {
		for (unsigned i=0;i<8192;i++) io.LayerCodeWrite(i,io.DecoderSavedWords()[i]);
		bool exact=true; for (unsigned i=0;i<8192;i++) exact &= io.LayerCodeRead(i)==io.DecoderSavedWords()[i];
		if (exact) code.dirty=false;
	}
	code.ready=false; code.unsafe |= !safe || code.dirty;
	if (code.unsafe) { store.recovery_required=true; io.QuarantineHostPage(); }
	return !code.unsafe;
}
template<class IO>
bool PioneerDecoderRun(IO& io,const tt_dram_samples_result& baseline,const tt_host_page_result& host,
	PioneerOutputBufferLease& transfer,PioneerWideOutputLease& wide,PioneerFleetWorker* fleet,PioneerBulkUploadLease& bulk,
	PioneerCoordinatorLease& stack,PioneerDecoderLease& code,PioneerAttentionTileLease& attention,PioneerResidentAttentionLease& kv,
	const PioneerDecoderJob& model,unsigned tokens,const uint32_t* embedding,const uint32_t* rope,uint32_t* output,tt_decoder_candidate_result& r)
{
	r={}; code.stages={}; r.tokens=tokens; auto& m=r.master; m.version=1; m.workers=119;
	if (!fleet || !embedding || !rope || !output || !tokens || tokens>256 || !code.ready || !code.saved || code.running || code.unsafe || !PioneerDecoderPeerOwners(io,fleet,code)
		|| !stack.ready || !stack.saved || !stack.fast || !stack.fused || !stack.compact || stack.readback || stack.running || stack.unsafe
		|| stack.nonce>=0x7ffffffe || !bulk.ready || !bulk.saved || bulk.unsafe || !kv.reserved || !kv.coordinated || !kv.payloadBlocks
		|| !attention.ready || !attention.saved || !attention.resident || attention.running || attention.unsafe
		|| !wide.ready || !wide.allocated || !wide.memory_type_set || wide.pending || wide.released || wide.quarantined) return false;
	for (unsigned i=0;i<1024;i++) { unsigned a=embedding[i]&0x7fffffff; if (a>0x477fe000 || (a && a<0x26800000)) return false; }
	for (unsigned i=0;i<128;i++) { unsigned a=rope[i]&0x7fffffff; if (a>0x3f800000 || (a && a<0x26800000)) return false; }
	for (unsigned i=0;i<28;i++) if (tokens>kv.tokens[i]+1) return false;
	for (unsigned w=1;w<120;w++) { auto& f=fleet[w];
		if (!f.coordinated || !f.persistentActive || !f.persistentPrepared || !f.persistentIdle || f.Compute().running || f.Compute().unsafe
			|| f.Store().pending || f.Store().recovery_required || f.persistentSequence>0x7ffffffe - PioneerDecoderSequenceAdvance(w-1,code.parallel)) return false; }
	auto& master=fleet[0]; auto& store=master.Store(); auto& compute=master.Compute();
	if (!compute.ready || !compute.saved || !compute.bulk || compute.running || compute.unsafe
		|| compute.buffer_words!=TT_Q8_BULK_SAVED_BUFFER_WORDS || io.RuntimeBufferWords()!=TT_Q8_BULK_SAVED_BUFFER_WORDS) return false;
	bool uncertain=false,bme=false; uint32_t previous=0; uint32_t* stage=io.RuntimeStageWords();
	if (!stage) return false;
	auto& job=*reinterpret_cast<PioneerDecoderJob*>(stage); const auto* original=reinterpret_cast<const uint32_t*>(&model);
	for (unsigned i=0;i<sizeof(job)/4;i++) stage[i]=original[i];
	job.magic=0x31444344; job.operation=8; job.version=code.parallel?2:1; job.tokens=tokens; job.nonce=stack.nonce+1; job.workers=119;
	job.kvBase=0x82400000; job.hostLow=uint32_t(wide.physical); job.hostHigh=uint32_t(wide.physical>>32); job.hostBytes=0x100000;
	for (unsigned i=0;i<4;i++) job.reserved[i]=0;
	for (unsigned i=0;i<119;i++) job.sequence[i]=fleet[i+1].persistentSequence+1;
	for (unsigned i=0;i<128;i++) job.rope[i]=rope[i];
	if (!PioneerDecoderJobValid(job,code.parallel?2:1)) return false;
	stack.inputValid=false; stack.inputK=0; m.state=TT_RUNTIME_CONFIG_FAILED;
	do {
		if (!PioneerCoordinatedMasterBoundary(io,baseline,fleet,true) || (!io.DecoderMappingStable() || !PioneerDecoderPeerOwners(io,fleet,code)) || !io.DecoderAttentionMappingStable()
			|| !PioneerWideOutputMapStable(io,wide,host,transfer) || !PioneerBulkUploadGuard(io,baseline,master.noc,host,store,transfer,0x402)) { uncertain=true; break; }
		previous=io.RuntimeCodeRead(0); bulk.dirty=true; compute.buffers_dirty=true; attention.data_dirty=true; store.l1_dirty=true; store.l1_restored=false;
		for (unsigned i=0;i<1024;i++) io.RuntimeBufferWrite(4096+i,embedding[i]);
		bool exact=true; for (unsigned i=0;i<1024;i++) exact &= io.RuntimeBufferRead(4096+i)==embedding[i];
		for (unsigned i=0;i<sizeof(job)/4;i++) io.BulkL1Write(6144+i,stage[i]);
		for (unsigned i=0;i<sizeof(job)/4;i++) exact &= io.BulkL1Read(6144+i)==stage[i];
		if (!exact) break;
		m.nonce=++stack.nonce; io.RuntimeProfileMark(TT_PROFILE_STAGED);
		io.WideHostFence(); for (unsigned i=0;i<151936;i++) io.WideHostStore(i,0xffffffff); io.WideHostFence();
		for (unsigned i=0;i<151936;i++) exact &= io.WideHostLoad(i)==0xffffffff;
		io.WideHostFence(); for (unsigned i=0;i<3;i++) io.RuntimeStatusWrite(i,0);
		for (unsigned i=0;i<3;i++) exact &= io.RuntimeStatusRead(i)==0;
		if (!exact) break;
		io.RuntimeCodeWrite(0,TT_DECODER_BOOT);
		if (io.RuntimeCodeRead(0)!=TT_DECODER_BOOT || !PioneerCoordinatorLocalHeld(io,baseline,master.noc,store) || (!io.DecoderMappingStable() || !PioneerDecoderPeerOwners(io,fleet,code))) break;
		m.prepared=1;
		if (!PioneerBulkUploadGuard(io,baseline,master.noc,host,store,transfer,0x402) || !PioneerWideOutputMapStable(io,wide,host,transfer)) { uncertain=true; break; }
		bme=true; io.HostPciWrite(1,4,2,0x406); if (io.HostPciRead(1,4,2)!=0x406) { uncertain=true; break; }
		io.HostPciWrite(0,4,2,0x406);
		if (!PioneerBulkUploadGuard(io,baseline,master.noc,host,store,transfer,0x406) || !PioneerWideOutputMapStable(io,wide,host,transfer)
			|| !io.DecoderAttentionMappingStable() || (!io.DecoderMappingStable() || !PioneerDecoderPeerOwners(io,fleet,code))) { uncertain=true; break; }
		io.RuntimeResetWrite(0x87ffff); io.Pause();
		if (io.ResetRead()!=0x87ffff || io.ResetRead()!=0x87ffff || !PioneerCoordinatorSources(io,baseline,master.noc,store,0x406)
			|| !BlackholeResidentIdle(io)) { uncertain=true; break; }
		PioneerResidentAttentionCounters(io,m.before);
		for (unsigned w=1;w<120;w++) { fleet[w].persistentSequence+=PioneerDecoderSequenceAdvance(w-1,code.parallel); fleet[w].persistentIdle=false; fleet[w].Compute().running=true; }
		code.running=stack.running=attention.running=true;
		if (code.parallel) for (unsigned i=0;i<8;i++) { code.peers[i].data_dirty=true; code.peers[i].running=true; }
		wide.pending=true; m.started=1; m.state=TT_RUNTIME_COMPLETION_FAILED;
		io.RuntimeProfileMark(TT_PROFILE_LAUNCHED); io.RuntimeResetWrite(0x46800);
		for (unsigned attempt=0;attempt<1000000;attempt++) {
			m.polls++; if (io.ResetRead()!=0x46800) break;
			m.phase=io.RuntimeStatusRead(0); m.count=io.RuntimeStatusRead(1); m.marker=io.RuntimeStatusRead(2);
			if (m.marker==TT_COORDINATOR_FAULT) break;
			if (m.phase==TT_DECODER_DONE && m.count==119 && m.marker==TT_COORDINATOR_MARKER) {
				io.Pause(); m.pc[0]=io.RuntimePcRead(); io.Pause(); m.pc[1]=io.RuntimePcRead();
				if (m.pc[0]==TT_DECODER_PARK && m.pc[1]==m.pc[0] && BlackholeResidentIdle(io)) { m.completed=1; break; }
			}
			io.Pause();
		}
		if (!m.completed || io.ResetRead()!=0x46800 || !PioneerCoordinatorSources(io,baseline,master.noc,store,0x406)
			|| (!io.DecoderMappingStable() || !PioneerDecoderPeerOwners(io,fleet,code)) || !io.DecoderAttentionMappingStable() || !BlackholeResidentIdle(io)) { uncertain=true; break; }
		PioneerResidentAttentionCounters(io,m.after); for (unsigned i=0;i<24;i++) r.summary[i]=io.BulkL1Read(3072+i);
		if (!PioneerDecoderSummary(r.summary,tokens,m.nonce,m.before,m.after,code.parallel)) { uncertain=true; break; }
		const unsigned seq=offsetof(PioneerDecoderJob,sequence)/4;
		for (unsigned i=0;i<sizeof(job)/4;i++) {
			uint32_t value=io.BulkL1Read(6144+i);
			if (i>=seq && i<seq+119) { bool okay=value==fleet[i-seq+1].persistentSequence+1; exact &= okay; r.sequences+=okay; }
			else exact &= value==stage[i];
		}
		if (!exact || r.sequences!=119) { uncertain=true; break; }
		m.summary_verified=1;
		io.HostPciWrite(0,4,2,0x402); io.HostPciWrite(1,4,2,0x402);
		// The master is still RELEASED at its proved park,not held yet.
		// Keep full maps/source/idle/PCI proofs here;do NOT misuse the bulk
		// staging guard (which correctly requires a held reset47800).
		if (!BlackholeHostReadPciQuiet(io,0x402) || !PioneerOutputBufferMapStable(io,transfer,host)
			|| !BlackholeHostReadMapStable(io,host) || !PioneerCoordinatorSources(io,baseline,master.noc,store)
			|| !BlackholeResidentIdle(io) || !PioneerWideOutputMapStable(io,wide,host,transfer)
			|| (!io.DecoderMappingStable() || !PioneerDecoderPeerOwners(io,fleet,code)) || !io.DecoderAttentionMappingStable()) { uncertain=true; break; }
		bme=false; io.RuntimeResetWrite(0x87ffff); io.Pause();
		if (io.ResetRead()!=0x87ffff || io.ResetRead()!=0x87ffff) { uncertain=true; break; }
		io.RuntimeResetWrite(0x47800); io.Pause();
		if (!PioneerCoordinatorLocalHeld(io,baseline,master.noc,store) || (!io.DecoderMappingStable() || !PioneerDecoderPeerOwners(io,fleet,code))) { uncertain=true; break; }
		code.running=stack.running=attention.running=false;
		if (code.parallel) for (unsigned i=0;i<8;i++) code.peers[i].running=false;
		wide.pending=false; m.stopped=1; m.matched=119;
		for (unsigned w=1;w<120;w++) { fleet[w].persistentIdle=true; fleet[w].Compute().running=false; }
		io.RuntimeProfileMark(TT_PROFILE_COMPLETED); io.WideHostFence();
		for (unsigned i=0;i<151936;i++) { output[i]=io.WideHostLoad(i); r.nonfinite+=(output[i]&0x7f800000)==0x7f800000; }
		io.WideHostFence();
		if (r.nonfinite || !PioneerWideOutputMapStable(io,wide,host,transfer) || !PioneerBulkUploadGuard(io,baseline,master.noc,host,store,transfer,0x402)) { uncertain=true; break; }
		r.words=151936; for (unsigned i=0;i<28;i++) kv.tokens[i]=tokens;
		m.state=TT_RUNTIME_PASSED; io.RuntimeProfileMark(TT_PROFILE_PIO);
	} while (false);
	if (bme) { bool off=false; for (unsigned i=0;i<2 && !off;i++) { io.HostPciWrite(0,4,2,0x402); io.HostPciWrite(1,4,2,0x402); off=BlackholeHostReadPciQuiet(io,0x402); } if (!off) uncertain=true; }
	if (!uncertain && !code.running && !stack.running && !attention.running && io.DecoderMappingStable() && PioneerDecoderPeerOwners(io,fleet,code)
		&& PioneerCoordinatorLocalHeld(io,baseline,master.noc,store)) {
		if (previous) { io.RuntimeCodeWrite(0,previous); if (io.RuntimeCodeRead(0)!=previous || !PioneerCoordinatedMasterBoundary(io,baseline,fleet,true)) uncertain=true; }
	} else uncertain=true;
	if (uncertain) {
		if (code.parallel) for (unsigned i=0;i<8;i++) code.peers[i].unsafe=true;
		m.recovery_required=1; code.unsafe=stack.unsafe=attention.unsafe=bulk.unsafe=true; store.recovery_required=true;
		PioneerQuarantineWideOutput(io,wide); PioneerPersistentQuarantine(io,fleet); PioneerQuarantineOutputBuffer(io,transfer);
	}
	bool proved=!uncertain && PioneerDecoderProof(r,tokens,stack.nonce,code.parallel);
	if (proved && code.profiling) {
		tt_decoder_stage_profile candidate={};
		for (unsigned i=0;i<32;i++) reinterpret_cast<uint32_t*>(&candidate)[i]=io.BulkL1Read((TT_DECODER_STAGE_L1-TT_BULK_UPLOAD_L1)/4+i);
		if (PioneerDecoderStageValid(candidate,m.nonce,tokens)) code.stages=candidate;
	}
	return proved; // Diagnostic availability NEVER authorizes inference/reset.
}
#endif
