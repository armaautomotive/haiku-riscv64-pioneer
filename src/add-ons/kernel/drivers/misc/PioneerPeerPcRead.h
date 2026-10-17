/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_PEER_PC_READ_H
#define PIONEER_PEER_PC_READ_H
#include "PioneerRuntimeFleet.h"
#include "PioneerBulkUpload.h"

// Qualification prerequisite for an on-card dispatcher, NOT a faster
// inference ABI. Read exactly one fixed peer debug-PC register through
// master's NoC, twice, and compare four CPU PC observations while all
// RISCs remain held. No peer write, RISC release, host DMA or reset.
struct tt_peer_pc_read_result {
	uint32_t version,state,worker,requests,completed,recovery_required;
	uint32_t cpu_before[2],cpu_after[2],noc_pc[2],polls[2];
	uint32_t before[2][4],after[2][4];
};
static inline bool PioneerPeerPcReadProof(const tt_peer_pc_read_result& r,unsigned worker)
{
	if (!worker || worker>=TT_PIONEER_WORKERS || r.version!=1 || r.state!=TT_STORE_PASSED
		|| r.worker!=worker || r.requests!=2 || r.completed!=2 || r.recovery_required) return false;
	for (unsigned i=0;i<2;i++) {
		if (!r.polls[i] || r.cpu_before[i]!=r.cpu_before[0] || r.cpu_after[i]!=r.cpu_before[0]
			|| r.noc_pc[i]!=r.cpu_before[0]) return false;
		for (unsigned w=0;w<4;w++) if (uint32_t(r.after[i][w]-r.before[i][w])!=1) return false;
	}
	return true;
}
template<class IO>
bool PioneerPeerPcMasterHeld(IO& io,const tt_dram_samples_result& baseline,
	PioneerFleetWorker* fleet,bool attentionReady)
{
	if (!PioneerFleetMasterBoundary(io,baseline,fleet,attentionReady) || !io.BulkMappingStable()) return false;
	for (unsigned s=0;s<2;s++) for (unsigned w=0;w<3;w++)
		if (io.DramTlbRead(s,w)!=BlackholeDramFixedTlbWord(s?0xffa00000:0,1,2,false,w)) return false;
	return true;
}
template<class IO>
bool PioneerPeerPcRead(IO& io,const tt_dram_samples_result& baseline,PioneerFleetWorker* fleet,
	PioneerBulkUploadLease& bulk,bool attentionReady,unsigned worker,tt_peer_pc_read_result& r)
{
	r={}; r.version=1; r.worker=worker;
	if (!fleet || !worker || worker>=TT_PIONEER_WORKERS || !bulk.ready || !bulk.saved || bulk.unsafe
		|| fleet[worker].noc.version!=PioneerWorkerVersion(worker)
		|| fleet[worker].noc.state!=TT_COMPUTE_NOC_PASSED || !fleet[worker].noc.tlb_restored
		|| !PioneerWorkerRoute(baseline,fleet[worker].noc.source[0],worker)
		|| !PioneerPeerPcMasterHeld(io,baseline,fleet,attentionReady)) return false;
	auto& master=fleet[0].Store();
	bool uncertain=false;
	r.state=TT_STORE_CONFIG_FAILED;
	do {
		if (!io.SelectRuntimeCore(worker) || io.ResetRead()!=0x47800 || io.ResetRead()!=0x47800) { uncertain=true; break; }
		for (unsigned i=0;i<2;i++) { io.Pause(); r.cpu_before[i]=io.RuntimePcRead(); }
		if (r.cpu_before[0]!=r.cpu_before[1]) { uncertain=true; break; }
		if (!PioneerPeerPcMasterHeld(io,baseline,fleet,attentionReady)) { uncertain=true; break; }
		for (unsigned step=0;step<2;step++) {
			bulk.dirty=true;
			io.BulkL1Write(0,~r.cpu_before[0]);
			if (io.BulkL1Read(0)!=~r.cpu_before[0]) break;
			const uint32_t command[]={0xffb13140,0,PioneerWorkerX(worker)|(PioneerWorkerY(worker)<<6),
				TT_BULK_UPLOAD_L1,0,129,0,0,4,0};
			if (!BlackholeHostWriteCommand(io,command)) break;
			for (unsigned w=0;w<4;w++) r.before[step][w]=io.ResidentNocRead(0x208+w*4);
			master.pending=true; r.requests++;
			io.ResidentNocWrite(0x40,1);
			for (unsigned attempt=0;attempt<256;attempt++) {
				r.polls[step]++;
				bool complete=!io.ResidentNocRead(0x40);
				for (unsigned w=0;w<4;w++) {
					r.after[step][w]=io.ResidentNocRead(0x208+w*4);
					complete &= uint32_t(r.after[step][w]-r.before[step][w])==1;
				}
				if (complete && BlackholeResidentIdle(io)) { master.pending=false; r.completed++; break; }
				io.Pause();
			}
			if (master.pending) { uncertain=true; r.state=TT_STORE_TIMEOUT; break; }
			if (!PioneerPeerPcMasterHeld(io,baseline,fleet,attentionReady)) { uncertain=true; break; }
			r.noc_pc[step]=io.BulkL1Read(0);
			if (r.noc_pc[step]!=r.cpu_before[0]) { r.state=TT_STORE_MISMATCH; break; }
		}
		if (uncertain || r.completed!=2 || r.noc_pc[0]!=r.cpu_before[0] || r.noc_pc[1]!=r.cpu_before[0]) break;
		if (!io.SelectRuntimeCore(worker) || io.ResetRead()!=0x47800 || io.ResetRead()!=0x47800) { uncertain=true; break; }
		for (unsigned i=0;i<2;i++) { io.Pause(); r.cpu_after[i]=io.RuntimePcRead(); }
		if (r.cpu_after[0]!=r.cpu_before[0] || r.cpu_after[1]!=r.cpu_before[0]) { uncertain=true; break; }
		if (!PioneerPeerPcMasterHeld(io,baseline,fleet,attentionReady)) { uncertain=true; break; }
		r.state=TT_STORE_PASSED;
	} while (false);
	if (!uncertain && !PioneerPeerPcMasterHeld(io,baseline,fleet,attentionReady)) uncertain=true;
	if (uncertain || master.pending) {
		r.recovery_required=1; master.recovery_required=true; bulk.unsafe=true;
		fleet[worker].Store().recovery_required=true; fleet[worker].Compute().unsafe=true;
		io.QuarantineHostPage();
	}
	return PioneerPeerPcReadProof(r,worker);
}
#endif
