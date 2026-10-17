/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_BULK_UPLOAD_H
#define PIONEER_BULK_UPLOAD_H
#include "PioneerResidentStream.h"
#include "PioneerOutputBuffer.h"

// Data-only, maximum one NoC burst (256 x 64B). These two fixed 16KiB
// regions are disjoint from all matrix/attention code, buffers and tails.
enum { TT_BULK_UPLOAD_MAX_PAGES=4, TT_BULK_UPLOAD_WORDS=4096,
	TT_BULK_UPLOAD_L1=0x140000, TT_BULK_RETURN_L1=0x144000,
	TT_BULK_UPLOAD_SAVED_WORDS=8192 };
struct PioneerBulkUploadLease {
	bool saved=false,dirty=false,ready=false,unsafe=false;
};
struct tt_bulk_upload_result {
	tt_resident_stream_result transfer;
	uint32_t pages,host_words,host_ready,l1_prepared;
};
static inline bool PioneerBulkUploadRange(uint64_t capacity,uint64_t offset,unsigned pages)
{
	return pages>=1 && pages<=4 && PioneerResidentOffset(capacity,offset)
		&& uint64_t(pages)*4096<=capacity-offset;
}

// Only ordinary private noncacheable HOST RAM uses these block operations.
// MMIO/control accesses retain their existing per-access IO fences. Full
// boundary fences bracket each complete fill/scan, never weaken acceptance.
template<class IO>
bool PioneerBulkHostFill(IO& io,const uint32_t* data,unsigned words,bool complement)
{
	if (!data || !words || words>TT_BULK_UPLOAD_WORDS) return false;
	io.BulkHostFence();
	for (unsigned w=0;w<words;w++) io.BulkHostStore(w,complement?~data[w]:data[w]);
	io.BulkHostFence();
	return true;
}
template<class IO>
unsigned PioneerBulkHostMismatches(IO& io,const uint32_t* data,unsigned words,bool complement=false)
{
	if (!data || !words || words>TT_BULK_UPLOAD_WORDS) return UINT32_MAX;
	unsigned mismatches=0; io.BulkHostFence();
	for (unsigned w=0;w<words;w++) mismatches+=io.BulkHostLoad(w)!=(complement?~data[w]:data[w]);
	io.BulkHostFence(); return mismatches;
}
template<class IO>
bool PioneerBulkUploadGuard(IO& io,const tt_dram_samples_result& baseline,const tt_compute_noc_result& noc,
	const tt_host_page_result& input,const PioneerResidentLease& store,const PioneerOutputBufferLease& host,uint32_t pci)
{
	bool okay=io.ResetRead()==0x47800 && store.ready && !store.pending && !store.recovery_required
		&& BlackholeResidentSourceStable(io,noc,store.config|0x10000)
		&& BlackholeDramSourceStable(io,baseline) && BlackholeResidentIdle(io)
		&& BlackholeHostReadPciQuiet(io,pci) && BlackholeHostReadMapStable(io,input)
		&& host.ready && host.memory_type_set && !host.pending
		&& PioneerOutputBufferMapStable(io,host,input)
		&& io.BulkMappingStable();
	for (unsigned s=0;s<2;s++) for (unsigned w=0;w<3;w++) okay &= io.DramTlbRead(s,w)
		==BlackholeDramFixedTlbWord(s?0xffa00000:0,1,2,false,w);
	return okay;
}
template<class IO>
bool PioneerBulkUploadPrepare(IO& io,const tt_dram_samples_result& baseline,const tt_compute_noc_result& noc,
	const tt_host_page_result& input,const PioneerResidentLease& store,const PioneerOutputBufferLease& host,PioneerBulkUploadLease& bulk)
{
	if (bulk.ready) return !bulk.unsafe;
	uint32_t* saved=io.BulkSavedWords();
	if (bulk.saved || bulk.unsafe || !saved || !PioneerBulkUploadGuard(io,baseline,noc,input,store,host,0x402)) return false;
	for (unsigned w=0;w<TT_BULK_UPLOAD_SAVED_WORDS;w++) saved[w]=io.BulkL1Read(w);
	bulk.saved=true;
	if (!PioneerBulkUploadGuard(io,baseline,noc,input,store,host,0x402)) { bulk.unsafe=true; return false; }
	bulk.ready=true; return true;
}
template<class IO>
bool PioneerBulkUploadCommand(IO& io,PioneerResidentLease& store,PioneerOutputBufferLease& host,
	const uint32_t* command,bool write,unsigned step,unsigned pages,tt_resident_stream_result& r,unsigned exactBytes=0)
{
	if (step>=6 || !pages || pages>4 || !BlackholeResidentIdle(io)) return false;
	if (exactBytes && (exactBytes>16384 || (exactBytes&63) || command[8]!=exactBytes)) return false;
	unsigned flits=exactBytes?exactBytes/64:64*pages;
	for (unsigned w=0;w<10;w++) io.ResidentNocWrite(w*4,command[w]);
	for (unsigned w=0;w<10;w++) if (io.ResidentNocRead(w*4)!=command[w]) return false;
	for (unsigned w=0;w<4;w++) r.before[step][w]=io.ResidentNocRead(write?BlackholeHostWriteCounter(w):0x208+w*4);
	store.pending=true; host.pending=true; r.requests++; r.phase=step+1;
	if (step==4) r.host_write_issued=1;
	io.ResidentNocWrite(0x40,1);
	for (unsigned attempt=0;attempt<256;attempt++) {
		r.polls[step]++; uint32_t pending=io.ResidentNocRead(0x40);
		for (unsigned w=0;w<4;w++) r.after[step][w]=io.ResidentNocRead(write?BlackholeHostWriteCounter(w):0x208+w*4);
		if (!pending && uint32_t(r.after[step][0]-r.before[step][0])==1
			&& uint32_t(r.after[step][1]-r.before[step][1])==flits
			&& uint32_t(r.after[step][2]-r.before[step][2])==1
			&& uint32_t(r.after[step][3]-r.before[step][3])==1 && BlackholeResidentIdle(io)) {
			store.pending=false; host.pending=false; r.completed++; return true;
		}
		io.Pause();
	}
	store.recovery_required=true; r.state=TT_STORE_TIMEOUT; return false;
}
template<class IO>
bool PioneerBulkUpload(IO& io,const tt_dram_samples_result& baseline,const tt_compute_noc_result& noc,
	const tt_host_page_result& input,PioneerResidentLease& store,PioneerOutputBufferLease& host,
	PioneerBulkUploadLease& bulk,uint64_t capacity,uint64_t offset,unsigned pages,const uint32_t* data,tt_bulk_upload_result& result)
{
	result={}; auto& r=result.transfer; r.version=1;
	if (!data || !PioneerBulkUploadRange(capacity,offset,pages) || !bulk.ready || bulk.unsafe
		|| !host.ready || host.pending || host.quarantined || store.pending || store.recovery_required) return false;
	result.pages=pages; result.host_words=pages*1024; result.host_ready=1; result.l1_prepared=1;
	r.state=TT_STORE_CONFIG_FAILED;
	unsigned words=pages*1024,bytes=pages*4096; uint32_t address=PioneerResidentAddress(capacity,offset);
	const uint32_t hostRead[]={uint32_t(host.physical),uint32_t(host.physical>>32),19|(24<<6),TT_BULK_UPLOAD_L1,0,1|(2<<6),0,0,bytes,0};
	const uint32_t toGddr[]={TT_BULK_UPLOAD_L1,0,1|(2<<6),address,0,baseline.translated[0][0]|(baseline.translated[0][1]<<6),0,0x212,bytes,0};
	const uint32_t fromGddr[]={address,0,baseline.translated[0][0]|(baseline.translated[0][1]<<6),TT_BULK_RETURN_L1,0,1|(2<<6),0,0,bytes,0};
	const uint32_t toHost[]={TT_BULK_RETURN_L1,0,1|(2<<6),uint32_t(host.physical),uint32_t(host.physical>>32),19|(24<<6),0,0x212,bytes,0};
	bool touched=false;
	do {
		if (!PioneerBulkUploadGuard(io,baseline,noc,input,store,host,0x402)) { bulk.unsafe=true; break; }
		if (!PioneerBulkHostFill(io,data,words,false)) break;
		r.mismatches=PioneerBulkHostMismatches(io,data,words); if (r.mismatches) { r.state=TT_STORE_MISMATCH; break; }
		touched=true; io.HostPciWrite(1,4,2,0x406); if (io.HostPciRead(1,4,2)!=0x406) break;
		io.HostPciWrite(0,4,2,0x406);
		if (!PioneerBulkUploadGuard(io,baseline,noc,input,store,host,0x406)) { bulk.unsafe=true; break; }
		bulk.dirty=true;
		if (!PioneerBulkUploadCommand(io,store,host,hostRead,false,0,pages,r)) break;
		io.HostPciWrite(0,4,2,0x402); io.HostPciWrite(1,4,2,0x402);
		if (!PioneerBulkUploadGuard(io,baseline,noc,input,store,host,0x402)) { bulk.unsafe=true; break; }
		r.wrote_device=1; if (!PioneerBulkUploadCommand(io,store,host,toGddr,true,1,pages,r)) break;
		if (!PioneerBulkUploadGuard(io,baseline,noc,input,store,host,0x402)) { bulk.unsafe=true; break; }
		if (!PioneerBulkUploadCommand(io,store,host,fromGddr,false,2,pages,r)) break;
		if (!PioneerBulkUploadGuard(io,baseline,noc,input,store,host,0x402)) { bulk.unsafe=true; break; }
		if (!PioneerBulkHostFill(io,data,words,true)) break;
		r.mismatches=PioneerBulkHostMismatches(io,data,words,true); if (r.mismatches) { r.state=TT_STORE_MISMATCH; break; }
		io.HostPciWrite(1,4,2,0x406); if (io.HostPciRead(1,4,2)!=0x406) break;
		io.HostPciWrite(0,4,2,0x406);
		if (!PioneerBulkUploadGuard(io,baseline,noc,input,store,host,0x406)) { bulk.unsafe=true; break; }
		if (!PioneerBulkUploadCommand(io,store,host,hostRead,false,3,pages,r)) break;
		if (!PioneerBulkUploadGuard(io,baseline,noc,input,store,host,0x406)) { bulk.unsafe=true; break; }
		if (!PioneerBulkUploadCommand(io,store,host,toHost,true,4,pages,r)) break;
		for (unsigned attempt=0;attempt<256;attempt++) {
			r.mismatches=PioneerBulkHostMismatches(io,data,words);
			if (!r.mismatches) { r.host_visible=1; break; }
			io.Pause();
		}
		if (!r.host_visible) { r.state=TT_STORE_MISMATCH; bulk.unsafe=true; break; }
		if (!PioneerBulkUploadGuard(io,baseline,noc,input,store,host,0x406)) { bulk.unsafe=true; break; }
		if (!PioneerBulkUploadCommand(io,store,host,hostRead,false,5,pages,r)) break;
		r.readback_complete=1;
		if (!PioneerBulkUploadGuard(io,baseline,noc,input,store,host,0x406)) { bulk.unsafe=true; break; }
		r.mismatches=PioneerBulkHostMismatches(io,data,words);
		if (r.mismatches) { r.state=TT_STORE_CHANGED; bulk.unsafe=true; break; }
		r.state=TT_STORE_PASSED;
	} while (false);
	if (r.host_write_issued && (!r.host_visible || !r.readback_complete)) bulk.unsafe=true;
	if (touched) {
		bool off=false;
		for (unsigned attempt=0;attempt<2 && !off;attempt++) {
			io.HostPciWrite(0,4,2,0x402); io.HostPciWrite(1,4,2,0x402); off=BlackholeHostReadPciQuiet(io,0x402);
		}
		if (!off) bulk.unsafe=true;
	}
	r.recovery_required=bulk.unsafe || store.recovery_required || store.pending || host.pending;
	if (r.recovery_required) { bulk.unsafe=true; store.recovery_required=true; io.QuarantineHostPage(); PioneerQuarantineOutputBuffer(io,host); }
	return r.state==TT_STORE_PASSED && !r.recovery_required;
}
template<class IO>
bool PioneerBulkUploadClose(IO& io,const tt_dram_samples_result& baseline,const tt_compute_noc_result& noc,
	const tt_host_page_result& input,PioneerResidentLease& store,PioneerOutputBufferLease& host,PioneerBulkUploadLease& bulk)
{
	if (!bulk.saved) return !bulk.unsafe;
	bool safe=!bulk.unsafe && PioneerBulkUploadGuard(io,baseline,noc,input,store,host,0x402);
	if (safe && bulk.dirty) {
		const uint32_t* saved=io.BulkSavedWords(); if (!saved) safe=false;
		if (safe) {
			for (unsigned w=0;w<TT_BULK_UPLOAD_SAVED_WORDS;w++) io.BulkL1Write(w,saved[w]);
			for (unsigned w=0;w<TT_BULK_UPLOAD_SAVED_WORDS;w++) safe &= io.BulkL1Read(w)==saved[w];
			if (safe) bulk.dirty=false;
		}
	}
	bulk.ready=false; bulk.unsafe |= !safe || bulk.dirty;
	if (bulk.unsafe) { store.recovery_required=true; io.QuarantineHostPage(); PioneerQuarantineOutputBuffer(io,host); }
	return !bulk.unsafe;
}
#endif
