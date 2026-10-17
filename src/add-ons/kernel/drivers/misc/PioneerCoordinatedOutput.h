/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_COORDINATED_OUTPUT_H
#define PIONEER_COORDINATED_OUTPUT_H
#include "PioneerCoordinatorProbe.h"
#include "PioneerBulkUpload.h"
// Separate OFFLINE output transport candidate. A proved-complete matrix
// supplies fixed84000..8c000 data. Kernel-controlled write/ordered-read DMA
// uses ONLY the existing distinct pinned noncacheable16KiB host owner.
// BME is OFF before the immutable on-card ALL-WORD comparison runs.
// No learned arithmetic, hash, arbitrary DMA address or new allocation.
struct PioneerCoordinatedOutputResult {
	tt_resident_stream_result transfer={};
	uint32_t words=0,offset=0,nonce=0,started=0,completed=0,stopped=0,compared=0;
	uint32_t pc[2]={},summary[24]={},before[8]={},after[8]={};
};
static inline bool PioneerCoordinatedOutputSummary(const uint32_t* summary,unsigned offset,unsigned words,unsigned nonce,
	const uint32_t* before,const uint32_t* after)
{
	if (!summary || !before || !after || (offset!=0 && offset!=4096) || !words || words>4096 || (words&31)
		|| !nonce || nonce>=0x7fffffff || summary[1]!=offset || summary[2] || summary[3] || summary[4]!=words
		|| summary[5]!=words || summary[6]!=nonce || summary[7]!=2 || before[2]!=before[6] || after[2]!=after[6]) return false;
	for (unsigned i=0;i<8;i++) if (before[i]!=after[i] || summary[8+i]!=before[i] || summary[16+i]!=after[i]) return false;
	// A nonzero summary[0] is a KNOWN completed mismatch, never success.
	// Completion/stop authority and output equality are separate gates.
	return true;
}
template<class IO>
bool PioneerCoordinatedOutputTransfer(IO& io,PioneerResidentLease& store,PioneerOutputBufferLease& host,
	unsigned offset,unsigned words,bool write,tt_resident_stream_result& r)
{
	if ((offset!=0 && offset!=4096) || !words || words>4096 || (words&31) || !BlackholeResidentIdle(io)) return false;
	const uint32_t command[]={write?0x84000+offset*4:uint32_t(host.physical),write?0:uint32_t(host.physical>>32),
		write?129U:19U|(24U<<6),write?uint32_t(host.physical):0x144000,write?uint32_t(host.physical>>32):0,
		write?19U|(24U<<6):129U,0,write?0x212U:0,words*4,0};
	for (unsigned w=0;w<10;w++) io.ResidentNocWrite(w*4,command[w]);
	for (unsigned w=0;w<10;w++) if (io.ResidentNocRead(w*4)!=command[w]) return false;
	unsigned step=write?0:1;
	for (unsigned w=0;w<4;w++) r.before[step][w]=io.ResidentNocRead(write?BlackholeHostWriteCounter(w):0x208+w*4);
	store.pending=true; host.pending=true; r.requests++; r.phase=step+1;
	if (write) r.host_write_issued=1;
	io.ResidentNocWrite(0x40,1);
	for (unsigned attempt=0;attempt<256;attempt++) {
		r.polls[step]++; uint32_t pending=io.ResidentNocRead(0x40);
		for (unsigned w=0;w<4;w++) r.after[step][w]=io.ResidentNocRead(write?BlackholeHostWriteCounter(w):0x208+w*4);
		if (!pending && uint32_t(r.after[step][0]-r.before[step][0])==1
			&& uint32_t(r.after[step][1]-r.before[step][1])==words/16
			&& uint32_t(r.after[step][2]-r.before[step][2])==1
			&& uint32_t(r.after[step][3]-r.before[step][3])==1 && BlackholeResidentIdle(io)) {
			store.pending=false; host.pending=false; r.completed++; return true;
		}
		io.Pause();
	}
	store.recovery_required=true; r.state=TT_STORE_TIMEOUT; return false;
}
template<class IO>
bool PioneerCoordinatedOutputCompare(IO& io,const tt_dram_samples_result& baseline,const tt_compute_noc_result& noc,
	PioneerResidentLease& store,PioneerBulkUploadLease& bulk,PioneerCoordinatorLease& lease,
	unsigned offset,unsigned words,PioneerCoordinatedOutputResult& r)
{
	if (!lease.ready || !lease.fast || !lease.readback || lease.running || lease.unsafe || lease.readbackNonce>=0x7ffffffe
		|| !bulk.ready || bulk.unsafe || (offset!=0 && offset!=4096) || !words || words>4096 || (words&31)
		|| !PioneerCoordinatorLocalHeld(io,baseline,noc,store)) return false;
	uint32_t* stage=io.RuntimeStageWords(); if (!stage) return false;
	for (unsigned i=0;i<7;i++) stage[i]=0;
	stage[0]=0x31445243; stage[1]=2; stage[2]=1; stage[3]=offset; stage[4]=words; stage[6]=++lease.readbackNonce;
	r.words=words; r.offset=offset; r.nonce=lease.readbackNonce; bulk.dirty=true;
	if (!PioneerBulkPayloadStage(io,stage,7,0)) return false;
	store.l1_dirty=true; store.l1_restored=false;
	for (unsigned i=0;i<3;i++) io.RuntimeStatusWrite(i,0);
	for (unsigned i=0;i<3;i++) if (io.RuntimeStatusRead(i)) return false;
	io.RuntimeCodeWrite(0,TT_COORDINATOR_BOOT);
	if (io.RuntimeCodeRead(0)!=TT_COORDINATOR_BOOT || !PioneerCoordinatorLocalHeld(io,baseline,noc,store)) return false;
	bool known=false;
	do {
		io.RuntimeResetWrite(0x87ffff); io.Pause();
		if (io.ResetRead()!=0x87ffff || io.ResetRead()!=0x87ffff || !PioneerCoordinatorSources(io,baseline,noc,store)
			|| !BlackholeResidentIdle(io)) break;
		PioneerResidentAttentionCounters(io,r.before);
		lease.running=true; r.started=1; io.RuntimeResetWrite(0x46800);
		for (unsigned attempt=0;attempt<256;attempt++) {
			if (io.ResetRead()!=0x46800 || io.RuntimeStatusRead(2)==TT_COORDINATOR_FAULT) break;
			if (io.RuntimeStatusRead(0)==TT_COORDINATOR_DONE && io.RuntimeStatusRead(1)==words
				&& io.RuntimeStatusRead(2)==TT_COORDINATOR_MARKER) {
				io.Pause(); r.pc[0]=io.RuntimePcRead(); io.Pause(); r.pc[1]=io.RuntimePcRead();
				if (r.pc[0]==TT_COORDINATOR_PARK && r.pc[1]==r.pc[0] && BlackholeResidentIdle(io)) { r.completed=1; break; }
			}
			io.Pause();
		}
		if (!r.completed || !PioneerCoordinatorSources(io,baseline,noc,store) || !BlackholeResidentIdle(io)) break;
		PioneerResidentAttentionCounters(io,r.after);
		for (unsigned i=0;i<24;i++) r.summary[i]=io.BulkL1Read(3072+i);
		bool exact=PioneerCoordinatedOutputSummary(r.summary,offset,words,r.nonce,r.before,r.after);
		if (!exact || io.ResetRead()!=0x46800) break; // no reset before complete physical/nonce proof
		io.RuntimeResetWrite(0x87ffff); io.Pause();
		if (io.ResetRead()!=0x87ffff || io.ResetRead()!=0x87ffff) break;
		io.RuntimeResetWrite(0x47800); io.Pause();
		if (!PioneerCoordinatorLocalHeld(io,baseline,noc,store)) break;
		lease.running=false; r.stopped=1; known=true;
		r.compared=!r.summary[0];
	} while (false);
	if (!known) { lease.unsafe=true; bulk.unsafe=true; store.recovery_required=true; io.QuarantineHostPage(); }
	return known && r.compared;
}
template<class IO>
bool PioneerCoordinatedOutput(IO& io,const tt_dram_samples_result& baseline,const tt_compute_noc_result& noc,
	const tt_host_page_result& input,PioneerResidentLease& store,PioneerOutputBufferLease& host,
	PioneerBulkUploadLease& bulk,PioneerRuntimeMatvecLease& compute,PioneerCoordinatorLease& lease,
	unsigned offset,unsigned words,uint32_t* output,PioneerCoordinatedOutputResult& result)
{
	result={}; auto& r=result.transfer; r.version=1; r.state=TT_STORE_CONFIG_FAILED;
	if (!output || (offset!=0 && offset!=4096) || !words || words>4096 || (words&31)
		|| !lease.ready || !lease.fast || !lease.readback || lease.running || lease.unsafe || lease.readbackNonce>=0x7ffffffe
		|| !compute.ready || !compute.saved || !compute.bulk || compute.running || compute.unsafe
		|| compute.buffer_words!=TT_Q8_BULK_SAVED_BUFFER_WORDS || !compute.buffers_dirty
		|| !bulk.ready || bulk.unsafe || !host.ready || host.pending || host.quarantined || store.pending || store.recovery_required) return false;
	bool touched=false,passed=false;
	do {
		if (!PioneerBulkUploadGuard(io,baseline,noc,input,store,host,0x402)
			|| !PioneerCoordinatorLocalHeld(io,baseline,noc,store)) { bulk.unsafe=true; break; }
		io.BulkHostFence(); for (unsigned w=0;w<words;w++) io.BulkHostStore(w,0xffffffff); io.BulkHostFence();
		for (unsigned w=0;w<words;w++) r.mismatches+=io.BulkHostLoad(w)!=0xffffffff;
		io.BulkHostFence(); if (r.mismatches) { r.state=TT_STORE_MISMATCH; break; }
		touched=true; io.HostPciWrite(1,4,2,0x406); if (io.HostPciRead(1,4,2)!=0x406) break;
		io.HostPciWrite(0,4,2,0x406);
		if (!PioneerBulkUploadGuard(io,baseline,noc,input,store,host,0x406)) { bulk.unsafe=true; break; }
		if (!PioneerCoordinatedOutputTransfer(io,store,host,offset,words,true,r)) break;
		for (unsigned attempt=0;attempt<256;attempt++) {
			r.mismatches=0; io.BulkHostFence();
			for (unsigned w=0;w<words;w++) r.mismatches+=(io.BulkHostLoad(w)&0x7f800000)==0x7f800000;
			io.BulkHostFence();
			if (!r.mismatches) { r.host_visible=1; break; }
			io.Pause();
		}
		if (!r.host_visible) { r.state=TT_STORE_MISMATCH; bulk.unsafe=true; break; }
		io.BulkHostFence(); for (unsigned w=0;w<words;w++) output[w]=io.BulkHostLoad(w); io.BulkHostFence();
		if (!PioneerBulkUploadGuard(io,baseline,noc,input,store,host,0x406)) { bulk.unsafe=true; break; }
		bulk.dirty=true; // ordered read overwrites the backed-up144000..148000 scratch
		if (!PioneerCoordinatedOutputTransfer(io,store,host,offset,words,false,r)) break;
		r.readback_complete=1;
		if (!PioneerBulkUploadGuard(io,baseline,noc,input,store,host,0x406)) { bulk.unsafe=true; break; }
		io.BulkHostFence(); for (unsigned w=0;w<words;w++) r.mismatches+=io.BulkHostLoad(w)!=output[w]; io.BulkHostFence();
		if (r.mismatches) { r.state=TT_STORE_CHANGED; bulk.unsafe=true; break; }
		passed=true;
	} while (false);
	if (touched) {
		bool off=false;
		for (unsigned attempt=0;attempt<2 && !off;attempt++) {
			io.HostPciWrite(0,4,2,0x402); io.HostPciWrite(1,4,2,0x402); off=BlackholeHostReadPciQuiet(io,0x402);
		}
		if (!off) bulk.unsafe=true;
	}
	if (passed && !bulk.unsafe) passed=PioneerBulkUploadGuard(io,baseline,noc,input,store,host,0x402)
		&& PioneerCoordinatedOutputCompare(io,baseline,noc,store,bulk,lease,offset,words,result);
	if (passed) {
		// Frozen CPU snapshot must still equal actual noncacheable host RAM
		// AFTER the on-card original/readback equality and known stop.
		io.BulkHostFence(); for (unsigned w=0;w<words;w++) r.mismatches+=io.BulkHostLoad(w)!=output[w]; io.BulkHostFence();
		passed=!r.mismatches && PioneerBulkUploadGuard(io,baseline,noc,input,store,host,0x402);
	}
	if (r.host_write_issued && (!passed || !r.host_visible || !r.readback_complete || !result.compared)) bulk.unsafe=true;
	r.recovery_required=bulk.unsafe || store.recovery_required || store.pending || host.pending || lease.unsafe || lease.running;
	if (r.recovery_required) {
		bulk.unsafe=true; compute.unsafe=true; store.recovery_required=true; io.QuarantineHostPage(); PioneerQuarantineOutputBuffer(io,host);
	}
	if (passed && !r.recovery_required) r.state=TT_STORE_PASSED;
	return passed && r.state==TT_STORE_PASSED && r.requests==2 && r.completed==2 && !r.recovery_required;
}
#endif
