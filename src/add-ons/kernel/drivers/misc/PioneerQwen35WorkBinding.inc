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

	bool Quiet()
	{
		for (unsigned i=0; i<4; i++)
			if (fIO.ResidentNocRead(0x40+i*0x800)) return false;
		for (unsigned i=0; i<32; i++)
			if (fIO.ResidentNocRead(0x240+i*4)&255) return false;
		return true;
	}

	bool WorkFaultSafe(unsigned reportedPc)
	{
		using namespace PioneerQwen35;
		auto terminalPc=[](uint32 pc) { return pc==WorkFaultPc || pc==WorkFaultPc+4; };
		if (!terminalPc(reportedPc) || !fDispatch.booted || fDispatch.closed || fDispatch.unsafe
			|| fDispatch.rootRunning || fDispatch.prepared!=120 || PioneerQ38DispatchUnsafe(fDispatch)
			|| !fIO.SelectRuntimeCore(0) || !Scope(0x46800) || !Quiet()) return false;
		fIO.Pause(); const uint32 firstPc=fIO.RuntimePcRead();
		fIO.Pause(); const uint32 secondPc=fIO.RuntimePcRead();
		if (!terminalPc(firstPc) || !terminalPc(secondPc) || Read(RootCompletionAddress)!=RootFault)
			return false;
		uint32 state[121];
		for (unsigned i=0;i<121;i++) state[i]=Read(RootDispatchStateAddress+i*4);
		if (state[0]!=RootDispatchStateMagic || state[1]<fDispatch.rootNonce
			|| state[1]>=0x7ffffffe) return false;
		for (unsigned worker=0;worker<119;worker++) {
			const uint32 recorded=state[2+worker];
			if (recorded<fDispatch.peerNonce[worker] || recorded>=0x7ffffffe
				|| recorded>state[1]) return false;
		}
		for (unsigned worker=1;worker<120;worker++) {
			auto& node=fDispatch.worker[worker];
			if (!fIO.SelectRuntimeCore(worker)
				|| !PioneerQ38DispatchSources(fIO,fDram,fBanks,node,0x46800)
				|| !PioneerQ38DispatchCodeMatches(fIO,worker,&fDispatch) || !Quiet()) return false;
			const uint32 actual=fIO.Q38PanelRead(0x74140);
			if (fIO.Q38PanelRead(0x74144)!=actual || actual<fDispatch.peerNonce[worker-1]
				|| actual<state[1+worker] || actual>state[1+worker]+1
				|| actual>=0x7ffffffe || actual>state[1]+1
				|| !PioneerQ38DispatchPeerStatus(fIO,actual,false,&fDispatch)) return false;
		}
		if (!fIO.SelectRuntimeCore(0) || !Scope(0x46800) || !Quiet()
			|| !terminalPc(fIO.RuntimePcRead()) || Read(RootCompletionAddress)!=RootFault) return false;
		for (unsigned i=0;i<121;i++)
			if (Read(RootDispatchStateAddress+i*4)!=state[i]) return false;
		return true;
	}

	bool RecoverWorkFault()
	{
		using namespace PioneerQwen35;
		auto terminalPc=[](uint32 pc) { return pc==WorkFaultPc || pc==WorkFaultPc+4; };
		if (!WorkFaultSafe(WorkFaultPc) || !fIO.SelectRuntimeCore(0) || !Scope(0x46800)
			|| !terminalPc(fIO.RuntimePcRead()) || Read(RootCompletionAddress)!=RootFault || !Quiet()) return false;
		uint32 rootNonce=Read(RootDispatchStateAddress+4),peerNonce[119];
		for (unsigned worker=1;worker<120;worker++) {
			if (!fIO.SelectRuntimeCore(worker)) return false;
			peerNonce[worker-1]=fIO.Q38PanelRead(0x74140);
		}
		if (!fIO.SelectRuntimeCore(0) || !Scope(0x46800) || !Quiet()
			|| Read(RootCompletionAddress)!=RootFault) return false;
		fIO.RuntimeResetWrite(0x87ffff);
		fIO.Pause();
		if (!Scope(0x87ffff) || !Quiet()) return false;
		fIO.RuntimeResetWrite(0x47800);
		fIO.Pause();
		if (!Scope(0x47800) || !Quiet()) return false;
		for (unsigned worker=1;worker<120;worker++) {
			auto& node=fDispatch.worker[worker];
			if (!fIO.SelectRuntimeCore(worker)
				|| !PioneerQ38DispatchSources(fIO,fDram,fBanks,node,0x46800)
				|| !PioneerQ38DispatchCodeMatches(fIO,worker,&fDispatch)
				|| !Quiet()
				|| !PioneerQ38DispatchPeerStatus(fIO,peerNonce[worker-1],false,&fDispatch)) return false;
		}
		if (!fIO.SelectRuntimeCore(0) || !Scope(0x47800)) return false;
		fDispatch.rootNonce=rootNonce;
		for (unsigned worker=0;worker<119;worker++) fDispatch.peerNonce[worker]=peerNonce[worker];
		return RootHeld();
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
