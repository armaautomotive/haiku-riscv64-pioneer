/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_OWNER_H
#define PIONEER_QWEN35_DATAFLOW_OWNER_H

#include "PioneerQwen35DataflowProgram.h"
#include "PioneerQwen35DataflowEmbeddingProgram.h"

namespace PioneerQwen35 {

struct DataflowWorkerSnapshot {
	unsigned commandSequence,acknowledgedSequence,completedSequence;
	unsigned state,failure,phase;
};

enum DataflowOwnerResult {
	DataflowOwnerComplete=0,
	DataflowOwnerInvalidState=1,
	DataflowOwnerInvalidProgram=2,
	DataflowOwnerSequenceExhausted=3,
	DataflowOwnerCoreNotReady=4,
	DataflowOwnerInvocationWriteFailed=5,
	DataflowOwnerInvocationReadbackFailed=6,
	DataflowOwnerCommandPublishFailed=7,
	DataflowOwnerWorkerFailed=8,
	DataflowOwnerAuditFailed=9,
	DataflowOwnerAcknowledgementFailed=10,
	DataflowOwnerRecoveryPending=11,
	DataflowOwnerRecovered=12,
	DataflowOwnerProgramUnauthorized=13,
	DataflowOwnerProjectionBeginFailed=14,
	DataflowOwnerProjectionCommitFailed=15
};

inline bool DataflowWorkerSnapshotReady(const DataflowWorkerSnapshot& snapshot)
{
	return snapshot.state==DataflowWorkerReady
		&& snapshot.commandSequence==snapshot.acknowledgedSequence
		&& snapshot.acknowledgedSequence==snapshot.completedSequence
		&& !snapshot.failure && !snapshot.phase;
}

// Host-side orchestration only. IO owns PCI/NoC access and must implement
// CoreReady, AuthorizeProjection, BeginProjection, WriteInvocation,
// InvocationMatches, PublishCommand, WaitForCompletion, AuditProjection,
// Acknowledge, CompleteProjection, ReadWorkerSnapshot, and AuditAndRecoverAll.
// BeginProjection invalidates the output activation before staging; only a
// fully audited and acknowledged result may pass CompleteProjection. The
// owner itself performs no numeric work.
class DataflowProjectionOwner {
	bool fInitialized=false;
	bool fFaulted=false;
	unsigned fNextSequence=0;
	unsigned fPendingSequence=0;
	DataflowWorkerSnapshot fSnapshot[DataflowCoreCount]{};

	static bool Terminal(const DataflowWorkerSnapshot& snapshot,unsigned sequence)
	{
		return snapshot.commandSequence==sequence
			&& snapshot.completedSequence==sequence
			&& (snapshot.state==DataflowWorkerComplete
				|| snapshot.state==DataflowWorkerFailed);
	}

public:
	DataflowProjectionOwner()=default;
	DataflowProjectionOwner(const DataflowProjectionOwner&)=delete;
	DataflowProjectionOwner& operator=(const DataflowProjectionOwner&)=delete;

	bool Initialized() const { return fInitialized; }
	bool Faulted() const { return fFaulted; }
	unsigned NextSequence() const { return fNextSequence; }

	template<class IO>
	DataflowOwnerResult Initialize(IO& io)
	{
		if (fInitialized || fFaulted) return DataflowOwnerInvalidState;
		unsigned newest=0;
		for (unsigned core=0;core<DataflowCoreCount;core++) {
			DataflowWorkerSnapshot snapshot{};
			if (!io.ReadWorkerSnapshot(core,snapshot)
				|| !DataflowWorkerSnapshotReady(snapshot)
				|| !io.CoreReady(core,snapshot))
				return DataflowOwnerCoreNotReady;
			if (snapshot.completedSequence>newest)
				newest=snapshot.completedSequence;
			fSnapshot[core]=snapshot;
		}
		if (newest>DataflowWorkerSequenceLimit)
			return DataflowOwnerSequenceExhausted;
		fNextSequence=newest;
		fInitialized=true;
		return DataflowOwnerComplete;
	}

	template<class IO>
	DataflowOwnerResult RunProjection(const DataflowProjectionProgram& program,IO& io)
	{
		if (!fInitialized || fFaulted) return DataflowOwnerInvalidState;
		if (!program.Valid()) return DataflowOwnerInvalidProgram;
		if (fNextSequence>=DataflowWorkerSequenceLimit)
			return DataflowOwnerSequenceExhausted;
		if (!io.AuthorizeProjection(program))
			return DataflowOwnerProgramUnauthorized;
		if (!io.BeginProjection(program))
			return DataflowOwnerProjectionBeginFailed;
		const unsigned sequence=fNextSequence+1;
		// Beginning invalidates the output activation. Any later preparation
		// failure must pass through the audited recovery path before another
		// program can reuse this owner.
		fPendingSequence=sequence;
		auto preparationFailed=[&](DataflowOwnerResult result) {
			fFaulted=true;
			return result;
		};
		for (unsigned core=0;core<DataflowCoreCount;core++) {
			DataflowWorkerSnapshot snapshot{};
			if (!io.ReadWorkerSnapshot(core,snapshot)
				|| !DataflowWorkerSnapshotReady(snapshot)
				|| !io.CoreReady(core,snapshot))
				return preparationFailed(DataflowOwnerCoreNotReady);
			fSnapshot[core]=snapshot;
		}

		// Materialize and read back every immutable invocation before making any
		// worker runnable. A preparation failure therefore launches no shard.
		for (unsigned core=0;core<DataflowCoreCount;core++) {
			DataflowCoreInvocation invocation;
			if (!DataflowProgramBuilder::BindCore(program,core,invocation))
				return preparationFailed(DataflowOwnerInvalidProgram);
			if (!io.WriteInvocation(core,invocation))
				return preparationFailed(DataflowOwnerInvocationWriteFailed);
			if (!io.InvocationMatches(core,invocation))
				return preparationFailed(DataflowOwnerInvocationReadbackFailed);
		}

		// Publish commandSequence last for each worker. A failed write is
		// ambiguous, so every failure from this point requires all-core audit.
		for (unsigned core=0;core<DataflowCoreCount;core++) {
			if (!io.PublishCommand(core,sequence)) {
				fFaulted=true;
				return DataflowOwnerCommandPublishFailed;
			}
		}

		bool allTerminal=true,allComplete=true;
		for (unsigned core=0;core<DataflowCoreCount;core++) {
			DataflowWorkerSnapshot receipt{};
			const bool observed=io.WaitForCompletion(core,sequence,receipt);
			fSnapshot[core]=receipt;
			if (!observed || !Terminal(receipt,sequence)) {
				allTerminal=false;
				allComplete=false;
			} else if (receipt.state!=DataflowWorkerComplete || receipt.failure) {
				allComplete=false;
			}
		}
		if (!allTerminal || !allComplete) {
			fFaulted=true;
			return DataflowOwnerWorkerFailed;
		}
		if (!io.AuditProjection(program,sequence,fSnapshot)) {
			fFaulted=true;
			return DataflowOwnerAuditFailed;
		}

		for (unsigned core=0;core<DataflowCoreCount;core++) {
			if (!io.Acknowledge(core,sequence)) {
				fFaulted=true;
				return DataflowOwnerAcknowledgementFailed;
			}
		}
		for (unsigned core=0;core<DataflowCoreCount;core++) {
			DataflowWorkerSnapshot snapshot{};
			bool ready=false;
			for (unsigned poll=0;poll<65536;poll++) {
				if (io.ReadWorkerSnapshot(core,snapshot)
					&& DataflowWorkerSnapshotReady(snapshot)
					&& snapshot.completedSequence==sequence) {
					ready=true;
					break;
				}
			}
			if (!ready) {
				fFaulted=true;
				return DataflowOwnerAcknowledgementFailed;
			}
			fSnapshot[core]=snapshot;
		}
		if (!io.CompleteProjection(program,sequence)) {
			fFaulted=true;
			return DataflowOwnerProjectionCommitFailed;
		}
		fNextSequence=sequence;
		fPendingSequence=0;
		return DataflowOwnerComplete;
	}

	template<class IO>
	DataflowOwnerResult RunEmbedding(const DataflowEmbeddingProgram& program,IO& io)
	{
		if (!fInitialized || fFaulted) return DataflowOwnerInvalidState;
		if (!program.Valid()) return DataflowOwnerInvalidProgram;
		if (fNextSequence>=DataflowWorkerSequenceLimit)
			return DataflowOwnerSequenceExhausted;
		if (!io.AuthorizeEmbedding(program))
			return DataflowOwnerProgramUnauthorized;
		if (!io.BeginEmbedding(program))
			return DataflowOwnerProjectionBeginFailed;
		const unsigned sequence=fNextSequence+1;
		fPendingSequence=sequence;
		auto preparationFailed=[&](DataflowOwnerResult result) {
			fFaulted=true;
			return result;
		};
		for (unsigned core=0;core<DataflowCoreCount;core++) {
			DataflowWorkerSnapshot snapshot{};
			if (!io.ReadWorkerSnapshot(core,snapshot)
				|| !DataflowWorkerSnapshotReady(snapshot)
				|| !io.CoreReady(core,snapshot))
				return preparationFailed(DataflowOwnerCoreNotReady);
			fSnapshot[core]=snapshot;
		}
		for (unsigned core=0;core<DataflowCoreCount;core++) {
			DataflowEmbeddingInvocation invocation{};
			if (!DataflowEmbeddingProgramBuilder::BindCore(program,core,invocation))
				return preparationFailed(DataflowOwnerInvalidProgram);
			if (!io.WriteEmbeddingInvocation(core,invocation))
				return preparationFailed(DataflowOwnerInvocationWriteFailed);
			if (!io.EmbeddingInvocationMatches(core,invocation))
				return preparationFailed(DataflowOwnerInvocationReadbackFailed);
		}
		for (unsigned core=0;core<DataflowCoreCount;core++) {
			if (!io.PublishCommand(core,sequence)) {
				fFaulted=true;
				return DataflowOwnerCommandPublishFailed;
			}
		}
		bool allTerminal=true,allComplete=true;
		for (unsigned core=0;core<DataflowCoreCount;core++) {
			DataflowWorkerSnapshot receipt{};
			const bool observed=io.WaitForCompletion(core,sequence,receipt);
			fSnapshot[core]=receipt;
			if (!observed || !Terminal(receipt,sequence)) {
				allTerminal=false;
				allComplete=false;
			} else if (receipt.state!=DataflowWorkerComplete || receipt.failure) {
				allComplete=false;
			}
		}
		if (!allTerminal || !allComplete) {
			fFaulted=true;
			return DataflowOwnerWorkerFailed;
		}
		if (!io.AuditEmbedding(program,sequence,fSnapshot)) {
			fFaulted=true;
			return DataflowOwnerAuditFailed;
		}
		for (unsigned core=0;core<DataflowCoreCount;core++) {
			if (!io.Acknowledge(core,sequence)) {
				fFaulted=true;
				return DataflowOwnerAcknowledgementFailed;
			}
		}
		for (unsigned core=0;core<DataflowCoreCount;core++) {
			DataflowWorkerSnapshot snapshot{};
			bool ready=false;
			for (unsigned poll=0;poll<65536;poll++) {
				if (io.ReadWorkerSnapshot(core,snapshot)
					&& DataflowWorkerSnapshotReady(snapshot)
					&& snapshot.completedSequence==sequence) {
					ready=true;
					break;
				}
			}
			if (!ready) {
				fFaulted=true;
				return DataflowOwnerAcknowledgementFailed;
			}
			fSnapshot[core]=snapshot;
		}
		if (!io.CompleteEmbedding(program,sequence)) {
			fFaulted=true;
			return DataflowOwnerProjectionCommitFailed;
		}
		fNextSequence=sequence;
		fPendingSequence=0;
		return DataflowOwnerComplete;
	}

	template<class IO>
	DataflowOwnerResult Recover(IO& io)
	{
		if (!fFaulted || !fPendingSequence) return DataflowOwnerInvalidState;
		if (!io.AuditAndRecoverAll(fPendingSequence,fSnapshot))
			return DataflowOwnerRecoveryPending;
		unsigned newest=fNextSequence;
		for (unsigned core=0;core<DataflowCoreCount;core++) {
			DataflowWorkerSnapshot snapshot{};
			if (!io.ReadWorkerSnapshot(core,snapshot))
				return DataflowOwnerRecoveryPending;
			if (snapshot.state==DataflowWorkerComplete
				|| snapshot.state==DataflowWorkerFailed) {
				if (!Terminal(snapshot,fPendingSequence)
					|| snapshot.acknowledgedSequence==fPendingSequence
					|| !io.Acknowledge(core,fPendingSequence))
					return DataflowOwnerRecoveryPending;
				if (!io.ReadWorkerSnapshot(core,snapshot))
					return DataflowOwnerRecoveryPending;
			}
			if (!DataflowWorkerSnapshotReady(snapshot)
				|| snapshot.completedSequence>DataflowWorkerSequenceLimit)
				return DataflowOwnerRecoveryPending;
			if (snapshot.completedSequence>newest)
				newest=snapshot.completedSequence;
			fSnapshot[core]=snapshot;
		}
		fNextSequence=newest;
		if (!io.AbortPendingProjection()) return DataflowOwnerRecoveryPending;
		fPendingSequence=0;
		fFaulted=false;
		return DataflowOwnerRecovered;
	}
};

} // namespace PioneerQwen35

#endif
