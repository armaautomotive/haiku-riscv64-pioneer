/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_SERVICE_OWNER_H
#define PIONEER_QWEN35_DATAFLOW_SERVICE_OWNER_H

#if defined(PIONEER_QWEN35_EXCLUSIVE_TAKEOVER_WITNESS) && !defined(PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS)
#error Dedicated takeover requires the shared role owner and reset retirement path
#endif
#ifdef PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS
#if !defined(PIONEER_QWEN35_DEVICE_DISPATCH) || !defined(PIONEER_QWEN35_RECOVERY_COMPILE_ONLY)
#error Shared-role service lifecycle is a non-loadable recovery witness only
#endif
#include "PioneerQwen35DataflowRoleMailbox.h"
#endif

#include "PioneerQwen35DataflowOwner.h"
#include "PioneerQwen35DataflowKernels.h"
#ifdef PIONEER_QWEN35_DEVICE_DISPATCH
#include "PioneerQwen35DataflowBarrierSetup.h"
#include "PioneerQwen35DataflowGraphCompletion.h"
#include "PioneerQwen35DataflowGraphFaultAudit.h"
#endif
#include <stddef.h>
#include <string.h>

namespace PioneerQwen35 {

enum : unsigned {
	DataflowServiceBootAddress=0x6000,
	DataflowServiceEntryAddress=0x72000,
	DataflowServiceTextLimit=0x80000,
	DataflowServiceDispatchAddress=0xa0400,
	DataflowServiceDispatchLimit=DataflowProgramAddress,
	DataflowServiceBootWord=0x0006c06f,
	DataflowServiceChunkWords=64
};
static_assert(sizeof(DataflowQueueControl)*DataflowCircularBufferCount
	==DataflowQueueStateBytes,
	"resident startup clears exactly the reserved queue-control region");
static_assert(DataflowServiceDispatchAddress>=DataflowLocalBufferEnd+0x400
	&& unsigned(DataflowServiceDispatchLimit)==unsigned(DataflowProgramAddress),
	"dispatch code follows Tensix initialization and ends before invocations");

struct DataflowServiceImageView {
	unsigned bootWord;
	const uint32_t* textWords;
	unsigned textWordCount;
#ifdef PIONEER_QWEN35_DEVICE_DISPATCH
	const uint32_t* dispatchWords;
	unsigned dispatchWordCount;
#endif

	bool Valid() const
	{
		return bootWord==DataflowServiceBootWord && textWords && textWordCount
			&& textWordCount<=((DataflowServiceTextLimit-DataflowServiceEntryAddress)
				/sizeof(uint32_t))
#ifdef PIONEER_QWEN35_DEVICE_DISPATCH
			&& dispatchWords && dispatchWordCount
			&& dispatchWordCount<=((DataflowServiceDispatchLimit-DataflowServiceDispatchAddress)
				/sizeof(uint32_t))
#endif
			;
	}
};

inline bool DataflowServicePcInImage(unsigned pc,unsigned textWords,unsigned dispatchWords)
{
	if ((pc&3) || !textWords || textWords>(DataflowServiceTextLimit-DataflowServiceEntryAddress)/4
		|| dispatchWords>(DataflowServiceDispatchLimit-DataflowServiceDispatchAddress)/4) return false;
	return (pc>=DataflowServiceEntryAddress && pc-DataflowServiceEntryAddress<textWords*4)
		|| (dispatchWords && pc>=DataflowServiceDispatchAddress
			&& pc-DataflowServiceDispatchAddress<dispatchWords*4);
}

enum DataflowServiceResult {
	DataflowServiceComplete=0,
	DataflowServiceInvalidState=1,
	DataflowServiceInvalidImage=2,
	DataflowServicePreflightFailed=3,
	DataflowServiceSnapshotFailed=4,
	DataflowServiceHoldFailed=5,
	DataflowServiceImageWriteFailed=6,
	DataflowServiceImageReadbackFailed=7,
	DataflowServiceStartupWriteFailed=8,
	DataflowServiceStartupReadbackFailed=9,
	DataflowServiceStartFailed=10,
	DataflowServiceReadyFailed=11,
	DataflowServiceProjectionFailed=12,
	DataflowServiceRecoveryPending=13,
	DataflowServiceRecovered=14,
	DataflowServiceStopAuditFailed=15,
	DataflowServiceRestoreFailed=16,
	DataflowServiceClosed=17,
	DataflowServiceResumeFailed=18,
	DataflowServiceEmbeddingFailed=19,
	DataflowServiceRmsFailed=20,
	DataflowServiceSiluFailed=21,
	DataflowServiceResidualFailed=22,
	DataflowServiceFfnFailed=23,
	DataflowServiceConvFailed=24,
	DataflowServiceDeltaFailed=25,
	DataflowServiceRecurrentLayerFailed=26,
	DataflowServiceDenseAttentionFailed=27,
	DataflowServiceDenseLayerFailed=28,
	DataflowServiceDecoderSequenceFailed=29,
	DataflowServiceArgmaxFailed=30,
	DataflowServiceGenerationFailed=31,
	DataflowServiceResetRequired=32,
	DataflowServiceScratchHeld=33
};

enum DataflowServiceDiagnosticStage : unsigned {
	DataflowServiceDiagnosticNone=0,
	DataflowServiceDiagnosticWorkerStart=1,
	DataflowServiceDiagnosticWorkerWait=2,
	DataflowServiceDiagnosticWorkerSnapshot=3,
	DataflowServiceDiagnosticWorkerProof=4,
	DataflowServiceDiagnosticOwnerInitialize=5,
	DataflowServiceDiagnosticOwnerCore=6,
	DataflowServiceDiagnosticStopAudit=7,
	DataflowServiceDiagnosticRestore=8,
	DataflowServiceDiagnosticResume=9
};

enum DataflowServiceResumeDetail : unsigned {
	DataflowServiceResumeResetMismatch=1,
	DataflowServiceResumeIdleProofFailed=2
};

struct DataflowServiceDiagnostic {
	unsigned stage,core,state,failure,phase,detail;
};

// Owns the resident image lifecycle and the projection coordinator. IO must
// prove session idleness and audit reset, routes, worker status and NoC state.
// The snapshot lifecycle restores each core before resuming; the dedicated
// takeover instead retains its owner until a verified device reset. Neither
// path starts a core until all 120 images and startup records are read back.
class DataflowServiceOwner {
	enum Phase { PhaseEmpty, PhaseInstalling, PhaseReady, PhaseFaulted, PhaseClosed
#ifdef PIONEER_QWEN35_DEVICE_DISPATCH
		, PhaseGraphRunning, PhaseGraphFaulted, PhaseGraphClosing
#endif
#if defined(PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS) && defined(PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS)
		, PhaseScratchHeld
#endif
	};
	Phase fPhase=PhaseEmpty;
	bool fSaved[DataflowCoreCount]{};
	bool fCloseRestored=false;
#if defined(PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS) && defined(PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS)
	const void* fScratchCloseOwner=nullptr;
	bool fScratchResumeAttempted=false;
#endif
#ifdef PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS
	bool fSharedAttempted=false;
#endif
#ifdef PIONEER_QWEN35_EXCLUSIVE_TAKEOVER_WITNESS
	bool fStoppedResetReady=false;
	bool fDedicatedTakeoverTouched=false;
	bool fDedicatedCleanStopPending=false;
#endif
	DataflowServiceDiagnostic fDiagnostic={DataflowServiceDiagnosticNone,
		DataflowCoreCount,0,0,0,0};
	DataflowProjectionOwner fProjectionOwner;
#ifdef PIONEER_QWEN35_DEVICE_DISPATCH
	unsigned fGraphPrevious=0,fGraphTrigger=0;
	const DataflowGraphPublisher* fGraphPublisher=nullptr;
#endif

	void SetDiagnostic(unsigned stage,unsigned core,
		const DataflowWorkerSnapshot& snapshot,unsigned detail=0)
	{
		fDiagnostic={stage,core,snapshot.state,snapshot.failure,snapshot.phase,detail};
	}

	template<class IO>
	static bool WriteAndVerifyRegion(unsigned core,unsigned start,const uint32_t* text,
		unsigned count,IO& io)
	{
		uint32_t readback[DataflowServiceChunkWords];
		for (unsigned offset=0;offset<count;) {
			unsigned words=count-offset;
			if (words>DataflowServiceChunkWords) words=DataflowServiceChunkWords;
			const unsigned address=start+offset*sizeof(uint32_t);
			if (!io.WriteCoreMemory(core,address,text+offset,words*sizeof(uint32_t)))
				return false;
			if (!io.ReadCoreMemory(core,address,readback,words*sizeof(uint32_t)))
				return false;
			for (unsigned i=0;i<words;i++)
				if (readback[i]!=text[offset+i]) return false;
			offset+=words;
		}
		return true;
	}

	template<class IO>
	static bool WriteAndVerifyImage(unsigned core,const DataflowServiceImageView& image,IO& io)
	{
		if (!WriteAndVerifyRegion(core,DataflowServiceEntryAddress,image.textWords,image.textWordCount,io))
			return false;
#ifdef PIONEER_QWEN35_DEVICE_DISPATCH
		if (!WriteAndVerifyRegion(core,DataflowServiceDispatchAddress,image.dispatchWords,image.dispatchWordCount,io))
			return false;
#endif
		return true;
	}

#ifdef PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS
	// A ready-looking compute mailbox is initialized by the host before
	// release. It cannot prove either separate firmware image has actually
	// run. Wait for both role-owned receipts before accepting the tile; leave
	// physical PC/NoC proof to SharedCoreReady.
	template<class IO>
	static bool SharedWaitRoleStartup(unsigned core,IO& io)
	{
		for (unsigned poll=0;poll<65536;poll++) {
			DataflowRoleMailbox first{},second{};
			DataflowWorkerSnapshot compute{};
			if (!io.ReadWorkerSnapshot(core,compute)) { io.Pause(); continue; }
			if (compute.state==DataflowWorkerFailed || compute.state==DataflowWorkerFatal) return false;
			if (!DataflowWorkerSnapshotReady(compute)) { io.Pause(); continue; }
			if (compute.commandSequence || compute.acknowledgedSequence || compute.completedSequence)
				return false; // startup cannot inherit an old completed command
			if (!io.ReadCoreMemory(core,DataflowRoleMailboxAddress,&first,sizeof(first))) return false;
			io.Pause();
			if (!io.ReadCoreMemory(core,DataflowRoleMailboxAddress,&second,sizeof(second))) return false;
			if (!memcmp(&first,&second,sizeof(first)) && DataflowRoleMailboxStartupReady(second)) {
				DataflowWorkerSnapshot after{};
				if (io.ReadWorkerSnapshot(core,after) && !memcmp(&compute,&after,sizeof(compute)))
					return true;
			}
		}
		return false;
	}

	template<class IO>
	static bool SharedVerifyStagedFleet(const DataflowServiceImageView& image,IO& io)
	{
		static_assert(DataflowServiceChunkWords*sizeof(uint32_t)>=DataflowBarrierPacketBytes,
			"shared staged audit buffer must hold a barrier packet");
		static_assert(DataflowServiceChunkWords*sizeof(uint32_t)>=sizeof(DataflowRoleMailbox)
			&& DataflowServiceChunkWords*sizeof(uint32_t)>=sizeof(DataflowWorkerMailbox)
			&& DataflowServiceChunkWords*sizeof(uint32_t)>=sizeof(DataflowQueueControl)*DataflowCircularBufferCount,
			"shared staged audit buffer must hold each startup record");
		if (!io.SharedFleetHeldAndQuiescent()) return false;
		uint32_t readback[DataflowServiceChunkWords];
		auto verifyRegion=[&](unsigned core,unsigned address,const uint32_t* words,unsigned count) {
			for (unsigned offset=0;offset<count;) {
				unsigned n=count-offset;
				if (n>DataflowServiceChunkWords) n=DataflowServiceChunkWords;
				if (!io.CoreHeld(core) || !io.ReadCoreMemory(core,address+offset*4,readback,n*4))
					return false;
				for (unsigned i=0;i<n;i++) if (readback[i]!=words[offset+i]) return false;
				offset+=n;
			}
			return true;
		};
		for (unsigned core=0;core<DataflowCoreCount;core++) {
			DataflowQueueControl queues[DataflowCircularBufferCount]{};
			DataflowWorkerMailbox mailbox{};
			DataflowRoleMailbox role{};
#ifndef PIONEER_QWEN35_EXCLUSIVE_TAKEOVER_WITNESS
			DataflowRoleSnapshotHandoff handoff{};
#endif
			mailbox.magic=DataflowWorkerMailboxMagic; mailbox.version=DataflowWorkerMailboxVersion;
			mailbox.state=DataflowWorkerReady; mailbox.logicalCore=core;
			role.magic=DataflowRoleMailboxMagic; role.version=DataflowRoleMailboxVersion;
			uint32_t boot=0;
			if (!io.CoreHeld(core)
				|| !verifyRegion(core,DataflowServiceEntryAddress,image.textWords,image.textWordCount)
				|| !verifyRegion(core,DataflowServiceDispatchAddress,image.dispatchWords,image.dispatchWordCount)
				|| !io.ReadCoreMemory(core,DataflowServiceBootAddress,&boot,sizeof(boot)) || boot!=image.bootWord
				|| !io.ReadCoreMemory(core,DataflowQueueStateAddress,readback,sizeof(queues))
				|| memcmp(queues,readback,sizeof(queues))
				|| !io.ReadCoreMemory(core,DataflowWorkerMailboxAddress,readback,sizeof(mailbox))
				|| memcmp(&mailbox,readback,sizeof(mailbox))
				|| !io.ReadCoreMemory(core,DataflowRoleMailboxAddress,readback,sizeof(role))
				|| memcmp(&role,readback,sizeof(role))
#ifndef PIONEER_QWEN35_EXCLUSIVE_TAKEOVER_WITNESS
				|| !io.ReadCoreMemory(core,DataflowRoleSnapshotHandoffAddress,readback,sizeof(handoff))
				|| memcmp(&handoff,readback,sizeof(handoff))
#endif
				) return false;
			auto zeroPacket=[&](unsigned address) {
				if (!io.ReadCoreMemory(core,address,readback,DataflowBarrierPacketBytes)) return false;
				for (unsigned i=0;i<DataflowBarrierPacketBytes/sizeof(uint32_t);i++)
					if (readback[i]) return false;
				return true;
			};
			if (!zeroPacket(DataflowBarrierOutbox) || !zeroPacket(DataflowBarrierReleasePayload)
				|| !zeroPacket(DataflowBarrierReleaseCommit)) return false;
			const unsigned first=core*DataflowBarrierReleaseFanout+1;
			for (unsigned worker=first;worker<first+DataflowBarrierReleaseFanout
				&& worker<DataflowBarrierWorkers;worker++)
				if (!zeroPacket(DataflowBarrierArrivalAddress(worker,false))
					|| !zeroPacket(DataflowBarrierArrivalAddress(worker,true))) return false;
		}
		return io.SharedFleetHeldAndQuiescent();
	}
#endif

	template<class IO>
	void ReleaseSnapshots(IO& io)
	{
		for (unsigned core=0;core<DataflowCoreCount;core++) {
			if (fSaved[core]) io.ReleaseCoreSnapshot(core);
			fSaved[core]=false;
		}
	}

public:
	DataflowServiceOwner()=default;
	DataflowServiceOwner(const DataflowServiceOwner&)=delete;
	DataflowServiceOwner& operator=(const DataflowServiceOwner&)=delete;

	bool Ready() const { return fPhase==PhaseReady; }
#ifdef PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS
	// True only inside Install's callback after all resident workers have
	// been snapshotted and held. This is not an incoming-NoC safety proof.
	bool SharedRoleCapturePending() const { return fPhase==PhaseInstalling && fSharedAttempted; }
#endif
	bool Faulted() const { return fPhase==PhaseFaulted
#ifdef PIONEER_QWEN35_DEVICE_DISPATCH
		|| fPhase==PhaseGraphFaulted || fPhase==PhaseGraphClosing
#endif
		; }
	bool Closed() const { return fPhase==PhaseClosed; }
#if defined(PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS) && defined(PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS)
	bool ScratchHeld() const { return fPhase==PhaseScratchHeld && fScratchCloseOwner; }
#endif
	bool Started() const { return fPhase!=PhaseEmpty; }
#ifdef PIONEER_QWEN35_EXCLUSIVE_TAKEOVER_WITNESS
	bool StoppedResetReady() const { return fStoppedResetReady && fPhase==PhaseFaulted; }
	bool SharedDedicatedCleanStopPending() const {
		return fDedicatedCleanStopPending && fPhase==PhaseFaulted;
	}
#endif
	unsigned NextSequence() const { return fProjectionOwner.NextSequence(); }
	const DataflowServiceDiagnostic& Diagnostic() const { return fDiagnostic; }
	const DataflowOwnerDiagnostic& OwnerDiagnostic() const
		{ return fProjectionOwner.Diagnostic(); }

#ifdef PIONEER_QWEN35_DEVICE_DISPATCH
	// The caller holds the exclusive session lock across the transaction.
	// GraphRunning blocks ordinary commands and close until either an inert
	// preflight cancellation or a fully audited graph plus state commit.
	bool BeginGraph(const DataflowGraphPublisher& publisher,unsigned& trigger) {
		trigger=0;
		if (fPhase!=PhaseReady || !publisher.Unpublished()
			|| fProjectionOwner.NextSequence()>=DataflowWorkerSequenceLimit) return false;
		const unsigned previous=fProjectionOwner.NextSequence();
		if (!fProjectionOwner.CanAdoptGraphSequence(previous,previous+1)) return false;
		fGraphPrevious=previous;fGraphTrigger=previous+1;fGraphPublisher=&publisher;fPhase=PhaseGraphRunning;
		trigger=fGraphTrigger;return true;
	}
	bool OwnsGraph(const DataflowGraphPublisher& publisher) const {
		return fPhase==PhaseGraphRunning && fGraphPublisher==&publisher;
	}
	bool GraphRunning() const { return fPhase==PhaseGraphRunning; }
	bool CancelGraphBeforePublication(const DataflowGraphPublisher& publisher) {
		if (fPhase!=PhaseGraphRunning || fGraphPublisher!=&publisher || !publisher.Unpublished()) return false;
		fPhase=PhaseReady;fGraphPrevious=fGraphTrigger=0;fGraphPublisher=nullptr;return true;
	}
	void RetainFaultedGraph() {
		if (fPhase==PhaseGraphRunning || fPhase==PhaseReady) fPhase=PhaseGraphFaulted;
	}
	bool OwnsFaultedGraph(const DataflowGraphPublisher& publisher) const {
		return fPhase==PhaseGraphFaulted && fGraphPublisher==&publisher;
	}
	// Close changes PhaseGraphClosing to PhaseFaulted before touching the
	// fleet. Keep the exact retained publisher identifiable across retries.
	bool OwnsRetainedFaultGraph(const DataflowGraphPublisher& publisher) const {
		return (fPhase==PhaseGraphFaulted || fPhase==PhaseGraphClosing || fPhase==PhaseFaulted)
			&& fGraphPublisher==&publisher;
	}
	// This transition permits close only. It never makes a faulted model ready
	// for inference, and only accepts proof for this service's retained run.
	bool PermitFaultedGraphClose(const DataflowGraphPublisher& publisher,
		const DataflowGraphFaultAudit& audit) {
		if (fPhase!=PhaseGraphFaulted || fGraphPublisher!=&publisher
			|| publisher.PreviousSequence()!=fGraphPrevious
			|| publisher.TriggerSequence()!=fGraphTrigger || !audit.SettledFor(publisher)) return false;
		fPhase=PhaseGraphClosing;return true;
	}
	template<class IO>
	bool CommitGraph(const DataflowGraphPublisher& publisher,const DataflowGraphCompletion& completion,IO& io) {
		if (fPhase!=PhaseGraphRunning) return false;
		if (fGraphPublisher!=&publisher || !publisher.Published() || publisher.PreviousSequence()!=fGraphPrevious
			|| publisher.TriggerSequence()!=fGraphTrigger || !completion.AuditedFor(publisher)
			|| !fProjectionOwner.CanAdoptGraphSequence(fGraphPrevious,fGraphTrigger)
			|| !io.CommitGraphModelState(publisher,completion)) {
			fPhase=PhaseGraphFaulted;return false;
		}
		fProjectionOwner.AdoptGraphSequence(fGraphTrigger);
		fPhase=PhaseReady;fGraphPrevious=fGraphTrigger=0;fGraphPublisher=nullptr;return true;
	}
#endif

	template<class IO>
	DataflowServiceResult Install(const DataflowServiceImageView& image,IO& io)
	{
		if (fPhase!=PhaseEmpty) return DataflowServiceInvalidState;
		fDiagnostic={DataflowServiceDiagnosticNone,DataflowCoreCount,0,0,0,0};
		if (!image.Valid()) return DataflowServiceInvalidImage;
		uint32_t failedStage=0,failedCore=0xff;
		if (!io.SessionIdle() || !io.ProveFleetRoutes(failedStage,failedCore)) {
			return DataflowServicePreflightFailed;
		}
		fPhase=PhaseInstalling;
		for (unsigned core=0;core<DataflowCoreCount;core++) {
			if (!io.CoreIdle(core)) {
				fPhase=PhaseEmpty;
				return DataflowServicePreflightFailed;
			}
		}
#ifndef PIONEER_QWEN35_EXCLUSIVE_TAKEOVER_WITNESS
		for (unsigned core=0;core<DataflowCoreCount;core++) {
			if (!io.CaptureCoreState(core)) {
				ReleaseSnapshots(io);
				fPhase=PhaseEmpty;
				return DataflowServiceSnapshotFailed;
			}
			fSaved[core]=true;
		}
#endif
		for (unsigned core=0;core<DataflowCoreCount;core++) {
#ifdef PIONEER_QWEN35_EXCLUSIVE_TAKEOVER_WITNESS
			// A failed hold may have changed a worker. From this point the
			// dedicated owner must retire through verified device reset.
			fDedicatedTakeoverTouched=true;
#endif
			if (!io.HoldCore(core) || !io.CoreHeld(core)) {
				fPhase=PhaseFaulted;
				return DataflowServiceHoldFailed;
			}
		}

#ifdef PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS
		// The caller already owns the session lock. The role transaction must
		// retain partial snapshots/writes on failure and support safe cleanup.
		fSharedAttempted=true;
		if (!io.SharedCaptureAndStageHeld()) {
			fPhase=PhaseFaulted;
			return DataflowServiceSnapshotFailed;
		}
#endif

		// Every core is held before the first image write. No partially staged
		// core is allowed to start.
		for (unsigned core=0;core<DataflowCoreCount;core++) {
			if (!WriteAndVerifyImage(core,image,io)) {
				fPhase=PhaseFaulted;
				return DataflowServiceImageWriteFailed;
			}
#ifdef PIONEER_QWEN35_DEVICE_DISPATCH
			// The new queued service uses L1 barrier slots only after the full
			// fleet is held and snapshotted; partial initialization stays owned.
			if (!DataflowBarrierPrepareHeldCore(io,core)) {
				fPhase=PhaseFaulted;
				return DataflowServiceStartupWriteFailed;
			}
#endif
			DataflowQueueControl queues[DataflowCircularBufferCount]{};
			DataflowWorkerMailbox mailbox{};
			mailbox.magic=DataflowWorkerMailboxMagic;
			mailbox.version=DataflowWorkerMailboxVersion;
			mailbox.state=DataflowWorkerReady;
			mailbox.logicalCore=core;
			if (!io.WriteCoreMemory(core,DataflowQueueStateAddress,queues,sizeof(queues))
				|| !io.WriteCoreMemory(core,DataflowWorkerMailboxAddress,
					&mailbox,sizeof(mailbox))) {
				fPhase=PhaseFaulted;
				return DataflowServiceStartupWriteFailed;
			}
#ifdef PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS
			DataflowRoleMailbox roleMailbox{};
			roleMailbox.magic=DataflowRoleMailboxMagic;
			roleMailbox.version=DataflowRoleMailboxVersion;
			if (!io.WriteCoreMemory(core,DataflowRoleMailboxAddress,&roleMailbox,sizeof(roleMailbox))) {
				fPhase=PhaseFaulted;
				return DataflowServiceStartupWriteFailed;
			}
			DataflowRoleMailbox roleReadback{};
			if (!io.ReadCoreMemory(core,DataflowRoleMailboxAddress,&roleReadback,sizeof(roleReadback))
				|| memcmp(&roleMailbox,&roleReadback,sizeof(roleMailbox))!=0) {
				fPhase=PhaseFaulted;
				return DataflowServiceStartupReadbackFailed;
			}
#ifndef PIONEER_QWEN35_EXCLUSIVE_TAKEOVER_WITNESS
			DataflowRoleSnapshotHandoff handoff{},handoffReadback{};
			if (!io.WriteCoreMemory(core,DataflowRoleSnapshotHandoffAddress,&handoff,sizeof(handoff))) {
				fPhase=PhaseFaulted; return DataflowServiceStartupWriteFailed;
			}
			if (!io.ReadCoreMemory(core,DataflowRoleSnapshotHandoffAddress,&handoffReadback,sizeof(handoffReadback))
				|| memcmp(&handoff,&handoffReadback,sizeof(handoff))) {
				fPhase=PhaseFaulted; return DataflowServiceStartupReadbackFailed;
			}
#endif
#endif
			DataflowQueueControl queueReadback[DataflowCircularBufferCount];
			DataflowWorkerMailbox mailboxReadback{};
			if (!io.ReadCoreMemory(core,DataflowQueueStateAddress,
					queueReadback,sizeof(queueReadback))
				|| memcmp(queues,queueReadback,sizeof(queues))!=0
				|| !io.ReadCoreMemory(core,DataflowWorkerMailboxAddress,
					&mailboxReadback,sizeof(mailboxReadback))
				|| memcmp(&mailbox,&mailboxReadback,sizeof(mailbox))!=0) {
				fPhase=PhaseFaulted;
				return DataflowServiceStartupReadbackFailed;
			}
			if (!io.WriteCoreMemory(core,DataflowServiceBootAddress,
				&image.bootWord,sizeof(image.bootWord))) {
				fPhase=PhaseFaulted;
				return DataflowServiceStartupWriteFailed;
			}
			unsigned bootReadback=0;
			if (!io.ReadCoreMemory(core,DataflowServiceBootAddress,
				&bootReadback,sizeof(bootReadback)) || bootReadback!=image.bootWord) {
				fPhase=PhaseFaulted;
				return DataflowServiceStartupReadbackFailed;
			}
		}

#ifdef PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS
		// The one-shot fleet ledger must be consumed before any reset release.
		if (!io.SharedArmAndBeginRelease() || !SharedVerifyStagedFleet(image,io)) {
			fPhase=PhaseFaulted;
			return DataflowServiceStartFailed;
		}
#endif
		for (unsigned core=0;core<DataflowCoreCount;core++) {
#ifdef PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS
			if (!io.SharedStartCore(core)) {
#else
			if (!io.StartCore(core)) {
#endif
				DataflowWorkerSnapshot snapshot{};
				SetDiagnostic(DataflowServiceDiagnosticWorkerStart,core,snapshot);
				fPhase=PhaseFaulted;
				return DataflowServiceStartFailed;
			}
#if defined(PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS) && !defined(PIONEER_QWEN35_EXCLUSIVE_TAKEOVER_WITNESS)
			// Retain this tile's private RAM before releasing another tile.
			// A partial launch can then leave at most the current tile
			// released without a completed stackless snapshot. The shims
			// remain stackless until the one-shot handoff succeeds.
			if (!io.SharedPrepareRoleStacks(core)) {
				DataflowWorkerSnapshot snapshot{};
				SetDiagnostic(DataflowServiceDiagnosticWorkerProof,core,snapshot);
				fPhase=PhaseFaulted; return DataflowServiceStartFailed;
			}
#endif
		}
		// Drain startup state from the whole fleet before returning a failure.
		// A core can publish an early startup fault while later cores are still
		// entering the service image. Stopping the owner scan at the first fault
		// lets cleanup race those workers and can strand a partially started
		// fleet. Keep the first diagnostic, but let every started core reach a
		// readable ready/fatal state before Close performs its all-core audit.
		bool startupReady=true;
		for (unsigned core=0;core<DataflowCoreCount;core++) {
			DataflowWorkerSnapshot snapshot{};
			if (!io.WaitWorkerReady(core,snapshot)) {
				if (startupReady) {
					unsigned detail=0;
					if (snapshot.state==DataflowWorkerFatal && snapshot.phase==2) {
						uint32_t route=0;
						const unsigned address=DataflowWorkerMailboxAddress
							+offsetof(DataflowWorkerMailbox,reserved);
						if (io.ReadCoreMemory(core,address,&route,sizeof(route)))
							detail=route;
					}
					SetDiagnostic(DataflowServiceDiagnosticWorkerWait,core,snapshot,
						detail);
				}
				startupReady=false;
				continue;
			}
			if (!DataflowWorkerSnapshotReady(snapshot)) {
				if (startupReady)
					SetDiagnostic(DataflowServiceDiagnosticWorkerSnapshot,core,snapshot);
				startupReady=false;
				continue;
			}
#ifdef PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS
			if (!SharedWaitRoleStartup(core,io) || !io.SharedCoreReady(core,snapshot)) {
#else
			if (!io.CoreReady(core,snapshot)) {
#endif
				if (startupReady)
					SetDiagnostic(DataflowServiceDiagnosticWorkerProof,core,snapshot);
				startupReady=false;
			}
		}
		if (!startupReady) {
			fPhase=PhaseFaulted;
			return DataflowServiceReadyFailed;
		}
#ifdef PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS
		struct SharedReadyView {
			IO& io;
			bool ReadWorkerSnapshot(unsigned core,DataflowWorkerSnapshot& snapshot)
				{ return io.ReadWorkerSnapshot(core,snapshot); }
			bool CoreReady(unsigned core,const DataflowWorkerSnapshot& snapshot)
				{ return io.SharedCoreReady(core,snapshot); }
		} readyView{io};
		if (fProjectionOwner.Initialize(readyView)!=DataflowOwnerComplete) {
#else
		if (fProjectionOwner.Initialize(io)!=DataflowOwnerComplete) {
#endif
			for (unsigned core=0;core<DataflowCoreCount;core++) {
				DataflowWorkerSnapshot snapshot{};
				if (!io.ReadWorkerSnapshot(core,snapshot)
					|| !DataflowWorkerSnapshotReady(snapshot)
#ifdef PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS
					|| !io.SharedCoreReady(core,snapshot)) {
#else
					|| !io.CoreReady(core,snapshot)) {
#endif
					SetDiagnostic(DataflowServiceDiagnosticOwnerCore,core,snapshot);
					break;
				}
			}
			if (fDiagnostic.stage==DataflowServiceDiagnosticNone)
				SetDiagnostic(DataflowServiceDiagnosticOwnerInitialize,
					DataflowCoreCount,DataflowWorkerSnapshot{});
			fPhase=PhaseFaulted;
			return DataflowServiceReadyFailed;
		}
		fPhase=PhaseReady;
		return DataflowServiceComplete;
	}

	template<class IO>
	DataflowServiceResult RunProjection(const DataflowProjectionProgram& program,IO& io)
	{
		if (fPhase!=PhaseReady) return DataflowServiceInvalidState;
		if (fProjectionOwner.RunProjection(program,io)!=DataflowOwnerComplete) {
			fPhase=PhaseFaulted;
			return DataflowServiceProjectionFailed;
		}
		return DataflowServiceComplete;
	}

	template<class IO>
	DataflowServiceResult EmbedToken(const DataflowEmbeddingProgram& program,IO& io)
	{
		if (fPhase!=PhaseReady) return DataflowServiceInvalidState;
		if (fProjectionOwner.RunEmbedding(program,io)!=DataflowOwnerComplete) {
			fPhase=PhaseFaulted;
			return DataflowServiceEmbeddingFailed;
		}
		return DataflowServiceComplete;
	}

	template<class IO>
	DataflowServiceResult RunRms(const DataflowRmsProgram& program,IO& io)
	{
		if (fPhase!=PhaseReady) return DataflowServiceInvalidState;
		if (fProjectionOwner.RunRms(program,io)!=DataflowOwnerComplete) {
			fPhase=PhaseFaulted;
			return DataflowServiceRmsFailed;
		}
		return DataflowServiceComplete;
	}

	template<class IO>
	DataflowServiceResult RunSilu(const DataflowSiluProgram& program,IO& io)
	{
		if (fPhase!=PhaseReady) return DataflowServiceInvalidState;
		if (fProjectionOwner.RunSilu(program,io)!=DataflowOwnerComplete) {
			fPhase=PhaseFaulted;
			return DataflowServiceSiluFailed;
		}
		return DataflowServiceComplete;
	}

	template<class IO>
	DataflowServiceResult RunResidual(const DataflowResidualProgram& program,IO& io)
	{
		if (fPhase!=PhaseReady) return DataflowServiceInvalidState;
		if (fProjectionOwner.RunResidual(program,io)!=DataflowOwnerComplete) {
			fPhase=PhaseFaulted;
			return DataflowServiceResidualFailed;
		}
		return DataflowServiceComplete;
	}

	template<class IO>
	DataflowServiceResult RunConv(const DataflowConvProgram& program,IO& io)
	{
		if (fPhase!=PhaseReady) return DataflowServiceInvalidState;
		if (fProjectionOwner.RunConv(program,io)!=DataflowOwnerComplete) {
			fPhase=PhaseFaulted;
			return DataflowServiceConvFailed;
		}
		return DataflowServiceComplete;
	}

	template<class IO>
	DataflowServiceResult RunDelta(const DataflowDeltaProgram& program,IO& io)
	{
		if (fPhase!=PhaseReady) return DataflowServiceInvalidState;
		if (fProjectionOwner.RunDelta(program,io)!=DataflowOwnerComplete) {
			fPhase=PhaseFaulted;
			return DataflowServiceDeltaFailed;
		}
		return DataflowServiceComplete;
	}

	template<class IO>
	DataflowServiceResult RunDenseAttention(
		const DataflowDenseAttentionProgram& program,IO& io)
	{
		if (fPhase!=PhaseReady) return DataflowServiceInvalidState;
		if (fProjectionOwner.RunDenseAttention(program,io)!=DataflowOwnerComplete) {
			fPhase=PhaseFaulted;
			return DataflowServiceDenseAttentionFailed;
		}
		return DataflowServiceComplete;
	}

	template<class IO>
	DataflowServiceResult RunArgmax(const DataflowArgmaxProgram& program,
		unsigned& token,IO& io)
	{
		token=Vocabulary;
		if (fPhase!=PhaseReady) return DataflowServiceInvalidState;
		if (fProjectionOwner.RunArgmax(program,token,io)!=DataflowOwnerComplete) {
			fPhase=PhaseFaulted;
			return DataflowServiceArgmaxFailed;
		}
		return DataflowServiceComplete;
	}

	template<class IO>
	DataflowServiceResult RecoverProjection(IO& io)
	{
		if (fPhase!=PhaseFaulted || !fProjectionOwner.Faulted())
			return DataflowServiceInvalidState;
		const DataflowOwnerResult result=fProjectionOwner.Recover(io);
		if (result==DataflowOwnerRecoveryPending)
			return DataflowServiceRecoveryPending;
		if (result!=DataflowOwnerRecovered) return DataflowServiceInvalidState;
		fPhase=PhaseReady;
		return DataflowServiceRecovered;
	}

	private:
	template<class IO>
	DataflowServiceResult FinishClose(IO& io)
	{
		unsigned resumeStage=0,resumeCore=DataflowCoreCount,resumeDetail=0;
		if (!io.ResumeRestoredFleet(resumeStage,resumeCore,resumeDetail)) {
			DataflowWorkerSnapshot snapshot{};
			snapshot.state=resumeStage;
			SetDiagnostic(DataflowServiceDiagnosticResume,resumeCore,snapshot,resumeDetail);
			return DataflowServiceResumeFailed;
		}
		ReleaseSnapshots(io);
		fPhase=PhaseClosed;
		return DataflowServiceClosed;
	}

	template<class IO>
	DataflowServiceResult CloseImpl(IO& io,bool holdForScratch)
	{
#if !defined(PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS) || !defined(PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS)
		(void)holdForScratch;
#endif
		if (fPhase==PhaseClosed) return DataflowServiceClosed;
#ifdef PIONEER_QWEN35_EXCLUSIVE_TAKEOVER_WITNESS
		if (StoppedResetReady()) return DataflowServiceResetRequired;
#endif
		if (fPhase!=PhaseReady && fPhase!=PhaseFaulted && fPhase!=PhaseInstalling
#ifdef PIONEER_QWEN35_DEVICE_DISPATCH
			&& fPhase!=PhaseGraphClosing
#endif
			)
			return DataflowServiceInvalidState;
#ifdef PIONEER_QWEN35_EXCLUSIVE_TAKEOVER_WITNESS
		// Remember only a clean ready-to-close transition. A retry after a
		// partial stop retains this marker; failed startup/graph does not.
		if (fPhase==PhaseReady) fDedicatedCleanStopPending=fSharedAttempted;
#endif
		fPhase=PhaseFaulted;
		if (!fCloseRestored) {
			unsigned auditStage=0,auditCore=DataflowCoreCount;
#ifdef PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS
			if (!(fSharedAttempted?io.SharedStopAndHoldAll(auditStage,auditCore)
				:io.StopAndAuditAll(auditStage,auditCore))) {
#else
			if (!io.StopAndAuditAll(auditStage,auditCore)) {
#endif
				DataflowWorkerSnapshot snapshot{};
				snapshot.state=auditStage;
				SetDiagnostic(DataflowServiceDiagnosticStopAudit,auditCore,snapshot);
				return DataflowServiceStopAuditFailed;
			}
#ifdef PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS
#ifdef PIONEER_QWEN35_EXCLUSIVE_TAKEOVER_WITNESS
			// A takeover discards previous core state. Keep the owner alive
			// until the reset/reinitialization transaction.
			if (fDedicatedTakeoverTouched || (fSharedAttempted && io.SharedTakeoverResetRequired())) {
				fStoppedResetReady=true;return DataflowServiceResetRequired;
			}
#endif
			// Must be retryable and leave every core held on any failure.
			if (fSharedAttempted && !io.SharedRestoreRolesHeld())
				return DataflowServiceRestoreFailed;
#endif
			for (unsigned end=DataflowCoreCount;end;end--) {
				const unsigned core=end-1;
				if (!fSaved[core]) continue;
				if (!io.RestoreCoreState(core)) {
					DataflowWorkerSnapshot snapshot{};
					snapshot.state=1;
					SetDiagnostic(DataflowServiceDiagnosticRestore,core,snapshot);
					return DataflowServiceRestoreFailed;
				}
				if (!io.VerifyCoreStateRestored(core)) {
					DataflowWorkerSnapshot snapshot{};
					snapshot.state=2;
					SetDiagnostic(DataflowServiceDiagnosticRestore,core,snapshot);
					return DataflowServiceRestoreFailed;
				}
			}
#ifdef PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS
			if (fSharedAttempted && !io.SharedVerifyRolesAfterResidentRestore())
				return DataflowServiceRestoreFailed;
			for (unsigned core=0;core<DataflowCoreCount;core++)
				if (fSaved[core] && !io.VerifyCoreStateRestored(core))
					return DataflowServiceRestoreFailed;
			if (fSharedAttempted && !io.SharedFinalizeBeforeResume())
				return DataflowServiceRestoreFailed;
#endif
			fCloseRestored=true;
		}
#if defined(PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS) && defined(PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS)
		if (holdForScratch) {
			fPhase=PhaseScratchHeld;
			return DataflowServiceScratchHeld;
		}
#endif
		return FinishClose(io);
	}
	public:
	template<class IO>
	DataflowServiceResult Close(IO& io)
	{
#if defined(PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS) && defined(PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS)
		// A scratch owner must explicitly authorize the held close. Never
		// resume by falling through the ordinary close after a partial hold.
		if (fScratchCloseOwner) return DataflowServiceInvalidState;
#endif
		return CloseImpl(io,false);
	}
#if defined(PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS) && defined(PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS)
	// Non-loadable witness only. Scope must prove the exact scratch owner,
	// exclusive fleet and transfer authority; this method grants none of them.
	template<class IO,class Scope>
	DataflowServiceResult CloseForScratch(IO& io,const void* owner,Scope& scope)
	{
		if (!owner || fScratchCloseOwner || fPhase!=PhaseReady || !fSharedAttempted
			|| !scope.QualificationHoldAllowed(owner,*this)) return DataflowServiceInvalidState;
		fScratchCloseOwner=owner;
		const auto result=CloseImpl(io,true);
		if (result==DataflowServiceScratchHeld
			&& (!scope.QualificationRestoreHeldRoute(owner,*this,io)
				|| !scope.QualificationHeldAllowed(owner,*this))) {
			DataflowWorkerSnapshot snapshot{};
			SetDiagnostic(DataflowServiceDiagnosticStopAudit,DataflowCoreCount,snapshot);
			fPhase=PhaseFaulted;
			return DataflowServiceStopAuditFailed;
		}
		return result;
	}
	template<class IO,class Scope>
	DataflowServiceResult FinishScratchClose(IO& io,const void* owner,Scope& scope)
	{
		if (!ScratchHeld() || owner!=fScratchCloseOwner || fScratchResumeAttempted
			|| !scope.QualificationResumeAllowed(owner,*this)) return DataflowServiceInvalidState;
		// An uncertain partial resume must not be replayed automatically.
		fScratchResumeAttempted=true;fPhase=PhaseFaulted;
		const auto result=FinishClose(io);
		if (result==DataflowServiceClosed) fScratchCloseOwner=nullptr;
		return result;
	}
#endif
};

} // namespace PioneerQwen35

#endif
