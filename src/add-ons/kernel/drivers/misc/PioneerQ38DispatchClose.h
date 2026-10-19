/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_DISPATCH_CLOSE_H
#define PIONEER_Q38_DISPATCH_CLOSE_H
#include "PioneerQ38DispatchRun.h"
// Experimental54 END controller;not native-qualified by offline tests.
// STOP is a private operation,never a userspace request/sequence capability.
template<class IO> bool PioneerQ38DispatchPeerStatus(IO& io,uint32_t nonce,bool stopped,PioneerQ38DispatchLease* diagnostic=nullptr)
{
	bool okay=true;
	for (unsigned i=0;i<16;i++) {
		uint32_t expected=i<2?nonce:i==3?TT_Q38_PEER_MARKER:i==4 && stopped?2:0;
		uint32_t actual=io.Q38PanelRead(0x74140+i*4);
		if (actual!=expected && diagnostic && !diagnostic->closeDetail[0]) {
			diagnostic->closeDetail[0]=0x100+i; diagnostic->closeDetail[1]=actual;
			diagnostic->closeDetail[2]=expected; diagnostic->closeDetail[3]=nonce;
		}
		okay &= actual==expected;
	}
	io.Pause(); uint32_t first=io.RuntimePcRead(); io.Pause(); uint32_t second=io.RuntimePcRead();
	bool pc=stopped?PioneerQ38PeerTerminalPc(first) && PioneerQ38PeerTerminalPc(second)
		:(first==TT_Q38_PEER_WAIT_LOAD || first==TT_Q38_PEER_WAIT_BRANCH)
		&& (second==TT_Q38_PEER_WAIT_LOAD || second==TT_Q38_PEER_WAIT_BRANCH);
	if (!pc && diagnostic && !diagnostic->closeDetail[0]) {
		diagnostic->closeDetail[0]=0x200; diagnostic->closeDetail[1]=first;
		diagnostic->closeDetail[2]=second; diagnostic->closeDetail[3]=stopped?unsigned(TT_Q38_PEER_STOP):unsigned(TT_Q38_PEER_WAIT_LOAD);
	}
	return okay && pc;
}
static inline bool PioneerQ38DispatchStopProof(const uint32_t* stats,const PioneerQ38DispatchAuthority& a)
{
	if (!a.nextRoot || a.nextRoot>=0x7fffffff || stats[0]!=TT_Q38_DISPATCH_MARKER || stats[1]!=1
		|| stats[2]!=2 || stats[3] || stats[4] || stats[5]!=119 || stats[12]!=a.nextRoot
		|| stats[13] || stats[14] || stats[15] || stats[10]<238 || stats[11]<238
		|| stats[11]>stats[10] || stats[10]>2*65536*119) return false;
	uint32_t reads=stats[10]+3*stats[11]+119,writes=238;
	if (stats[6]!=reads || stats[8]!=reads || stats[7]!=writes || stats[9]!=writes) return false;
	for (unsigned c=0;c<8;c++) {
		uint32_t expected=c==0 || c==1 || c==3?reads:c==2 || c==6?reads+writes:writes;
		if (stats[16+c]!=a.before[c] || stats[24+c]!=a.after[c] || uint32_t(a.after[c]-a.before[c])!=expected) return false;
	}
	for (unsigned i=0;i<16;i++) if (stats[32+i]!=a.source[0][i]) return false;
	for (unsigned i=48;i<64;i++) if (stats[i]) return false;
	return true;
}
// Single-attempt restoration:unlike the legacy generic helper,this path
// performs NO further write after a failed NIU/L1/TLB readback.
template<class IO> bool PioneerQ38DispatchPeerLeaseEnd(IO& io,const tt_compute_noc_result& noc,PioneerResidentLease& store)
{
	if (!store.ready || !store.configured || !store.saved || !store.controls_saved || store.pending || store.recovery_required
		|| io.ResetRead()!=0x47800 || !BlackholeResidentSourceStable(io,noc,store.config|0x10000)
		|| !BlackholeResidentIdle(io) || !BlackholeHostReadPciQuiet(io,0x402)) return false;
	for (unsigned w=0;w<10;w++) io.ResidentNocWrite(w*4,0);
	io.ResidentNocWrite(0x100,store.config);
	bool okay=BlackholeResidentSourceStable(io,noc,store.config);
	for (unsigned w=0;w<10;w++) okay &= io.ResidentNocRead(w*4)==0;
	if (!okay) return false;
	store.niu_restored=true;
	if (store.l1_dirty) {
		for (unsigned w=0;w<2048;w++) io.ResidentL1Write(w,store.saved[w]);
		okay=io.ResetRead()==0x47800;
		for (unsigned w=0;w<2048;w++) okay &= io.ResidentL1Read(w)==store.saved[w];
		if (!okay) return false;
		store.l1_restored=true;
	}
	for (unsigned s=0;s<2;s++) for (unsigned w=0;w<3;w++) io.DramTlbWrite(s,w,store.tlb[s][w]);
	for (unsigned s=0;s<2;s++) for (unsigned w=0;w<3;w++) okay &= io.DramTlbRead(s,w)==store.tlb[s][w];
	if (!okay) return false;
	store.tlb_restored=true; store.ready=false; return true;
}
template<class IO> bool PioneerQ38DispatchClose(IO& io,const tt_dram_samples_result& dram,
	PioneerResidentBanks& banks,PioneerQ38DispatchLease& lease,bool clean)
{
#ifdef PIONEER_Q38_REUSE_ACTIVATIONS
	lease.inputReuse.valid=false;
#endif
	if (lease.unsafe) return false; // especially NO select/reset/restore/free
	if (!clean || lease.rootRunning) return PioneerQ38DispatchQuarantine(lease,banks);
	if (lease.closed) return true;
	lease.closePhase=1; lease.closeWorker=0;
	if (!lease.prepared) { lease.closed=true; return true; }
	if (lease.bootAttempted && !lease.booted) return PioneerQ38DispatchQuarantine(lease,banks);
	if (lease.booted && (lease.prepared!=120 || lease.rootNonce>=0x7ffffffe)) return PioneerQ38DispatchQuarantine(lease,banks);
	// ALL current role code/source/physical state audited BEFORE ANY STOP.
	for (unsigned w=0;w<lease.prepared;w++) {
		lease.closePhase=10; lease.closeWorker=w;
		auto& node=lease.worker[w];
		if (!node.image.saved || !node.image.ready || (w && lease.peerNonce[w-1]>=0x7ffffffe)
			|| !io.SelectRuntimeCore(w) || !PioneerQ38DispatchSources(io,dram,banks,node,w && lease.booted?0x46800:0x47800)
			|| !PioneerQ38DispatchCodeMatches(io,w,&lease)
			|| (w && lease.booted && !PioneerQ38DispatchPeerStatus(io,lease.peerNonce[w-1],false,&lease))) return PioneerQ38DispatchQuarantine(lease,banks);
	}
	if (lease.booted) {
		lease.closePhase=20; lease.closeWorker=0;
		auto& a=lease.authority; auto& root=lease.worker[0]; a.nextRoot=lease.rootNonce+1;
		if (!io.SelectRuntimeCore(0)) return PioneerQ38DispatchQuarantine(lease,banks);
		for (unsigned w=0;w<119;w++) {
			a.nextPeer[w]=lease.peerNonce[w]+1;
			const uint32_t task[]={0,0,0,a.nextPeer[w],0,a.source[w+1][0],0,0};
			if (!PioneerQ38DispatchStage(io,0x74080+w*32,task,8,0)) return PioneerQ38DispatchQuarantine(lease,banks);
		}
		const uint32_t header[]={TT_Q38_DISPATCH_MARKER,1,2,0,0,119,a.nextRoot,0,0,0,0,0,0,0,0,0};
		if (!PioneerQ38DispatchStage(io,0x74000,header,16,0)) return PioneerQ38DispatchQuarantine(lease,banks);
		root.Store().l1_dirty=true; root.Store().l1_restored=false;
		for (unsigned c=0;c<3;c++) io.RuntimeStatusWrite(c,0);
		for (unsigned c=0;c<3;c++) if (io.RuntimeStatusRead(c)) return PioneerQ38DispatchQuarantine(lease,banks);
		if (!PioneerQ38DispatchSources(io,dram,banks,root,0x47800)) return PioneerQ38DispatchQuarantine(lease,banks);
		for (unsigned c=0;c<8;c++) a.before[c]=io.ResidentNocRead(PioneerQ38PanelCounter(c));
		lease.rootRunning=root.image.running=true;
		io.RuntimeResetWrite(0x87ffff); io.Pause();
		if (!PioneerQ38DispatchSources(io,dram,banks,root,0x87ffff)) return PioneerQ38DispatchQuarantine(lease,banks);
		io.RuntimeResetWrite(0x46800); if (io.ResetRead()!=0x46800) return PioneerQ38DispatchQuarantine(lease,banks);
		lease.closePhase=30;
		const uint64_t deadline=io.Q38CohortNow()+UINT64_C(30000000); bool complete=false;
		while (io.Q38CohortNow()<deadline) {
			uint32_t phase=io.RuntimeStatusRead(0),count=io.RuntimeStatusRead(1),marker=io.RuntimeStatusRead(2);
			lease.closeStatus[0]=phase; lease.closeStatus[1]=count; lease.closeStatus[2]=marker;
			if (marker==TT_Q38_DISPATCH_FAULT) break;
			if (phase==TT_Q38_DISPATCH_DONE && count==119 && marker==TT_Q38_DISPATCH_MARKER) {
				io.Pause(); uint32_t first=io.RuntimePcRead(); io.Pause(); uint32_t second=io.RuntimePcRead();
				if (PioneerQ38RootTerminalPc(first) && PioneerQ38RootTerminalPc(second)) { complete=true; break; }
			}
			io.Pause();
		}
		if (!complete || !PioneerQ38DispatchSources(io,dram,banks,root,0x46800)) return PioneerQ38DispatchQuarantine(lease,banks);
		lease.closePhase=40;
		for (unsigned c=0;c<8;c++) a.after[c]=io.ResidentNocRead(PioneerQ38PanelCounter(c));
		uint32_t stats[64];
		if (!PioneerQ38DispatchRead(io,0x7f800,stats,64,0) || !PioneerQ38DispatchStopProof(stats,a)) return PioneerQ38DispatchQuarantine(lease,banks);
		const uint32_t command[]={0xffb121b0,0,PioneerWorkerX(119)|(PioneerWorkerY(119)<<6),0x75010,0,129,0,0,4,0};
		for (unsigned i=0;i<10;i++) if (io.ResidentNocRead(i*4)!=command[i]) return PioneerQ38DispatchQuarantine(lease,banks);
		// Independent ALL120 physical audits AFTER STOP and BEFORE ANY hold.
		for (unsigned w=0;w<120;w++) {
			lease.closePhase=51; lease.closeWorker=w;
			if (!io.SelectRuntimeCore(w) || !PioneerQ38DispatchSources(io,dram,banks,lease.worker[w],0x46800)) return PioneerQ38DispatchQuarantine(lease,banks);
			lease.closePhase=52;
			if (!PioneerQ38DispatchCodeMatches(io,w,&lease)) return PioneerQ38DispatchQuarantine(lease,banks);
			lease.closePhase=53;
			if (w && !PioneerQ38DispatchPeerStatus(io,a.nextPeer[w-1],true,&lease)) return PioneerQ38DispatchQuarantine(lease,banks);
		}
		// No reset above. Only after EVERY terminal proof may ANY core hold.
		for (unsigned w=0;w<120;w++) {
			lease.closePhase=60; lease.closeWorker=w;
			if (!io.SelectRuntimeCore(w)) return PioneerQ38DispatchQuarantine(lease,banks);
			io.RuntimeResetWrite(0x87ffff); io.Pause();
			if (io.ResetRead()!=0x87ffff || io.ResetRead()!=0x87ffff) return PioneerQ38DispatchQuarantine(lease,banks);
			io.RuntimeResetWrite(0x47800); io.Pause();
			if (!PioneerQ38DispatchSources(io,dram,banks,lease.worker[w],0x47800)) return PioneerQ38DispatchQuarantine(lease,banks);
			lease.worker[w].image.running=false;
		}
		lease.rootRunning=false;
	}
	// ALL held,source and code rechecked BEFORE restoring the FIRST byte.
	for (unsigned w=0;w<lease.prepared;w++) {
		lease.closePhase=70; lease.closeWorker=w;
		if (!io.SelectRuntimeCore(w) || !PioneerQ38DispatchSources(io,dram,banks,lease.worker[w],0x47800)
			|| !PioneerQ38DispatchCodeMatches(io,w)) return PioneerQ38DispatchQuarantine(lease,banks);
	}
	for (unsigned end=lease.prepared;end;end--) {
		unsigned w=end-1; auto& node=lease.worker[w];
		lease.closePhase=80; lease.closeWorker=w;
		if (!io.SelectRuntimeCore(w) || !PioneerQ38PanelClose(io,dram,node.noc,node.Store(),banks,node.image,true)
			|| (w && !PioneerQ38DispatchPeerLeaseEnd(io,node.noc,node.Store()))) return PioneerQ38DispatchQuarantine(lease,banks);
	}
	if (!io.SelectRuntimeCore(0)) return PioneerQ38DispatchQuarantine(lease,banks);
	lease.booted=false; lease.closed=true; lease.closePhase=90; lease.closeWorker=0; return true;
}
#endif
