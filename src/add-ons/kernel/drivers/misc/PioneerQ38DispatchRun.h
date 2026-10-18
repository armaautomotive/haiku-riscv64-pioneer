/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_DISPATCH_RUN_H
#define PIONEER_Q38_DISPATCH_RUN_H
#include "PioneerQ38DispatchBootstrap.h"
#include "PioneerPayloadBlock.h"
// Experimental54 controller. OFFLINE tests are not native qualification.
static inline bool PioneerQ38DispatchStageRange(uint32_t a,unsigned words)
{
	return (a==0x74000 && words==16) || (a>=0x74080 && a<0x74f60 && !((a-0x74080)&31) && words==8)
		|| (a==0xa0000 && (words==2560 || words==3072 || words==8704))
		|| (a==0xab000 && (words==32 || words==80 || words==160 || words==192 || words==544));
}
template<class IO> auto PioneerQ38DispatchStage(IO& io,uint32_t a,const uint32_t* input,unsigned words,int)
	-> decltype(io.Q38PayloadData(a),bool())
{
	if (!input || !PioneerQ38DispatchStageRange(a,words) || !io.Q38PanelMappingStable()) return false;
	auto data=io.Q38PayloadData(a); return PioneerPayloadStageBlock(data,words,words,input);
}
template<class IO> bool PioneerQ38DispatchStage(IO& io,uint32_t a,const uint32_t* input,unsigned words,long)
{ return input && PioneerQ38DispatchStageRange(a,words) && io.Q38PanelMappingStable() && PioneerQ38PanelStage(io,a,input,words); }
static inline bool PioneerQ38DispatchReadRange(uint32_t a,unsigned words)
{
	return (a==0x7f800 && words==64) || (a>=0x76000 && a<0x7d700 && !((a-0x76000)&255) && words==64)
		|| (a>=0x80000 && a<0x9dc00 && !((a-0x80000)&1023) && words && words<=256 && !(words&15))
		|| PioneerQ38DispatchStageRange(a,words);
}
template<class IO> auto PioneerQ38DispatchRead(IO& io,uint32_t a,uint32_t* output,unsigned words,int)
	-> decltype(io.Q38PayloadData(a),bool())
{
	if (!output || !PioneerQ38DispatchReadRange(a,words) || !io.Q38PanelMappingStable()) return false;
	auto data=io.Q38PayloadData(a); return PioneerPayloadReadBlock(data,0,words,words,output);
}
template<class IO> bool PioneerQ38DispatchRead(IO& io,uint32_t a,uint32_t* output,unsigned words,long)
{
	if (!output || !PioneerQ38DispatchReadRange(a,words) || !io.Q38PanelMappingStable()) return false;
	for (unsigned i=0;i<words;i++) output[i]=io.Q38PanelRead(a+i*4);
	return true;
}
template<class IO> auto PioneerQ38DispatchInputCompare(IO& io,uint32_t a,const uint32_t* input,unsigned words,int)
	-> decltype(io.Q38PayloadData(a),bool())
{
	if (!input || (a!=0xa0000 && a!=0xab000) || !PioneerQ38DispatchStageRange(a,words) || !io.Q38PanelMappingStable()) return false;
	auto data=io.Q38PayloadData(a); bool okay=true; data.PayloadFence();
	for (unsigned w=0;w<words;w+=2) {
		uint64_t value=PioneerPayloadPairRead(data,w,0);
		okay &= uint32_t(value)==input[w]; okay &= uint32_t(value>>32)==input[w+1];
	}
	data.PayloadFence(); return okay; // ALL bits compared,not a checksum
}
template<class IO> bool PioneerQ38DispatchInputCompare(IO& io,uint32_t a,const uint32_t* input,unsigned words,long)
{
	if (!input || (a!=0xa0000 && a!=0xab000) || !PioneerQ38DispatchStageRange(a,words) || !io.Q38PanelMappingStable()) return false;
	bool okay=true; for (unsigned w=0;w<words;w++) okay &= io.Q38PanelRead(a+w*4)==input[w];
	return okay;
}
template<class IO> bool PioneerQ38DispatchInputsMatch(IO& io,const tt_q38_dispatch_probe& job)
{
	unsigned padded=(job.inputs/PioneerQ38CohortBlockInputs(job.type)+15)&~15U;
	bool okay=PioneerQ38DispatchInputCompare(io,0xa0000,job.activation,job.inputs/2,0);
	okay &= PioneerQ38DispatchInputCompare(io,0xab000,job.scales,padded,0);
	return okay;
}
static inline bool PioneerQ38DispatchRunFail(PioneerQ38DispatchLease& lease,PioneerResidentBanks& banks,tt_q38_dispatch_probe& job)
{
	job.result.state=TT_RUNTIME_COMPLETION_FAILED; job.result.recovery_required=1; job.result.root_sources_verified=0;
	for (auto& rows:job.output) for (auto& word:rows) word=0;
	return PioneerQ38DispatchQuarantine(lease,banks);
}
template<class IO> bool PioneerQ38DispatchRun(IO& io,const tt_dram_samples_result& dram,PioneerResidentBanks& banks,
	PioneerQ38DispatchLease& lease,const uint8_t* initialized,uint32_t initializedPages,tt_q38_dispatch_probe& job)
{
	if (lease.unsafe || !lease.booted || lease.closed || lease.rootRunning || !PioneerQ38DispatchValid(job)
		|| lease.rootNonce>=0x7ffffffe) return false;
	auto& a=lease.authority; auto& root=lease.worker[0]; auto& r=job.result;
	// Complete ALL private initialized extents BEFORE any IO/nonce advance.
	for (unsigned w=0;w<job.peers;w++) if (lease.peerNonce[w]>=0x7ffffffe
		|| !PioneerQ38CohortAuthorizePart(banks,dram,initialized,initializedPages,job.type,job.inputs,job.part[w],w+1,a.extent[w])) return false;
	a.nextRoot=lease.rootNonce+1;
	for (unsigned w=0;w<job.peers;w++) a.nextPeer[w]=lease.peerNonce[w]+1;
	r={}; r.version=1; r.state=TT_RUNTIME_CONFIG_FAILED;
	for (auto& rows:job.output) for (auto& word:rows) word=0;
	if (!io.SelectRuntimeCore(0) || !PioneerQ38DispatchSources(io,dram,banks,root,0x47800)) return PioneerQ38DispatchRunFail(lease,banks,job);
	unsigned padded=(job.inputs/PioneerQ38CohortBlockInputs(job.type)+15)&~15U;
	if (!PioneerQ38DispatchStage(io,0xa0000,job.activation,job.inputs/2,0)
		|| !PioneerQ38DispatchStage(io,0xab000,job.scales,padded,0)) return PioneerQ38DispatchRunFail(lease,banks,job);
	for (unsigned w=0;w<119;w++) {
		uint32_t task[8]={};
		if (w<job.peers) { task[0]=a.extent[w].address; task[1]=job.part[w].bank; task[2]=job.part[w].groups;
			task[3]=a.nextPeer[w]; task[4]=a.extent[w].route; task[5]=a.source[w+1][0]; }
		if (!PioneerQ38DispatchStage(io,0x74080+w*32,task,8,0)) return PioneerQ38DispatchRunFail(lease,banks,job);
	}
	const uint32_t header[]={TT_Q38_DISPATCH_MARKER,1,1,job.type,job.inputs,job.peers,a.nextRoot,0,0,0,0,0,0,0,0,0};
	if (!PioneerQ38DispatchStage(io,0x74000,header,16,0)) return PioneerQ38DispatchRunFail(lease,banks,job);
	root.Store().l1_dirty=true; root.Store().l1_restored=false; // runtime status lives in separately saved17e000..180000
	for (unsigned c=0;c<3;c++) io.RuntimeStatusWrite(c,0);
	for (unsigned c=0;c<3;c++) if (io.RuntimeStatusRead(c)) return PioneerQ38DispatchRunFail(lease,banks,job);
	if (!PioneerQ38DispatchSources(io,dram,banks,root,0x47800)) return PioneerQ38DispatchRunFail(lease,banks,job);
	for (unsigned c=0;c<8;c++) a.before[c]=io.ResidentNocRead(PioneerQ38PanelCounter(c));
	lease.rootRunning=root.image.running=true;
	io.RuntimeResetWrite(0x87ffff); io.Pause();
	if (!PioneerQ38DispatchSources(io,dram,banks,root,0x87ffff)) return PioneerQ38DispatchRunFail(lease,banks,job);
	r.root_started=1;
	io.RuntimeResetWrite(0x46800); if (io.ResetRead()!=0x46800) return PioneerQ38DispatchRunFail(lease,banks,job);
	const uint64_t deadline=io.Q38CohortNow()+UINT64_C(30000000); bool complete=false;
	while (io.Q38CohortNow()<deadline) {
		r.root_phase=io.RuntimeStatusRead(0); r.root_count=io.RuntimeStatusRead(1); r.root_marker=io.RuntimeStatusRead(2);
		if (r.root_marker==TT_Q38_DISPATCH_FAULT) break;
		if (r.root_phase==TT_Q38_DISPATCH_DONE && r.root_count==job.peers && r.root_marker==TT_Q38_DISPATCH_MARKER) {
			io.Pause(); r.root_pc[0]=io.RuntimePcRead(); io.Pause(); r.root_pc[1]=io.RuntimePcRead();
			if (PioneerQ38RootTerminalPc(r.root_pc[0]) && PioneerQ38RootTerminalPc(r.root_pc[1])) { complete=true; break; }
		}
		io.Pause();
	}
	if (!complete || !PioneerQ38DispatchSources(io,dram,banks,root,0x46800)) return PioneerQ38DispatchRunFail(lease,banks,job);
	for (unsigned c=0;c<8;c++) { a.after[c]=io.ResidentNocRead(PioneerQ38PanelCounter(c)); r.before[c]=a.before[c]; r.after[c]=a.after[c]; }
	// Still PARKED/released. Never reset/hold root before ALL metadata and
	// source proofs pass,even if an EARLIER peer already completed cleanly.
	r.root_completed=r.root_sources_verified=1; r.peers=r.started=r.completed=r.waiting=job.peers;
	r.nonce=a.nextRoot; r.state=TT_RUNTIME_PASSED;
	if (!PioneerQ38DispatchRead(io,0x7f800,r.stats,64,0)) return PioneerQ38DispatchRunFail(lease,banks,job);
	for (unsigned w=0;w<job.peers;w++) if (!PioneerQ38DispatchRead(io,0x76000+w*256,r.peer[w],64,0)) return PioneerQ38DispatchRunFail(lease,banks,job);
	if (!PioneerQ38DispatchProof(job,a,false,false) || !PioneerQ38DispatchInputsMatch(io,job)) return PioneerQ38DispatchRunFail(lease,banks,job);
	const uint32_t command[]={0x83f00,0,PioneerWorkerX(job.peers)|(PioneerWorkerY(job.peers)<<6),0x75000,0,129,0,0,
		256+job.part[job.peers-1].groups*64,0};
	for (unsigned i=0;i<10;i++) if (io.ResidentNocRead(i*4)!=command[i]) return PioneerQ38DispatchRunFail(lease,banks,job);
	// ALL metadata proved BEFORE any host output read. ALL output stays in
	// the kernel-private request until final hold/proof succeeds.
	for (unsigned w=0;w<job.peers;w++) if (!PioneerQ38DispatchRead(io,0x80000+w*1024,job.output[w],job.part[w].groups*16,0)) return PioneerQ38DispatchRunFail(lease,banks,job);
	if (!PioneerQ38DispatchProof(job,a,false)) {
		for (auto& rows:job.output) for (auto& word:rows) word=0;
		return PioneerQ38DispatchRunFail(lease,banks,job);
	}
	io.RuntimeResetWrite(0x87ffff); io.Pause();
	if (io.ResetRead()!=0x87ffff || io.ResetRead()!=0x87ffff) {
		for (auto& rows:job.output) for (auto& word:rows) word=0;
		return PioneerQ38DispatchRunFail(lease,banks,job);
	}
	io.RuntimeResetWrite(0x47800); io.Pause();
	if (!PioneerQ38DispatchSources(io,dram,banks,root,0x47800)) {
		for (auto& rows:job.output) for (auto& word:rows) word=0;
		return PioneerQ38DispatchRunFail(lease,banks,job);
	}
	lease.rootRunning=root.image.running=false; r.root_held=1;
	if (!PioneerQ38DispatchProof(job,a)) return PioneerQ38DispatchRunFail(lease,banks,job);
	lease.rootNonce=a.nextRoot; for (unsigned w=0;w<job.peers;w++) lease.peerNonce[w]=a.nextPeer[w];
	return true;
}
#endif
