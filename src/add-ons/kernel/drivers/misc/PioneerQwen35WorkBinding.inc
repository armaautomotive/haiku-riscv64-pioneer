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
					if (!fIO.SelectRuntimeCore(worker)) return reject(16,worker);
					uint32 first[16],second[16];
					for (unsigned i=0;i<16;i++) first[i]=fIO.Q38PanelRead(0x74140+i*4);
					fIO.Pause();
					for (unsigned i=0;i<16;i++) second[i]=fIO.Q38PanelRead(0x74140+i*4);
					unsigned changed=16;
					for (unsigned i=0;i<16;i++) if (first[i]!=second[i]) { changed=i; break; }
					if (changed!=16) {
						allReady=false; pendingWorker=worker; pendingDetail=changed;
						continue;
					}
					if (second[2]) return reject(17,(worker<<24)|(second[2]&0xffffff));
					if (second[3]!=TT_Q38_PEER_MARKER) return reject(18,(worker<<24)|(second[3]&0xffffff));
					if (second[4]>1) return reject(19,(worker<<24)|(second[4]&0xffffff));
					for (unsigned i=5;i<16;i++) if (second[i])
						return reject(20,(worker<<24)|(i<<16)|(second[i]&0xffff));
					if (!WorkPeerStatusSafe(state[1+worker],second[0],second[1],
						fDispatch.peerNonce[worker-1],state[1]))
						return reject(21,(worker<<24)|((second[0]&0xfff)<<12)|(second[1]&0xfff));
					fIO.Pause(); const uint32 peerPc0=fIO.RuntimePcRead();
					fIO.Pause(); const uint32 peerPc1=fIO.RuntimePcRead();
					const bool waiting=(peerPc0==TT_Q38_PEER_WAIT_LOAD || peerPc0==TT_Q38_PEER_WAIT_BRANCH)
						&& (peerPc1==TT_Q38_PEER_WAIT_LOAD || peerPc1==TT_Q38_PEER_WAIT_BRANCH);
					auto executable=[](uint32 pc) {
						return pc>=0x72000 && pc<0x72000+sizeof(PioneerQwen35::WorkPeerWords)
							&& pc!=TT_Q38_PEER_FAULT_ENTRY && pc!=TT_Q38_PEER_FAULT_PARK
							&& !PioneerQ38PeerTerminalPc(pc);
					};
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
							|| !PioneerQ38DispatchCodeMatches(fIO,worker,&fDispatch)
							|| !PioneerQ38DispatchPeerStatus(fIO,fIO.Q38PanelRead(0x74140),false,&fDispatch))
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
		const uint32 rootPc=fIO.RuntimePcRead();
		if (!terminalPc(rootPc)) return reject(33,rootPc);
		const uint32 marker=Read(RootCompletionAddress);
		if (marker!=RootFault) return reject(34,marker);
		uint32 quietDetail=0;
		if (!Quiet(&quietDetail)) return reject(35,quietDetail);
		uint32 rootNonce=Read(RootDispatchStateAddress+4),peerNonce[119];
		for (unsigned worker=1;worker<120;worker++) {
			if (!fIO.SelectRuntimeCore(worker)) return reject(36,worker);
			peerNonce[worker-1]=fIO.Q38PanelRead(0x74140);
		}
		if (!fIO.SelectRuntimeCore(0)) return reject(37,0);
		if (!Scope(0x46800)) return reject(38,fIO.ResetRead());
		if (!Quiet(&quietDetail)) return reject(39,quietDetail);
		if (Read(RootCompletionAddress)!=RootFault) return reject(40,Read(RootCompletionAddress));
		fIO.RuntimeResetWrite(0x87ffff);
		fIO.Pause();
		if (!Scope(0x87ffff)) return reject(41,fIO.ResetRead());
		if (!Quiet(&quietDetail)) return reject(42,quietDetail);
		fIO.RuntimeResetWrite(0x47800);
		fIO.Pause();
		if (!Scope(0x47800)) return reject(43,fIO.ResetRead());
		if (!Quiet(&quietDetail)) return reject(44,quietDetail);
		for (unsigned worker=1;worker<120;worker++) {
			auto& node=fDispatch.worker[worker];
			if (!fIO.SelectRuntimeCore(worker)) return reject(45,worker);
			if (!PioneerQ38DispatchSources(fIO,fDram,fBanks,node,0x46800)) return reject(46,worker);
			if (!PioneerQ38DispatchCodeMatches(fIO,worker,&fDispatch)) return reject(47,worker);
			if (!Quiet(&quietDetail)) return reject(48,(worker<<24)|(quietDetail&0xffffff));
			if (!PioneerQ38DispatchPeerStatus(fIO,peerNonce[worker-1],false,&fDispatch)) return reject(49,worker);
		}
		if (!fIO.SelectRuntimeCore(0)) return reject(50,0);
		if (!Scope(0x47800)) return reject(51,fIO.ResetRead());
		fDispatch.rootNonce=rootNonce;
		for (unsigned worker=0;worker<119;worker++) fDispatch.peerNonce[worker]=peerNonce[worker];
		if (!RootHeld()) return reject(52,0);
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
