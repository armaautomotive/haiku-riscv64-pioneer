/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_BLACKHOLE_WORKER_H
#define PIONEER_QWEN35_DATAFLOW_BLACKHOLE_WORKER_H

#include "PioneerQwen35DataflowBlackholeIO.h"
#include "PioneerQwen35DataflowTensixOps.h"

namespace PioneerQwen35 {

// One resident BRISC worker owns one immutable shard. It moves its own pages
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
	if (!reader.Bind(program)) return fail(DataflowFailureReader);
	if (!compute.Bind(program)) return fail(DataflowFailureCompute);
	if (!writer.Bind(program)) return fail(DataflowFailureWriter);

	if (!reader.ReadActivation(program.input,program.inputRows,activationCache,
		program.inputRows*sizeof(float))) return fail(DataflowFailureReader);
	if (!DataflowPackActivationCache(program,queues[1],activationCache,reader))
		return fail(DataflowFailureReader);

	uint8_t* const weightBase=queues[0].local;
	uint8_t* const inputBase=queues[1].local;
	uint8_t* const outputPage=queues[2].local;
	const unsigned tileBytes=program.circularBuffer[0].pageBytes;
	const unsigned inputStride=program.circularBuffer[1].pageStride;
	for (unsigned segmentIndex=0;segmentIndex<program.assignment.segmentCount;
		segmentIndex++) {
		const auto& weight=program.assignment.weight[segmentIndex];
		const auto& output=program.assignment.output[segmentIndex];
		for (unsigned group=0;group<weight.groups;group++) {
			if (!compute.TensixBeginOutput(program.type))
				return fail(DataflowFailureCompute);
			for (unsigned block=0;block<program.blocks;block++) {
				const unsigned page=block%DataflowCircularBufferPages;
				uint8_t* const weightPage=weightBase
					+page*program.circularBuffer[0].pageStride;
				uint8_t* const inputPage=inputBase+page*inputStride;
				const unsigned activationChunk=DataflowActivationChunk(
					program.type,block);
				const uint64_t weightOffset=uint64_t(group)*16*program.weights.rowBytes
					+uint64_t(block)*tileBytes;
				const uint64_t weightAddress=uint64_t(weight.address)+weightOffset;
				if (weightOffset>weight.bytes || tileBytes>weight.bytes-weightOffset
					|| weightAddress>UINT32_MAX
					|| activationChunk>=program.inputs/DataflowActivationChunkInputs
					|| !reader.ReadWeight(weight.bank,uint32_t(weightAddress),
						weightPage,tileBytes)) return fail(DataflowFailureReader);
				const uint8_t* const packed=activationCache
					+activationChunk*DataflowInputPageBytes;
				for (unsigned byte=0;byte<DataflowInputPageBytes;byte++)
					inputPage[byte]=packed[byte];
				DataflowFence();
				if (!compute.TensixMatmul(program.type,block,weightPage,inputPage))
					return fail(DataflowFailureCompute);
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
	DataflowDeviceContext context{};
	if (!DataflowBindLocalContext(context,DataflowFailureQueue)) return false;
	DataflowBlackholeReaderOps reader;
	DataflowBlackholeTensixOps compute;
	DataflowBlackholeWriterOps writer;
	return PioneerQwen35DataflowBlackholeWorker(*context.program,context.queues,
		context.activationCache,reader,compute,writer);
}

} // namespace PioneerQwen35

#endif
