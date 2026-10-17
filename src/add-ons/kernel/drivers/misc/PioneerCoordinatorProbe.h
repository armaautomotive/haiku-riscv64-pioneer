/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_COORDINATOR_PROBE_H
#define PIONEER_COORDINATOR_PROBE_H
#include "PioneerPeerPcRead.h"
#include "PioneerResidentAttention.h"
#include "BlackholeCoordinatorImage.h"
#include "BlackholeFastCoordinatorImage.h"
#include "BlackholeCoordinatorReadbackImage.h"
#include "BlackholeFusedCoordinatorImage.h"
#include "BlackholeCompactCoordinatorImage.h"
struct PioneerCoordinatorLease {
	bool saved=false,dirty=false,ready=false,running=false,unsafe=false;
	bool fast=false; // immutable for this code/stack ownership lease
	bool readback=false;
	bool fused=false;
	bool compact=false,inputValid=false;
	uint32_t inputK=0,inputShadow[3072]={},scaleShadow[192]={};
	uint32_t readbackNonce=0;
	uint32_t nonce=0,expected[119]={};
};
static inline uint32_t PioneerCoordinatorWord(bool fast,bool readback,unsigned word,bool fused=false,bool compact=false)
{
	return compact?BlackholeCompactCoordinatorCodeWord(word):fused?BlackholeFusedCoordinatorCodeWord(word):readback?BlackholeCoordinatorReadbackCodeWord(word):fast?BlackholeFastCoordinatorCodeWord(word):BlackholeCoordinatorCodeWord(word);
}
struct tt_coordinator_probe_result {
	uint32_t version,state,workers,matched,nonce,prepared,started,completed,stopped,recovery_required;
	uint32_t polls,phase,count,marker,pc[2],before[8],after[8],summary_verified;
};
static inline bool PioneerCoordinatorProbeCounters(unsigned workers,const uint32_t* before,const uint32_t* after)
{
	if (!workers || workers>119) return false;
	const unsigned expected[]={4*workers,4*workers,4*workers,4*workers,0,0,4*workers,0};
	for (unsigned i=0;i<8;i++) if (uint32_t(after[i]-before[i])!=expected[i]) return false;
	return true;
}
static inline bool PioneerCoordinatorProbeProof(const tt_coordinator_probe_result& r,unsigned workers,unsigned nonce)
{
	return r.version==1 && r.state==TT_RUNTIME_PASSED && r.workers==workers && r.matched==workers
		&& r.nonce==nonce && r.nonce && r.nonce<0x7fffffff && r.prepared==1 && r.started==1 && r.completed==1 && r.stopped==1
		&& !r.recovery_required && r.polls && r.phase==TT_COORDINATOR_DONE && r.count==workers
		&& r.marker==TT_COORDINATOR_MARKER && r.pc[0]==TT_COORDINATOR_PARK && r.pc[1]==r.pc[0]
		&& r.before[2]==r.before[6] && r.after[2]==r.after[6]
		&& r.summary_verified==1 && PioneerCoordinatorProbeCounters(workers,r.before,r.after);
}
template<class IO>
bool PioneerCoordinatorSources(IO& io,const tt_dram_samples_result& baseline,
	const tt_compute_noc_result& noc,const PioneerResidentLease& store,uint32_t pci=0x402)
{
	if (!store.ready || store.pending || store.recovery_required || !io.CoordinatorMappingStable()
		|| !io.BulkMappingStable() || !PioneerRuntimeSources(io,baseline,noc,store,pci)) return false;
	for (unsigned s=0;s<2;s++) for (unsigned w=0;w<3;w++)
		if (io.DramTlbRead(s,w)!=BlackholeDramFixedTlbWord(s?0xffa00000:0,1,2,false,w)) return false;
	return true;
}
template<class IO>
bool PioneerCoordinatorLocalHeld(IO& io,const tt_dram_samples_result& baseline,
	const tt_compute_noc_result& noc,const PioneerResidentLease& store)
{
	return io.SelectRuntimeCore(0) && io.ResetRead()==0x47800 && io.ResetRead()==0x47800
		&& BlackholeResidentIdle(io) && PioneerCoordinatorSources(io,baseline,noc,store);
}
template<class IO>
bool PioneerCoordinatorPrepare(IO& io,const tt_dram_samples_result& baseline,
	const tt_compute_noc_result& noc,PioneerResidentLease& store,PioneerCoordinatorLease& lease,bool fast=false,bool readback=false,bool fused=false,bool compact=false)
{
	if ((readback && !fast) || (fused && (!fast || readback)) || (compact && !fused)) return false;
	if (lease.ready) return !lease.unsafe && !lease.running && lease.fast==fast && lease.readback==readback && lease.fused==fused && lease.compact==compact;
	if (lease.saved || lease.unsafe || !io.CoordinatorSavedWords() || !PioneerCoordinatorLocalHeld(io,baseline,noc,store)) return false;
	uint32_t* saved=io.CoordinatorSavedWords();
	for (unsigned w=0;w<TT_COORDINATOR_SAVED_WORDS;w++) saved[w]=io.CoordinatorRead(w);
	lease.saved=true; lease.fast=fast; lease.readback=readback; lease.fused=fused; lease.compact=compact;
	lease.inputValid=false; lease.inputK=0;
	if (!PioneerCoordinatorLocalHeld(io,baseline,noc,store)) { lease.unsafe=true; return false; }
	lease.dirty=true;
	for (unsigned w=0;w<TT_COORDINATOR_CODE_WORDS;w++) io.CoordinatorWrite(w,PioneerCoordinatorWord(fast,readback,w,fused,compact));
	bool exact=true;
	for (unsigned w=0;w<TT_COORDINATOR_CODE_WORDS;w++) exact &= io.CoordinatorRead(w)==PioneerCoordinatorWord(fast,readback,w,fused,compact);
	lease.ready=exact; return exact;
}
template<class IO>
bool PioneerCoordinatorClose(IO& io,const tt_dram_samples_result& baseline,
	const tt_compute_noc_result& noc,PioneerResidentLease& store,PioneerCoordinatorLease& lease,bool safe)
{
	if (!lease.saved) return safe && !lease.unsafe;
	safe=safe && !lease.unsafe && !lease.running && io.CoordinatorSavedWords()
		&& PioneerCoordinatorLocalHeld(io,baseline,noc,store);
	if (safe && lease.dirty) {
		const uint32_t* saved=io.CoordinatorSavedWords();
		for (unsigned w=0;w<TT_COORDINATOR_SAVED_WORDS;w++) io.CoordinatorWrite(w,saved[w]);
		bool exact=true;
		for (unsigned w=0;w<TT_COORDINATOR_SAVED_WORDS;w++) exact &= io.CoordinatorRead(w)==saved[w];
		if (exact) lease.dirty=false;
	}
	lease.ready=false; lease.unsafe |= !safe || lease.dirty;
	if (lease.unsafe) { store.recovery_required=true; io.QuarantineHostPage(); }
	return !lease.unsafe;
}
template<class IO>
bool PioneerCoordinatorProbe(IO& io,const tt_dram_samples_result& baseline,PioneerFleetWorker* fleet,
	PioneerBulkUploadLease& bulk,PioneerCoordinatorLease& lease,bool attentionReady,unsigned workers,tt_coordinator_probe_result& r)
{
	r={}; r.version=1; r.workers=workers;
	if (!workers || workers>119 || !fleet || !bulk.ready || !bulk.saved || bulk.unsafe
		|| !lease.ready || lease.fast || lease.running || lease.unsafe || lease.nonce>=0x7ffffffe
		|| !PioneerPeerPcMasterHeld(io,baseline,fleet,attentionReady)) return false;
	auto& store=fleet[0].Store(); const auto& noc=fleet[0].noc;
	r.prepared=1; r.state=TT_RUNTIME_CONFIG_FAILED;
	bool uncertain=false; uint32_t previous=io.RuntimeCodeRead(0);
	uint32_t* stage=io.RuntimeStageWords(); if (!stage) return false;
	do {
		for (unsigned w=1;w<=workers;w++) {
			if (fleet[w].noc.version!=PioneerWorkerVersion(w) || fleet[w].noc.state!=TT_COMPUTE_NOC_PASSED
				|| !PioneerWorkerRoute(baseline,fleet[w].noc.source[0],w) || !fleet[w].noc.tlb_restored
				|| !io.SelectRuntimeCore(w) || io.ResetRead()!=0x47800 || io.ResetRead()!=0x47800) { uncertain=true; break; }
			io.Pause(); uint32_t first=io.RuntimePcRead(); io.Pause(); uint32_t second=io.RuntimePcRead();
			if (first!=second) { uncertain=true; break; }
			lease.expected[w-1]=first;
		}
		if (uncertain || !PioneerPeerPcMasterHeld(io,baseline,fleet,attentionReady)) { uncertain=true; break; }
		for (unsigned i=0;i<1024;i++) stage[i]=0;
		stage[0]=0x31445243; stage[1]=0; stage[2]=workers; // operation is ALWAYS read-only, never caller-selectable
		stage[6]=++lease.nonce; r.nonce=lease.nonce;
		for (unsigned w=0;w<workers;w++) stage[64+w]=lease.expected[w];
		bulk.dirty=true;
		if (!PioneerBulkPayloadStage(io,stage,1024,0)) break;
		store.l1_dirty=true; store.l1_restored=false;
		for (unsigned i=0;i<3;i++) io.RuntimeStatusWrite(i,0);
		bool exact=true; for (unsigned i=0;i<3;i++) exact &= io.RuntimeStatusRead(i)==0;
		if (!exact) break;
		io.RuntimeCodeWrite(0,TT_COORDINATOR_BOOT);
		if (io.RuntimeCodeRead(0)!=TT_COORDINATOR_BOOT || !PioneerCoordinatorLocalHeld(io,baseline,noc,store)) break;
		io.RuntimeResetWrite(0x87ffff); io.Pause();
		if (io.ResetRead()!=0x87ffff || io.ResetRead()!=0x87ffff || !PioneerCoordinatorSources(io,baseline,noc,store)
			|| !BlackholeResidentIdle(io)) { uncertain=true; break; }
		PioneerResidentAttentionCounters(io,r.before);
		lease.running=true; r.started=1; r.state=TT_RUNTIME_COMPLETION_FAILED;
		io.RuntimeResetWrite(0x46800);
		for (unsigned attempt=0;attempt<50000;attempt++) {
			r.polls++;
			if (io.ResetRead()!=0x46800) break;
			r.phase=io.RuntimeStatusRead(0); r.count=io.RuntimeStatusRead(1); r.marker=io.RuntimeStatusRead(2);
			if (r.marker==TT_COORDINATOR_FAULT) break;
			if (r.phase==TT_COORDINATOR_DONE && r.count==workers && r.marker==TT_COORDINATOR_MARKER) {
				io.Pause(); r.pc[0]=io.RuntimePcRead(); io.Pause(); r.pc[1]=io.RuntimePcRead();
				PioneerResidentAttentionCounters(io,r.after);
				if (r.pc[0]==TT_COORDINATOR_PARK && r.pc[1]==r.pc[0] && BlackholeResidentIdle(io)
					&& PioneerCoordinatorProbeCounters(workers,r.before,r.after)) { r.completed=1; break; }
			}
			io.Pause();
		}
		if (!r.completed || io.ResetRead()!=0x46800 || !PioneerCoordinatorSources(io,baseline,noc,store)
			|| !BlackholeResidentIdle(io)) { uncertain=true; break; }
		io.RuntimeResetWrite(0x87ffff); io.Pause();
		if (io.ResetRead()!=0x87ffff || io.ResetRead()!=0x87ffff) { uncertain=true; break; }
		io.RuntimeResetWrite(0x47800); io.Pause();
		if (!PioneerCoordinatorLocalHeld(io,baseline,noc,store)) { uncertain=true; break; }
		lease.running=false; r.stopped=1;
		uint32_t summary[24]; for (unsigned i=0;i<24;i++) summary[i]=io.BulkL1Read(3072+i);
		bool summaryExact=summary[0]==workers*4 && summary[1]==workers*4 && !summary[2] && !summary[3]
			&& summary[4]==workers && !summary[5] && summary[6]==r.nonce && !summary[7];
		for (unsigned i=0;i<8;i++) summaryExact &= summary[8+i]==r.before[i] && summary[16+i]==r.after[i];
		if (!summaryExact) break;
		r.summary_verified=1;
		for (unsigned w=1;w<=workers;w++) {
			if (!io.SelectRuntimeCore(0) || io.BulkL1Read(1024+w-1)!=lease.expected[w-1]
				|| !io.SelectRuntimeCore(w) || io.ResetRead()!=0x47800 || io.ResetRead()!=0x47800) { uncertain=true; break; }
			io.Pause(); uint32_t first=io.RuntimePcRead(); io.Pause(); uint32_t second=io.RuntimePcRead();
			if (first!=lease.expected[w-1] || second!=first) { uncertain=true; break; }
			r.matched++;
		}
		if (uncertain || !PioneerCoordinatorLocalHeld(io,baseline,noc,store)) { uncertain=true; break; }
		r.state=TT_RUNTIME_PASSED;
	} while (false);
	// Only a proved-held/not-started or proved-complete/stopped master can
	// regain its previous immutable entry. Never reset on an unknown start.
	if (!uncertain && !lease.running && io.ResetRead()==0x47800
		&& PioneerCoordinatorLocalHeld(io,baseline,noc,store)) {
		io.RuntimeCodeWrite(0,previous);
		if (io.RuntimeCodeRead(0)!=previous) uncertain=true;
	} else uncertain=true;
	if (uncertain) {
		lease.unsafe=true; store.recovery_required=true; bulk.unsafe=true; r.recovery_required=1;
		io.QuarantineHostPage();
	}
	return PioneerCoordinatorProbeProof(r,workers,lease.nonce);
}
#endif
