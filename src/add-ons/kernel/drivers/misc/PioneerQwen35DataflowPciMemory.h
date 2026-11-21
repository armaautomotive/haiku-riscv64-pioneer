/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_PCI_MEMORY_H
#define PIONEER_QWEN35_DATAFLOW_PCI_MEMORY_H

#include <stdint.h>
#include <stddef.h>
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

static inline bool DataflowPciRunningWriteRange(unsigned address,unsigned bytes)
{
	if (!bytes || (address&3) || (bytes&3)) return false;
	const unsigned end=address+bytes;
	if (end<address) return false;
	const unsigned command=PioneerQwen35::DataflowWorkerMailboxAddress
		+offsetof(PioneerQwen35::DataflowWorkerMailbox,commandSequence);
	const unsigned acknowledgement=PioneerQwen35::DataflowWorkerMailboxAddress
		+offsetof(PioneerQwen35::DataflowWorkerMailbox,acknowledgedSequence);
	const unsigned shutdown=PioneerQwen35::DataflowWorkerMailboxAddress
		+offsetof(PioneerQwen35::DataflowWorkerMailbox,shutdownSequence);
	const unsigned invocationBytes=sizeof(PioneerQwen35::DataflowCoreInvocation)
		>sizeof(PioneerQwen35::DataflowEmbeddingInvocation)
		?sizeof(PioneerQwen35::DataflowCoreInvocation)
		:sizeof(PioneerQwen35::DataflowEmbeddingInvocation);
	return (address>=PioneerQwen35::DataflowProgramAddress
			&& end<=PioneerQwen35::DataflowProgramAddress
				+invocationBytes)
		|| (address==command && bytes==sizeof(uint32_t))
		|| (address==acknowledgement && bytes==sizeof(uint32_t))
		|| (address==shutdown && bytes==sizeof(uint32_t));
}

static inline bool DataflowPciMailboxReadyForStop(
	const PioneerQwen35::DataflowWorkerMailbox& mailbox)
{
	if (mailbox.magic!=PioneerQwen35::DataflowWorkerMailboxMagic
		|| mailbox.version!=PioneerQwen35::DataflowWorkerMailboxVersion
		|| mailbox.commandSequence!=mailbox.acknowledgedSequence
		|| mailbox.acknowledgedSequence!=mailbox.completedSequence
		|| mailbox.state!=PioneerQwen35::DataflowWorkerReady || mailbox.failure
		|| mailbox.phase
		|| (mailbox.shutdownSequence
			&& mailbox.shutdownSequence!=PioneerQwen35::DataflowWorkerShutdownRequest))
		return false;
	for (unsigned i=0;i<7;i++) if (mailbox.reserved[i]) return false;
	return true;
}

static inline bool DataflowPciMailboxStopped(
	const PioneerQwen35::DataflowWorkerMailbox& mailbox)
{
	if (mailbox.magic!=PioneerQwen35::DataflowWorkerMailboxMagic
		|| mailbox.version!=PioneerQwen35::DataflowWorkerMailboxVersion
		|| mailbox.shutdownSequence!=PioneerQwen35::DataflowWorkerShutdownRequest
		|| mailbox.commandSequence!=mailbox.acknowledgedSequence
		|| mailbox.acknowledgedSequence!=mailbox.completedSequence
		|| mailbox.state!=PioneerQwen35::DataflowWorkerStopped || mailbox.failure
		|| mailbox.phase) return false;
	for (unsigned i=0;i<7;i++) if (mailbox.reserved[i]) return false;
	return true;
}

// Fixed local-memory transport for the dataflow service. Raw must bind a
// selected physical core to the full 0x42000 L1 view and implement
// DataflowCoreServiceImageMatches(core) as a boot/text readback. Its
// DataflowCoreIdleForHold(core) must identify either the original dispatcher
// wait loop or that exact service image with stable status and idle NoC. For a
// running service, DataflowCoreIdleForWrite(core,address,bytes) must prove its
// wait/receipt state, immutable code, and idle NoC before invocation or the
// single-word command/acknowledgement/shutdown writes are admitted. The
// service owner performs that proof for every core before it calls HoldCore.
// No address or routing value comes from a userspace request.
template<class Raw>
class PioneerQwen35DataflowPciMemory {
	Raw& fRaw;
	DataflowPciCoreSnapshot fSnapshots[PioneerQwen35::DataflowCoreCount]{};
	static constexpr unsigned StopPollLimit=65536;

	bool SelectMapped(unsigned core)
	{
		return core<PioneerQwen35::DataflowCoreCount
			&& fRaw.SelectRuntimeCore(core)
			&& fRaw.DataflowPanelMappingStable();
	}

	bool ReadLocal(unsigned address,uint32_t& value)
	{
		if ((address&3) || address<DataflowPciL1ViewBase
			|| address>=DataflowPciL1ViewBase+DataflowPciL1ViewBytes
			|| !fRaw.DataflowPanelMappingStable()) return false;
		value=fRaw.Q38PanelRead(address);
		return true;
	}

	bool CoreResetMatches(unsigned core,unsigned expected)
	{
		return SelectMapped(core) && fRaw.ResetRead()==expected
			&& fRaw.ResetRead()==expected;
	}

	bool ReadStableMailbox(unsigned core,PioneerQwen35::DataflowWorkerMailbox& mailbox)
	{
		PioneerQwen35::DataflowWorkerMailbox first{},second{};
		if (!ReadCoreMemory(core,PioneerQwen35::DataflowWorkerMailboxAddress,
				&first,sizeof(first))) return false;
		fRaw.Pause();
		if (!ReadCoreMemory(core,PioneerQwen35::DataflowWorkerMailboxAddress,
				&second,sizeof(second))
			|| memcmp(&first,&second,sizeof(first))!=0) return false;
		mailbox=second;
		return true;
	}

	bool QueuesDrained(unsigned core)
	{
		PioneerQwen35::DataflowQueueControl queues[
			PioneerQwen35::DataflowCircularBufferCount]{};
		if (!ReadCoreMemory(core,PioneerQwen35::DataflowQueueStateAddress,
				queues,sizeof(queues))) return false;
		for (const auto& queue:queues) {
			if (queue.produced || queue.consumed || queue.readerFailure
				|| queue.computeFailure || queue.writerFailure || queue.queueFailure
				|| queue.reserved[0] || queue.reserved[1]) return false;
		}
		return true;
	}

public:
	explicit PioneerQwen35DataflowPciMemory(Raw& raw):fRaw(raw) {}
	~PioneerQwen35DataflowPciMemory()
	{
		for (auto& snapshot:fSnapshots) ReleaseCoreSnapshot(snapshot);
	}
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
			if (!ReadLocal(address+i*sizeof(uint32_t),words[i])) return false;
		return true;
	}

	bool WriteCoreMemory(unsigned core,unsigned address,const void* input,unsigned bytes)
	{
		if (!input || !RangeAllowed(address,bytes) || !SelectMapped(core)) return false;
		const bool held=CoreHeld(core);
		if (!held && (fRaw.ResetRead()!=0x46800 || fRaw.ResetRead()!=0x46800
			|| !DataflowPciRunningWriteRange(address,bytes)
			|| !fRaw.DataflowCoreIdleForWrite(core,address,bytes))) return false;
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
			if (fRaw.Q38PanelRead(current)!=words[i]
				|| !fRaw.DataflowPanelMappingStable()
				|| (held && !CoreHeld(core))) return false;
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
		for (unsigned i=0;i<DataflowPciL1ViewWords;i++) {
			if (!ReadLocal(DataflowPciL1ViewBase+i*sizeof(uint32_t),words[i])) {
				free(words);
				return false;
			}
		}
		fRaw.Pause();
		bool stable=fRaw.ResetRead()==reset && fRaw.ResetRead()==reset
			&& fRaw.RuntimeCodeRead(0)==boot;
		for (unsigned i=0;stable && i<DataflowPciL1ViewWords;i++) {
			uint32_t observed=0;
			stable=ReadLocal(DataflowPciL1ViewBase+i*sizeof(uint32_t),observed)
				&& observed==words[i];
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

	// These overloads match DataflowServiceOwner's per-core lifecycle contract.
	// A single owner instance retains every snapshot until the whole restored
	// fleet has resumed successfully.
	bool CaptureCoreState(unsigned core)
	{
		return core<PioneerQwen35::DataflowCoreCount
			&& CaptureCoreState(core,fSnapshots[core]);
	}

	void ReleaseCoreSnapshot(unsigned core)
	{
		if (core<PioneerQwen35::DataflowCoreCount)
			ReleaseCoreSnapshot(fSnapshots[core]);
	}

	bool RestoreCoreState(unsigned core,const DataflowPciCoreSnapshot& snapshot)
	{
		if (!snapshot.captured || snapshot.core!=core || !snapshot.localWords
			|| (snapshot.reset!=0x46800 && snapshot.reset!=0x47800)
			|| !CoreHeld(core)) return false;
		for (unsigned i=0;i<DataflowPciL1ViewWords;i++) {
			const unsigned address=DataflowPciL1ViewBase+i*sizeof(uint32_t);
			fRaw.Q38PanelWrite(address,snapshot.localWords[i]);
			uint32_t observed=0;
			if (!ReadLocal(address,observed) || observed!=snapshot.localWords[i]
				|| !CoreHeld(core)) return false;
		}
		fRaw.RuntimeCodeWrite(0,snapshot.boot);
		return fRaw.RuntimeCodeRead(0)==snapshot.boot && CoreHeld(core);
	}

	bool RestoreCoreState(unsigned core)
	{
		return core<PioneerQwen35::DataflowCoreCount
			&& RestoreCoreState(core,fSnapshots[core]);
	}

	bool VerifyCoreStateRestored(unsigned core,
		const DataflowPciCoreSnapshot& snapshot)
	{
		if (!snapshot.captured || snapshot.core!=core || !snapshot.localWords
			|| !CoreHeld(core) || fRaw.RuntimeCodeRead(0)!=snapshot.boot) return false;
		for (unsigned i=0;i<DataflowPciL1ViewWords;i++) {
			uint32_t observed=0;
			if (!ReadLocal(DataflowPciL1ViewBase+i*sizeof(uint32_t),observed)
				|| observed!=snapshot.localWords[i]) return false;
		}
		return fRaw.ResetRead()==0x47800;
	}

	bool VerifyCoreStateRestored(unsigned core)
	{
		return core<PioneerQwen35::DataflowCoreCount
			&& VerifyCoreStateRestored(core,fSnapshots[core]);
	}

	// Stops only recognized resident dataflow workers. A running original
	// dispatcher remains in its verified wait loop and is simply held after the
	// all-core audit. Cores already held after a partial install need no stop.
	bool StopAndAuditAll()
	{
		bool held[PioneerQwen35::DataflowCoreCount]{};
		bool service[PioneerQwen35::DataflowCoreCount]{};
		bool requestStop[PioneerQwen35::DataflowCoreCount]{};
		for (unsigned core=0;core<PioneerQwen35::DataflowCoreCount;core++) {
			if (!SelectMapped(core)) return false;
			const unsigned reset=fRaw.ResetRead();
			if (reset!=fRaw.ResetRead()) return false;
			if (reset==0x47800) {
				held[core]=true;
				continue;
			}
			if (reset!=0x46800 || !fRaw.DataflowCoreIdleForHold(core)) return false;
			service[core]=fRaw.DataflowCoreServiceImageMatches(core);
			if (!service[core]) continue;
			PioneerQwen35::DataflowWorkerMailbox mailbox{};
			if (!ReadStableMailbox(core,mailbox)) return false;
			if (DataflowPciMailboxStopped(mailbox)) {
				if (!QueuesDrained(core)) return false;
				continue;
			}
			if (!DataflowPciMailboxReadyForStop(mailbox)) return false;
			requestStop[core]=mailbox.state==PioneerQwen35::DataflowWorkerReady
				&& mailbox.shutdownSequence==0;
		}

		for (unsigned core=0;core<PioneerQwen35::DataflowCoreCount;core++) {
			if (!requestStop[core]) continue;
			const uint32_t stop=PioneerQwen35::DataflowWorkerShutdownRequest;
			const unsigned address=PioneerQwen35::DataflowWorkerMailboxAddress
				+offsetof(PioneerQwen35::DataflowWorkerMailbox,shutdownSequence);
			if (!WriteCoreMemory(core,address,&stop,sizeof(stop))) return false;
		}

		for (unsigned core=0;core<PioneerQwen35::DataflowCoreCount;core++) {
			if (!service[core] || held[core]) continue;
			bool stopped=false;
			for (unsigned poll=0;poll<StopPollLimit;poll++) {
				uint32_t state=0;
				const unsigned address=PioneerQwen35::DataflowWorkerMailboxAddress
					+offsetof(PioneerQwen35::DataflowWorkerMailbox,state);
				if (!ReadCoreMemory(core,address,&state,sizeof(state))) return false;
				if (state==PioneerQwen35::DataflowWorkerStopped) {
					stopped=true;
					break;
				}
				if (state==PioneerQwen35::DataflowWorkerFatal
					|| state==PioneerQwen35::DataflowWorkerFailed) return false;
				fRaw.Pause();
			}
			if (!stopped) return false;
			PioneerQwen35::DataflowWorkerMailbox mailbox{};
			if (!ReadStableMailbox(core,mailbox)
				|| !DataflowPciMailboxStopped(mailbox) || !QueuesDrained(core)
				|| !fRaw.DataflowCoreIdleForHold(core)) return false;
		}

		// Prove all participants parked before holding any running core.
		for (unsigned core=0;core<PioneerQwen35::DataflowCoreCount;core++) {
			if (held[core]) continue;
			if (!SelectMapped(core) || fRaw.ResetRead()!=0x46800
				|| fRaw.ResetRead()!=0x46800
				|| !fRaw.DataflowCoreIdleForHold(core)) return false;
		}
		for (unsigned core=0;core<PioneerQwen35::DataflowCoreCount;core++) {
			if (held[core]) continue;
			if (!HoldCore(core) || !CoreHeld(core)) return false;
		}
		for (unsigned core=0;core<PioneerQwen35::DataflowCoreCount;core++) {
			if (!CoreHeld(core)) return false;
			if (service[core] && !held[core]) {
				PioneerQwen35::DataflowWorkerMailbox mailbox{};
				if (!ReadStableMailbox(core,mailbox)
					|| !DataflowPciMailboxStopped(mailbox) || !QueuesDrained(core))
					return false;
			}
		}
		return true;
	}

	bool ResumeRestoredFleet()
	{
		for (unsigned core=0;core<PioneerQwen35::DataflowCoreCount;core++) {
			if (!fSnapshots[core].captured
				|| !VerifyCoreStateRestored(core,fSnapshots[core])) return false;
		}
		for (unsigned core=0;core<PioneerQwen35::DataflowCoreCount;core++) {
			if (fSnapshots[core].reset==0x46800 && !StartCore(core)) return false;
		}
		for (unsigned core=0;core<PioneerQwen35::DataflowCoreCount;core++) {
			if (!CoreResetMatches(core,fSnapshots[core].reset)
				|| !fRaw.DataflowCoreIdleForHold(core)) return false;
		}
		return true;
	}

	static void ReleaseCoreSnapshot(DataflowPciCoreSnapshot& snapshot)
	{
		free(snapshot.localWords);
		snapshot={};
	}
};

#endif
