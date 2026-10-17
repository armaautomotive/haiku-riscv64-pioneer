/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_LAYER_GLU_H
#define PIONEER_LAYER_GLU_H
#include "PioneerCoordinatedFleet.h"
// Correct separately scheduled GLU path. Fusion may omit these host transfers
// ONLY after proving a complete unobserved FFN graph. Never CPU SiLU fallback.
static inline bool PioneerLayerGluProof(const tt_coordinated_fleet_result& r,unsigned n,unsigned nonce)
{
	const auto& m=r.master;
	return !r.inputs && r.outputs==n && r.workers==1 && !r.staged_pages && !r.nonfinite && r.dma_words==n && r.gathered==n
		&& m.version==1 && m.state==TT_RUNTIME_PASSED && m.workers==1 && m.matched==1 && m.nonce==nonce
		&& m.prepared==1 && m.started==1 && m.completed==1 && m.stopped==1 && !m.recovery_required && m.polls
		&& m.phase==TT_LAYER_DONE && m.count==1 && m.marker==TT_COORDINATOR_MARKER
		&& m.pc[0]==TT_LAYER_PARK && m.pc[1]==m.pc[0] && m.summary_verified==1
		&& PioneerLayerSummary(r.summary,5,1,n,nonce,m.before,m.after);
}
template<class IO>
bool PioneerLayerGlu(IO& io,const tt_dram_samples_result& baseline,const tt_host_page_result& host,
	PioneerOutputBufferLease& transfer,PioneerFleetWorker* fleet,PioneerBulkUploadLease& bulk,PioneerCoordinatorLease& coordinator,
	PioneerLayerLease& code,bool attentionReady,unsigned n,const uint32_t* gate,const uint32_t* up,uint32_t* output,tt_coordinated_fleet_result& r)
{
	r={}; auto& m=r.master; m.version=1; m.workers=r.workers=1; r.outputs=n;
	if (!fleet || !gate || !up || !output || !n || n>3072 || (n&31) || !code.ready || !code.saved || code.running || code.unsafe
		|| !coordinator.ready || !coordinator.saved || !coordinator.fast || !coordinator.compact || !coordinator.fused || coordinator.readback
		|| coordinator.running || coordinator.unsafe || coordinator.nonce>=0x7ffffffe || !bulk.ready || bulk.unsafe) return false;
	for (unsigned i=0;i<n;i++) if ((gate[i]&0x7f800000)==0x7f800000 || (up[i]&0x7f800000)==0x7f800000) return false;
	auto& master=fleet[0]; auto& store=master.Store(); auto& compute=master.Compute();
	if (!compute.ready || !compute.saved || !compute.bulk || compute.buffer_words!=TT_Q8_BULK_SAVED_BUFFER_WORDS
		|| io.RuntimeBufferWords()!=TT_Q8_BULK_SAVED_BUFFER_WORDS || compute.running || compute.unsafe) return false;
	bool uncertain=false,bme=false; uint32_t previous=0;
	coordinator.inputValid=false; coordinator.inputK=0;
	m.state=TT_RUNTIME_CONFIG_FAILED;
	do {
		if (!PioneerCoordinatedMasterBoundary(io,baseline,fleet,attentionReady) || !PioneerLayerMapping(io,0)
			|| !PioneerCoordinatorLocalHeld(io,baseline,master.noc,store)
			|| !PioneerBulkUploadGuard(io,baseline,master.noc,host,store,transfer,0x402)) { uncertain=true; break; }
		previous=io.RuntimeCodeRead(0); uint32_t* stage=io.RuntimeStageWords(); if (!stage) break;
		// Fixed runtime buffer lease owns both disjoint F32 spans. This
		// separately scheduled path favors simple fully verified PIO; fused
		// FFN does not stage/intermediately read back either span at all.
		compute.buffers_dirty=true;
		for (unsigned i=0;i<n;i++) { io.RuntimeBufferWrite(4096+i,up[i]); io.RuntimeBufferWrite(7168+i,gate[i]); }
		bool exact=true;
		for (unsigned i=0;i<n;i++) exact &= io.RuntimeBufferRead(4096+i)==up[i] && io.RuntimeBufferRead(7168+i)==gate[i];
		if (!exact) break;
		for (unsigned i=0;i<64;i++) stage[i]=0;
		stage[0]=0x31445243; stage[1]=5; stage[2]=1; stage[4]=n; stage[6]=++coordinator.nonce;
		stage[7]=uint32_t(transfer.physical); stage[8]=uint32_t(transfer.physical>>32); stage[9]=16384; m.nonce=coordinator.nonce;
		bulk.dirty=true;
		if (!PioneerBulkPayloadStage(io,stage,64,0) || !PioneerCoordinatedMasterBoundary(io,baseline,fleet,attentionReady)
			|| !PioneerLayerMapping(io,0)) break;
		store.l1_dirty=true; store.l1_restored=false;
		for (unsigned i=0;i<3;i++) io.RuntimeStatusWrite(i,0);
		for (unsigned i=0;i<3;i++) exact &= io.RuntimeStatusRead(i)==0;
		if (!exact) break;
		io.RuntimeCodeWrite(0,TT_LAYER_BOOT);
		if (io.RuntimeCodeRead(0)!=TT_LAYER_BOOT || !PioneerLayerMapping(io,0) || !PioneerCoordinatorLocalHeld(io,baseline,master.noc,store)) break;
		m.prepared=1;
		io.BulkHostFence(); for (unsigned i=0;i<n;i++) io.BulkHostStore(i,0xffffffff); io.BulkHostFence();
		for (unsigned i=0;i<n;i++) exact &= io.BulkHostLoad(i)==0xffffffff;
		io.BulkHostFence(); if (!exact) break;
		if (!PioneerBulkUploadGuard(io,baseline,master.noc,host,store,transfer,0x402)) { uncertain=true; break; }
		bme=true; io.HostPciWrite(1,4,2,0x406); if (io.HostPciRead(1,4,2)!=0x406) { uncertain=true; break; }
		io.HostPciWrite(0,4,2,0x406);
		if (!PioneerBulkUploadGuard(io,baseline,master.noc,host,store,transfer,0x406) || !PioneerLayerMapping(io,0)) { uncertain=true; break; }
		io.RuntimeResetWrite(0x87ffff); io.Pause();
		if (io.ResetRead()!=0x87ffff || io.ResetRead()!=0x87ffff || !PioneerCoordinatorSources(io,baseline,master.noc,store,0x406)
			|| !PioneerLayerMapping(io,0) || !BlackholeResidentIdle(io)) { uncertain=true; break; }
		PioneerResidentAttentionCounters(io,m.before);
		code.running=coordinator.running=true; transfer.pending=true;
		m.started=1; m.state=TT_RUNTIME_COMPLETION_FAILED; io.RuntimeResetWrite(0x46800);
		for (unsigned attempt=0;attempt<50000;attempt++) {
			m.polls++; if (io.ResetRead()!=0x46800) break;
			m.phase=io.RuntimeStatusRead(0); m.count=io.RuntimeStatusRead(1); m.marker=io.RuntimeStatusRead(2);
			if (m.marker==TT_COORDINATOR_FAULT) break;
			if (m.phase==TT_LAYER_DONE && m.count==1 && m.marker==TT_COORDINATOR_MARKER) {
				io.Pause(); m.pc[0]=io.RuntimePcRead(); io.Pause(); m.pc[1]=io.RuntimePcRead();
				if (m.pc[0]==TT_LAYER_PARK && m.pc[1]==m.pc[0] && BlackholeResidentIdle(io)) { m.completed=1; break; }
			}
			io.Pause();
		}
		if (!m.completed || io.ResetRead()!=0x46800 || !PioneerCoordinatorSources(io,baseline,master.noc,store,0x406)
			|| !PioneerLayerMapping(io,0) || !BlackholeResidentIdle(io)) { uncertain=true; break; }
		PioneerResidentAttentionCounters(io,m.after);
		for (unsigned i=0;i<24;i++) r.summary[i]=io.BulkL1Read(3072+i);
		if (!PioneerLayerSummary(r.summary,5,1,n,m.nonce,m.before,m.after)) { uncertain=true; break; }
		m.summary_verified=1;
		io.HostPciWrite(0,4,2,0x402); io.HostPciWrite(1,4,2,0x402);
		if (!BlackholeHostReadPciQuiet(io,0x402) || !PioneerOutputBufferMapStable(io,transfer,host)
			|| !BlackholeHostReadMapStable(io,host) || !PioneerCoordinatorSources(io,baseline,master.noc,store)
			|| !PioneerLayerMapping(io,0) || !BlackholeResidentIdle(io)) { uncertain=true; break; }
		bme=false; transfer.pending=false;
		io.RuntimeResetWrite(0x87ffff); io.Pause();
		if (io.ResetRead()!=0x87ffff || io.ResetRead()!=0x87ffff) { uncertain=true; break; }
		io.RuntimeResetWrite(0x47800); io.Pause();
		if (!PioneerCoordinatorLocalHeld(io,baseline,master.noc,store) || !PioneerLayerMapping(io,0)) { uncertain=true; break; }
		code.running=coordinator.running=false; m.stopped=m.matched=1;
		io.BulkHostFence();
		for (unsigned i=0;i<n;i++) { output[i]=io.BulkHostLoad(i); r.nonfinite+=(output[i]&0x7f800000)==0x7f800000; }
		io.BulkHostFence();
		if (r.nonfinite || !PioneerBulkUploadGuard(io,baseline,master.noc,host,store,transfer,0x402)) { uncertain=true; break; }
		r.dma_words=r.gathered=n; m.state=TT_RUNTIME_PASSED;
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
		PioneerPersistentQuarantine(io,fleet); PioneerQuarantineOutputBuffer(io,transfer);
	}
	return !uncertain && PioneerLayerGluProof(r,n,coordinator.nonce);
}
#endif
