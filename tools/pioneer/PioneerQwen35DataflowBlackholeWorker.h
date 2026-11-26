/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_BLACKHOLE_WORKER_H
#define PIONEER_QWEN35_DATAFLOW_BLACKHOLE_WORKER_H

#include "PioneerQwen35DataflowBlackholeIO.h"
#include "PioneerQwen35DataflowTensixOps.h"
#include "PioneerQwen35DataflowBlackholeEmbeddingWorker.h"
#include "PioneerQwen35DataflowBlackholeRmsWorker.h"
#include "PioneerQwen35DataflowBlackholeSiluWorker.h"
#include "PioneerQwen35DataflowBlackholeResidualWorker.h"
#include "PioneerQwen35DataflowBlackholeConvWorker.h"
#include "PioneerQwen35DataflowBlackholeDeltaWorker.h"
#ifdef PIONEER_QWEN35_DELTA_SHARD_COMPILE_ONLY
#include "PioneerQwen35DataflowDeltaShards.h"
#endif
#include "PioneerQwen35DataflowBlackholeDenseAttentionWorker.h"
#ifdef PIONEER_QWEN35_DENSE_SHARD_COMPILE_ONLY
#include "PioneerQwen35DataflowDenseAttentionShards.h"
#endif
#include "PioneerQwen35DataflowBlackholeArgmaxWorker.h"
#ifdef PIONEER_QWEN35_SHARED_ROLE_GRAPH_WITNESS
#if !defined(PIONEER_QWEN35_DEVICE_DISPATCH) || !defined(PIONEER_QWEN35_RECOVERY_COMPILE_ONLY)
#error Shared-role graph bridge is a non-loadable recovery witness only
#endif
#include "PioneerQwen35DataflowRoleCoordinator.h"
#include "PioneerQwen35DataflowResidentStreamCache.h"
extern "C" unsigned qwen35_shared_compute_coordinate(
	PioneerQwen35::DataflowRoleMailbox*,unsigned,unsigned);
#endif
#ifdef PIONEER_QWEN35_DEVICE_DISPATCH
#include "PioneerQwen35DataflowGraphRunProtocol.h"
extern "C" bool qwen35_graph_run_descriptor(
	const PioneerQwen35::DataflowGraphRunDescriptor*,PioneerQwen35::DataflowWorkerMailbox*);
#endif

namespace PioneerQwen35 {

// One resident TRISC0 worker owns one immutable shard. It moves its own pages
// over NoC 0/1 and issues projection work to the Tensix matrix engine. The
// two L1 pages alternate as bounded staging buffers; there is no CPU dot
// product and no producer/consumer wait between firmware roles.
template<class ReaderOps,class ComputeOps,class WriterOps>
bool PioneerQwen35DataflowBlackholeWorker(
	const DataflowCoreInvocation& program,
	DataflowQueueView (&queues)[DataflowCircularBufferCount],
	uint8_t* activationCache,ReaderOps& reader,ComputeOps& compute,
	WriterOps& writer)
{
	static_assert(ComputeOps::TensixMatrixEngine,
		"Qwen35 projection compute requires the Tensix matrix engine");
	auto fail=[&](unsigned reason) {
		for (unsigned i=0;i<DataflowCircularBufferCount;i++)
			DataflowQueueFail(queues[i],reason);
		return false;
	};
	if (!program.Valid() || !activationCache) return fail(DataflowFailureQueue);
	for (unsigned i=0;i<DataflowCircularBufferCount;i++) {
		if (!DataflowQueueValid(queues[i])
			|| queues[i].layout!=&program.circularBuffer[i]
			|| reinterpret_cast<uintptr_t>(queues[i].local)
				!=program.circularBuffer[i].address
			|| DataflowLoadRelaxed(queues[i].control->produced)
			|| DataflowLoadRelaxed(queues[i].control->consumed)
			|| DataflowQueueFailure(*queues[i].control))
			return fail(DataflowFailureQueue);
	}
	if (reinterpret_cast<uintptr_t>(activationCache)!=DataflowActivationCacheAddress)
		return fail(DataflowFailureQueue);
	// Small graph projections still use the fleet-wide receipt protocol. Cores
	// with no output panel acknowledge the command without reading the full
	// activation or touching the Tensix engine.
	if (!program.assignment.active) return true;
	if (!reader.Bind(program)) return fail(DataflowFailureReader);
	if (!compute.Bind(program)) return fail(DataflowFailureCompute);
	if (!writer.Bind(program)) return fail(DataflowFailureWriter);

	if (!reader.ReadActivation(program.input,program.inputRows,activationCache,
		program.inputRows*sizeof(float))) return fail(DataflowFailureReader);
	if (!DataflowPackActivationCache(program,queues[1],activationCache,reader))
		return fail(DataflowFailureReader);

	uint8_t* const weightBase=queues[0].local;
	uint8_t* const outputPage=queues[2].local;
	const unsigned tileBytes=program.circularBuffer[0].pageBytes;
	for (unsigned segmentIndex=0;segmentIndex<program.assignment.segmentCount;
		segmentIndex++) {
		const auto& weight=program.assignment.weight[segmentIndex];
		const auto& output=program.assignment.output[segmentIndex];
		for (unsigned group=0;group<weight.groups;group++) {
			if (!compute.TensixBeginOutput(program.type))
				return fail(DataflowFailureCompute);
			DataflowNocReadTicket firstRead;
			firstRead.active=0;
			const uint64_t firstOffset=uint64_t(group)*16*program.weights.rowBytes;
			const uint64_t firstAddress=uint64_t(weight.address)+firstOffset;
			uint8_t* const firstWeightPage=weightBase;
			if (firstOffset>weight.bytes || tileBytes>weight.bytes-firstOffset
				|| firstAddress>UINT32_MAX
				|| !reader.BeginWeightRead(weight.bank,uint32_t(firstAddress),
					firstWeightPage,tileBytes,firstRead)
				|| !reader.CompleteWeightRead(firstRead))
				return fail(DataflowFailureReader);
			for (unsigned block=0;block<program.blocks;block++) {
				const unsigned page=block%DataflowCircularBufferPages;
				uint8_t* const weightPage=weightBase
					+page*program.circularBuffer[0].pageStride;
				const uint8_t* const inputPage=DataflowPackedActivationPage(
					activationCache,program.type,program.inputs,block);
				const uint64_t weightOffset=uint64_t(group)*16*program.weights.rowBytes
					+uint64_t(block)*tileBytes;
				const uint64_t weightAddress=uint64_t(weight.address)+weightOffset;
				if (weightOffset>weight.bytes || tileBytes>weight.bytes-weightOffset
					|| weightAddress>UINT32_MAX
					|| !inputPage)
					return fail(DataflowFailureReader);
				DataflowNocReadTicket nextRead;
				nextRead.active=0;
				const bool hasNext=block+1<program.blocks;
				if (hasNext) {
					const uint64_t nextOffset=weightOffset+tileBytes;
					const uint64_t nextAddress=uint64_t(weight.address)+nextOffset;
					uint8_t* const nextPage=weightBase
						+((page+1)%DataflowCircularBufferPages)
							*program.circularBuffer[0].pageStride;
					if (nextOffset>weight.bytes || tileBytes>weight.bytes-nextOffset
						|| nextAddress>UINT32_MAX
						|| !reader.BeginWeightRead(weight.bank,uint32_t(nextAddress),
							nextPage,tileBytes,nextRead))
						return fail(DataflowFailureReader);
				}
				if (!compute.TensixMatmul(program.type,block,weightPage,inputPage))
					return fail(DataflowFailureCompute);
				if (hasNext && !reader.CompleteWeightRead(nextRead))
					return fail(DataflowFailureReader);
			}
			if (!compute.TensixFinishOutput(outputPage,
				program.circularBuffer[2].pageBytes))
				return fail(DataflowFailureCompute);
			const uint64_t outputAddress=uint64_t(output.address)
				+uint64_t(group)*DataflowOutputPageBytes;
			if (group>=output.rows/16 || outputAddress>UINT32_MAX
				|| !writer.WriteOutput(output.bank,uint32_t(outputAddress),
					outputPage,DataflowOutputPageBytes))
				return fail(DataflowFailureWriter);
		}
	}
	return true;
}

inline bool PioneerQwen35DataflowBlackholeWorkerEntry()
{
	DataflowDeviceContext context;
	if (!DataflowBindLocalContext(context,DataflowFailureQueue)) return false;
	DataflowBlackholeReaderOps reader;
	DataflowBlackholeTensixOps compute;
	DataflowBlackholeWriterOps writer;
	return PioneerQwen35DataflowBlackholeWorker(*context.program,context.queues,
		context.activationCache,reader,compute,writer);
}

inline bool DataflowWorkerQueuesClean()
{
	auto* controls=reinterpret_cast<DataflowQueueControl*>(
		static_cast<uintptr_t>(DataflowQueueStateAddress));
	for (unsigned i=0;i<DataflowCircularBufferCount;i++) {
		const auto& control=controls[i];
		if (DataflowLoadAcquire(control.produced)
			|| DataflowLoadAcquire(control.consumed)
			|| DataflowQueueFailure(control)
			|| control.reserved[0] || control.reserved[1]) return false;
	}
	return true;
}

inline void DataflowWorkerPublish(DataflowWorkerMailbox& mailbox,
	unsigned sequence,unsigned state,unsigned failure,unsigned phase)
{
	DataflowStoreRelease(mailbox.failure,failure);
	DataflowStoreRelease(mailbox.phase,phase);
	DataflowStoreRelease(mailbox.completedSequence,sequence);
	DataflowStoreRelease(mailbox.state,state);
}

inline void DataflowWorkerClearReceipt(DataflowWorkerMailbox& mailbox)
{
	for (unsigned i=0;i<sizeof(mailbox.reserved)/sizeof(mailbox.reserved[0]);i++)
		DataflowStoreRelease(mailbox.reserved[i],0);
}

inline bool DataflowWorkerCommandSequenceValid(unsigned completed,
	unsigned acknowledged,unsigned command)
{
	return command && command<=DataflowWorkerSequenceLimit
		&& acknowledged==completed && command>completed;
}

inline bool DataflowWorkerStopRequestValid(const DataflowWorkerMailbox& mailbox)
{
	if (DataflowLoadAcquire(mailbox.shutdownSequence)!=DataflowWorkerShutdownRequest
		|| DataflowLoadAcquire(mailbox.commandSequence)
			!=DataflowLoadAcquire(mailbox.completedSequence)
		|| DataflowLoadAcquire(mailbox.acknowledgedSequence)
			!=DataflowLoadAcquire(mailbox.completedSequence)
		|| DataflowLoadAcquire(mailbox.state)!=DataflowWorkerReady
		|| DataflowLoadAcquire(mailbox.failure)
		|| DataflowLoadAcquire(mailbox.phase)) return false;
	for (unsigned i=0;i<6;i++) if (mailbox.reserved[i]) return false;
	return mailbox.logicalCore<DataflowCoreCount;
}

template<class QueueArray>
inline bool DataflowWorkerResetQueueRecords(QueueArray& controls)
{
	for (unsigned i=0;i<DataflowCircularBufferCount;i++) {
		auto& control=controls[i];
		if (DataflowLoadAcquire(control.produced)
			!=DataflowLoadAcquire(control.consumed)) return false;
	}
	for (unsigned i=0;i<DataflowCircularBufferCount;i++) {
		auto& control=controls[i];
		DataflowStoreRelease(control.produced,0);
		DataflowStoreRelease(control.consumed,0);
		DataflowStoreRelease(control.readerFailure,0);
		DataflowStoreRelease(control.computeFailure,0);
		DataflowStoreRelease(control.writerFailure,0);
		DataflowStoreRelease(control.queueFailure,0);
		control.reserved[0]=control.reserved[1]=0;
	}
	DataflowFence();
	return true;
}

inline bool DataflowWorkerResetQueues()
{
	auto* controls=reinterpret_cast<DataflowQueueControl*>(
		static_cast<uintptr_t>(DataflowQueueStateAddress));
	return DataflowWorkerResetQueueRecords(controls);
}

// Execute one already-authorized local command. The caller owns publication,
// fleet coordination and the decision to reuse or retain the graph.
struct DataflowWorkerExecutionResult {
	bool okay=false;
	uint32_t executionDetail=0;
	unsigned embeddingFailure=0,embeddingDetail=0;
	uint32_t embeddingStatus=0,embeddingExpected=0;
	uint32_t embeddingNoCState=0,embeddingNoCOutstandingIdMask=0;
	uint32_t embeddingAddress=0,embeddingRoute=0,embeddingBytes=0;
};

#ifdef PIONEER_QWEN35_DEVICE_DISPATCH
__attribute__((noinline))
#endif
inline DataflowWorkerExecutionResult DataflowWorkerExecuteAuthorized(
	unsigned core,unsigned sequence,DataflowWorkerMailbox* mailbox)
{
	DataflowWorkerExecutionResult result;
	const unsigned invocationMagic=*reinterpret_cast<volatile uint32_t*>(
		static_cast<uintptr_t>(DataflowProgramAddress));
	bool okay=false;
	unsigned& embeddingFailure=result.embeddingFailure;
	unsigned& embeddingDetail=result.embeddingDetail;
	uint32_t& embeddingStatus=result.embeddingStatus;
	uint32_t& embeddingExpected=result.embeddingExpected;
	uint32_t& embeddingNoCState=result.embeddingNoCState;
	uint32_t& embeddingNoCOutstandingIdMask=result.embeddingNoCOutstandingIdMask;
	uint32_t& embeddingAddress=result.embeddingAddress;
	uint32_t& embeddingRoute=result.embeddingRoute;
	uint32_t& embeddingBytes=result.embeddingBytes;
	if (invocationMagic==DataflowProgramMagic) {
		DataflowDeviceContext context;
		okay=DataflowBindLocalContext(context,DataflowFailureQueue);
		if (okay && context.program->core!=core) {
			DataflowSignalLocalFailure(DataflowFailureQueue);
			okay=false;
		}
		if (okay) {
#ifdef PIONEER_QWEN35_SHARED_ROLE_GRAPH_WITNESS
			// The graph sequence also counts non-projection steps. The role
			// mailbox has its own contiguous sequence for projections only.
			auto* roleBox=reinterpret_cast<DataflowRoleMailbox*>(
				static_cast<uintptr_t>(DataflowRoleMailboxAddress));
			uint32_t roleSequence=0;
			okay=DataflowRoleNextProjectionSequence(*roleBox,roleSequence)
				&& qwen35_shared_compute_coordinate(roleBox,roleSequence,
					DataflowQueuePollLimit)==DataflowRoleCoordinatorComplete;
			if (!okay) DataflowSignalLocalFailure(DataflowFailureCompute);
#else
			DataflowBlackholeReaderOps reader;
			DataflowBlackholeTensixOps compute;
			DataflowBlackholeWriterOps writer;
			okay=PioneerQwen35DataflowBlackholeWorker(*context.program,
				context.queues,context.activationCache,reader,compute,writer);
#endif
		}
	} else if (invocationMagic==DataflowEmbeddingMagic) {
		const auto* embedding=reinterpret_cast<const DataflowEmbeddingInvocation*>(
			static_cast<uintptr_t>(DataflowProgramAddress));
		if (!DataflowWorkerQueuesClean()) {
			embeddingFailure=DataflowFailureQueue;
		} else if (!embedding->Valid() || embedding->core!=core) {
			embeddingFailure=DataflowFailureEmbeddingInvocation;
		} else {
			DataflowBlackholeEmbeddingOps io;
			if (!io.Bind(*embedding)) {
				embeddingFailure=DataflowFailureEmbeddingBind;
			} else {
				okay=PioneerQwen35DataflowEmbedWorker(*embedding,
					reinterpret_cast<uint8_t*>(static_cast<uintptr_t>(
						DataflowWeightBufferAddress)),
					reinterpret_cast<float*>(static_cast<uintptr_t>(
						DataflowOutputBufferAddress)),io,&embeddingFailure,
					&embeddingDetail,&embeddingStatus,&embeddingExpected,
					&embeddingNoCState,&embeddingNoCOutstandingIdMask,
					&embeddingAddress,&embeddingRoute,&embeddingBytes);
			}
		}
		if (!okay) DataflowSignalLocalFailure(embeddingFailure
			?embeddingFailure:DataflowFailureReader);
	} else if (invocationMagic==DataflowRmsMagic) {
		const auto* rms=reinterpret_cast<const DataflowRmsInvocation*>(
			static_cast<uintptr_t>(DataflowProgramAddress));
		unsigned rmsStage=DataflowRmsWorkerInvalid;
		DataflowBlackholeRmsOps io;
		if (DataflowWorkerQueuesClean() && rms->Valid()
			&& rms->core==core) {
			okay=io.Bind(*rms,sequence)
				&& PioneerQwen35DataflowRmsWorker(*rms,sequence,
					reinterpret_cast<float*>(static_cast<uintptr_t>(
						DataflowActivationCacheAddress)),
					reinterpret_cast<float*>(static_cast<uintptr_t>(
						DataflowInputBufferAddress)),
					reinterpret_cast<float*>(static_cast<uintptr_t>(
						DataflowWeightBufferAddress)),
					reinterpret_cast<float*>(static_cast<uintptr_t>(
						DataflowOutputBufferAddress)),io,&rmsStage);
		}
		if (!okay) {
			const auto& diagnostic=io.Diagnostic();
			result.executionDetail=0x80000000U | (rmsStage&0xfU)
				| ((diagnostic.stage&0xfU)<<4) | ((diagnostic.index&0xffU)<<8)
				| ((diagnostic.counterMatchMask&0xffU)<<16)
				| ((diagnostic.initiatorBusyMask&0xfU)<<24);
		}
		if (!okay) DataflowSignalLocalFailure(DataflowFailureReader);
	} else if (invocationMagic==DataflowSiluMagic) {
		const auto* silu=reinterpret_cast<const DataflowSiluInvocation*>(
			static_cast<uintptr_t>(DataflowProgramAddress));
		if (DataflowWorkerQueuesClean() && silu->Valid()
			&& silu->core==core) {
			DataflowBlackholeSiluOps io;
			okay=io.Bind(*silu)
				&& PioneerQwen35DataflowSiluWorker(*silu,
					reinterpret_cast<float*>(static_cast<uintptr_t>(
						DataflowInputBufferAddress)),
					reinterpret_cast<float*>(static_cast<uintptr_t>(
						DataflowInputBufferAddress+DataflowInputPageBytes)),
					reinterpret_cast<float*>(static_cast<uintptr_t>(
						DataflowOutputBufferAddress)),io);
		}
		if (!okay) DataflowSignalLocalFailure(DataflowFailureReader);
	} else if (invocationMagic==DataflowResidualMagic) {
		const auto* residual=reinterpret_cast<const DataflowResidualInvocation*>(
			static_cast<uintptr_t>(DataflowProgramAddress));
		if (DataflowWorkerQueuesClean() && residual->Valid()
			&& residual->core==core) {
			DataflowBlackholeResidualOps io;
			okay=io.Bind(*residual)
				&& PioneerQwen35DataflowResidualWorker(*residual,
					reinterpret_cast<float*>(static_cast<uintptr_t>(
						DataflowInputBufferAddress)),
					reinterpret_cast<float*>(static_cast<uintptr_t>(
						DataflowInputBufferAddress+DataflowInputPageBytes)),
					reinterpret_cast<float*>(static_cast<uintptr_t>(
						DataflowOutputBufferAddress)),io);
		}
		if (!okay) DataflowSignalLocalFailure(DataflowFailureReader);
	} else if (invocationMagic==DataflowConvMagic) {
		const auto* conv=reinterpret_cast<const DataflowConvInvocation*>(
			static_cast<uintptr_t>(DataflowProgramAddress));
		DataflowConvTrace(0,DataflowConvProgressQueues);
		if (DataflowWorkerQueuesClean()
			&& (DataflowConvTrace(0,DataflowConvProgressValidate),conv->Valid())
			&& conv->core==core) {
			DataflowBlackholeConvOps io;
			okay=PioneerQwen35DataflowConvWorker(*conv,
					reinterpret_cast<float*>(static_cast<uintptr_t>(
						DataflowInputBufferAddress)),
					reinterpret_cast<float*>(static_cast<uintptr_t>(
						DataflowWeightBufferAddress)),
					reinterpret_cast<float*>(static_cast<uintptr_t>(
						DataflowConvHistoryBufferAddress)),
					reinterpret_cast<float*>(static_cast<uintptr_t>(
						DataflowConvNextHistoryBufferAddress)),
					reinterpret_cast<float*>(static_cast<uintptr_t>(
						DataflowOutputBufferAddress)),io);
		}
		if (!okay) DataflowSignalLocalFailure(DataflowFailureReader);
#ifndef PIONEER_QWEN35_DELTA_SHARD_COMPILE_ONLY
	} else if (invocationMagic==DataflowDeltaMagic) {
		const auto* delta=reinterpret_cast<const DataflowDeltaInvocation*>(
			static_cast<uintptr_t>(DataflowProgramAddress));
		if (DataflowWorkerQueuesClean() && delta->Valid()
			&& delta->core==core) {
			DataflowBlackholeDeltaOps io;
			okay=io.Bind(*delta)
				&& PioneerQwen35DataflowDeltaWorker(*delta,
					reinterpret_cast<float*>(static_cast<uintptr_t>(
						DataflowInputBufferAddress)),
					reinterpret_cast<float*>(static_cast<uintptr_t>(
						DataflowInputBufferAddress+DataflowInputPageBytes)),
					reinterpret_cast<float*>(static_cast<uintptr_t>(
						DataflowOutputBufferAddress)),
					reinterpret_cast<float*>(static_cast<uintptr_t>(
						DataflowDeltaStateInputAddress)),
					reinterpret_cast<float*>(static_cast<uintptr_t>(
						DataflowDeltaStateOutputAddress)),
					reinterpret_cast<float*>(static_cast<uintptr_t>(
						DataflowActivationCacheAddress)),
					reinterpret_cast<float*>(static_cast<uintptr_t>(
						DataflowActivationCacheAddress
							+DataflowDeltaScalarCount*sizeof(float))),
					reinterpret_cast<float*>(static_cast<uintptr_t>(
						DataflowActivationCacheAddress
							+(DataflowDeltaScalarCount+DeltaWidth)*sizeof(float))),
					reinterpret_cast<float*>(static_cast<uintptr_t>(
						DataflowActivationCacheAddress
							+(DataflowDeltaScalarCount+2*DeltaWidth)*sizeof(float))),io);
		}
		if (!okay) DataflowSignalLocalFailure(DataflowFailureReader);
#endif
#ifdef PIONEER_QWEN35_DELTA_SHARD_COMPILE_ONLY
	} else if (invocationMagic==DataflowDeltaShardMagic) {
		const auto* delta=reinterpret_cast<const DataflowDeltaShardInvocation*>(
			static_cast<uintptr_t>(DataflowProgramAddress));
		if (DataflowWorkerQueuesClean() && delta->Valid() && delta->core==core) {
			DataflowBlackholeDeltaOps io;
			DataflowDeltaShardScratch scratch{
				reinterpret_cast<float*>(static_cast<uintptr_t>(DataflowInputBufferAddress)),
				reinterpret_cast<float*>(static_cast<uintptr_t>(DataflowInputBufferAddress+DataflowInputPageBytes)),
				reinterpret_cast<float*>(static_cast<uintptr_t>(DataflowOutputBufferAddress)),
				reinterpret_cast<float*>(static_cast<uintptr_t>(DataflowDeltaStateInputAddress)),
				reinterpret_cast<float*>(static_cast<uintptr_t>(DataflowDeltaStateOutputAddress)),
				reinterpret_cast<float*>(static_cast<uintptr_t>(DataflowActivationCacheAddress)),
				reinterpret_cast<float*>(static_cast<uintptr_t>(DataflowActivationCacheAddress+DataflowDeltaScalarCount*sizeof(float))),
				reinterpret_cast<float*>(static_cast<uintptr_t>(DataflowActivationCacheAddress+(DataflowDeltaScalarCount+DeltaWidth)*sizeof(float))),
				reinterpret_cast<float*>(static_cast<uintptr_t>(DataflowActivationCacheAddress+(DataflowDeltaScalarCount+2*DeltaWidth)*sizeof(float)))};
			okay=PioneerQwen35DataflowDeltaShardWorker(delta->program,core,
				static_cast<DataflowDeltaShardPhase>(delta->phase),scratch,io);
		}
		if (!okay) DataflowSignalLocalFailure(DataflowFailureReader);
#endif
#ifdef PIONEER_QWEN35_DENSE_SHARD_COMPILE_ONLY
	} else if (invocationMagic==DataflowDenseAttentionShardMagic) {
		const auto* dense=reinterpret_cast<const DataflowDenseAttentionShardInvocation*>(
			static_cast<uintptr_t>(DataflowProgramAddress));
		if (DataflowWorkerQueuesClean() && dense->Valid() && dense->core==core) {
			DataflowBlackholeDenseAttentionOps io;
			float* const norm=reinterpret_cast<float*>(static_cast<uintptr_t>(DataflowDenseNormAddress));
			DataflowDenseAttentionShardScratch scratch{
				reinterpret_cast<float*>(static_cast<uintptr_t>(DataflowDenseKeyTileAddress)),
				reinterpret_cast<float*>(static_cast<uintptr_t>(DataflowDenseValueTileAddress)),
				reinterpret_cast<float*>(static_cast<uintptr_t>(DataflowDenseQueryAddress)),
				reinterpret_cast<float*>(static_cast<uintptr_t>(DataflowDenseGateAddress)),norm,
				reinterpret_cast<float*>(static_cast<uintptr_t>(DataflowDenseRopeAddress)),norm};
			okay=PioneerQwen35DataflowDenseAttentionShardWorker(dense->program,core,scratch,io);
		}
		if (!okay) DataflowSignalLocalFailure(DataflowFailureReader);
#endif
	} else if (invocationMagic==DataflowDenseAttentionMagic) {
		const auto* dense=reinterpret_cast<const DataflowDenseAttentionInvocation*>(
			static_cast<uintptr_t>(DataflowProgramAddress));
		if (DataflowWorkerQueuesClean() && dense->Valid()
#if defined(PIONEER_QWEN35_DENSE_SHARD_COMPILE_ONLY) || defined(PIONEER_QWEN35_FLASH_DECODE_COMPILE_ONLY)
			&& dense->program.mode!=DataflowDenseAttentionRun
#endif
			&& dense->core==core) {
			DataflowBlackholeDenseAttentionOps io;
			float* const norm=reinterpret_cast<float*>(static_cast<uintptr_t>(
				DataflowDenseNormAddress));
			okay=PioneerQwen35DataflowDenseAttentionWorker(*dense,
				reinterpret_cast<float*>(static_cast<uintptr_t>(
					DataflowDenseKeyTileAddress)),
				reinterpret_cast<float*>(static_cast<uintptr_t>(
					DataflowDenseValueTileAddress)),
				reinterpret_cast<float*>(static_cast<uintptr_t>(
					DataflowDenseQueryAddress)),
				reinterpret_cast<float*>(static_cast<uintptr_t>(
					DataflowDenseGateAddress)),norm,
				reinterpret_cast<float*>(static_cast<uintptr_t>(
					DataflowDenseRopeAddress)),norm,io);
		}
		if (!okay) DataflowSignalLocalFailure(DataflowFailureReader);
	} else if (invocationMagic==DataflowArgmaxMagic) {
		const auto* argmax=reinterpret_cast<const DataflowArgmaxInvocation*>(
			static_cast<uintptr_t>(DataflowProgramAddress));
		if (DataflowWorkerQueuesClean() && argmax->Valid()
			&& argmax->core==core) {
			DataflowBlackholeArgmaxOps io;
			okay=io.Bind(*argmax)
				&& PioneerQwen35DataflowArgmaxWorker(*argmax,sequence,
					reinterpret_cast<float*>(static_cast<uintptr_t>(
						DataflowActivationCacheAddress)),*mailbox,io);
		}
		if (!okay) DataflowSignalLocalFailure(DataflowFailureReader);
	} else {
		DataflowSignalLocalFailure(DataflowFailureQueue);
	}
	result.okay=okay;
	return result;
}

// Persistent per-core service loop. A new descriptor is visible only after
// the host publishes commandSequence. The host may acknowledge a result only
// after its physical completion/fault audit; that acknowledgement permits the
// worker to clear local failure slots and accept the next invocation.
[[noreturn]] inline void PioneerQwen35DataflowBlackholeWorkerService()
{
	auto* mailbox=reinterpret_cast<DataflowWorkerMailbox*>(
		static_cast<uintptr_t>(DataflowWorkerMailboxAddress));
	if (!mailbox || !DataflowWorkerMailboxStartupValid(*mailbox)) {
		if (mailbox) DataflowWorkerPublish(*mailbox,
			DataflowLoadRelaxed(mailbox->completedSequence),DataflowWorkerFatal,
			DataflowFailureQueue,1);
		for (;;) asm volatile("" ::: "memory");
	}
	const unsigned core=mailbox->logicalCore;
#ifdef PIONEER_QWEN35_SHARED_ROLE_GRAPH_WITNESS
	// Invalidate stale cache metadata before the worker accepts a command.
	auto* streamCache=reinterpret_cast<DataflowResidentStreamCache*>(
		static_cast<uintptr_t>(DataflowResidentStreamCacheAddress));
	DataflowResidentStreamCacheReset(*streamCache);
#endif
	const unsigned rawLocalRoute=*reinterpret_cast<volatile uint32_t*>(
		static_cast<uintptr_t>(DataflowNoc0Base+DataflowNocLogicalIdOffset));
	uint32_t localRoute=0;
	if (!DataflowNocDecodeLocalRoute(rawLocalRoute,localRoute)
		|| !DataflowNocRouteMatchesWorker(localRoute,core)) {
		DataflowStoreRelease(mailbox->reserved[0],rawLocalRoute);
		DataflowFence();
		DataflowWorkerPublish(*mailbox,0,DataflowWorkerFatal,
			DataflowFailureQueue,2);
		for (;;) asm volatile("" ::: "memory");
	}
	unsigned completed=0;
	for (;;) {
		const unsigned shutdown=DataflowLoadAcquire(mailbox->shutdownSequence);
		if (shutdown) {
			if (shutdown!=DataflowWorkerShutdownRequest
				|| !DataflowWorkerStopRequestValid(*mailbox)
				|| !DataflowWorkerResetQueues()) {
				DataflowWorkerPublish(*mailbox,completed,DataflowWorkerFatal,
					DataflowFailureQueue,7);
				for (;;) asm volatile("" ::: "memory");
			}
			DataflowWorkerPublish(*mailbox,completed,DataflowWorkerStopped,0,0);
			DataflowFence();
#ifdef PIONEER_QWEN35_SHARED_ROLE_GRAPH_WITNESS
			// The driver needs a fixed PC to distinguish the clean stopped
			// loop from a ready or failed worker in the same text image.
			asm volatile(".globl qwen35_dataflow_stopped_park\n"
				"qwen35_dataflow_stopped_park:\n"
				"j qwen35_dataflow_stopped_park\n" ::: "memory");
			__builtin_unreachable();
#else
			for (;;) asm volatile("" ::: "memory");
#endif
		}
		const unsigned sequence=DataflowLoadAcquire(mailbox->commandSequence);
		if (sequence==completed) continue;
		if (!DataflowWorkerCommandSequenceValid(completed,
			DataflowLoadAcquire(mailbox->acknowledgedSequence),sequence)
			|| DataflowLoadAcquire(mailbox->completedSequence)!=completed
			|| DataflowLoadAcquire(mailbox->state)!=DataflowWorkerReady) {
			DataflowWorkerPublish(*mailbox,sequence,DataflowWorkerFatal,
				DataflowFailureQueue,3);
			for (;;) asm volatile("" ::: "memory");
		}
		DataflowWorkerPublish(*mailbox,completed,DataflowWorkerRunning,0,1);
		DataflowFence();
#ifdef PIONEER_QWEN35_DEVICE_DISPATCH
		DataflowWorkerExecutionResult result;
		const auto* command=reinterpret_cast<const volatile uint32_t*>(static_cast<uintptr_t>(DataflowProgramAddress));
		const bool graphCommand=DataflowLoadAcquire(*command)==DataflowGraphRunCommand;
		if (graphCommand) {
			DataflowGraphRunDescriptor descriptor{};
			const auto* source=reinterpret_cast<const volatile uint32_t*>(static_cast<uintptr_t>(DataflowGraphRunAddress));
			auto* target=reinterpret_cast<uint8_t*>(&descriptor);
			for (unsigned i=0;i<sizeof(descriptor)/4;i++) {
				const uint32_t word=DataflowLoadAcquire(source[i]);
				for (unsigned byte=0;byte<4;byte++) target[i*4+byte]=uint8_t(word>>(byte*8));
			}
			result.okay=DataflowGraphRunValid(descriptor,core) && descriptor.triggerSequence==sequence
				&& qwen35_graph_run_descriptor(&descriptor,mailbox);
			if (!result.okay) DataflowSignalLocalFailure(DataflowFailureQueue);
		} else {
#ifdef PIONEER_QWEN35_ACTIVATION_SHARING_WITNESS
			if (DataflowLoadAcquire(*command)==DataflowProgramMagic) {
				auto* reuse=reinterpret_cast<DataflowActivationReuse*>(
					static_cast<uintptr_t>(DataflowActivationReuseAddress));
				if (!DataflowActivationReuseSetStandalone(*reuse,sequence)) {
					DataflowSignalLocalFailure(DataflowFailureQueue);
					result.okay=false;
				} else result=DataflowWorkerExecuteAuthorized(core,sequence,mailbox);
			} else
#endif
				result=DataflowWorkerExecuteAuthorized(core,sequence,mailbox);
		}
#else
		const DataflowWorkerExecutionResult result=DataflowWorkerExecuteAuthorized(core,sequence,mailbox);
#endif
		const bool okay=result.okay;
		unsigned failure=0;
		auto* controls=reinterpret_cast<DataflowQueueControl*>(
			static_cast<uintptr_t>(DataflowQueueStateAddress));
		for (unsigned i=0;i<DataflowCircularBufferCount;i++) {
			const unsigned queueFailure=DataflowQueueFailure(controls[i]);
			if (!failure && queueFailure) failure=queueFailure;
		}
		const unsigned state=okay?DataflowWorkerComplete:DataflowWorkerFailed;
		if (state==DataflowWorkerFailed
			&& failure==DataflowFailureEmbeddingWeightRead
			&& result.embeddingFailure==DataflowFailureEmbeddingWeightRead) {
			DataflowStoreRelease(mailbox->reserved[0],result.embeddingAddress);
			DataflowStoreRelease(mailbox->reserved[1],
				DataflowPackEmbeddingFailureDetailRoute(
					result.embeddingDetail,result.embeddingRoute));
			DataflowStoreRelease(mailbox->reserved[2],result.embeddingStatus);
			DataflowStoreRelease(mailbox->reserved[3],result.embeddingExpected);
			DataflowStoreRelease(mailbox->reserved[4],
				DataflowPackEmbeddingFailureNoCStateBytes(
					result.embeddingNoCState,result.embeddingBytes));
			DataflowStoreRelease(mailbox->reserved[5],
				result.embeddingNoCOutstandingIdMask);
		}
		DataflowWorkerPublish(*mailbox,sequence,state,failure,okay?2:5);
		DataflowFence();
		completed=sequence;
		while (DataflowLoadAcquire(mailbox->acknowledgedSequence)!=completed)
			asm volatile("" ::: "memory");
		bool queuesReset=DataflowWorkerResetQueues();
#ifdef PIONEER_QWEN35_SHARED_ROLE_GRAPH_WITNESS
		// Shared-mode failed acknowledgements are admitted only by the
		// retained graph's all-core fault audit. A failed role may have left
		// valid queue pages unconsumed; discard them only after that audit
		// and after both reader/writer roles have returned.
		if (!queuesReset && graphCommand && !okay && failure) {
			const auto* roles=reinterpret_cast<const DataflowRoleMailbox*>(
				static_cast<uintptr_t>(DataflowRoleMailboxAddress));
			queuesReset=DataflowRoleDiscardFailedQueues(controls,*roles,true);
		}
#endif
		if (!queuesReset) {
			DataflowWorkerPublish(*mailbox,completed,DataflowWorkerFatal,
				DataflowFailureQueue,6);
			for (;;) asm volatile("" ::: "memory");
		}
		DataflowWorkerClearReceipt(*mailbox);
		DataflowWorkerPublish(*mailbox,completed,DataflowWorkerReady,0,0);
		DataflowFence();
	}
}

} // namespace PioneerQwen35

#endif
