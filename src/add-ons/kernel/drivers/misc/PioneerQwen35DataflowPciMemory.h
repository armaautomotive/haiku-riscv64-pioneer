/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_PCI_MEMORY_H
#define PIONEER_QWEN35_DATAFLOW_PCI_MEMORY_H

#include <stdint.h>
#include <stdlib.h>

#include "../../../../../tools/pioneer/PioneerQwen35DataflowServiceOwner.h"

enum : unsigned {
	DataflowPciL1ViewBase=0x6e000,
	DataflowPciL1ViewBytes=0x42000,
	DataflowPciL1ViewWords=DataflowPciL1ViewBytes/sizeof(uint32_t)
};

struct DataflowPciCoreSnapshot {
	unsigned core=UINT32_MAX;
	unsigned reset=0;
	unsigned boot=0;
	uint32_t* localWords=NULL;
	bool captured=false;
};

static inline bool DataflowPciL1Range(unsigned address,unsigned bytes)
{
	if (!bytes || (address&3) || (bytes&3)) return false;
	const unsigned end=address+bytes;
	if (end<address) return false;
	return (address>=PioneerQwen35::DataflowServiceEntryAddress
			&& end<=PioneerQwen35::DataflowServiceTextLimit)
		|| (address>=PioneerQwen35::DataflowQueueStateAddress
			&& end<=PioneerQwen35::DataflowQueueStateAddress
				+PioneerQwen35::DataflowQueueStateBytes)
		|| (address>=PioneerQwen35::DataflowProgramAddress
			&& end<=PioneerQwen35::DataflowProgramRegionEnd)
		|| (address>=PioneerQwen35::DataflowWorkerMailboxAddress
			&& end<=PioneerQwen35::DataflowWorkerMailboxAddress
				+PioneerQwen35::DataflowWorkerMailboxBytes);
}

// Fixed local-memory transport for the dataflow service. Raw must bind a
// selected physical core to the full 0x42000 L1 view and implement
// DataflowCoreIdleForHold(core) from the current worker/reset/NoC proof. The
// service owner performs that proof for every core before it calls HoldCore.
// No address or routing value comes from a userspace request.
template<class Raw>
class PioneerQwen35DataflowPciMemory {
	Raw& fRaw;

	bool SelectMapped(unsigned core)
	{
		return core<PioneerQwen35::DataflowCoreCount
			&& fRaw.SelectRuntimeCore(core)
			&& fRaw.DataflowPanelMappingStable();
	}

	bool ReadLocal(unsigned address,uint32_t& value)
	{
		value=fRaw.Q38PanelRead(address);
		return true; // every caller has already proved the fixed mapping/range
	}

public:
	explicit PioneerQwen35DataflowPciMemory(Raw& raw):fRaw(raw) {}
	PioneerQwen35DataflowPciMemory(const PioneerQwen35DataflowPciMemory&)=delete;
	PioneerQwen35DataflowPciMemory& operator=(
		const PioneerQwen35DataflowPciMemory&)=delete;

	static bool RangeAllowed(unsigned address,unsigned bytes)
	{
		return (bytes==sizeof(uint32_t)
			&& address==PioneerQwen35::DataflowServiceBootAddress)
			|| DataflowPciL1Range(address,bytes);
	}

	bool CoreHeld(unsigned core)
	{
		return SelectMapped(core) && fRaw.ResetRead()==0x47800
			&& fRaw.ResetRead()==0x47800;
	}

	bool HoldCore(unsigned core)
	{
		if (!SelectMapped(core) || !fRaw.DataflowCoreIdleForHold(core)) return false;
		const unsigned reset=fRaw.ResetRead();
		if (reset!=0x46800 && reset!=0x47800) return false;
		if (reset==0x46800) {
			fRaw.RuntimeResetWrite(0x47800);
			fRaw.Pause();
		}
		return fRaw.ResetRead()==0x47800 && fRaw.ResetRead()==0x47800;
	}

	bool StartCore(unsigned core)
	{
		if (!CoreHeld(core)) return false;
		fRaw.RuntimeResetWrite(0x46800);
		fRaw.Pause();
		return fRaw.ResetRead()==0x46800 && fRaw.ResetRead()==0x46800;
	}

	bool ReadCoreMemory(unsigned core,unsigned address,void* output,unsigned bytes)
	{
		if (!output || !RangeAllowed(address,bytes) || !SelectMapped(core)) return false;
		if (address==PioneerQwen35::DataflowServiceBootAddress) {
			if (bytes!=sizeof(uint32_t)) return false;
			*static_cast<uint32_t*>(output)=fRaw.RuntimeCodeRead(0);
			return true;
		}
		uint32_t* words=static_cast<uint32_t*>(output);
		for (unsigned i=0;i<bytes/sizeof(uint32_t);i++)
			ReadLocal(address+i*sizeof(uint32_t),words[i]);
		return true;
	}

	bool WriteCoreMemory(unsigned core,unsigned address,const void* input,unsigned bytes)
	{
		if (!input || !RangeAllowed(address,bytes) || !CoreHeld(core)) return false;
		if (address==PioneerQwen35::DataflowServiceBootAddress) {
			if (bytes!=sizeof(uint32_t)) return false;
			const uint32_t value=*static_cast<const uint32_t*>(input);
			if (value!=PioneerQwen35::DataflowServiceBootWord) return false;
			fRaw.RuntimeCodeWrite(0,value);
			return fRaw.RuntimeCodeRead(0)==value;
		}
		const uint32_t* words=static_cast<const uint32_t*>(input);
		for (unsigned i=0;i<bytes/sizeof(uint32_t);i++) {
			const unsigned current=address+i*sizeof(uint32_t);
			fRaw.Q38PanelWrite(current,words[i]);
			if (fRaw.Q38PanelRead(current)!=words[i] || !CoreHeld(core)) return false;
		}
		return true;
	}

	bool CaptureCoreState(unsigned core,DataflowPciCoreSnapshot& snapshot)
	{
		if (snapshot.captured || snapshot.localWords || !SelectMapped(core)
			|| !fRaw.DataflowCoreIdleForHold(core)) return false;
		const unsigned reset=fRaw.ResetRead();
		if (reset!=0x46800 && reset!=0x47800) return false;
		uint32_t* words=static_cast<uint32_t*>(malloc(DataflowPciL1ViewBytes));
		if (!words) return false;
		const unsigned boot=fRaw.RuntimeCodeRead(0);
		for (unsigned i=0;i<DataflowPciL1ViewWords;i++)
			ReadLocal(DataflowPciL1ViewBase+i*sizeof(uint32_t),words[i]);
		fRaw.Pause();
		bool stable=fRaw.ResetRead()==reset && fRaw.ResetRead()==reset
			&& fRaw.RuntimeCodeRead(0)==boot;
		for (unsigned i=0;stable && i<DataflowPciL1ViewWords;i++) {
			uint32_t observed=0;
			ReadLocal(DataflowPciL1ViewBase+i*sizeof(uint32_t),observed);
			stable=observed==words[i];
		}
		if (!stable) {
			free(words);
			return false;
		}
		snapshot.core=core;
		snapshot.reset=reset;
		snapshot.boot=boot;
		snapshot.localWords=words;
		snapshot.captured=true;
		return true;
	}

	bool RestoreCoreState(unsigned core,const DataflowPciCoreSnapshot& snapshot)
	{
		if (!snapshot.captured || snapshot.core!=core || !snapshot.localWords
			|| (snapshot.reset!=0x46800 && snapshot.reset!=0x47800)
			|| !CoreHeld(core)) return false;
		for (unsigned i=0;i<DataflowPciL1ViewWords;i++) {
			const unsigned address=DataflowPciL1ViewBase+i*sizeof(uint32_t);
			fRaw.Q38PanelWrite(address,snapshot.localWords[i]);
			if (fRaw.Q38PanelRead(address)!=snapshot.localWords[i]
				|| !CoreHeld(core)) return false;
		}
		fRaw.RuntimeCodeWrite(0,snapshot.boot);
		return fRaw.RuntimeCodeRead(0)==snapshot.boot && CoreHeld(core);
	}

	bool VerifyCoreStateRestored(unsigned core,
		const DataflowPciCoreSnapshot& snapshot)
	{
		if (!snapshot.captured || snapshot.core!=core || !snapshot.localWords
			|| !CoreHeld(core) || fRaw.RuntimeCodeRead(0)!=snapshot.boot) return false;
		for (unsigned i=0;i<DataflowPciL1ViewWords;i++) {
			uint32_t observed=0;
			ReadLocal(DataflowPciL1ViewBase+i*sizeof(uint32_t),observed);
			if (observed!=snapshot.localWords[i]) return false;
		}
		return fRaw.ResetRead()==0x47800;
	}

	static void ReleaseCoreSnapshot(DataflowPciCoreSnapshot& snapshot)
	{
		free(snapshot.localWords);
		snapshot={};
	}
};

#endif
