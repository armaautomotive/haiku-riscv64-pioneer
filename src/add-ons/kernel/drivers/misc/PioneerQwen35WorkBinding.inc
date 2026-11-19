/* SPDX-License-Identifier: MIT */
#ifdef TT_PIONEER_Q35_WORK
#include "PioneerQwen35WorkOwner.h"

// A single uncached alias covers the fixed ROOT snapshot. The bootstrap
// word and final status cache line use the existing exact aliases.
class PioneerQ35WorkBinding {
	PioneerSessionIO& fIO;
	const tt_dram_samples_result& fDram;
	const tt_compute_noc_result& fNoc;
	const PioneerResidentLease& fLease;
	const PioneerResidentBanks& fBanks;
	PioneerQ38DispatchLease& fDispatch;
	area_id fArea=-1;
	volatile uint32* fL1=NULL;
	uint32 fTlb[2][3]={};

public:
	PioneerQwen35::WorkOwner owner;

	PioneerQ35WorkBinding(PioneerSessionIO& io,const tt_dram_samples_result& dram,
		const tt_compute_noc_result& noc,const PioneerResidentLease& lease,
		const PioneerResidentBanks& banks,PioneerQ38DispatchLease& dispatch)
		: fIO(io),fDram(dram),fNoc(noc),fLease(lease),fBanks(banks),fDispatch(dispatch)
	{
	}

	~PioneerQ35WorkBinding()
	{
		if (fArea>=B_OK) delete_area(fArea);
	}

	bool Map()
	{
		using namespace PioneerQwen35;
		if (fL1 || fArea>=B_OK || !fIO.runtimeMapped || fIO.runtimeCore
			|| !fIO.SelectRuntimeCore(0)) return false;
		for (unsigned tlb=0; tlb<2; tlb++)
			for (unsigned word=0; word<3; word++)
				fTlb[tlb][word]=fIO.DramTlbRead(tlb,word);
		void* view=NULL;
		fArea=map_physical_memory("P100A 4B owned ROOT L1",
			fIO.Profile().bar0_host_start+0x19000000+RootTextAddress,RootEnd-RootTextAddress,
			B_ANY_KERNEL_ADDRESS,B_KERNEL_READ_AREA|B_KERNEL_WRITE_AREA|B_UNCACHED_MEMORY,&view);
		if (fArea<B_OK || !view) return false;
		fL1=static_cast<volatile uint32*>(view);
		return Scope(0x47800);
	}

	bool Scope(unsigned reset)
	{
		if (!fL1 || fArea<B_OK || !fIO.runtimeMapped || fIO.runtimeCore || !fIO.fleetEnabled
			|| !fLease.ready || fLease.pending || fLease.recovery_required || !fBanks.Matches(fDram))
			return false;
		for (unsigned tlb=0; tlb<2; tlb++)
			for (unsigned word=0; word<3; word++)
				if (fIO.DramTlbRead(tlb,word)!=fTlb[tlb][word]) return false;
		return fIO.ResetRead()==reset && fIO.ResetRead()==reset
			&& PioneerRuntimeSources(fIO,fDram,fNoc,fLease);
	}

	bool RootHeld()
	{
		if (!fDispatch.booted || fDispatch.closed || PioneerQ38DispatchUnsafe(fDispatch)
			|| !fIO.SelectRuntimeCore(0) || !Scope(0x47800) || !Quiet()) return false;
		for (unsigned worker=1;worker<120;worker++) {
			auto& node=fDispatch.worker[worker];
			if (!fIO.SelectRuntimeCore(worker)
				|| !PioneerQ38DispatchSources(fIO,fDram,fBanks,node,0x46800)
				|| !PioneerQ38DispatchPeerStatus(fIO,fDispatch.peerNonce[worker-1],false,&fDispatch))
				return false;
		}
		return fIO.SelectRuntimeCore(0) && Scope(0x47800);
	}

	bool SyncDispatchState()
	{
		using namespace PioneerQwen35;
		if (!fIO.SelectRuntimeCore(0) || !Scope(0x47800)) return false;
		const unsigned base=(RootDispatchStateAddress-RootTextAddress)/4;
		if (PioneerRead32(fL1+base)!=RootDispatchStateMagic) return false;
		const unsigned rootNonce=PioneerRead32(fL1+base+1);
		if (rootNonce<fDispatch.rootNonce || rootNonce>=0x7ffffffe) return false;
		for (unsigned worker=0;worker<119;worker++) {
			const unsigned sequence=PioneerRead32(fL1+base+2+worker);
			if (sequence<fDispatch.peerNonce[worker] || sequence>rootNonce || sequence>=0x7ffffffe)
				return false;
		}
		fDispatch.rootNonce=rootNonce;
		for (unsigned worker=0;worker<119;worker++)
			fDispatch.peerNonce[worker]=PioneerRead32(fL1+base+2+worker);
		return true;
	}

	bool Quiet(uint32* detail=NULL)
	{
		for (unsigned i=0; i<4; i++) {
			const uint32 value=fIO.ResidentNocRead(0x40+i*0x800);
			if (value) { if (detail) *detail=0x10000000|(i<<20)|(value&0xfffff); return false; }
		}
		for (unsigned i=0; i<32; i++) {
			const uint32 value=fIO.ResidentNocRead(0x240+i*4)&255;
			if (value) { if (detail) *detail=0x20000000|(i<<8)|value; return false; }
		}
		return true;
	}

	bool WorkFaultNocSnapshot(PioneerQwen35::WorkPoll& result)
	{
		uint32 command[10],counters[8];
		if (!fIO.SelectRuntimeCore(0) || !Scope(0x46800) || !Quiet()) return false;
		for (unsigned i=0;i<10;i++) {
			command[i]=fIO.ResidentNocRead(i*4);
			if (command[i]==UINT32_MAX) return false;
		}
		for (unsigned i=0;i<8;i++) {
			counters[i]=fIO.ResidentNocRead(PioneerQ38PanelCounter(i));
			if (counters[i]==UINT32_MAX) return false;
		}
		fIO.Pause();
		if (!Scope(0x46800) || !Quiet() || Read(PioneerQwen35::RootCompletionAddress)
			!=PioneerQwen35::RootFault) return false;
		memcpy(result.faultNocCommand,command,sizeof(command));
		memcpy(result.faultNocCounters,counters,sizeof(counters));
		for (unsigned i=0;i<8;i++)
			result.faultNocCounterDelta[i]=counters[i]-fDispatch.bootCounter[0][i];
		result.faultNocValid=1;

		uint32 peerStatusFirst[16],peerStatus[16],peerPc0=0,peerPc1=0;
		auto& node=fDispatch.worker[119];
		if (!fIO.SelectRuntimeCore(119)
			|| !PioneerQ38DispatchSources(fIO,fDram,fBanks,node,0x46800)
			|| !PeerStatusSnapshot(119,peerStatusFirst,peerStatus)
			|| !PeerPcSnapshot(peerPc0,peerPc1)
			|| !fIO.SelectRuntimeCore(0) || !Scope(0x46800)
			|| Read(PioneerQwen35::RootCompletionAddress)!=PioneerQwen35::RootFault || !Quiet())
			return true; // retain the validated ROOT snapshot if the optional peer sample changes
		memcpy(result.faultPeer119Status,peerStatus,sizeof(peerStatus));
		result.faultPeer119Pc[0]=peerPc0;
		result.faultPeer119Pc[1]=peerPc1;
		result.faultPeer119Valid=1;
		return true;
	}

	enum : unsigned { WorkPeerFaultMarker=0x46503851 };

	static bool PeerFaultHandlerPc(uint32 pc)
	{
		return PioneerQwen35::WorkPeerFaultHandlerPc(pc);
	}

	static bool PeerWaitPc(uint32 pc)
	{
		return pc==TT_Q38_PEER_WAIT_LOAD || pc==TT_Q38_PEER_WAIT_BRANCH;
	}

	bool PeerStatusSnapshot(unsigned worker,uint32 first[16],uint32 second[16])
	{
		if (!fIO.SelectRuntimeCore(worker)) return false;
		for (unsigned i=0;i<16;i++) first[i]=fIO.Q38PanelRead(0x74140+i*4);
		fIO.Pause();
		for (unsigned i=0;i<16;i++) second[i]=fIO.Q38PanelRead(0x74140+i*4);
		for (unsigned i=0;i<16;i++) if (first[i]!=second[i]) return false;
		return true;
	}

	bool PeerPcSnapshot(uint32& first,uint32& second)
	{
		fIO.Pause(); first=fIO.RuntimePcRead();
		fIO.Pause(); second=fIO.RuntimePcRead();
		return true;
	}

	unsigned PeerRecoveryStatusFailure(unsigned worker,const uint32 status[16],
		unsigned recorded,unsigned root,bool faultHandler,unsigned* detail)
	{
		if (worker==0 || worker>=120) { if (detail) *detail=worker; return 14; }
		return PioneerQwen35::WorkPeerRecoveryStatusFailure(status,recorded,
			fDispatch.peerNonce[worker-1],root,TT_Q38_PEER_MARKER,WorkPeerFaultMarker,
			0x72000,sizeof(PioneerQwen35::WorkPeerWords),faultHandler,detail);
	}

	bool WorkFaultSafe(unsigned reportedPc,unsigned* reason=NULL,unsigned* detail=NULL)
	{
		using namespace PioneerQwen35;
		if (reason) *reason=0;
		if (detail) *detail=0;
		auto reject=[&](uint32 why,uint32 value) {
			if (reason) *reason=why;
			if (detail) *detail=value;
			return false;
		};
		auto terminalPc=[](uint32 pc) { return pc==WorkFaultPc || pc==WorkFaultPc+4; };
		if (!terminalPc(reportedPc)) return reject(1,reportedPc);
		uint32 flags=0;
		if (!fDispatch.booted) flags|=1;
		if (fDispatch.closed) flags|=2;
		if (fDispatch.unsafe) flags|=4;
		if (fDispatch.rootRunning) flags|=8;
		if (fDispatch.prepared!=120) flags|=16;
		if (PioneerQ38DispatchUnsafe(fDispatch)) flags|=32;
		if (flags) return reject(2,flags);
		if (!fIO.SelectRuntimeCore(0)) return reject(3,0);
		if (!Scope(0x46800)) return reject(4,fIO.ResetRead());
		uint32 quietDetail=0;
		fIO.Pause(); const uint32 firstPc=fIO.RuntimePcRead();
		fIO.Pause(); const uint32 secondPc=fIO.RuntimePcRead();
		if (!terminalPc(firstPc) || !terminalPc(secondPc)) return reject(6,secondPc);
		const uint32 marker=Read(RootCompletionAddress);
		if (marker!=RootFault) return reject(7,marker);
		uint32 state[121];
		for (unsigned i=0;i<121;i++) state[i]=Read(RootDispatchStateAddress+i*4);
		if (state[0]!=RootDispatchStateMagic) return reject(8,state[0]);
		if (state[1]<fDispatch.rootNonce || state[1]>=0x7ffffffe) return reject(9,state[1]);
		for (unsigned worker=0;worker<119;worker++) {
			const uint32 recorded=state[2+worker];
			if (recorded<fDispatch.peerNonce[worker] || recorded>=0x7ffffffe
				|| recorded>state[1]) return reject(10,worker);
		}
		const uint64_t began=fIO.Q38CohortNow();
		if (began>UINT64_MAX-UINT64_C(30000000)) return reject(11,0);
		const uint64_t deadline=began+UINT64_C(30000000);
		unsigned pendingWorker=0,pendingDetail=0;
		for (;;) {
			if (!fIO.SelectRuntimeCore(0)) return reject(12,0);
			if (!Scope(0x46800)) return reject(13,fIO.ResetRead());
			fIO.Pause(); const uint32 rootPc0=fIO.RuntimePcRead();
			fIO.Pause(); const uint32 rootPc1=fIO.RuntimePcRead();
			if (!terminalPc(rootPc0) || !terminalPc(rootPc1)) return reject(14,rootPc1);
			if (Read(RootCompletionAddress)!=RootFault) return reject(15,Read(RootCompletionAddress));
			quietDetail=0;
			if (!Quiet(&quietDetail)) {
				pendingWorker=0;
				pendingDetail=quietDetail;
			} else {
				bool allReady=true;
				for (unsigned worker=1;worker<120;worker++) {
					uint32 first[16],second[16];
					if (!PeerStatusSnapshot(worker,first,second)) {
						allReady=false; pendingWorker=worker; pendingDetail=16;
						continue;
					}
					uint32 peerPc0=0,peerPc1=0;
					PeerPcSnapshot(peerPc0,peerPc1);
					const bool handler0=PeerFaultHandlerPc(peerPc0),handler1=PeerFaultHandlerPc(peerPc1);
					const bool inFaultHandler=handler0 && handler1;
					unsigned statusDetail=0;
					const unsigned statusFailure=PeerRecoveryStatusFailure(worker,second,
						state[1+worker],state[1],inFaultHandler,&statusDetail);
					if (statusFailure)
						return reject(70+statusFailure,(worker<<24)|(statusDetail&0xffffff));
					const bool faulted=second[2]==WorkPeerFaultMarker;
					if (handler0!=handler1)
						return reject(22,(worker<<24)|(peerPc1&0xffffff));
					if (faulted || inFaultHandler) {
						if (!inFaultHandler)
							return reject(22,(worker<<24)|(peerPc1&0xffffff));
						if (faulted && WorkPeerFaultParkPc(peerPc0)
							&& WorkPeerFaultParkPc(peerPc1)) continue;
						allReady=false; pendingWorker=worker;
						pendingDetail=((second[0]&0xfff)<<12)|(second[1]&0xfff);
						continue;
					}
					auto executable=[](uint32 pc) {
						return pc>=0x72000 && pc<0x72000+sizeof(PioneerQwen35::WorkPeerWords)
							&& pc!=WorkPeerFaultEntry && pc!=WorkPeerFaultPark
							&& !PioneerQ38PeerTerminalPc(pc);
					};
					const bool waiting=PeerWaitPc(peerPc0) && PeerWaitPc(peerPc1);
					if (!executable(peerPc0) || !executable(peerPc1))
						return reject(22,(worker<<24)|(peerPc1&0xffffff));
					if (second[0]!=second[1] || second[4] || !waiting) {
						allReady=false; pendingWorker=worker;
						pendingDetail=((second[0]&0xfff)<<12)|(second[1]&0xfff);
					}
				}
				if (allReady) {
					for (unsigned worker=1;worker<120;worker++) {
						auto& node=fDispatch.worker[worker];
						if (!fIO.SelectRuntimeCore(worker)
							|| !PioneerQ38DispatchSources(fIO,fDram,fBanks,node,0x46800)
							|| !PioneerQ38DispatchCodeMatches(fIO,worker,&fDispatch))
							return reject(23,(worker<<24)|(fDispatch.closeDetail[0]&0xffffff));
						uint32 first[16],second[16],peerPc0=0,peerPc1=0;
						if (!PeerStatusSnapshot(worker,first,second)) return reject(23,worker);
						PeerPcSnapshot(peerPc0,peerPc1);
						const bool inFaultHandler=PeerFaultHandlerPc(peerPc0)&&PeerFaultHandlerPc(peerPc1);
						unsigned statusDetail=0;
						const unsigned statusFailure=PeerRecoveryStatusFailure(worker,second,
							state[1+worker],state[1],inFaultHandler,&statusDetail);
						if (statusFailure)
							return reject(70+statusFailure,(worker<<24)|(statusDetail&0xffffff));
						if (second[2]==WorkPeerFaultMarker) {
							if (!WorkPeerFaultParkPc(peerPc0) || !WorkPeerFaultParkPc(peerPc1))
								return reject(23,(worker<<24)|(peerPc1&0xffffff));
						} else if (!PioneerQ38DispatchPeerStatus(fIO,fDispatch.peerNonce[worker-1],false,&fDispatch))
							return reject(23,(worker<<24)|(fDispatch.closeDetail[0]&0xffffff));
					}
					if (!fIO.SelectRuntimeCore(0) || !Scope(0x46800)) return reject(24,fIO.ResetRead());
					quietDetail=0;
					if (!Quiet(&quietDetail)) return reject(25,quietDetail);
					fIO.Pause(); const uint32 finalPc=fIO.RuntimePcRead();
					if (!terminalPc(finalPc)) return reject(26,finalPc);
					if (Read(RootCompletionAddress)!=RootFault) return reject(27,Read(RootCompletionAddress));
					for (unsigned i=0;i<121;i++)
						if (Read(RootDispatchStateAddress+i*4)!=state[i]) return reject(28,i);
					return true;
				}
			}
			if (fIO.Q38CohortNow()>=deadline)
				return reject(29,(pendingWorker<<24)|(pendingDetail&0xffffff));
			fIO.Pause();
		}
	}

	bool RecoverWorkFault(unsigned* reason=NULL,unsigned* detail=NULL)
	{
		using namespace PioneerQwen35;
		if (reason) *reason=0;
		if (detail) *detail=0;
		auto reject=[&](uint32 why,uint32 value) {
			if (reason) *reason=why;
			if (detail) *detail=value;
			return false;
		};
		auto terminalPc=[](uint32 pc) { return pc==WorkFaultPc || pc==WorkFaultPc+4; };
		if (!WorkFaultSafe(WorkFaultPc,reason,detail)) return false;
		if (!fIO.SelectRuntimeCore(0)) return reject(31,0);
		if (!Scope(0x46800)) return reject(32,fIO.ResetRead());
		fIO.Pause(); const uint32 rootPc0=fIO.RuntimePcRead();
		fIO.Pause(); const uint32 rootPc1=fIO.RuntimePcRead();
		if (!terminalPc(rootPc0) || !terminalPc(rootPc1)) return reject(33,rootPc1);
		if (Read(RootCompletionAddress)!=RootFault) return reject(34,Read(RootCompletionAddress));
		uint32 quietDetail=0;
		if (!Quiet(&quietDetail)) return reject(35,quietDetail);
		uint32 rootState[121];
		for (unsigned i=0;i<121;i++) rootState[i]=Read(RootDispatchStateAddress+i*4);
		if (rootState[0]!=RootDispatchStateMagic || rootState[1]<fDispatch.rootNonce
			|| rootState[1]>=0x7ffffffe) return reject(36,rootState[1]);
		for (unsigned worker=1;worker<120;worker++) {
			const unsigned recorded=rootState[1+worker];
			if (recorded<fDispatch.peerNonce[worker-1] || recorded>=0x7ffffffe
				|| recorded>rootState[1]) return reject(37,worker);
		}
		// Audit the complete parked fleet again before the first reset. Normal
		// waiters and explicit Q8PF fault parks are the only restartable states.
		for (unsigned worker=1;worker<120;worker++) {
			auto& node=fDispatch.worker[worker];
			if (!fIO.SelectRuntimeCore(worker)
				|| !PioneerQ38DispatchSources(fIO,fDram,fBanks,node,0x46800)
				|| !PioneerQ38DispatchCodeMatches(fIO,worker,&fDispatch)) return reject(38,worker);
			uint32 first[16],second[16],peerPc0=0,peerPc1=0;
			if (!PeerStatusSnapshot(worker,first,second)) return reject(39,worker);
			PeerPcSnapshot(peerPc0,peerPc1);
			const bool inFaultHandler=PeerFaultHandlerPc(peerPc0)&&PeerFaultHandlerPc(peerPc1);
			unsigned statusDetail=0;
			const unsigned statusFailure=PeerRecoveryStatusFailure(worker,second,
				rootState[1+worker],rootState[1],inFaultHandler,&statusDetail);
			if (statusFailure)
				return reject(110+statusFailure,(worker<<24)|(statusDetail&0xffffff));
			if (PeerFaultHandlerPc(peerPc0)!=PeerFaultHandlerPc(peerPc1))
				return reject(41,(worker<<24)|(peerPc1&0xffffff));
			if (second[2]==WorkPeerFaultMarker) {
				if (!inFaultHandler || !WorkPeerFaultParkPc(peerPc0)
					|| !WorkPeerFaultParkPc(peerPc1))
					return reject(42,(worker<<24)|(peerPc1&0xffffff));
			} else if (inFaultHandler || second[0]!=second[1] || second[4]
				|| !PeerWaitPc(peerPc0) || !PeerWaitPc(peerPc1)) return reject(43,worker);
			if (!Quiet(&quietDetail)) return reject(44,(worker<<24)|(quietDetail&0xffffff));
		}
		if (!fIO.SelectRuntimeCore(0) || !Scope(0x46800)
			|| Read(RootCompletionAddress)!=RootFault || !Quiet(&quietDetail)) return reject(45,quietDetail);
		for (unsigned i=0;i<121;i++)
			if (Read(RootDispatchStateAddress+i*4)!=rootState[i]) return reject(46,i);
		// Root and every peer are now terminal or idle, every role/source is
		// sealed, and all NoC queues are quiet. Hold ROOT, then restart the peer
		// fleet through the same boot contract used by DispatchBootstrap.
		fIO.RuntimeResetWrite(0x87ffff); fIO.Pause();
		if (!Scope(0x87ffff) || !Quiet(&quietDetail)) return reject(47,fIO.ResetRead());
		fIO.RuntimeResetWrite(0x47800); fIO.Pause();
		if (!Scope(0x47800) || !Quiet(&quietDetail)) return reject(48,fIO.ResetRead());
		for (unsigned i=0;i<121;i++)
			if (Read(RootDispatchStateAddress+i*4)!=rootState[i]) return reject(49,i);
		uint32 zero[16]={},sentinel[16];
		for (unsigned& value:sentinel) value=0xdeadbeef;
		for (unsigned worker=1;worker<120;worker++) {
			auto& node=fDispatch.worker[worker];
			if (!fIO.SelectRuntimeCore(worker)
				|| !PioneerQ38DispatchSources(fIO,fDram,fBanks,node,0x46800)
				|| !PioneerQ38DispatchCodeMatches(fIO,worker,&fDispatch)) return reject(50,worker);
			fIO.RuntimeResetWrite(0x87ffff); fIO.Pause();
			if (!PioneerQ38DispatchSources(fIO,fDram,fBanks,node,0x87ffff)
				|| !PioneerQ38DispatchCodeMatches(fIO,worker,&fDispatch)) return reject(51,worker);
		}
		for (unsigned worker=1;worker<120;worker++) {
			auto& node=fDispatch.worker[worker];
			if (!fIO.SelectRuntimeCore(worker) || fIO.ResetRead()!=0x87ffff
				|| !PioneerQ38DispatchSources(fIO,fDram,fBanks,node,0x87ffff)) return reject(52,worker);
			const uint32 boot[]={TT_Q38_PEER_MARKER,worker,
				PioneerWorkerX(worker)|(PioneerWorkerY(worker)<<6),node.Store().config|0x10000,0,0,0,0};
			if (!PioneerQ38PanelStage(fIO,0x74040,boot,8)
				|| !PioneerQ38PanelStage(fIO,0x74100,zero,16)
				|| !PioneerQ38PanelStage(fIO,0x74140,sentinel,16)) return reject(53,worker);
			const uint32 command[]={0,0,0,0,0,boot[2],0,0,0,0};
			if (!BlackholeHostWriteCommand(fIO,command)) return reject(54,worker);
			for (unsigned c=0;c<8;c++) fDispatch.bootCounter[worker][c]=
				fIO.ResidentNocRead(PioneerQ38PanelCounter(c));
			node.Store().l1_dirty=true; node.Store().l1_restored=false;
		}
		for (unsigned worker=1;worker<120;worker++) {
			auto& node=fDispatch.worker[worker];
			if (!fIO.SelectRuntimeCore(worker) || fIO.ResetRead()!=0x87ffff
				|| !PioneerQ38DispatchCodeMatches(fIO,worker,&fDispatch)) return reject(55,worker);
			node.image.running=true;
			fIO.RuntimeResetWrite(0x46800);
			if (fIO.ResetRead()!=0x46800
				|| !PioneerQ38DispatchSources(fIO,fDram,fBanks,node,0x46800)) return reject(56,worker);
		}
		bool ready[119]={}; unsigned finished=0;
		const uint64_t now=fIO.Q38CohortNow();
		if (now>UINT64_MAX-UINT64_C(30000000)) return reject(57,0);
		const uint64_t deadline=now+UINT64_C(30000000);
		while (finished<119) {
			if (fIO.Q38CohortNow()>=deadline) return reject(58,finished);
			for (unsigned worker=1;worker<120;worker++) if (!ready[worker-1]) {
				auto& node=fDispatch.worker[worker];
				if (!fIO.SelectRuntimeCore(worker)
					|| !PioneerQ38DispatchSources(fIO,fDram,fBanks,node,0x46800)) return reject(59,worker);
				uint32 first[16],second[16];
				for (unsigned i=0;i<16;i++) first[i]=fIO.Q38PanelRead(0x74140+i*4);
				fIO.Pause();
				for (unsigned i=0;i<16;i++) second[i]=fIO.Q38PanelRead(0x74140+i*4);
				bool changed=false;
				for (unsigned i=0;i<16;i++) changed|=first[i]!=second[i];
				if (changed) continue;
				if (second[2]==WorkPeerFaultMarker) return reject(60,(worker<<24)|(second[6]&0xffffff));
				if (second[3]!=TT_Q38_PEER_MARKER) {
					if (second[3] && second[3]!=0xdeadbeef) return reject(61,(worker<<24)|(second[3]&0xffffff));
					for (unsigned i=0;i<16;i++) if (second[i] && second[i]!=0xdeadbeef)
						return reject(62,(worker<<24)|(i<<16)|(second[i]&0xffff));
					continue;
				}
				if (!WorkPeerFreshStatus(second,TT_Q38_PEER_MARKER)) return reject(63,worker);
				uint32 peerPc0=0,peerPc1=0;
				PeerPcSnapshot(peerPc0,peerPc1);
				if (!PeerWaitPc(peerPc0) || !PeerWaitPc(peerPc1)) {
					if (PeerFaultHandlerPc(peerPc0) || PeerFaultHandlerPc(peerPc1))
						return reject(64,(worker<<24)|(peerPc1&0xffffff));
					continue;
				}
				if (!PioneerQ38DispatchCodeMatches(fIO,worker,&fDispatch)) return reject(65,worker);
				for (unsigned c=0;c<8;c++) if (fIO.ResidentNocRead(PioneerQ38PanelCounter(c))
					!=fDispatch.bootCounter[worker][c]) return reject(66,(worker<<24)|c);
				ready[worker-1]=true; finished++;
			}
			fIO.Pause();
		}
		for (unsigned worker=1;worker<120;worker++) {
			auto& node=fDispatch.worker[worker];
			if (!fIO.SelectRuntimeCore(worker)
				|| !PioneerQ38DispatchSources(fIO,fDram,fBanks,node,0x46800)
				|| !PioneerQ38DispatchCodeMatches(fIO,worker,&fDispatch)) return reject(67,worker);
			uint32 first[16],second[16],peerPc0=0,peerPc1=0;
			if (!PeerStatusSnapshot(worker,first,second)
				|| !WorkPeerFreshStatus(second,TT_Q38_PEER_MARKER)) return reject(68,worker);
			PeerPcSnapshot(peerPc0,peerPc1);
			if (!PeerWaitPc(peerPc0) || !PeerWaitPc(peerPc1)) return reject(69,(worker<<24)|(peerPc1&0xffffff));
			for (unsigned c=0;c<8;c++) if (fIO.ResidentNocRead(PioneerQ38PanelCounter(c))
				!=fDispatch.bootCounter[worker][c]) return reject(70,(worker<<24)|c);
		}
		if (!fIO.SelectRuntimeCore(0) || !Scope(0x47800)
			|| Read(RootCompletionAddress)!=RootFault || !Quiet(&quietDetail)) return reject(71,quietDetail);
		for (unsigned i=0;i<121;i++)
			if (Read(RootDispatchStateAddress+i*4)!=rootState[i]) return reject(72,i);
		fDispatch.rootNonce=rootState[1];
		for (unsigned worker=0;worker<119;worker++) fDispatch.peerNonce[worker]=0;
		if (!RootHeld()) return reject(73,0);
		return true;
	}

	uint32 Read(unsigned address)
	{
		using namespace PioneerQwen35;
		if (address==RootBootAddress) return fIO.RuntimeCodeRead(0);
		if (address>=RootCheckpointAddress && address<RootCompletionAddress)
			return fIO.ResidentL1Read((address-(RootCompletionAddress-0x1fc0))/4);
		if (address>=RootCompletionAddress) return fIO.RuntimeStatusRead((address-RootCompletionAddress)/4);
		return PioneerRead32(fL1+(address-RootTextAddress)/4);
	}

	void Write(unsigned address,uint32 value)
	{
		using namespace PioneerQwen35;
		if (address==RootBootAddress) fIO.RuntimeCodeWrite(0,value);
		else if (address>=RootCheckpointAddress && address<RootCompletionAddress)
			fIO.ResidentL1Write((address-(RootCompletionAddress-0x1fc0))/4,value);
		else if (address>=RootCompletionAddress) fIO.RuntimeStatusWrite((address-RootCompletionAddress)/4,value);
		else PioneerWrite32(fL1+(address-RootTextAddress)/4,value);
	}

	uint32 Command(unsigned word) { return fIO.ResidentNocRead(word*4); }
	void WriteCommand(unsigned word,uint32 value) { fIO.ResidentNocWrite(word*4,value); }
	uint32 Pc() { return fIO.RuntimePcRead(); }
	void Reset(unsigned value) { fIO.RuntimeResetWrite(value); }
	void Pause() { fIO.Pause(); }
	unsigned long long Now() { return static_cast<unsigned long long>(system_time()); }
};
#endif
