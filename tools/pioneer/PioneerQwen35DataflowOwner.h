/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_OWNER_H
#define PIONEER_QWEN35_DATAFLOW_OWNER_H

#include "PioneerQwen35DataflowProgram.h"
#include "PioneerQwen35DataflowBlackholeIO.h"
#include "PioneerQwen35DataflowEmbeddingProgram.h"
#include "PioneerQwen35DataflowRmsProgram.h"
#include "PioneerQwen35DataflowSiluProgram.h"
#include "PioneerQwen35DataflowResidualProgram.h"
#include "PioneerQwen35DataflowConvProgram.h"
#include "PioneerQwen35DataflowDeltaProgram.h"
#include "PioneerQwen35DataflowDenseAttentionProgram.h"
#include "PioneerQwen35DataflowArgmaxProgram.h"

namespace PioneerQwen35 {

struct DataflowWorkerSnapshot {
	unsigned commandSequence,acknowledgedSequence,completedSequence;
	unsigned state,failure,phase,failureDetail,failureStatus;
};

enum DataflowRecoveryAuditStage : unsigned {
	DataflowRecoveryAuditNone=0,
	DataflowRecoveryAuditInvalidContext=1,
	DataflowRecoveryAuditNoPendingSequence=2,
	DataflowRecoveryAuditWorkerWaitTimeout=3,
	DataflowRecoveryAuditFatalWorker=4,
	DataflowRecoveryAuditSequenceAhead=5,
	DataflowRecoveryAuditRuntimeUnstable=6,
	DataflowRecoveryAuditSnapshotUnreadable=7,
	DataflowRecoveryAuditReadyQueuesNotDrained=8,
	DataflowRecoveryAuditInvalidReceipt=9,
	DataflowRecoveryAuditQueuesUnbalanced=10,
	DataflowRecoveryAuditReceiptChanged=11,
	DataflowRecoveryAuditAcknowledgementWriteFailed=12,
	DataflowRecoveryAuditReadyAfterAckFailed=13,
	DataflowRecoveryAuditRuntimeSelectFailed=14,
	DataflowRecoveryAuditDetailMissing=15
};

enum : unsigned {
	DataflowOwnerOperationNone=0,
	DataflowOwnerOperationEmbedding=1
};

struct DataflowOwnerDiagnostic {
	unsigned operation,result,core,pendingSequence;
	DataflowWorkerSnapshot worker;
	unsigned recoveryAuditStage,recoveryAuditCore,recoveryAuditDetail;
	unsigned failureExpected;
	unsigned failureNoCState,failureNoCOutstandingIdMask;
	unsigned failureAddress,failureRoute,failureBytes;
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
		&& !snapshot.failure && !snapshot.phase
		&& !snapshot.failureDetail && !snapshot.failureStatus;
}

// Host-side orchestration only. IO owns PCI/NoC access and must implement
// CoreReady, AuthorizeProjection, BeginProjection, WriteInvocation,
// InvocationMatches, PublishCommand, WaitForCompletion, AuditProjection,
// Acknowledge, CompleteProjection, ReadWorkerSnapshot, and AuditAndRecoverAll.
// BeginProjection invalidates the output activation before staging; only a
// fully audited and acknowledged result may pass CompleteProjection. The
// owner itself performs no numeric work.
class DataflowProjectionOwner {
#ifdef PIONEER_QWEN35_DEVICE_DISPATCH
	friend class DataflowServiceOwner;
#endif
	bool fInitialized=false;
	bool fFaulted=false;
	unsigned fNextSequence=0;
	unsigned fPendingSequence=0;
	DataflowWorkerSnapshot fSnapshot[DataflowCoreCount]{};
	DataflowOwnerDiagnostic fDiagnostic={DataflowOwnerOperationNone,
		DataflowOwnerComplete,DataflowCoreCount,0,{},DataflowRecoveryAuditNone,
		DataflowCoreCount,0,0,0,0,0,0,0};

#ifdef PIONEER_QWEN35_DEVICE_DISPATCH
	bool CanAdoptGraphSequence(unsigned previous,unsigned trigger) const {
		return fInitialized && !fFaulted && !fPendingSequence && fNextSequence==previous
			&& previous<DataflowWorkerSequenceLimit && trigger==previous+1;
	}
	void AdoptGraphSequence(unsigned trigger) { fNextSequence=trigger; }
#endif

	static bool Terminal(const DataflowWorkerSnapshot& snapshot,unsigned sequence)
	{
		return snapshot.commandSequence==sequence
			&& snapshot.completedSequence==sequence
			&& (snapshot.state==DataflowWorkerComplete
				|| snapshot.state==DataflowWorkerFailed);
	}

	template<class IO>
	static auto ReadFailureExpected(IO& io,unsigned core,unsigned& expected,int)
		-> decltype(io.ReadWorkerFailureExpected(core,expected),bool())
	{
		return io.ReadWorkerFailureExpected(core,expected);
	}

	template<class IO>
	static bool ReadFailureExpected(IO&,unsigned,unsigned&,long)
	{
		return false;
	}

	template<class IO>
	static auto ReadFailureNoC(IO& io,unsigned core,unsigned& state,
		unsigned& outstandingIdMask,int)
		-> decltype(io.ReadWorkerFailureNoC(core,state,outstandingIdMask),bool())
	{
		return io.ReadWorkerFailureNoC(core,state,outstandingIdMask);
	}

	template<class IO>
	static bool ReadFailureNoC(IO&,unsigned,unsigned&,unsigned&,long)
	{
		return false;
	}

	template<class IO>
	static auto ReadFailureRequest(IO& io,unsigned core,unsigned& address,
		unsigned& route,unsigned& bytes,int)
		-> decltype(io.ReadWorkerFailureRequest(core,address,route,bytes),bool())
	{
		return io.ReadWorkerFailureRequest(core,address,route,bytes);
	}

	template<class IO>
	static bool ReadFailureRequest(IO&,unsigned,unsigned&,unsigned&,unsigned&,long)
	{
		return false;
	}

public:
	DataflowProjectionOwner()=default;
	DataflowProjectionOwner(const DataflowProjectionOwner&)=delete;
	DataflowProjectionOwner& operator=(const DataflowProjectionOwner&)=delete;

	bool Initialized() const { return fInitialized; }
	bool Faulted() const { return fFaulted; }
	unsigned NextSequence() const { return fNextSequence; }
	const DataflowOwnerDiagnostic& Diagnostic() const { return fDiagnostic; }

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
	DataflowOwnerResult RunArgmax(const DataflowArgmaxProgram& program,
		unsigned& token,IO& io)
	{
		token=Vocabulary;
		if (!fInitialized || fFaulted) return DataflowOwnerInvalidState;
		if (!program.Valid()) return DataflowOwnerInvalidProgram;
		if (fNextSequence>=DataflowWorkerSequenceLimit)
			return DataflowOwnerSequenceExhausted;
		if (!io.AuthorizeArgmax(program)) return DataflowOwnerProgramUnauthorized;
		if (!io.BeginArgmax(program)) return DataflowOwnerProjectionBeginFailed;
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
			DataflowArgmaxInvocation invocation;
			DataflowArgmaxZeroWords(invocation);
			if (!DataflowArgmaxProgramBuilder::BindCore(program,core,invocation))
				return preparationFailed(DataflowOwnerInvalidProgram);
			if (!io.WriteArgmaxInvocation(core,invocation))
				return preparationFailed(DataflowOwnerInvocationWriteFailed);
			if (!io.ArgmaxInvocationMatches(core,invocation))
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
		DataflowArgmaxResult result{};
		if (!io.ReadArgmaxResult(sequence,result)
			|| !DataflowArgmaxResultValid(result)
			|| result.sequence!=sequence
			|| !io.AuditArgmax(program,sequence,fSnapshot,result)) {
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
		if (!io.CompleteArgmax(program,sequence,result.token)) {
			fFaulted=true;
			return DataflowOwnerProjectionCommitFailed;
		}
		fNextSequence=sequence;
		fPendingSequence=0;
		token=result.token;
		return DataflowOwnerComplete;
	}

	template<class IO>
	DataflowOwnerResult RunEmbedding(const DataflowEmbeddingProgram& program,IO& io)
	{
		fDiagnostic={DataflowOwnerOperationEmbedding,DataflowOwnerComplete,
			DataflowCoreCount,0,{},DataflowRecoveryAuditNone,DataflowCoreCount,
			0,0,0,0,0,0,0};
		auto record=[&](DataflowOwnerResult result,unsigned core,
			const DataflowWorkerSnapshot* worker,bool faulted) {
			fDiagnostic.result=result;
			fDiagnostic.core=core;
			fDiagnostic.pendingSequence=fPendingSequence;
			if (worker) {
				fDiagnostic.worker=*worker;
				if (core<DataflowCoreCount
					&& worker->failure==DataflowFailureEmbeddingWeightRead
					&& worker->phase==5
					&& ((worker->failureDetail>>8)&0xffU)
						==DataflowNocTransferCounterTimeout
					&& (worker->failureDetail&0xffU)>=0x80U
					&& (worker->failureDetail&0xffU)<0x88U)
					ReadFailureExpected(io,core,fDiagnostic.failureExpected,0);
				if (core<DataflowCoreCount
					&& worker->failure==DataflowFailureEmbeddingWeightRead
					&& worker->phase==5
					&& ((worker->failureDetail>>8)&0xffU)
						==DataflowNocTransferCounterTimeout
					&& (worker->failureDetail&0xffU)>=0x80U
					&& (worker->failureDetail&0xffU)<0x88U)
					ReadFailureNoC(io,core,fDiagnostic.failureNoCState,
						fDiagnostic.failureNoCOutstandingIdMask,0);
				if (core<DataflowCoreCount
					&& worker->failure==DataflowFailureEmbeddingWeightRead
					&& worker->phase==5
					&& ((worker->failureDetail>>8)&0xffU)
						==DataflowNocTransferCounterTimeout
					&& (worker->failureDetail&0xffU)>=0x80U
					&& (worker->failureDetail&0xffU)<0x88U)
					ReadFailureRequest(io,core,fDiagnostic.failureAddress,
						fDiagnostic.failureRoute,fDiagnostic.failureBytes,0);
			}
			if (faulted) fFaulted=true;
			return result;
		};
		if (!fInitialized || fFaulted)
			return record(DataflowOwnerInvalidState,DataflowCoreCount,nullptr,false);
		if (!program.Valid())
			return record(DataflowOwnerInvalidProgram,DataflowCoreCount,nullptr,false);
		if (fNextSequence>=DataflowWorkerSequenceLimit)
			return record(DataflowOwnerSequenceExhausted,DataflowCoreCount,nullptr,false);
		if (!io.AuthorizeEmbedding(program))
			return record(DataflowOwnerProgramUnauthorized,DataflowCoreCount,nullptr,false);
		if (!io.BeginEmbedding(program))
			return record(DataflowOwnerProjectionBeginFailed,DataflowCoreCount,nullptr,false);
		const unsigned sequence=fNextSequence+1;
		fPendingSequence=sequence;
		for (unsigned core=0;core<DataflowCoreCount;core++) {
			DataflowWorkerSnapshot snapshot{};
			if (!io.ReadWorkerSnapshot(core,snapshot)
				|| !DataflowWorkerSnapshotReady(snapshot)
				|| !io.CoreReady(core,snapshot))
				return record(DataflowOwnerCoreNotReady,core,&snapshot,true);
			fSnapshot[core]=snapshot;
		}
		for (unsigned core=0;core<DataflowCoreCount;core++) {
			DataflowEmbeddingInvocation invocation{};
			if (!DataflowEmbeddingProgramBuilder::BindCore(program,core,invocation))
				return record(DataflowOwnerInvalidProgram,core,&fSnapshot[core],true);
			if (!io.WriteEmbeddingInvocation(core,invocation))
				return record(DataflowOwnerInvocationWriteFailed,core,
					&fSnapshot[core],true);
			if (!io.EmbeddingInvocationMatches(core,invocation))
				return record(DataflowOwnerInvocationReadbackFailed,core,
					&fSnapshot[core],true);
		}
		for (unsigned core=0;core<DataflowCoreCount;core++) {
			if (!io.PublishCommand(core,sequence)) {
				return record(DataflowOwnerCommandPublishFailed,core,
					&fSnapshot[core],true);
			}
		}
		bool allTerminal=true,allComplete=true;
		unsigned failedCore=DataflowCoreCount;
		DataflowWorkerSnapshot failedSnapshot{};
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
			if (failedCore==DataflowCoreCount
				&& (!observed || !Terminal(receipt,sequence)
					|| receipt.state!=DataflowWorkerComplete || receipt.failure)) {
				failedCore=core;
				failedSnapshot=receipt;
			}
		}
		if (!allTerminal || !allComplete) {
			return record(DataflowOwnerWorkerFailed,failedCore,
				&failedSnapshot,true);
		}
		if (!io.AuditEmbedding(program,sequence,fSnapshot)) {
			return record(DataflowOwnerAuditFailed,DataflowCoreCount,nullptr,true);
		}
		for (unsigned core=0;core<DataflowCoreCount;core++) {
			if (!io.Acknowledge(core,sequence)) {
				return record(DataflowOwnerAcknowledgementFailed,core,
					&fSnapshot[core],true);
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
				return record(DataflowOwnerAcknowledgementFailed,core,
					&snapshot,true);
			}
			fSnapshot[core]=snapshot;
		}
		if (!io.CompleteEmbedding(program,sequence)) {
			return record(DataflowOwnerProjectionCommitFailed,DataflowCoreCount,
				nullptr,true);
		}
		fNextSequence=sequence;
		fPendingSequence=0;
		fDiagnostic={DataflowOwnerOperationEmbedding,DataflowOwnerComplete,
			DataflowCoreCount,0,{},DataflowRecoveryAuditNone,DataflowCoreCount,
			0,0,0,0,0,0,0};
		return DataflowOwnerComplete;
	}

	template<class IO>
	DataflowOwnerResult RunRms(const DataflowRmsProgram& program,IO& io)
	{
		if (!fInitialized || fFaulted) return DataflowOwnerInvalidState;
		if (!program.Valid()) return DataflowOwnerInvalidProgram;
		if (fNextSequence>=DataflowWorkerSequenceLimit)
			return DataflowOwnerSequenceExhausted;
		if (!io.AuthorizeRms(program)) return DataflowOwnerProgramUnauthorized;
		if (!io.BeginRms(program)) return DataflowOwnerProjectionBeginFailed;
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
			DataflowRmsInvocation invocation;
			if (!DataflowRmsProgramBuilder::BindCore(program,core,invocation))
				return preparationFailed(DataflowOwnerInvalidProgram);
			if (!io.WriteRmsInvocation(core,invocation))
				return preparationFailed(DataflowOwnerInvocationWriteFailed);
			if (!io.RmsInvocationMatches(core,invocation))
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
		if (!io.AuditRms(program,sequence,fSnapshot)) {
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
		if (!io.CompleteRms(program,sequence)) {
			fFaulted=true;
			return DataflowOwnerProjectionCommitFailed;
		}
		fNextSequence=sequence;
		fPendingSequence=0;
		return DataflowOwnerComplete;
	}

	template<class IO>
	DataflowOwnerResult RunSilu(const DataflowSiluProgram& program,IO& io)
	{
		if (!fInitialized || fFaulted) return DataflowOwnerInvalidState;
		if (!program.Valid()) return DataflowOwnerInvalidProgram;
		if (fNextSequence>=DataflowWorkerSequenceLimit)
			return DataflowOwnerSequenceExhausted;
		if (!io.AuthorizeSilu(program)) return DataflowOwnerProgramUnauthorized;
		if (!io.BeginSilu(program)) return DataflowOwnerProjectionBeginFailed;
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
			DataflowSiluInvocation invocation;
			if (!DataflowSiluProgramBuilder::BindCore(program,core,invocation))
				return preparationFailed(DataflowOwnerInvalidProgram);
			if (!io.WriteSiluInvocation(core,invocation))
				return preparationFailed(DataflowOwnerInvocationWriteFailed);
			if (!io.SiluInvocationMatches(core,invocation))
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
		if (!io.AuditSilu(program,sequence,fSnapshot)) {
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
		if (!io.CompleteSilu(program,sequence)) {
			fFaulted=true;
			return DataflowOwnerProjectionCommitFailed;
		}
		fNextSequence=sequence;
		fPendingSequence=0;
		return DataflowOwnerComplete;
	}

	template<class IO>
	DataflowOwnerResult RunResidual(const DataflowResidualProgram& program,IO& io)
	{
		if (!fInitialized || fFaulted) return DataflowOwnerInvalidState;
		if (!program.Valid()) return DataflowOwnerInvalidProgram;
		if (fNextSequence>=DataflowWorkerSequenceLimit)
			return DataflowOwnerSequenceExhausted;
		if (!io.AuthorizeResidual(program)) return DataflowOwnerProgramUnauthorized;
		if (!io.BeginResidual(program)) return DataflowOwnerProjectionBeginFailed;
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
			DataflowResidualInvocation invocation{};
			if (!DataflowResidualProgramBuilder::BindCore(program,core,invocation))
				return preparationFailed(DataflowOwnerInvalidProgram);
			if (!io.WriteResidualInvocation(core,invocation))
				return preparationFailed(DataflowOwnerInvocationWriteFailed);
			if (!io.ResidualInvocationMatches(core,invocation))
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
		if (!io.AuditResidual(program,sequence,fSnapshot)) {
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
		if (!io.CompleteResidual(program,sequence)) {
			fFaulted=true;
			return DataflowOwnerProjectionCommitFailed;
		}
		fNextSequence=sequence;
		fPendingSequence=0;
		return DataflowOwnerComplete;
	}

	template<class IO>
	DataflowOwnerResult RunConv(const DataflowConvProgram& program,IO& io)
	{
		if (!fInitialized || fFaulted) return DataflowOwnerInvalidState;
		if (!program.Valid()) return DataflowOwnerInvalidProgram;
		if (fNextSequence>=DataflowWorkerSequenceLimit)
			return DataflowOwnerSequenceExhausted;
		if (!io.AuthorizeConv(program)) return DataflowOwnerProgramUnauthorized;
		if (!io.BeginConv(program)) return DataflowOwnerProjectionBeginFailed;
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
			DataflowConvInvocation invocation{};
			if (!DataflowConvProgramBuilder::BindCore(program,core,invocation))
				return preparationFailed(DataflowOwnerInvalidProgram);
			if (!io.WriteConvInvocation(core,invocation))
				return preparationFailed(DataflowOwnerInvocationWriteFailed);
			if (!io.ConvInvocationMatches(core,invocation))
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
		if (!io.AuditConv(program,sequence,fSnapshot)) {
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
		if (!io.CompleteConv(program,sequence)) {
			fFaulted=true;
			return DataflowOwnerProjectionCommitFailed;
		}
		fNextSequence=sequence;
		fPendingSequence=0;
		return DataflowOwnerComplete;
	}

	template<class IO>
	DataflowOwnerResult RunDelta(const DataflowDeltaProgram& program,IO& io)
	{
#ifdef PIONEER_QWEN35_DELTA_SHARD_COMPILE_ONLY
		// This witness supports delta only through the two-step resident graph.
		// Reject the legacy one-command owner before it can mutate authority.
		(void)program;(void)io;return DataflowOwnerInvalidState;
#else
		if (!fInitialized || fFaulted) return DataflowOwnerInvalidState;
		if (!program.Valid()) return DataflowOwnerInvalidProgram;
		if (fNextSequence>=DataflowWorkerSequenceLimit)
			return DataflowOwnerSequenceExhausted;
		if (!io.AuthorizeDelta(program)) return DataflowOwnerProgramUnauthorized;
		if (!io.BeginDelta(program)) return DataflowOwnerProjectionBeginFailed;
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
			DataflowDeltaInvocation invocation{};
			if (!DataflowDeltaProgramBuilder::BindCore(program,core,invocation))
				return preparationFailed(DataflowOwnerInvalidProgram);
			if (!io.WriteDeltaInvocation(core,invocation))
				return preparationFailed(DataflowOwnerInvocationWriteFailed);
			if (!io.DeltaInvocationMatches(core,invocation))
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
		if (!io.AuditDelta(program,sequence,fSnapshot)) {
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
		if (!io.CompleteDelta(program,sequence)) {
			fFaulted=true;
			return DataflowOwnerProjectionCommitFailed;
		}
		fNextSequence=sequence;
		fPendingSequence=0;
		return DataflowOwnerComplete;
#endif
	}

	template<class IO>
	DataflowOwnerResult RunDenseAttention(
		const DataflowDenseAttentionProgram& program,IO& io)
	{
		if (!fInitialized || fFaulted) return DataflowOwnerInvalidState;
		if (!program.Valid()) return DataflowOwnerInvalidProgram;
		if (fNextSequence>=DataflowWorkerSequenceLimit)
			return DataflowOwnerSequenceExhausted;
		if (!io.AuthorizeDenseAttention(program))
			return DataflowOwnerProgramUnauthorized;
		if (!io.BeginDenseAttention(program))
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
			DataflowDenseAttentionInvocation invocation;
			if (!DataflowDenseAttentionProgramBuilder::BindCore(program,core,
				invocation)) return preparationFailed(DataflowOwnerInvalidProgram);
			if (!io.WriteDenseAttentionInvocation(core,invocation))
				return preparationFailed(DataflowOwnerInvocationWriteFailed);
			if (!io.DenseAttentionInvocationMatches(core,invocation))
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
		if (!io.AuditDenseAttention(program,sequence,fSnapshot)) {
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
		if (!io.CompleteDenseAttention(program,sequence)) {
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
		fDiagnostic.recoveryAuditStage=DataflowRecoveryAuditNone;
		fDiagnostic.recoveryAuditCore=DataflowCoreCount;
		fDiagnostic.recoveryAuditDetail=0;
		if (!io.AuditAndRecoverAll(fPendingSequence,fSnapshot,
			fDiagnostic.recoveryAuditStage,fDiagnostic.recoveryAuditCore,
			fDiagnostic.recoveryAuditDetail)) {
			if (fDiagnostic.recoveryAuditStage==DataflowRecoveryAuditNone)
				fDiagnostic.recoveryAuditStage=DataflowRecoveryAuditDetailMissing;
			return DataflowOwnerRecoveryPending;
		}
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
