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
	area_id fArea=-1;
	volatile uint32* fL1=NULL;
	uint32 fTlb[2][3]={};

public:
	PioneerQwen35::WorkOwner owner;

	PioneerQ35WorkBinding(PioneerSessionIO& io,const tt_dram_samples_result& dram,
		const tt_compute_noc_result& noc,const PioneerResidentLease& lease,
		const PioneerResidentBanks& banks)
		: fIO(io),fDram(dram),fNoc(noc),fLease(lease),fBanks(banks)
	{
	}

	~PioneerQ35WorkBinding()
	{
		if (fArea>=B_OK) delete_area(fArea);
	}

	bool Map()
	{
		using namespace PioneerQwen35;
		if (fL1 || fArea>=B_OK || !fIO.runtimeMapped || fIO.runtimeCore || fIO.fleetEnabled
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
		if (!fL1 || fArea<B_OK || !fIO.runtimeMapped || fIO.runtimeCore || fIO.fleetEnabled
			|| !fLease.ready || fLease.pending || fLease.recovery_required || !fBanks.Matches(fDram))
			return false;
		for (unsigned tlb=0; tlb<2; tlb++)
			for (unsigned word=0; word<3; word++)
				if (fIO.DramTlbRead(tlb,word)!=fTlb[tlb][word]) return false;
		return fIO.ResetRead()==reset && fIO.ResetRead()==reset
			&& PioneerRuntimeSources(fIO,fDram,fNoc,fLease);
	}

	bool AllHeld()
	{
		tt_dram_samples_result samples={};
		return BlackholeDramAllCoresHeld(fIO,samples) && samples.cores_checked==120
			&& fIO.SelectRuntimeCore(0) && Scope(0x47800);
	}

	bool Quiet()
	{
		for (unsigned i=0; i<4; i++)
			if (fIO.ResidentNocRead(0x40+i*0x800)) return false;
		for (unsigned i=0; i<32; i++)
			if (fIO.ResidentNocRead(0x240+i*4)&255) return false;
		return true;
	}

	uint32 Read(unsigned address)
	{
		using namespace PioneerQwen35;
		if (address==RootBootAddress) return fIO.RuntimeCodeRead(0);
		if (address>=RootCompletionAddress) return fIO.RuntimeStatusRead((address-RootCompletionAddress)/4);
		return PioneerRead32(fL1+(address-RootTextAddress)/4);
	}

	void Write(unsigned address,uint32 value)
	{
		using namespace PioneerQwen35;
		if (address==RootBootAddress) fIO.RuntimeCodeWrite(0,value);
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
