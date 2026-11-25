/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_PCI_MEMORY_H
#define PIONEER_QWEN35_DATAFLOW_PCI_MEMORY_H

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>

#include "../../../../../tools/pioneer/PioneerQwen35DataflowServiceOwner.h"
#ifdef PIONEER_QWEN35_DEVICE_DISPATCH
#include "../../../../../tools/pioneer/PioneerQwen35DataflowGraphRunProtocol.h"
#include "../../../../../tools/pioneer/PioneerQwen35DataflowGraphTrace.h"
#endif
#ifdef PIONEER_QWEN35_RESIDENT_SERVICE_WITNESS
#include "../../../../../tools/pioneer/PioneerQwen35DataflowResidentSenderReceipt.h"
#endif

enum : unsigned {
	DataflowPciL1ViewBase=0x6e000,
	DataflowPciL1ViewBytes=0x42000,
	DataflowPciL1ViewWords=DataflowPciL1ViewBytes/sizeof(uint32_t),
	DataflowPciResumeSettlePauses=10000,
#ifdef PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS
	DataflowPciRunningReset=0x6000
#else
	DataflowPciRunningReset=0x46800
#endif
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
#ifdef PIONEER_QWEN35_DEVICE_DISPATCH
		|| (address>=PioneerQwen35::DataflowServiceDispatchAddress
			&& end<=PioneerQwen35::DataflowServiceDispatchLimit)
		|| (address>=PioneerQwen35::DataflowGraphRunAddress
			&& end<=PioneerQwen35::DataflowGraphRunAddress+PioneerQwen35::DataflowGraphRunBytes)
#ifdef PIONEER_QWEN35_RESIDENT_SERVICE_WITNESS
		|| (address>=PioneerQwen35::DataflowResidentSenderReceiptAddress
			&& end<=PioneerQwen35::DataflowResidentSenderReceiptAddress+sizeof(PioneerQwen35::DataflowResidentSenderReceipt))
#endif
		|| (address==PioneerQwen35::DataflowGraphProgressAddress
			&& bytes==sizeof(uint32_t))
		|| (address==PioneerQwen35::DataflowGraphOperationProgressAddress
			&& bytes==sizeof(uint32_t))
		|| (address>=PioneerQwen35::DataflowGraphTraceAddress
			&& end<=PioneerQwen35::DataflowGraphTraceAddress
				+PioneerQwen35::DataflowGraphTraceSlots*sizeof(PioneerQwen35::DataflowGraphTraceRecord))
#endif
		|| (address>=PioneerQwen35::DataflowQueueStateAddress
			&& end<=PioneerQwen35::DataflowQueueStateAddress
				+PioneerQwen35::DataflowQueueStateBytes)
		|| (address>=PioneerQwen35::DataflowProgramAddress
			&& end<=PioneerQwen35::DataflowProgramRegionEnd)
		|| (address>=PioneerQwen35::DataflowWorkerMailboxAddress
			&& end<=PioneerQwen35::DataflowWorkerMailboxAddress
				+PioneerQwen35::DataflowWorkerMailboxBytes)
#ifdef PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS
		|| (address>=PioneerQwen35::DataflowRoleMailboxAddress
			&& end<=PioneerQwen35::DataflowRoleMailboxAddress
				+sizeof(PioneerQwen35::DataflowRoleMailbox))
		|| (address>=PioneerQwen35::DataflowRoleSnapshotHandoffAddress
			&& end<=PioneerQwen35::DataflowRoleSnapshotHandoffAddress
				+sizeof(PioneerQwen35::DataflowRoleSnapshotHandoff))
#endif
		|| (address>=PioneerQwen35::DataflowBarrierOutbox
			&& end<=PioneerQwen35::DataflowBarrierReleaseCommit
				+PioneerQwen35::DataflowBarrierPacketBytes)
#ifdef PIONEER_QWEN35_ACTIVATION_SHARING_WITNESS
		|| (address>=PioneerQwen35::DataflowActivationShareReadyAddress
			&& end<=PioneerQwen35::DataflowActivationShareScratchAddress
				+PioneerQwen35::DataflowActivationSharePacketBytes)
#endif
		|| (address>=PioneerQwen35::DataflowBarrierArrivalBase
			&& end<=PioneerQwen35::DataflowBarrierArrivalEnd);
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
	unsigned invocationBytes=sizeof(PioneerQwen35::DataflowCoreInvocation);
	if (invocationBytes<sizeof(PioneerQwen35::DataflowEmbeddingInvocation))
		invocationBytes=sizeof(PioneerQwen35::DataflowEmbeddingInvocation);
	if (invocationBytes<sizeof(PioneerQwen35::DataflowRmsInvocation))
		invocationBytes=sizeof(PioneerQwen35::DataflowRmsInvocation);
	return (address>=PioneerQwen35::DataflowProgramAddress
			&& end<=PioneerQwen35::DataflowProgramAddress
				+invocationBytes)
#ifdef PIONEER_QWEN35_DEVICE_DISPATCH
		|| (address>=PioneerQwen35::DataflowGraphRunAddress
			&& end<=PioneerQwen35::DataflowGraphRunAddress+PioneerQwen35::DataflowGraphRunBytes)
#endif
		|| (address==command && bytes==sizeof(uint32_t))
		|| (address==acknowledgement && bytes==sizeof(uint32_t))
		|| (address==shutdown && bytes==sizeof(uint32_t))
#if defined(PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS) && !defined(PIONEER_QWEN35_EXCLUSIVE_TAKEOVER_WITNESS)
		|| (address==PioneerQwen35::DataflowRoleSnapshotHandoffAddress
			+offsetof(PioneerQwen35::DataflowRoleSnapshotHandoff,release)
			&& bytes==sizeof(uint32_t))
#endif
		;
}

static inline bool DataflowPciMailboxReadyForStop(
	const PioneerQwen35::DataflowWorkerMailbox& mailbox,unsigned expectedCore)
{
	if (mailbox.magic!=PioneerQwen35::DataflowWorkerMailboxMagic
		|| mailbox.version!=PioneerQwen35::DataflowWorkerMailboxVersion
		|| mailbox.commandSequence!=mailbox.acknowledgedSequence
		|| mailbox.acknowledgedSequence!=mailbox.completedSequence
		|| mailbox.state!=PioneerQwen35::DataflowWorkerReady || mailbox.failure
		|| mailbox.phase
		|| mailbox.logicalCore!=expectedCore
		|| (mailbox.shutdownSequence
			&& mailbox.shutdownSequence!=PioneerQwen35::DataflowWorkerShutdownRequest))
		return false;
	for (unsigned i=0;i<6;i++) if (mailbox.reserved[i]) return false;
	return true;
}

// Startup requires sequence zero; an idle resident worker after an
// acknowledged command keeps its nonzero sequence. Verify the stable
// mailbox against the separately sampled ready snapshot instead.
static inline bool DataflowPciMailboxMatchesReadySnapshot(
	const PioneerQwen35::DataflowWorkerMailbox& mailbox,unsigned core,
	const PioneerQwen35::DataflowWorkerSnapshot& snapshot)
{
	return DataflowPciMailboxReadyForStop(mailbox,core) && !mailbox.shutdownSequence
		&& PioneerQwen35::DataflowWorkerSnapshotReady(snapshot)
		&& snapshot.commandSequence==mailbox.commandSequence
		&& snapshot.acknowledgedSequence==mailbox.acknowledgedSequence
		&& snapshot.completedSequence==mailbox.completedSequence
		&& snapshot.state==mailbox.state && snapshot.failure==mailbox.failure
		&& snapshot.phase==mailbox.phase;
}

// Only the worker's two pre-command fatal exits may be recovered by holding
// the selected core directly. The host assigned logicalCore before start;
// phase 2 may store an invalid raw local route in reserved[0] for diagnostics.
static inline bool DataflowPciMailboxStartupFatal(
	const PioneerQwen35::DataflowWorkerMailbox& mailbox,unsigned expectedCore)
{
	if (mailbox.magic!=PioneerQwen35::DataflowWorkerMailboxMagic
		|| mailbox.version!=PioneerQwen35::DataflowWorkerMailboxVersion
		|| mailbox.commandSequence || mailbox.acknowledgedSequence
		|| mailbox.completedSequence || mailbox.shutdownSequence
		|| mailbox.state!=PioneerQwen35::DataflowWorkerFatal
		|| mailbox.logicalCore!=expectedCore
		|| mailbox.failure!=PioneerQwen35::DataflowFailureQueue
		|| (mailbox.phase!=1 && mailbox.phase!=2)) return false;
	for (unsigned i=0;i<6;i++) {
		if (i==0 && mailbox.phase==2) continue;
		if (mailbox.reserved[i]) return false;
	}
	return true;
}

static inline bool DataflowPciMailboxStopped(
	const PioneerQwen35::DataflowWorkerMailbox& mailbox,unsigned expectedCore)
{
	if (mailbox.magic!=PioneerQwen35::DataflowWorkerMailboxMagic
		|| mailbox.version!=PioneerQwen35::DataflowWorkerMailboxVersion
		|| mailbox.shutdownSequence!=PioneerQwen35::DataflowWorkerShutdownRequest
		|| mailbox.commandSequence!=mailbox.acknowledgedSequence
		|| mailbox.acknowledgedSequence!=mailbox.completedSequence
		|| mailbox.state!=PioneerQwen35::DataflowWorkerStopped || mailbox.failure
		|| mailbox.logicalCore!=expectedCore
		|| mailbox.phase) return false;
	for (unsigned i=0;i<6;i++) if (mailbox.reserved[i]) return false;
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
	bool fResumeStartAttempted=false;
	bool fResumeFleetStarted=false;
	bool fUnconfirmedHeldWrite=false;
#ifdef PIONEER_QWEN35_FABRIC_GUARD_WITNESS
	bool fFabricParentRestoreAttempted[PioneerQwen35::DataflowCoreCount]{};
	bool fFabricParentRestoreVerified[PioneerQwen35::DataflowCoreCount]{};
#endif
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
	// A successful later store cannot prove a previously unconfirmed held
	// store has drained. This ledger covers only WriteCoreMemory's held path.
	bool HeldWritesConfirmed() const { return !fUnconfirmedHeldWrite; }
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

	bool HoldCoreAfterStartupFatal(unsigned core)
	{
		if (!SelectMapped(core) || !fRaw.DataflowCoreStartupFatalIdle(core)
			|| fRaw.ResetRead()!=0x46800 || fRaw.ResetRead()!=0x46800)
			return false;
		fRaw.RuntimeResetWrite(0x47800);
		fRaw.Pause();
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
		return WriteCoreMemoryInternal(core,address,input,bytes,nullptr);
	}

#ifdef PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS
	// The one-shot ledger changes immediately before the actual MMIO store,
	// after all software preflight checks. A failure before that point is
	// distinguishable from an issued write with uncertain completion.
	bool WriteStackReleaseTracked(unsigned core,bool& issued)
	{
		if (issued) return false;
		const uint32_t value=PioneerQwen35::DataflowRoleSnapshotRelease;
		return WriteCoreMemoryInternal(core,PioneerQwen35::DataflowRoleSnapshotHandoffAddress
			+offsetof(PioneerQwen35::DataflowRoleSnapshotHandoff,release),&value,sizeof(value),&issued);
	}
#endif

private:
	bool WriteCoreMemoryInternal(unsigned core,unsigned address,const void* input,unsigned bytes,bool* issued)
	{
		if (!input || !RangeAllowed(address,bytes) || !SelectMapped(core)) return false;
		const bool held=CoreHeld(core);
		if (issued && held) return false; // tracked handoff is only for a running tile
		if (!held && (fRaw.ResetRead()!=DataflowPciRunningReset || fRaw.ResetRead()!=DataflowPciRunningReset
			|| !DataflowPciRunningWriteRange(address,bytes)
			|| !fRaw.DataflowCoreIdleForWrite(core,address,bytes))) return false;
		// A held-core write can replace code, even when the restored boot word
		// equals the dataflow boot word. Invalidate before the first write so
		// a partial write cannot leave a stale trusted-image proof.
		if (held && !fRaw.DataflowInvalidateServiceImage(core)) return false;
		if (address==PioneerQwen35::DataflowServiceBootAddress) {
			if (bytes!=sizeof(uint32_t)) return false;
			const uint32_t value=*static_cast<const uint32_t*>(input);
			if (value!=PioneerQwen35::DataflowServiceBootWord) return false;
			const bool previous=fUnconfirmedHeldWrite;
			if (held) { fUnconfirmedHeldWrite=true;asm volatile("" ::: "memory"); }
			fRaw.RuntimeCodeWrite(0,value);
			if (fRaw.RuntimeCodeRead(0)!=value || (held && !CoreHeld(core))) return false;
			if (held) fUnconfirmedHeldWrite=previous;
			return true;
		}
		const uint32_t* words=static_cast<const uint32_t*>(input);
		for (unsigned i=0;i<bytes/sizeof(uint32_t);i++) {
			const unsigned current=address+i*sizeof(uint32_t);
			if (issued) {
				*issued=true;
				asm volatile("" ::: "memory");
			}
			const bool previous=fUnconfirmedHeldWrite;
			if (held) { fUnconfirmedHeldWrite=true;asm volatile("" ::: "memory"); }
			fRaw.Q38PanelWrite(current,words[i]);
			if (fRaw.Q38PanelRead(current)!=words[i]
				|| !fRaw.DataflowPanelMappingStable()
				|| (held && !CoreHeld(core))
				|| (!held && (fRaw.ResetRead()!=DataflowPciRunningReset
					|| fRaw.ResetRead()!=DataflowPciRunningReset))) return false;
			if (held) fUnconfirmedHeldWrite=previous;
		}
		return true;
	}

public:

#ifdef PIONEER_QWEN35_DEVICE_DISPATCH
	// Once the first descriptor has been verified, a warm graph only changes
	// the trigger, poll budget, ticket, token/position and checksum. An audited
	// previous record may be reused only while exclusive ownership prevents
	// intervening L1 writers; otherwise read the entire record. Even with an
	// audited record, compare every mutable device word before the first write.
	// The worker validates the complete descriptor and checksum before use.
	bool UpdateGraphRun(unsigned core,unsigned previous,
		const PioneerQwen35::DataflowGraphRunDescriptor& next,
		const PioneerQwen35::DataflowGraphRunDescriptor* audited=nullptr)
	{
		using namespace PioneerQwen35;
		if (!previous || !DataflowGraphRunValid(next,core)
			|| next.triggerSequence!=previous+1 || !SelectMapped(core)
			|| fRaw.ResetRead()!=DataflowPciRunningReset || fRaw.ResetRead()!=DataflowPciRunningReset
			|| !fRaw.DataflowCoreIdleForWrite(core,DataflowGraphRunAddress,
				DataflowGraphRunBytes)) return false;
		DataflowGraphRunDescriptor old{};
		if (audited) old=*audited;
		else if (!ReadCoreMemory(core,DataflowGraphRunAddress,&old,sizeof(old))) return false;
		if (!DataflowGraphRunValid(old,core) || old.triggerSequence!=previous)
			return false;
		constexpr unsigned Words=DataflowGraphRunBytes/sizeof(uint32_t);
		auto wordAt=[](const DataflowGraphRunDescriptor& record,unsigned word) {
			uint32_t value=0;
			memcpy(&value,reinterpret_cast<const uint8_t*>(&record)+word*sizeof(value),sizeof(value));
			return value;
		};
		auto mutableWord=[](unsigned byte) {
			return byte==offsetof(DataflowGraphRunDescriptor,triggerSequence)
				|| byte==offsetof(DataflowGraphRunDescriptor,pollLimit)
				|| (byte>=offsetof(DataflowGraphRunDescriptor,ticket)
					&& byte<offsetof(DataflowGraphRunDescriptor,ticket)+sizeof(DataflowGraphRunTicket))
				|| byte==offsetof(DataflowGraphRunDescriptor,token)
				|| byte==offsetof(DataflowGraphRunDescriptor,token)+sizeof(uint32_t)
				|| byte==offsetof(DataflowGraphRunDescriptor,checksum);
		};
		for (unsigned word=0;word<Words;word++)
			if (wordAt(old,word)!=wordAt(next,word) && !mutableWord(word*sizeof(uint32_t))) return false;
		if (audited) {
			for (unsigned word=0;word<Words;word++) {
				if (!mutableWord(word*sizeof(uint32_t))) continue;
				if (fRaw.Q38PanelRead(DataflowGraphRunAddress+word*sizeof(uint32_t))!=wordAt(old,word)
					|| !fRaw.DataflowPanelMappingStable()) return false;
			}
		}
		for (unsigned word=0;word<Words;word++) {
			const uint32_t value=wordAt(next,word);
			if (wordAt(old,word)==value) continue;
			const unsigned address=DataflowGraphRunAddress+word*sizeof(uint32_t);
			fRaw.Q38PanelWrite(address,value);
			if (fRaw.Q38PanelRead(address)!=value
				|| !fRaw.DataflowPanelMappingStable()) return false;
		}
		return fRaw.ResetRead()==DataflowPciRunningReset && fRaw.ResetRead()==DataflowPciRunningReset;
	}
#endif

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
		if (core<PioneerQwen35::DataflowCoreCount) {
			ReleaseCoreSnapshot(fSnapshots[core]);
			bool any=false;
			for (const auto& snapshot:fSnapshots) any |= snapshot.captured;
			if (!any) fResumeStartAttempted=fResumeFleetStarted=false;
		}
	}

	bool RestoreCoreState(unsigned core,const DataflowPciCoreSnapshot& snapshot)
	{
		if (!snapshot.captured || snapshot.core!=core || !snapshot.localWords
			|| (snapshot.reset!=0x46800 && snapshot.reset!=0x47800)
			|| !CoreHeld(core)) return false;
		if (!fRaw.DataflowInvalidateServiceImage(core)) return false;
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

#ifdef PIONEER_QWEN35_FABRIC_GUARD_WITNESS
	// Restore the old owner's exact retained snapshot through a separately
	// owned fresh transport. Check is metadata/authority only and must bind
	// both owners, ticket and lifetime exclusion; FreshMemory must guard each
	// physical access. A failed write may have landed, so this is one-shot.
	bool FabricParentSnapshotPresent(unsigned core) const {
		return core<PioneerQwen35::DataflowCoreCount && fSnapshots[core].captured
			&& fSnapshots[core].core==core && fSnapshots[core].localWords;
	}
	bool FabricParentSnapshotAbsent(unsigned core) const {
		return core<PioneerQwen35::DataflowCoreCount && !fSnapshots[core].captured
			&& !fSnapshots[core].localWords && fSnapshots[core].core==UINT32_MAX
			&& !fSnapshots[core].reset && !fSnapshots[core].boot;
	}
	template<class FreshMemory,class Check>
	bool FabricRestoreParentCore(unsigned core,FreshMemory& fresh,Check& check) {
		if (!FabricParentSnapshotPresent(core) || fFabricParentRestoreAttempted[core]
			|| !check(core,fSnapshots[core]) || !fresh.FabricRecoveryAllowed()) return false;
		fFabricParentRestoreAttempted[core]=true;
		if (!fresh.RestoreCoreState(core,fSnapshots[core]) || !fresh.FabricRecoveryAllowed()
			|| !check(core,fSnapshots[core]) || !fresh.FabricRecoveryAllowed()) return false;
		fFabricParentRestoreVerified[core]=true;return true;
	}
	template<class FreshMemory,class Check>
	bool FabricVerifyParentCore(unsigned core,FreshMemory& fresh,Check& check) {
		return FabricParentSnapshotPresent(core) && fFabricParentRestoreAttempted[core]
			&& fFabricParentRestoreVerified[core] && check(core,fSnapshots[core])
			&& fresh.FabricRecoveryAllowed() && fresh.VerifyCoreStateRestored(core,fSnapshots[core])
			&& fresh.FabricRecoveryAllowed() && check(core,fSnapshots[core]) && fresh.FabricRecoveryAllowed();
	}
	bool FabricRecoveryAllowed() { return fRaw.FabricRecoveryAllowed(); }
#endif

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
	bool StopAndAuditAll(unsigned& failedStage,unsigned& failedCore)
	{
		failedStage=0;
		failedCore=PioneerQwen35::DataflowCoreCount;
		auto fail=[&](unsigned stage,unsigned core) {
			failedStage=stage;
			failedCore=core;
			return false;
		};
		bool held[PioneerQwen35::DataflowCoreCount]{};
		bool service[PioneerQwen35::DataflowCoreCount]{};
		bool startupFatal[PioneerQwen35::DataflowCoreCount]{};
		bool requestStop[PioneerQwen35::DataflowCoreCount]{};
		for (unsigned core=0;core<PioneerQwen35::DataflowCoreCount;core++) {
			if (!SelectMapped(core)) return fail(1,core);
			const unsigned reset=fRaw.ResetRead();
			if (reset!=fRaw.ResetRead()) return fail(1,core);
			if (reset==0x47800) {
				held[core]=true;
				continue;
			}
			if (reset!=0x46800) return fail(1,core);
			service[core]=fRaw.DataflowCoreServiceImageMatches(core);
			const bool idle=fRaw.DataflowCoreIdleForHold(core);
			if (!idle) {
				if (!service[core]
					|| !fRaw.DataflowCoreStartupFatalIdle(core))
					return fail(1,core);
				startupFatal[core]=true;
			}
			if (!service[core]) continue;
			PioneerQwen35::DataflowWorkerMailbox mailbox{};
			if (!ReadStableMailbox(core,mailbox)) return fail(2,core);
			if (DataflowPciMailboxStopped(mailbox,core)) {
				if (!QueuesDrained(core)) return fail(2,core);
				continue;
			}
			if (startupFatal[core]) {
				if (!DataflowPciMailboxStartupFatal(mailbox,core)
					|| !QueuesDrained(core)) return fail(2,core);
				continue;
			}
			if (!DataflowPciMailboxReadyForStop(mailbox,core)) return fail(2,core);
			requestStop[core]=mailbox.state==PioneerQwen35::DataflowWorkerReady
				&& mailbox.shutdownSequence==0;
		}

		for (unsigned core=0;core<PioneerQwen35::DataflowCoreCount;core++) {
			if (!requestStop[core]) continue;
			failedStage=3;
			failedCore=core;
			const uint32_t stop=PioneerQwen35::DataflowWorkerShutdownRequest;
			const unsigned address=PioneerQwen35::DataflowWorkerMailboxAddress
				+offsetof(PioneerQwen35::DataflowWorkerMailbox,shutdownSequence);
			if (!WriteCoreMemory(core,address,&stop,sizeof(stop))) return fail(3,core);
		}

		for (unsigned core=0;core<PioneerQwen35::DataflowCoreCount;core++) {
			if (!service[core] || held[core] || startupFatal[core]) continue;
			failedStage=4;
			failedCore=core;
			bool stopped=false;
			for (unsigned poll=0;poll<StopPollLimit;poll++) {
				uint32_t state=0;
				const unsigned address=PioneerQwen35::DataflowWorkerMailboxAddress
					+offsetof(PioneerQwen35::DataflowWorkerMailbox,state);
				if (!ReadCoreMemory(core,address,&state,sizeof(state)))
					return fail(4,core);
				if (state==PioneerQwen35::DataflowWorkerStopped) {
					stopped=true;
					break;
				}
				if (state==PioneerQwen35::DataflowWorkerFatal
					|| state==PioneerQwen35::DataflowWorkerFailed)
					return fail(4,core);
				fRaw.Pause();
			}
			if (!stopped) return fail(4,core);
			PioneerQwen35::DataflowWorkerMailbox mailbox{};
			if (!ReadStableMailbox(core,mailbox)
				|| !DataflowPciMailboxStopped(mailbox,core) || !QueuesDrained(core)
				|| !fRaw.DataflowCoreIdleForHold(core)) return fail(4,core);
		}

		// Prove all participants parked before holding any running core.
		for (unsigned core=0;core<PioneerQwen35::DataflowCoreCount;core++) {
			if (held[core]) continue;
			failedStage=5;
			failedCore=core;
			if (!SelectMapped(core) || fRaw.ResetRead()!=0x46800
				|| fRaw.ResetRead()!=0x46800) return fail(5,core);
			if (startupFatal[core]) {
				if (!fRaw.DataflowCoreStartupFatalIdle(core))
					return fail(5,core);
			} else if (!fRaw.DataflowCoreIdleForHold(core))
				return fail(5,core);
		}
		for (unsigned core=0;core<PioneerQwen35::DataflowCoreCount;core++) {
			if (held[core]) continue;
			failedStage=6;
			failedCore=core;
			if (startupFatal[core]) {
				if (!HoldCoreAfterStartupFatal(core)) return fail(6,core);
				held[core]=true;
			} else if (!HoldCore(core) || !CoreHeld(core))
				return fail(6,core);
		}
		for (unsigned core=0;core<PioneerQwen35::DataflowCoreCount;core++) {
			failedStage=7;
			failedCore=core;
			if (!CoreHeld(core)) return fail(7,core);
			if (service[core] && !held[core]) {
				PioneerQwen35::DataflowWorkerMailbox mailbox{};
				if (!ReadStableMailbox(core,mailbox)
					|| !DataflowPciMailboxStopped(mailbox,core) || !QueuesDrained(core))
					return fail(7,core);
			}
		}
		failedStage=0;
		failedCore=PioneerQwen35::DataflowCoreCount;
		return true;
	}

	bool ResumeRestoredFleet(unsigned& failedStage,unsigned& failedCore)
	{
		unsigned failedDetail=0;
		return ResumeRestoredFleet(failedStage,failedCore,failedDetail);
	}

	bool ResumeRestoredFleet(unsigned& failedStage,unsigned& failedCore,
		unsigned& failedDetail)
	{
		failedStage=0;
		failedCore=PioneerQwen35::DataflowCoreCount;
		failedDetail=0;
		if (!fResumeFleetStarted) {
			// A partial start is not safe to repeat. A failure in the final
			// read-only fleet audit, however, can be audited again.
			if (fResumeStartAttempted) return false;
			for (unsigned core=0;core<PioneerQwen35::DataflowCoreCount;core++) {
				failedStage=1;
				failedCore=core;
				if (!fSnapshots[core].captured
					|| !VerifyCoreStateRestored(core,fSnapshots[core])) return false;
			}
			fResumeStartAttempted=true;
			for (unsigned core=0;core<PioneerQwen35::DataflowCoreCount;core++) {
				failedStage=2;
				failedCore=core;
				if (fSnapshots[core].reset==0x46800 && !StartCore(core)) return false;
			}
			fResumeFleetStarted=true;
		}
		// Let resumed dispatcher harts and their idle NoC state settle before
		// the final all-core proof. In fleet mode Pause is a bounded 1 us spin.
		for (unsigned pause=0;pause<DataflowPciResumeSettlePauses;pause++)
			fRaw.Pause();
		for (unsigned core=0;core<PioneerQwen35::DataflowCoreCount;core++) {
			failedStage=3;
			failedCore=core;
			if (!CoreResetMatches(core,fSnapshots[core].reset)) {
				failedDetail=PioneerQwen35::DataflowServiceResumeResetMismatch;
				return false;
			}
			// The original dispatcher root is restored to its captured held
			// reset state (0x47800). Its complete local image, boot word, and
			// reset were already verified above; the running-idle proof applies
			// to resumed dispatcher workers, not to this intentionally held core.
			if (fSnapshots[core].reset==0x47800) continue;
			if (!fRaw.DataflowCoreIdleForHold(core,failedDetail)) {
				if (!failedDetail)
					failedDetail=PioneerQwen35::DataflowServiceResumeIdleProofFailed;
				return false;
			}
		}
		failedStage=0;
		failedCore=PioneerQwen35::DataflowCoreCount;
		return true;
	}

	static void ReleaseCoreSnapshot(DataflowPciCoreSnapshot& snapshot)
	{
		free(snapshot.localWords);
		snapshot={};
	}
};

#endif
