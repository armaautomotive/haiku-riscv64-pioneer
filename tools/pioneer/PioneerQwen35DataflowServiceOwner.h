/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_SERVICE_OWNER_H
#define PIONEER_QWEN35_DATAFLOW_SERVICE_OWNER_H

#include "PioneerQwen35DataflowOwner.h"
#include "PioneerQwen35DataflowKernels.h"
#include <string.h>

namespace PioneerQwen35 {

enum : unsigned {
	DataflowServiceBootAddress=0x6000,
	DataflowServiceEntryAddress=0x72000,
	DataflowServiceTextLimit=0x80000,
	DataflowServiceBootWord=0x0006c06f,
	DataflowServiceChunkWords=64
};
static_assert(sizeof(DataflowQueueControl)*DataflowCircularBufferCount
	==DataflowQueueStateBytes,
	"resident startup clears exactly the reserved queue-control region");

struct DataflowServiceImageView {
	unsigned bootWord;
	const uint32_t* textWords;
	unsigned textWordCount;

	bool Valid() const
	{
		return bootWord==DataflowServiceBootWord && textWords && textWordCount
			&& textWordCount<=((DataflowServiceTextLimit-DataflowServiceEntryAddress)
				/sizeof(uint32_t));
	}
};

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
	DataflowServiceGenerationFailed=31
};

enum DataflowServiceDiagnosticStage : unsigned {
	DataflowServiceDiagnosticNone=0,
	DataflowServiceDiagnosticWorkerStart=1,
	DataflowServiceDiagnosticWorkerWait=2,
	DataflowServiceDiagnosticWorkerSnapshot=3,
	DataflowServiceDiagnosticWorkerProof=4,
	DataflowServiceDiagnosticOwnerInitialize=5,
	DataflowServiceDiagnosticOwnerCore=6
};

struct DataflowServiceDiagnostic {
	unsigned stage,core,state,failure,phase;
};

// Owns the resident image lifecycle and the projection coordinator. IO is the
// only platform-specific layer: it must prove driver/session idleness, save
// and restore each core's touched L1/runtime state, perform bounded L1 copies,
	// and audit reset, route, worker status, and NoC state. RestoreCoreState
	// must leave cores held; ResumeRestoredFleet restores their captured reset
	// states only after every core's L1 and boot state has been verified. This class does no
// inference and starts no core until all 120 images and startup records have
// been read back successfully.
class DataflowServiceOwner {
	enum Phase { PhaseEmpty, PhaseInstalling, PhaseReady, PhaseFaulted, PhaseClosed };
	Phase fPhase=PhaseEmpty;
	bool fSaved[DataflowCoreCount]{};
	DataflowServiceDiagnostic fDiagnostic={DataflowServiceDiagnosticNone,
		DataflowCoreCount,0,0,0};
	DataflowProjectionOwner fProjectionOwner;

	void SetDiagnostic(unsigned stage,unsigned core,
		const DataflowWorkerSnapshot& snapshot)
	{
		fDiagnostic={stage,core,snapshot.state,snapshot.failure,snapshot.phase};
	}

	template<class IO>
	static bool WriteAndVerifyImage(unsigned core,const DataflowServiceImageView& image,
		IO& io)
	{
		uint32_t readback[DataflowServiceChunkWords];
		for (unsigned offset=0;offset<image.textWordCount;) {
			unsigned words=image.textWordCount-offset;
			if (words>DataflowServiceChunkWords) words=DataflowServiceChunkWords;
			const unsigned address=DataflowServiceEntryAddress+offset*sizeof(uint32_t);
			if (!io.WriteCoreMemory(core,address,image.textWords+offset,words*sizeof(uint32_t)))
				return false;
			if (!io.ReadCoreMemory(core,address,readback,words*sizeof(uint32_t)))
				return false;
			for (unsigned i=0;i<words;i++)
				if (readback[i]!=image.textWords[offset+i]) return false;
			offset+=words;
		}
		return true;
	}

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
	bool Faulted() const { return fPhase==PhaseFaulted; }
	bool Closed() const { return fPhase==PhaseClosed; }
	bool Started() const { return fPhase!=PhaseEmpty; }
	unsigned NextSequence() const { return fProjectionOwner.NextSequence(); }
	const DataflowServiceDiagnostic& Diagnostic() const { return fDiagnostic; }

	template<class IO>
	DataflowServiceResult Install(const DataflowServiceImageView& image,IO& io)
	{
		if (fPhase!=PhaseEmpty) return DataflowServiceInvalidState;
		fDiagnostic={DataflowServiceDiagnosticNone,DataflowCoreCount,0,0,0};
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
		for (unsigned core=0;core<DataflowCoreCount;core++) {
			if (!io.CaptureCoreState(core)) {
				ReleaseSnapshots(io);
				fPhase=PhaseEmpty;
				return DataflowServiceSnapshotFailed;
			}
			fSaved[core]=true;
		}
		for (unsigned core=0;core<DataflowCoreCount;core++) {
			if (!io.HoldCore(core) || !io.CoreHeld(core)) {
				fPhase=PhaseFaulted;
				return DataflowServiceHoldFailed;
			}
		}

		// Every original state is captured and every core held before the first
		// image write. No partially staged core is allowed to start.
		for (unsigned core=0;core<DataflowCoreCount;core++) {
			if (!WriteAndVerifyImage(core,image,io)) {
				fPhase=PhaseFaulted;
				return DataflowServiceImageWriteFailed;
			}
			DataflowQueueControl queues[DataflowCircularBufferCount]{};
			DataflowWorkerMailbox mailbox{};
			mailbox.magic=DataflowWorkerMailboxMagic;
			mailbox.version=DataflowWorkerMailboxVersion;
			mailbox.state=DataflowWorkerReady;
			if (!io.WriteCoreMemory(core,DataflowQueueStateAddress,queues,sizeof(queues))
				|| !io.WriteCoreMemory(core,DataflowWorkerMailboxAddress,
					&mailbox,sizeof(mailbox))) {
				fPhase=PhaseFaulted;
				return DataflowServiceStartupWriteFailed;
			}
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

		for (unsigned core=0;core<DataflowCoreCount;core++) {
			if (!io.StartCore(core)) {
				DataflowWorkerSnapshot snapshot{};
				SetDiagnostic(DataflowServiceDiagnosticWorkerStart,core,snapshot);
				fPhase=PhaseFaulted;
				return DataflowServiceStartFailed;
			}
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
				if (startupReady)
					SetDiagnostic(DataflowServiceDiagnosticWorkerWait,core,snapshot);
				startupReady=false;
				continue;
			}
			if (!DataflowWorkerSnapshotReady(snapshot)) {
				if (startupReady)
					SetDiagnostic(DataflowServiceDiagnosticWorkerSnapshot,core,snapshot);
				startupReady=false;
				continue;
			}
			if (!io.CoreReady(core,snapshot)) {
				if (startupReady)
					SetDiagnostic(DataflowServiceDiagnosticWorkerProof,core,snapshot);
				startupReady=false;
			}
		}
		if (!startupReady) {
			fPhase=PhaseFaulted;
			return DataflowServiceReadyFailed;
		}
		if (fProjectionOwner.Initialize(io)!=DataflowOwnerComplete) {
			for (unsigned core=0;core<DataflowCoreCount;core++) {
				DataflowWorkerSnapshot snapshot{};
				if (!io.ReadWorkerSnapshot(core,snapshot)
					|| !DataflowWorkerSnapshotReady(snapshot)
					|| !io.CoreReady(core,snapshot)) {
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

	template<class IO>
	DataflowServiceResult Close(IO& io)
	{
		if (fPhase==PhaseClosed) return DataflowServiceClosed;
		if (fPhase!=PhaseReady && fPhase!=PhaseFaulted && fPhase!=PhaseInstalling)
			return DataflowServiceInvalidState;
		fPhase=PhaseFaulted;
		if (!io.StopAndAuditAll()) return DataflowServiceStopAuditFailed;
		for (unsigned end=DataflowCoreCount;end;end--) {
			const unsigned core=end-1;
			if (!fSaved[core]) continue;
			if (!io.RestoreCoreState(core)
				|| !io.VerifyCoreStateRestored(core))
				return DataflowServiceRestoreFailed;
		}
		if (!io.ResumeRestoredFleet()) return DataflowServiceResumeFailed;
		ReleaseSnapshots(io);
		fPhase=PhaseClosed;
		return DataflowServiceClosed;
	}
};

} // namespace PioneerQwen35

#endif
