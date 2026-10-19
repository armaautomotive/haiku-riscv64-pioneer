/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_SPAN_CONTROLLER_H
#define PIONEER_Q38_SPAN_CONTROLLER_H
#include "PioneerQ38SpanAuthority.h"
#include "PioneerQ38SpanImage.h"
#include "PioneerRuntimeMatvec.h"
#include "PioneerBulkUpload.h"
#include "PioneerWideOutput.h"
// Lease/result/descriptor/physical addresses are KERNEL-PRIVATE,not a wire
// request. The adapter forbids any cohort/legacy compute while this lease
// is open,and retains the entire context/cookie/ALL host owners after
// unsafe. Closing this image does NOT end the underlying bank/NIU owner.
#if defined(PIONEER_Q38_SPAN_REUSE_CODE) && !defined(PIONEER_Q38_SPAN_FAST)
#error Work code-proof reuse requires the separately qualified Work transport
#endif
enum { TT_Q38_SPAN_VIEW_BASE=0x6e000,TT_Q38_SPAN_VIEW_BYTES=0x42000,
	TT_Q38_SPAN_VIEW_WORDS=TT_Q38_SPAN_VIEW_BYTES/4 };
struct PioneerQ38SpanLease {
	bool saved=false,dirty=false,ready=false,running=false,unsafe=false,closed=false;
	uint32_t nonce=0;
};
struct PioneerQ38SpanResult {
	uint32_t version=1,state=0,bytes=0,pages=0,nonce=0,started=0,completed=0,held=0,recovery_required=0;
	uint32_t host_exact=0,matched_words=0,phase=0,count=0,marker=0,pc[2]={};
	uint32_t before[8]={},after[8]={},summary[32]={}; // private host physical echo NEVER exported
};
static inline unsigned PioneerQ38SpanCounter(unsigned w)
{
	const unsigned offsets[]={0x208,0x20c,0x210,0x214,0x204,0x220,0x210,0x228}; return offsets[w];
}
template<class IO> bool PioneerQ38SpanFail(IO& io,PioneerResidentBanks& banks,PioneerResidentLease& store,
	PioneerBulkUploadLease& bulk,PioneerOutputBufferLease& transfer,PioneerWideOutputLease& wide,PioneerQ38SpanLease& lease)
{
	lease.unsafe=true; banks.Quarantine(); store.recovery_required=true; bulk.unsafe=true;
	io.QuarantineHostPage(); PioneerQuarantineOutputBuffer(io,transfer); PioneerQuarantineWideOutput(io,wide);
	return false; // memory bookkeeping ONLY;NO card read/write/END/retry/free
}
template<class IO> bool PioneerQ38SpanSources(IO& io,const tt_dram_samples_result& dram,const tt_compute_noc_result& noc,
	const PioneerResidentBanks& banks,const tt_host_page_result& input,const PioneerResidentLease& store,
	const PioneerBulkUploadLease& bulk,const PioneerOutputBufferLease& transfer,const PioneerWideOutputLease& wide,
	const PioneerQ38SpanLease& lease,uint32_t reset,uint32_t pci)
{
	if (lease.unsafe || lease.closed || !io.SpanBeforeCohort() || !banks.Matches(dram)
		|| !store.ready || store.pending || store.recovery_required || !bulk.ready || !bulk.saved || bulk.unsafe
		|| !transfer.ready || transfer.pending || transfer.quarantined || !wide.ready || !wide.memory_type_set
		|| wide.released || wide.quarantined || wide.pending!=lease.running) return false;
	return io.SelectRuntimeCore(0) && io.Q38PanelMappingStable() && io.BulkMappingStable()
		&& io.ResetRead()==reset && io.ResetRead()==reset && PioneerRuntimeSources(io,dram,noc,store,pci)
		&& BlackholeResidentIdle(io) && BlackholeHostReadMapStable(io,input)
		&& PioneerOutputBufferMapStable(io,transfer,input) && PioneerWideOutputMapStable(io,wide,input,transfer);
}
template<class IO> bool PioneerQ38SpanCodeMatches(IO& io)
{
	if (!io.Q38PanelMappingStable() || io.RuntimeCodeRead(0)!=TT_Q38_SPAN_BOOT) return false;
	for (uint32_t a=0x70000;a<0x74000;a+=4)
		if (io.Q38PanelRead(a)!=(a<0x72000?0:PioneerQ38SpanCodeWord((a-0x72000)/4))) return false;
	return true;
}
template<class IO,bool Work> bool PioneerQ38SpanLiveCode(IO& io)
{
#ifdef PIONEER_Q38_SPAN_REUSE_CODE
	// Work ONLY: Prepare proves ALL immutable code; Close proves it again
	// BEFORE any restoration. The exclusive span lease blocks every other
	// writer/compute path. Its qualified RV32/NoC image can write ONLY its
	// disjoint stack/status/data/command extents, NEVER this code. Other
	// cores are still physically proved held on EVERY upload. Keep the
	// live boot/mapping check, not thousands of redundant PCIe code reads.
	// This intentionally detects an out-of-contract code mutation at END,
	// not on every Work job. Strict transport NEVER reuses this proof.
	if constexpr (Work) return io.Q38PanelMappingStable() && io.RuntimeCodeRead(0)==TT_Q38_SPAN_BOOT;
#endif
	return PioneerQ38SpanCodeMatches(io);
}
template<class IO> bool PioneerQ38SpanPrepare(IO& io,const tt_dram_samples_result& dram,const tt_compute_noc_result& noc,
	PioneerResidentBanks& banks,const tt_host_page_result& input,PioneerResidentLease& store,PioneerBulkUploadLease& bulk,
	PioneerOutputBufferLease& transfer,PioneerWideOutputLease& wide,PioneerQ38SpanLease& lease)
{
	if (lease.saved || lease.dirty || lease.ready || lease.running || lease.unsafe || lease.closed
		|| !io.SpanBeforeCohort() || !banks.Matches(dram) || wide.pending) return false;
	uint32_t* saved=io.Q38PanelSavedWords();
	if (!saved || !io.SpanViewCapacity(TT_Q38_SPAN_VIEW_WORDS+1)) return false;
	if (!io.SpanOtherCoresHeld() || !PioneerQ38SpanSources(io,dram,noc,banks,input,store,bulk,transfer,wide,lease,0x47800,0x402))
		return PioneerQ38SpanFail(io,banks,store,bulk,transfer,wide,lease);
	saved[0]=io.RuntimeCodeRead(0);
	for (unsigned w=0;w<TT_Q38_SPAN_VIEW_WORDS;w++) saved[w+1]=io.Q38PanelRead(TT_Q38_SPAN_VIEW_BASE+w*4);
	lease.saved=true;
	if (!PioneerQ38SpanSources(io,dram,noc,banks,input,store,bulk,transfer,wide,lease,0x47800,0x402))
		return PioneerQ38SpanFail(io,banks,store,bulk,transfer,wide,lease);
	lease.dirty=true;
	for (uint32_t a=0x70000;a<0x74000;a+=4) io.Q38PanelWrite(a,a<0x72000?0:PioneerQ38SpanCodeWord((a-0x72000)/4));
	io.RuntimeCodeWrite(0,TT_Q38_SPAN_BOOT);
	if (!PioneerQ38SpanCodeMatches(io) || !PioneerQ38SpanSources(io,dram,noc,banks,input,store,bulk,transfer,wide,lease,0x47800,0x402))
		return PioneerQ38SpanFail(io,banks,store,bulk,transfer,wide,lease);
	lease.ready=true; return true;
}
template<class IO,bool Work=false> bool PioneerQ38SpanUpload(IO& io,const tt_dram_samples_result& dram,const tt_compute_noc_result& noc,
	PioneerResidentBanks& banks,const tt_host_page_result& input,PioneerResidentLease& store,PioneerBulkUploadLease& bulk,
	PioneerOutputBufferLease& transfer,PioneerWideOutputLease& wide,PioneerQ38SpanLease& lease,
	const uint8_t* initialized,uint32_t initializedPages,unsigned bank,uint64_t offset,unsigned bytes,
	const uint32_t* data,unsigned dataWords,PioneerQ38SpanResult& r)
{
#ifndef PIONEER_Q38_SPAN_FAST
	static_assert(!Work,"Work transport requires the separate two-packet image");
#endif
	r={}; r.version=1; r.state=TT_RUNTIME_CONFIG_FAILED;
	PioneerQ38SpanExtent e;
	if (!data || dataWords!=bytes/4 || !lease.ready || lease.unsafe || lease.running || lease.closed
		|| lease.nonce>=0x7ffffffe || !io.SpanBeforeCohort()
		|| !PioneerQ38SpanAuthorize(banks,dram,initialized,initializedPages,bank,offset,bytes,e)) return false;
	auto fail=[&]() {
		r.state=TT_RUNTIME_COMPLETION_FAILED; r.recovery_required=1; r.pages=r.host_exact=0;
		return PioneerQ38SpanFail(io,banks,store,bulk,transfer,wide,lease);
	};
	if (!io.SpanOtherCoresHeld() || !PioneerQ38SpanSources(io,dram,noc,banks,input,store,bulk,transfer,wide,lease,0x47800,0x402)
		|| !PioneerQ38SpanLiveCode<IO,Work>(io)) return fail();
	io.WideHostFence();
	for (unsigned w=0;w<dataWords;w++) io.WideHostStore(w,data[w]);
	io.WideHostFence();
	if constexpr (!Work) {
		bool exact=true; for (unsigned w=0;w<dataWords;w++) exact &= io.WideHostLoad(w)==data[w];
		io.WideHostFence(); if (!exact) return fail();
	}
	uint32_t d[64]={}; d[0]=TT_Q38_SPAN_MARKER; d[1]=1; d[2]=Work?2:1; d[4]=bytes; d[5]=e.address;
	d[6]=lease.nonce+1; d[7]=uint32_t(wide.physical); d[8]=uint32_t(wide.physical>>32);
	d[9]=TT_WIDE_OUTPUT_BYTES; d[10]=bank; d[11]=e.route;
	for (unsigned w=0;w<16;w++) d[16+w]=w?noc.source[0][w]:store.config|0x10000;
	if (!PioneerQ38SpanShape(d)) return fail();
	bulk.dirty=true;
	for (unsigned w=0;w<64;w++) io.BulkL1Write(w,d[w]);
	for (unsigned w=0;w<64;w++) if (io.BulkL1Read(w)!=d[w]) return fail();
	store.l1_dirty=true; store.l1_restored=false; // global status owner retained until overall END
	for (unsigned w=0;w<3;w++) io.RuntimeStatusWrite(w,0);
	for (unsigned w=0;w<3;w++) if (io.RuntimeStatusRead(w)) return fail();
	if (!PioneerQ38SpanSources(io,dram,noc,banks,input,store,bulk,transfer,wide,lease,0x47800,0x402)) return fail();
	for (unsigned w=0;w<8;w++) r.before[w]=io.ResidentNocRead(PioneerQ38SpanCounter(w));
	// First uncertain BME/reset write retains ALL owners;there is NO rollback.
	lease.running=true; wide.pending=true; lease.nonce=d[6]; r.nonce=d[6]; r.bytes=bytes;
	io.HostPciWrite(1,4,2,0x406); if (io.HostPciRead(1,4,2)!=0x406) return fail();
	io.HostPciWrite(0,4,2,0x406);
	if (!PioneerQ38SpanSources(io,dram,noc,banks,input,store,bulk,transfer,wide,lease,0x47800,0x406)) return fail();
	io.RuntimeResetWrite(0x87ffff); io.Pause();
	if (!PioneerQ38SpanSources(io,dram,noc,banks,input,store,bulk,transfer,wide,lease,0x87ffff,0x406)) return fail();
	r.started=1; io.RuntimeResetWrite(0x46800);
	if (io.ResetRead()!=0x46800) return fail();
	const uint64_t deadline=io.Q38CohortNow()+UINT64_C(30000000);
	while (io.Q38CohortNow()<deadline) {
		// Marker LAST on device,FIRST in each host sample:stable terminal detail.
		r.marker=io.RuntimeStatusRead(2); r.phase=io.RuntimeStatusRead(0); r.count=io.RuntimeStatusRead(1);
		if (r.marker==TT_Q38_SPAN_FAULT) break;
		if (r.marker==TT_Q38_SPAN_MARKER && r.phase==TT_Q38_SPAN_DONE && r.count==e.pages) {
			io.Pause(); r.pc[0]=io.RuntimePcRead(); io.Pause(); r.pc[1]=io.RuntimePcRead();
			if ((r.pc[0]==TT_Q38_SPAN_PARK || r.pc[0]==TT_Q38_SPAN_PARK_FETCH)
				&& (r.pc[1]==TT_Q38_SPAN_PARK || r.pc[1]==TT_Q38_SPAN_PARK_FETCH)) { r.completed=1; break; }
		}
		io.Pause();
	}
	if (!r.completed || !PioneerQ38SpanSources(io,dram,noc,banks,input,store,bulk,transfer,wide,lease,0x46800,0x406)
		|| !PioneerQ38SpanLiveCode<IO,Work>(io)) return fail();
	for (unsigned w=0;w<8;w++) r.after[w]=io.ResidentNocRead(PioneerQ38SpanCounter(w));
	for (unsigned w=0;w<32;w++) r.summary[w]=io.BulkL1Read(3072+w);
	if (!PioneerQ38SpanSummary(r.summary,d,r.before,r.after)) return fail();
	for (unsigned w=0;w<64;w++) if (io.BulkL1Read(w)!=d[w]) return fail();
	unsigned last=(bytes-1)&~16383U;
	const uint32_t command[]={Work?0x144000:d[7]+last,Work?0U:d[8],Work?129U:1555U,
		Work?d[5]+last:0x144000,0,Work?d[11]:129U,0,Work?0x212U:0U,bytes-last,0};
	for (unsigned w=0;w<10;w++) if (io.ResidentNocRead(w*4)!=command[w]) return fail();
	if constexpr (!Work) {
		io.WideHostFence(); bool exact=true;
		for (unsigned w=0;w<dataWords;w++) { bool same=io.WideHostLoad(w)==data[w]; exact &= same; r.matched_words+=same; }
		io.WideHostFence(); if (!exact) return fail();
	}
	PioneerQ38SpanExtent confirmed;
	if (!PioneerQ38SpanAuthorize(banks,dram,initialized,initializedPages,bank,offset,bytes,confirmed)
		|| confirmed.address!=e.address || confirmed.route!=e.route || confirmed.index!=e.index
		|| !PioneerQ38SpanSources(io,dram,noc,banks,input,store,bulk,transfer,wide,lease,0x46800,0x406)) return fail();
	io.Pause(); r.pc[0]=io.RuntimePcRead(); io.Pause(); r.pc[1]=io.RuntimePcRead();
	if ((r.pc[0]!=TT_Q38_SPAN_PARK && r.pc[0]!=TT_Q38_SPAN_PARK_FETCH)
		|| (r.pc[1]!=TT_Q38_SPAN_PARK && r.pc[1]!=TT_Q38_SPAN_PARK_FETCH)) return fail();
	// ALL code/source/command/counter/CPU-byte/terminal proofs precede ANY
	// disable/hold. Each following write/readback is single-attempt.
	io.HostPciWrite(0,4,2,0x402); if (io.HostPciRead(0,4,2)!=0x402) return fail();
	io.HostPciWrite(1,4,2,0x402); if (io.HostPciRead(1,4,2)!=0x402) return fail();
	if (!PioneerQ38SpanSources(io,dram,noc,banks,input,store,bulk,transfer,wide,lease,0x46800,0x402)) return fail();
	io.RuntimeResetWrite(0x87ffff); io.Pause();
	if (!PioneerQ38SpanSources(io,dram,noc,banks,input,store,bulk,transfer,wide,lease,0x87ffff,0x402)) return fail();
	io.RuntimeResetWrite(0x47800); io.Pause();
	if (!PioneerQ38SpanSources(io,dram,noc,banks,input,store,bulk,transfer,wide,lease,0x47800,0x402)) return fail();
	lease.running=false; wide.pending=false; r.held=1; r.host_exact=Work?0:1; r.pages=e.pages; r.state=TT_RUNTIME_PASSED;
	return true; // ONLY future kernel handler may now commit ALL initialized pages
}
template<class IO> bool PioneerQ38SpanClose(IO& io,const tt_dram_samples_result& dram,const tt_compute_noc_result& noc,
	PioneerResidentBanks& banks,const tt_host_page_result& input,PioneerResidentLease& store,PioneerBulkUploadLease& bulk,
	PioneerOutputBufferLease& transfer,PioneerWideOutputLease& wide,PioneerQ38SpanLease& lease,bool knownClean)
{
	if (lease.unsafe) return false; // especially NO select/reset/restore/free
	if (lease.closed) return true;
	auto fail=[&]() { return PioneerQ38SpanFail(io,banks,store,bulk,transfer,wide,lease); };
	if (!knownClean || lease.running) return fail();
	if (!lease.saved) {
		if (lease.dirty || lease.ready) return fail();
		lease.closed=true; return true;
	}
	// ALL physical/core/code/source/map proofs precede the FIRST restore.
	// Do NOT invoke legacy helpers with retry/rollback-on-UNKNOWN behavior.
	if (!lease.ready || !io.SpanOtherCoresHeld()
		|| !PioneerQ38SpanSources(io,dram,noc,banks,input,store,bulk,transfer,wide,lease,0x47800,0x402)
		|| !PioneerQ38SpanCodeMatches(io)) return fail();
	const uint32_t* saved=io.Q38PanelSavedWords(); const uint32_t* bulkSaved=io.BulkSavedWords();
	if (!saved || !bulkSaved) return fail();
	if (lease.dirty) {
		for (unsigned w=0;w<TT_Q38_SPAN_VIEW_WORDS;w++) io.Q38PanelWrite(TT_Q38_SPAN_VIEW_BASE+w*4,saved[w+1]);
		for (unsigned w=0;w<TT_Q38_SPAN_VIEW_WORDS;w++)
			if (io.Q38PanelRead(TT_Q38_SPAN_VIEW_BASE+w*4)!=saved[w+1]) return fail();
	}
	if (bulk.dirty) {
		for (unsigned w=0;w<TT_BULK_UPLOAD_SAVED_WORDS;w++) io.BulkL1Write(w,bulkSaved[w]);
		for (unsigned w=0;w<TT_BULK_UPLOAD_SAVED_WORDS;w++) if (io.BulkL1Read(w)!=bulkSaved[w]) return fail();
	}
	io.RuntimeCodeWrite(0,saved[0]); if (io.RuntimeCodeRead(0)!=saved[0]) return fail();
	if (!PioneerQ38SpanSources(io,dram,noc,banks,input,store,bulk,transfer,wide,lease,0x47800,0x402)) return fail();
	lease.dirty=false; lease.ready=false; lease.closed=true; bulk.dirty=false;
	// Global17e000 status/NIU/bank/host leases stay owned until overall END.
	// Only this code/panel/bulk scratch owner is now restored/closed.
	return true;
}
#endif
