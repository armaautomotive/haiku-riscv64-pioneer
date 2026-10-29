/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_CONV_HISTORY_ZERO_CONTROLLER_H
#define PIONEER_Q38_CONV_HISTORY_ZERO_CONTROLLER_H
#include <stddef.h>
#include "PioneerQ38ConvHistoryBanks.h"
#include "PioneerQ38StateZeroImage.h"
#include "PioneerQ38StateZeroShape.h"
#include "PioneerQ38SpanController.h"
// Real canonical convolution-history zero controller;336 regions x5pages.
// Reuses the EXACT frozen zero-only image:each private20KiB stripe is
// independently zeroed and freshly read back,including allocation padding.
// Only full1680page coverage plus known-clean code/scratch/hold/source
// CLOSE publishes initialization. NOT current-token/compute readiness.
// Native ConvHistoryZeroBeforeCohort MUST bind the actual new joint owner,
// source/cookie/clock/ALL host leases and exclude EVERY other initiator.
// No native selector/factory yet;old projection/zero/Work roles unchanged.
struct PioneerQ38ConvHistoryZeroLease {
	bool saved=false,dirty=false,ready=false,running=false,unsafe=false,closed=false;
	uint32_t nonce=0;
	PioneerQ38GdnIdentity identity={};
	const PioneerQ38ConvHistoryBanks* owner=nullptr;
	const PioneerResidentBanks* banks=nullptr;
	const tt_dram_samples_result* proof=nullptr;
	PioneerQ38ConvHistoryZeroLease()=default;
	PioneerQ38ConvHistoryZeroLease(const PioneerQ38ConvHistoryZeroLease&)=delete;
	PioneerQ38ConvHistoryZeroLease& operator=(const PioneerQ38ConvHistoryZeroLease&)=delete;
};
struct PioneerQ38ConvHistoryZeroResult {
	uint32_t version=1,state=0,bytes=0,pages=0,nonce=0,started=0,completed=0,held=0,recovery_required=0;
	uint32_t phase=0,count=0,marker=0,pc[2]={},before[8]={},after[8]={},summary[32]={};
};
static inline bool PioneerQ38ConvHistoryZeroPc(uint32_t pc) { return pc==0x72038 || pc==0x7203c; }
static inline bool PioneerQ38ConvHistoryZeroBound(const PioneerQ38ConvHistoryBanks& state,const PioneerResidentBanks& banks,
	const tt_dram_samples_result& proof,const PioneerQ38ConvHistoryZeroLease& lease)
{
	// Structural owner identity only; physical/source changes on an already
	// bound operation must still reach Sources and latch CLOSE ONLY.
	return lease.owner==&state && lease.banks==&banks && lease.proof==&proof;
}
static inline bool PioneerQ38ConvHistoryZeroPristine(const PioneerQ38ConvHistoryZeroLease& lease)
{
	return !lease.saved && !lease.dirty && !lease.ready && !lease.running && !lease.unsafe
		&& !lease.nonce && !lease.owner && !lease.banks && !lease.proof
		&& lease.identity==PioneerQ38GdnIdentity{};
}
template<class IO> bool PioneerQ38ConvHistoryZeroFail(IO& io,PioneerQ38ConvHistoryBanks& state,PioneerResidentBanks& banks,
	PioneerResidentLease& store,PioneerBulkUploadLease& bulk,PioneerOutputBufferLease& transfer,
	PioneerWideOutputLease& wide,PioneerQ38ConvHistoryZeroLease& lease)
{
	lease.unsafe=true; state.Fault(); banks.Quarantine(); store.recovery_required=true; bulk.unsafe=true;
	io.QuarantineHostPage(); PioneerQuarantineOutputBuffer(io,transfer); PioneerQuarantineWideOutput(io,wide);
	return false; // MEMORY ONLY;no card read/hold/END/restore/retry/release
}
template<class IO> bool PioneerQ38ConvHistoryZeroSources(IO& io,const tt_dram_samples_result& dram,const tt_compute_noc_result& noc,
	const PioneerQ38ConvHistoryBanks& state,const PioneerResidentBanks& banks,const tt_host_page_result& input,
	const PioneerResidentLease& store,const PioneerBulkUploadLease& bulk,const PioneerOutputBufferLease& transfer,
	const PioneerWideOutputLease& wide,const PioneerQ38ConvHistoryZeroLease& lease,uint32_t reset)
{
	PioneerQ38ConvHistoryExtent probe;
	if (!PioneerQ38ConvHistoryZeroBound(state,banks,dram,lease) || lease.unsafe || lease.closed
		|| !state.OwnsInitializationLease(&lease)
		|| !state.MatchesInitializationOwner(banks,dram,lease.identity)
		|| !io.ConvHistoryZeroBeforeCohort() || state.Unsafe() || !banks.Matches(dram)
		|| !state.InitializeSpan(lease.identity,0,0,1,probe) || !store.ready || store.pending || store.recovery_required
		|| !bulk.ready || !bulk.saved || bulk.unsafe || !transfer.ready || transfer.pending || transfer.quarantined
		|| !wide.ready || !wide.memory_type_set || wide.released || wide.quarantined || wide.pending) return false;
	return io.ConvHistoryZeroOtherCoresHeld() && io.SelectRuntimeCore(0) && io.Q38PanelMappingStable() && io.BulkMappingStable()
		&& io.ResetRead()==reset && io.ResetRead()==reset && PioneerRuntimeSources(io,dram,noc,store,0x402)
#ifdef PIONEER_Q38_TOKEN_NATIVE_CONTEXT
  && io.TokenInitializationFifoEmpty(1)
#endif
		&& BlackholeResidentIdle(io) && BlackholeHostReadMapStable(io,input)
		&& PioneerOutputBufferMapStable(io,transfer,input) && PioneerWideOutputMapStable(io,wide,input,transfer);
}
template<class IO> bool PioneerQ38ConvHistoryZeroCodeMatches(IO& io)
{
	if (!io.Q38PanelMappingStable() || io.RuntimeCodeRead(0)!=TT_Q38_STATE_ZERO_BOOT) return false;
	for (uint32_t at=0x70000;at<0x74000;at+=4)
		if (io.Q38PanelRead(at)!=(at<0x72000?0:PioneerQ38StateZeroCodeWord((at-0x72000)/4))) return false;
	return true;
}
template<class IO> bool PioneerQ38ConvHistoryZeroPrepare(IO& io,const tt_dram_samples_result& dram,const tt_compute_noc_result& noc,
	PioneerQ38ConvHistoryBanks& state,PioneerResidentBanks& banks,const PioneerQ38GdnIdentity& identity,const tt_host_page_result& input,
	PioneerResidentLease& store,PioneerBulkUploadLease& bulk,PioneerOutputBufferLease& transfer,
	PioneerWideOutputLease& wide,PioneerQ38ConvHistoryZeroLease& lease)
{
	PioneerQ38ConvHistoryExtent probe;
	if (!PioneerQ38ConvHistoryZeroPristine(lease) || lease.closed
		|| !state.MatchesInitializationOwner(banks,dram,identity) || state.InitializationPublished(identity)
		|| !io.ConvHistoryZeroBeforeCohort() || !state.InitializeSpan(identity,0,0,1,probe) || wide.pending) return false;
	uint32_t* saved=io.Q38PanelSavedWords();
	if (!saved || !io.SpanViewCapacity(TT_Q38_SPAN_VIEW_WORDS+1)) return false;
	if (!state.ClaimInitialization(identity,&lease)) return false;
	lease.identity=identity; lease.owner=&state; lease.banks=&banks; lease.proof=&dram;
	auto fail=[&]() { return PioneerQ38ConvHistoryZeroFail(io,state,banks,store,bulk,transfer,wide,lease); };
	if (!PioneerQ38ConvHistoryZeroSources(io,dram,noc,state,banks,input,store,bulk,transfer,wide,lease,0x47800)) return fail();
	saved[0]=io.RuntimeCodeRead(0);
	for (unsigned word=0;word<TT_Q38_SPAN_VIEW_WORDS;word++) saved[word+1]=io.Q38PanelRead(TT_Q38_SPAN_VIEW_BASE+word*4);
	lease.saved=true;
	if (!PioneerQ38ConvHistoryZeroSources(io,dram,noc,state,banks,input,store,bulk,transfer,wide,lease,0x47800)) return fail();
	lease.dirty=true;
	for (uint32_t at=0x70000;at<0x74000;at+=4) io.Q38PanelWrite(at,at<0x72000?0:PioneerQ38StateZeroCodeWord((at-0x72000)/4));
	io.RuntimeCodeWrite(0,TT_Q38_STATE_ZERO_BOOT);
	if (!PioneerQ38ConvHistoryZeroCodeMatches(io)
		|| !PioneerQ38ConvHistoryZeroSources(io,dram,noc,state,banks,input,store,bulk,transfer,wide,lease,0x47800)) return fail();
	lease.ready=true; return true;
}
template<class IO> bool PioneerQ38ConvHistoryZeroRun(IO& io,const tt_dram_samples_result& dram,const tt_compute_noc_result& noc,
	PioneerQ38ConvHistoryBanks& state,PioneerResidentBanks& banks,const tt_host_page_result& input,PioneerResidentLease& store,
	PioneerBulkUploadLease& bulk,PioneerOutputBufferLease& transfer,PioneerWideOutputLease& wide,PioneerQ38ConvHistoryZeroLease& lease,
	const PioneerQ38GdnIdentity& identity,unsigned region,unsigned firstPage,unsigned pages,PioneerQ38ConvHistoryZeroResult& r)
{
	r={}; r.version=1; r.state=TT_RUNTIME_CONFIG_FAILED;
	PioneerQ38ConvHistoryExtent extent;
	if (!PioneerQ38ConvHistoryZeroBound(state,banks,dram,lease) || !state.OwnsInitializationLease(&lease)
		|| !lease.ready || lease.unsafe || lease.closed
		|| lease.running || lease.nonce>=0x7ffffffe || !(identity==lease.identity)) return false;
	// Duplicate/overlapping/published spans are configuration refusals BEFORE
	// any I/O. A valid fresh request still re-proves every source below.
	if (region>=PioneerQ38ConvHistoryBanks::Regions || !pages || pages>PioneerQ38ConvHistoryBanks::RegionPages
		|| firstPage>=PioneerQ38ConvHistoryBanks::RegionPages
		|| pages>PioneerQ38ConvHistoryBanks::RegionPages-firstPage) return false;
	auto fail=[&]() {
		r.state=TT_RUNTIME_COMPLETION_FAILED; r.recovery_required=1; r.pages=0;
		return PioneerQ38ConvHistoryZeroFail(io,state,banks,store,bulk,transfer,wide,lease);
	};
	if (!state.FreshInitializationSpan(identity,region,firstPage,pages,extent)) {
		unsigned progress=0;
		if (!state.InitializationProgress(identity,progress)) return fail();
		return false;
	}
	if (!PioneerQ38ConvHistoryZeroSources(io,dram,noc,state,banks,input,store,bulk,transfer,wide,lease,0x47800)
		|| !PioneerQ38ConvHistoryZeroCodeMatches(io)) return fail();
	uint32_t descriptor[64]={}; descriptor[0]=TT_Q38_ZERO_MARKER; descriptor[1]=descriptor[2]=1;
	descriptor[4]=extent.bytes; descriptor[5]=extent.address; descriptor[6]=lease.nonce+1;
	descriptor[7]=extent.bank; descriptor[8]=extent.route;
	for (unsigned word=0;word<16;word++) descriptor[16+word]=word?noc.source[0][word]:store.config|0x10000;
	if (!PioneerQ38StateZeroShape(descriptor)) return fail();
	bulk.dirty=true;
	for (unsigned word=0;word<64;word++) io.BulkL1Write(word,descriptor[word]);
	for (unsigned word=0;word<64;word++) if (io.BulkL1Read(word)!=descriptor[word]) return fail();
	store.l1_dirty=true; store.l1_restored=false;
	for (unsigned word=0;word<3;word++) io.RuntimeStatusWrite(word,0);
	for (unsigned word=0;word<3;word++) if (io.RuntimeStatusRead(word)) return fail();
	if (!PioneerQ38ConvHistoryZeroSources(io,dram,noc,state,banks,input,store,bulk,transfer,wide,lease,0x47800)) return fail();
	for (unsigned word=0;word<8;word++) r.before[word]=io.ResidentNocRead(PioneerQ38SpanCounter(word));
	uint64_t began=io.Q38CohortNow(); if (began>UINT64_MAX-UINT64_C(30000000)) return fail();
	lease.running=true; lease.nonce=descriptor[6]; r.nonce=lease.nonce; r.bytes=extent.bytes;
	// First uncertain reset retains ALL owners. NEVER enable PCI bus mastering.
	io.RuntimeResetWrite(0x87ffff); io.Pause();
	if (!PioneerQ38ConvHistoryZeroSources(io,dram,noc,state,banks,input,store,bulk,transfer,wide,lease,0x87ffff)) return fail();
	r.started=1; io.RuntimeResetWrite(0x46800); if (io.ResetRead()!=0x46800) return fail();
	for (unsigned attempt=0;attempt<65536;attempt++) {
		uint64_t now=io.Q38CohortNow(); if (now<began || now-began>=UINT64_C(30000000)) return fail();
		r.marker=io.RuntimeStatusRead(2);
		if (r.marker && r.marker!=TT_Q38_ZERO_MARKER) return fail();
		if (r.marker) {
			r.phase=io.RuntimeStatusRead(0); if (r.phase!=0x7200c) return fail();
			r.count=io.RuntimeStatusRead(1); if (r.count!=extent.pages) return fail();
			io.Pause(); r.pc[0]=io.RuntimePcRead(); io.Pause(); r.pc[1]=io.RuntimePcRead();
			if (!PioneerQ38ConvHistoryZeroPc(r.pc[0]) || !PioneerQ38ConvHistoryZeroPc(r.pc[1])) return fail();
			r.completed=1; break;
		}
		io.Pause();
	}
	if (!r.completed) return fail(); // also bounded if the CPU clock stalls
	if (!PioneerQ38ConvHistoryZeroSources(io,dram,noc,state,banks,input,store,bulk,transfer,wide,lease,0x46800)
		|| !PioneerQ38ConvHistoryZeroCodeMatches(io)) return fail();
	for (unsigned word=0;word<8;word++) r.after[word]=io.ResidentNocRead(PioneerQ38SpanCounter(word));
	for (unsigned word=0;word<32;word++) r.summary[word]=io.BulkL1Read(3072+word);
	if (!PioneerQ38StateZeroSummary(r.summary,descriptor,r.before,r.after)) return fail();
	for (unsigned word=0;word<64;word++) if (io.BulkL1Read(word)!=descriptor[word]) return fail();
	unsigned last=(extent.bytes-1)&~16383U;
	const uint32_t command[]={extent.address+last,0,extent.route,0x144000,0,129,0,0,extent.bytes-last,0};
	for (unsigned word=0;word<10;word++) if (io.ResidentNocRead(word*4)!=command[word]) return fail();
	PioneerQ38ConvHistoryExtent confirmed;
	if (!state.InitializeSpan(identity,region,firstPage,pages,confirmed) || confirmed.bank!=extent.bank || confirmed.address!=extent.address
		|| confirmed.route!=extent.route || confirmed.index!=extent.index || confirmed.pages!=extent.pages || confirmed.bytes!=extent.bytes
		|| !PioneerQ38ConvHistoryZeroSources(io,dram,noc,state,banks,input,store,bulk,transfer,wide,lease,0x46800)) return fail();
	io.Pause(); r.pc[0]=io.RuntimePcRead(); io.Pause(); r.pc[1]=io.RuntimePcRead();
	if (!PioneerQ38ConvHistoryZeroPc(r.pc[0]) || !PioneerQ38ConvHistoryZeroPc(r.pc[1])) return fail();
	// ALL extent/code/source/descriptor/summary/command/counter/PC proofs
	// precede the FIRST post-execution hold. No retry/rollback on any failure.
	io.RuntimeResetWrite(0x87ffff); io.Pause();
	if (!PioneerQ38ConvHistoryZeroSources(io,dram,noc,state,banks,input,store,bulk,transfer,wide,lease,0x87ffff)) return fail();
	io.RuntimeResetWrite(0x47800); io.Pause();
	if (!PioneerQ38ConvHistoryZeroSources(io,dram,noc,state,banks,input,store,bulk,transfer,wide,lease,0x47800)) return fail();
	// Private CPU progress ONLY,after EVERY physical completion/hold/source
	// proof. Never mark learned-weight pages or publish model/GDN readiness.
	if (!state.CommitInitialization(identity,region,firstPage,pages)) return fail();
	lease.running=false; r.pages=extent.pages; r.held=1; r.state=TT_RUNTIME_PASSED;
	return true; // NOT a whole-state/model-ready or initialized-weight commit
}
template<class IO> bool PioneerQ38ConvHistoryZeroClose(IO& io,const tt_dram_samples_result& dram,const tt_compute_noc_result& noc,
	PioneerQ38ConvHistoryBanks& state,PioneerResidentBanks& banks,const tt_host_page_result& input,PioneerResidentLease& store,
	PioneerBulkUploadLease& bulk,PioneerOutputBufferLease& transfer,PioneerWideOutputLease& wide,PioneerQ38ConvHistoryZeroLease& lease,bool knownClean)
{
	// A pristine/already-closed sibling is NOT a clean-context grant after
	// another role quarantined the shared reservation. No further IO.
	if (lease.unsafe || state.Unsafe() || banks.Quarantined()) return false;
	if (PioneerQ38ConvHistoryZeroPristine(lease)) {
		if (!knownClean) return false;
		lease.closed=true; return true; // no owner was acquired or touched
	}
	if (!PioneerQ38ConvHistoryZeroBound(state,banks,dram,lease)) return false;
	if (lease.closed) return knownClean && state.MatchesInitializationOwner(banks,dram,lease.identity);
	if (!state.OwnsInitializationLease(&lease)) return false;
	auto fail=[&]() { return PioneerQ38ConvHistoryZeroFail(io,state,banks,store,bulk,transfer,wide,lease); };
	if (!knownClean || lease.running) return fail();
	if (!lease.saved) return fail(); // a bound operation cannot have a pristine CLOSE
	if (!lease.ready || !PioneerQ38ConvHistoryZeroSources(io,dram,noc,state,banks,input,store,bulk,transfer,wide,lease,0x47800)
		|| !PioneerQ38ConvHistoryZeroCodeMatches(io)) return fail();
	const uint32_t* saved=io.Q38PanelSavedWords(); const uint32_t* bulkSaved=io.BulkSavedWords();
	if (!saved || !bulkSaved) return fail();
	if (lease.dirty) {
		for (unsigned word=0;word<TT_Q38_SPAN_VIEW_WORDS;word++) io.Q38PanelWrite(TT_Q38_SPAN_VIEW_BASE+word*4,saved[word+1]);
		for (unsigned word=0;word<TT_Q38_SPAN_VIEW_WORDS;word++) if (io.Q38PanelRead(TT_Q38_SPAN_VIEW_BASE+word*4)!=saved[word+1]) return fail();
	}
	if (bulk.dirty) {
		for (unsigned word=0;word<TT_BULK_UPLOAD_SAVED_WORDS;word++) io.BulkL1Write(word,bulkSaved[word]);
		for (unsigned word=0;word<TT_BULK_UPLOAD_SAVED_WORDS;word++) if (io.BulkL1Read(word)!=bulkSaved[word]) return fail();
	}
	io.RuntimeCodeWrite(0,saved[0]); if (io.RuntimeCodeRead(0)!=saved[0]) return fail();
	if (!PioneerQ38ConvHistoryZeroSources(io,dram,noc,state,banks,input,store,bulk,transfer,wide,lease,0x47800)) return fail();
	lease.dirty=lease.ready=false; lease.closed=true; bulk.dirty=false;
	unsigned initializedPages=0;
	if (!state.InitializationProgress(lease.identity,initializedPages)) return fail();
	// A full count is NOT publication. ONLY after every physical code/
	// scratch/source/hold proof above and successful known-clean CLOSE may
	// this private initialization seal be minted. Partial CLOSE is safe
	// for END but does not publish. No compute/read/model-ready API here.
	if (initializedPages==PioneerQ38ConvHistoryBanks::Pages && !state.PublishInitialization(lease.identity)) return fail();
	if (!state.CloseInitialization(lease.identity,&lease)) return fail();
	return true; // bank/mutable claims/global status/host owners STILL owned
}
#endif
