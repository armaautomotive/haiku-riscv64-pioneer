/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_SPAN_UPLOAD_H
#define PIONEER_SPAN_UPLOAD_H
#include "PioneerSpanProof.h"
// Contract11 private NEW-weight transport. Caller freezes bytes and commits
// initialized-page metadata only after the complete proof returned here.
template<class IO>
bool PioneerSpanUpload(IO& io,const tt_dram_samples_result& baseline,const tt_host_page_result& host,
	PioneerOutputBufferLease& transfer,PioneerWideOutputLease& wide,PioneerFleetWorker* fleet,PioneerBulkUploadLease& bulk,
	PioneerCoordinatorLease& coordinator,PioneerLayerLease& code,bool attentionReady,uint64_t capacity,uint64_t offset,
	unsigned bytes,const uint32_t* data,tt_layer_span_result& r)
{
	r={}; r.bytes=bytes; auto& m=r.master; m.version=1; m.workers=1;
	if (!fleet || !data || !bytes || bytes>TT_WIDE_OUTPUT_BYTES || (bytes&4095) || capacity!=UINT64_C(0x80000000)
		|| !PioneerResidentOffset(capacity,offset) || offset>=capacity-TT_RESIDENT_KV_BYTES || bytes>capacity-TT_RESIDENT_KV_BYTES-offset
		|| !code.ready || !code.saved || code.running || code.unsafe || !coordinator.ready || !coordinator.saved
		|| !coordinator.fast || !coordinator.fused || !coordinator.compact || coordinator.readback || coordinator.running || coordinator.unsafe
		|| coordinator.nonce>=0x7ffffffe || !bulk.ready || bulk.unsafe || !wide.ready || !wide.allocated || !wide.memory_type_set
		|| wide.pending || wide.released || wide.quarantined) return false;
	auto& master=fleet[0]; auto& store=master.Store(); auto& compute=master.Compute();
	if (!compute.ready || !compute.saved || !compute.bulk || compute.running || compute.unsafe
		|| compute.buffer_words!=TT_Q8_BULK_SAVED_BUFFER_WORDS || io.RuntimeBufferWords()!=TT_Q8_BULK_SAVED_BUFFER_WORDS) return false;
	bool uncertain=false,bme=false; uint32_t previous=0; unsigned words=bytes/4;
	coordinator.inputValid=false; coordinator.inputK=0; m.state=TT_RUNTIME_CONFIG_FAILED;
	do {
		if (!PioneerCoordinatedMasterBoundary(io,baseline,fleet,attentionReady) || !PioneerLayerMapping(io,0)
			|| !PioneerCoordinatorLocalHeld(io,baseline,master.noc,store) || !PioneerWideOutputMapStable(io,wide,host,transfer)
			|| !PioneerBulkUploadGuard(io,baseline,master.noc,host,store,transfer,0x402)) { uncertain=true; break; }
		previous=io.RuntimeCodeRead(0); uint32_t* stage=io.RuntimeStageWords(); if (!stage) break;
		io.WideHostFence(); for (unsigned i=0;i<words;i++) io.WideHostStore(i,data[i]); io.WideHostFence();
		bool exact=true; for (unsigned i=0;i<words;i++) exact &= io.WideHostLoad(i)==data[i]; io.WideHostFence();
		if (!exact) break;
		for (unsigned i=0;i<64;i++) stage[i]=0;
		stage[0]=0x31445243; stage[1]=7; stage[2]=1; stage[4]=bytes; stage[5]=PioneerResidentAddress(capacity,offset);
		stage[6]=++coordinator.nonce; stage[7]=uint32_t(wide.physical); stage[8]=uint32_t(wide.physical>>32); stage[9]=TT_WIDE_OUTPUT_BYTES;
		m.nonce=coordinator.nonce; bulk.dirty=true;
		if (!PioneerBulkPayloadStage(io,stage,64,0) || !PioneerCoordinatedMasterBoundary(io,baseline,fleet,attentionReady) || !PioneerLayerMapping(io,0)) break;
		io.RuntimeProfileMark(TT_PROFILE_STAGED);
		store.l1_dirty=true; store.l1_restored=false; compute.buffers_dirty=true; // private original mirror overwrites packed input
		for (unsigned i=0;i<3;i++) io.RuntimeStatusWrite(i,0);
		for (unsigned i=0;i<3;i++) exact &= io.RuntimeStatusRead(i)==0;
		if (!exact) break;
		io.RuntimeCodeWrite(0,TT_LAYER_BOOT);
		if (io.RuntimeCodeRead(0)!=TT_LAYER_BOOT || !PioneerCoordinatorLocalHeld(io,baseline,master.noc,store) || !PioneerLayerMapping(io,0)) break;
		m.prepared=1;
		if (!PioneerBulkUploadGuard(io,baseline,master.noc,host,store,transfer,0x402) || !PioneerWideOutputMapStable(io,wide,host,transfer)) { uncertain=true; break; }
		bme=true; io.HostPciWrite(1,4,2,0x406); if (io.HostPciRead(1,4,2)!=0x406) { uncertain=true; break; }
		io.HostPciWrite(0,4,2,0x406);
		if (!PioneerBulkUploadGuard(io,baseline,master.noc,host,store,transfer,0x406) || !PioneerWideOutputMapStable(io,wide,host,transfer)) { uncertain=true; break; }
		io.RuntimeResetWrite(0x87ffff); io.Pause();
		if (io.ResetRead()!=0x87ffff || io.ResetRead()!=0x87ffff || !PioneerCoordinatorSources(io,baseline,master.noc,store,0x406)
			|| !PioneerLayerMapping(io,0) || !BlackholeResidentIdle(io)) { uncertain=true; break; }
		PioneerResidentAttentionCounters(io,m.before);
		code.running=coordinator.running=true; wide.pending=true; m.started=1; m.state=TT_RUNTIME_COMPLETION_FAILED;
		io.RuntimeProfileMark(TT_PROFILE_LAUNCHED); io.RuntimeResetWrite(0x46800);
		for (unsigned attempt=0;attempt<250000;attempt++) {
			m.polls++; if (io.ResetRead()!=0x46800) break;
			m.phase=io.RuntimeStatusRead(0); m.count=io.RuntimeStatusRead(1); m.marker=io.RuntimeStatusRead(2);
			if (m.marker==TT_COORDINATOR_FAULT) break;
			if (m.phase==TT_LAYER_DONE && m.count==bytes/4096 && m.marker==TT_COORDINATOR_MARKER) {
				io.Pause(); m.pc[0]=io.RuntimePcRead(); io.Pause(); m.pc[1]=io.RuntimePcRead();
				if (m.pc[0]==TT_LAYER_PARK && m.pc[1]==m.pc[0] && BlackholeResidentIdle(io)) { m.completed=1; break; }
			}
			io.Pause();
		}
		if (!m.completed || io.ResetRead()!=0x46800 || !PioneerCoordinatorSources(io,baseline,master.noc,store,0x406)
			|| !PioneerLayerMapping(io,0) || !BlackholeResidentIdle(io)) { uncertain=true; break; }
		PioneerResidentAttentionCounters(io,m.after);
		for (unsigned i=0;i<24;i++) r.summary[i]=io.BulkL1Read(3072+i);
		if (!PioneerSpanSummary(r.summary,bytes,m.nonce,m.before,m.after)) { uncertain=true; break; }
		m.summary_verified=1;
		io.HostPciWrite(0,4,2,0x402); io.HostPciWrite(1,4,2,0x402);
		if (!BlackholeHostReadPciQuiet(io,0x402) || !PioneerOutputBufferMapStable(io,transfer,host)
			|| !BlackholeHostReadMapStable(io,host) || !PioneerWideOutputMapStable(io,wide,host,transfer)
			|| !PioneerCoordinatorSources(io,baseline,master.noc,store) || !PioneerLayerMapping(io,0) || !BlackholeResidentIdle(io)) { uncertain=true; break; }
		bme=false;
		io.RuntimeResetWrite(0x87ffff); io.Pause();
		if (io.ResetRead()!=0x87ffff || io.ResetRead()!=0x87ffff) { uncertain=true; break; }
		io.RuntimeResetWrite(0x47800); io.Pause();
		if (!PioneerCoordinatorLocalHeld(io,baseline,master.noc,store) || !PioneerLayerMapping(io,0)) { uncertain=true; break; }
		code.running=coordinator.running=false; wide.pending=false; m.stopped=1;
		io.RuntimeProfileMark(TT_PROFILE_COMPLETED);
		// Mandatory independent CPU transport comparison. This is frozen
		// supplied DATA,not CPU model arithmetic or a learned output check.
		io.WideHostFence(); exact=true;
		for (unsigned i=0;i<words;i++) { bool match=io.WideHostLoad(i)==data[i]; exact &= match; r.matched_words+=match; }
		io.WideHostFence();
		if (!exact || !PioneerWideOutputMapStable(io,wide,host,transfer) || !PioneerBulkUploadGuard(io,baseline,master.noc,host,store,transfer,0x402)) { uncertain=true; break; }
		r.host_exact=1; r.pages=bytes/4096; m.matched=1; m.state=TT_RUNTIME_PASSED; io.RuntimeProfileMark(TT_PROFILE_PIO);
	} while (false);
	if (bme) {
		bool off=false; for (unsigned i=0;i<2 && !off;i++) { io.HostPciWrite(0,4,2,0x402); io.HostPciWrite(1,4,2,0x402); off=BlackholeHostReadPciQuiet(io,0x402); }
		if (!off) uncertain=true;
	}
	if (!uncertain && !code.running && !coordinator.running && PioneerLayerMapping(io,0) && PioneerCoordinatorLocalHeld(io,baseline,master.noc,store)) {
		if (previous) { io.RuntimeCodeWrite(0,previous); if (io.RuntimeCodeRead(0)!=previous || !PioneerCoordinatedMasterBoundary(io,baseline,fleet,attentionReady)) uncertain=true; }
	} else uncertain=true;
	if (uncertain) {
		m.recovery_required=1; code.unsafe=coordinator.unsafe=bulk.unsafe=true; store.recovery_required=true;
		PioneerQuarantineWideOutput(io,wide); PioneerPersistentQuarantine(io,fleet); PioneerQuarantineOutputBuffer(io,transfer);
	}
	return !uncertain && PioneerSpanProof(r,bytes,coordinator.nonce);
}
#endif
