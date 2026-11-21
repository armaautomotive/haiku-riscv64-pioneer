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
	DataflowServiceClosed=17
};

// Owns the resident image lifecycle and the projection coordinator. IO is the
// only platform-specific layer: it must prove driver/session idleness, save
// and restore each core's touched L1/runtime state, perform bounded L1 copies,
// and audit reset, route, worker status, and NoC state. This class does no
// inference and starts no core until all 120 images and startup records have
// been read back successfully.
class DataflowServiceOwner {
	enum Phase { PhaseEmpty, PhaseInstalling, PhaseReady, PhaseFaulted, PhaseClosed };
	Phase fPhase=PhaseEmpty;
	bool fSaved[DataflowCoreCount]{};
	DataflowProjectionOwner fProjectionOwner;

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
	unsigned NextSequence() const { return fProjectionOwner.NextSequence(); }

	template<class IO>
	DataflowServiceResult Install(const DataflowServiceImageView& image,IO& io)
	{
		if (fPhase!=PhaseEmpty) return DataflowServiceInvalidState;
		if (!image.Valid()) return DataflowServiceInvalidImage;
		fPhase=PhaseInstalling;
		if (!io.SessionIdle() || !io.ProveFleetRoutes()) {
			fPhase=PhaseEmpty;
			return DataflowServicePreflightFailed;
		}
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
				fPhase=PhaseFaulted;
				return DataflowServiceStartFailed;
			}
		}
		for (unsigned core=0;core<DataflowCoreCount;core++) {
			DataflowWorkerSnapshot snapshot{};
			if (!io.WaitWorkerReady(core,snapshot)
				|| !DataflowWorkerSnapshotReady(snapshot)
				|| !io.CoreReady(core,snapshot)) {
				fPhase=PhaseFaulted;
				return DataflowServiceReadyFailed;
			}
		}
		if (fProjectionOwner.Initialize(io)!=DataflowOwnerComplete) {
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
		ReleaseSnapshots(io);
		fPhase=PhaseClosed;
		return DataflowServiceClosed;
	}
};

} // namespace PioneerQwen35

#endif
