/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_KERNELS_H
#define PIONEER_QWEN35_DATAFLOW_KERNELS_H

#include "PioneerQwen35DataflowProgram.h"

namespace PioneerQwen35 {

enum : unsigned {
	DataflowQueueControlBytes=32,
	DataflowQueuePollLimit=0x01000000,
	DataflowFailureReader=1,
	DataflowFailureCompute=2,
	DataflowFailureWriter=3,
	DataflowFailureQueue=4,
	// Embedding failures use the queue slot so the fixed mailbox can report
	// the worker operation without changing its size or receipt ABI.
	DataflowFailureEmbeddingInvocation=5,
	DataflowFailureEmbeddingBind=6,
	DataflowFailureEmbeddingWeightRead=7,
	DataflowFailureEmbeddingUnpack=8,
	DataflowFailureEmbeddingOutputWrite=9
};

struct DataflowQueueControl {
	volatile uint32_t produced,consumed;
	volatile uint32_t readerFailure,computeFailure,writerFailure,queueFailure;
	volatile uint32_t reserved[2];
};
static_assert(sizeof(DataflowQueueControl)==DataflowQueueControlBytes,
	"each role queue owns one fixed 32-byte control record");

struct DataflowQueueView {
	const DataflowCircularBuffer* layout;
	DataflowQueueControl* control;
	uint8_t* local;
};

inline void DataflowFence()
{
#if defined(__riscv)
	__asm__ __volatile__("fence rw,rw" ::: "memory");
#else
	__atomic_thread_fence(__ATOMIC_SEQ_CST);
#endif
}

inline uint32_t DataflowLoadAcquire(const volatile uint32_t& value)
{
#if defined(__riscv)
	const uint32_t result=value;
	__asm__ __volatile__("fence r,rw" ::: "memory");
	return result;
#else
	return __atomic_load_n(&value,__ATOMIC_ACQUIRE);
#endif
}

inline uint32_t DataflowLoadRelaxed(const volatile uint32_t& value)
{
#if defined(__riscv)
	return value;
#else
	return __atomic_load_n(&value,__ATOMIC_RELAXED);
#endif
}

inline void DataflowStoreRelease(volatile uint32_t& value,uint32_t next)
{
#if defined(__riscv)
	__asm__ __volatile__("fence rw,w" ::: "memory");
	value=next;
#else
	__atomic_store_n(&value,next,__ATOMIC_RELEASE);
#endif
}

inline bool DataflowWorkerMailboxStartupValid(
	const DataflowWorkerMailbox& mailbox)
{
	if (mailbox.magic!=DataflowWorkerMailboxMagic
		|| mailbox.version!=DataflowWorkerMailboxVersion
		|| DataflowLoadRelaxed(mailbox.commandSequence)
		|| DataflowLoadRelaxed(mailbox.acknowledgedSequence)
		|| DataflowLoadRelaxed(mailbox.completedSequence)
		|| DataflowLoadRelaxed(mailbox.state)!=DataflowWorkerReady
		|| DataflowLoadRelaxed(mailbox.failure)
		|| DataflowLoadRelaxed(mailbox.phase)
		|| DataflowLoadRelaxed(mailbox.shutdownSequence)) return false;
	for (unsigned i=0;i<6;i++) if (mailbox.reserved[i]) return false;
	return mailbox.logicalCore<DataflowCoreCount;
}

inline volatile uint32_t* DataflowRoleFailureSlot(DataflowQueueControl& control,
	unsigned role)
{
	switch (role) {
	case DataflowFailureReader: return &control.readerFailure;
	case DataflowFailureCompute: return &control.computeFailure;
	case DataflowFailureWriter: return &control.writerFailure;
	default: return &control.queueFailure;
	}
}

// Each device role has one writer per core, so it can publish its own first
// error with an aligned 32-bit store; no RISC-V A extension or libatomic call
// is required by the RV32 work image.
inline void DataflowStoreRoleFailure(DataflowQueueControl& control,
	unsigned reason)
{
	const unsigned failure=reason?reason:DataflowFailureQueue;
	volatile uint32_t* slot=DataflowRoleFailureSlot(control,failure);
	if (!DataflowLoadRelaxed(*slot)) DataflowStoreRelease(*slot,failure);
}

inline unsigned DataflowQueueFailure(const DataflowQueueControl& control)
{
#if defined(__riscv)
	const uint32_t reader=control.readerFailure;
	const uint32_t compute=control.computeFailure;
	const uint32_t writer=control.writerFailure;
	const uint32_t queue=control.queueFailure;
	__asm__ __volatile__("fence r,rw" ::: "memory");
#else
	const uint32_t reader=DataflowLoadAcquire(control.readerFailure);
	const uint32_t compute=DataflowLoadAcquire(control.computeFailure);
	const uint32_t writer=DataflowLoadAcquire(control.writerFailure);
	const uint32_t queue=DataflowLoadAcquire(control.queueFailure);
#endif
	return reader?reader:compute?compute:writer?writer:queue;
}

inline bool DataflowQueueValid(const DataflowQueueView& queue)
{
	if (!queue.layout || !queue.control || !queue.local
		|| queue.layout->pages!=DataflowCircularBufferPages
		|| !queue.layout->pageBytes || (queue.layout->pageBytes&63)
		|| queue.layout->pageBytes>UINT32_MAX/queue.layout->pages
		|| queue.layout->bytes!=queue.layout->pageBytes*queue.layout->pages
		|| !DataflowLocalCircularBufferValid(*queue.layout)
		|| queue.layout->address<DataflowWeightBufferAddress
		|| queue.layout->address>=DataflowLocalBufferEnd)
		return false;
	return true;
}

inline void DataflowQueueFail(DataflowQueueView& queue,unsigned reason)
{
	if (!queue.control) return;
	DataflowStoreRoleFailure(*queue.control,reason);
}

inline bool DataflowQueueReserveBack(DataflowQueueView& queue,uint32_t& ticket)
{
	if (!DataflowQueueValid(queue)) return false;
	for (unsigned attempt=0;attempt<DataflowQueuePollLimit;attempt++) {
		if (DataflowQueueFailure(*queue.control)) return false;
		const uint32_t produced=DataflowLoadRelaxed(queue.control->produced);
		const uint32_t consumed=DataflowLoadAcquire(queue.control->consumed);
		const uint32_t used=produced-consumed;
		if (used>queue.layout->pages) return false;
		if (used<queue.layout->pages) {
			ticket=produced;
			return true;
		}
	}
	return false;
}

inline uint8_t* DataflowQueueWritePage(DataflowQueueView& queue,uint32_t ticket)
{
	if (!DataflowQueueValid(queue)
		|| ticket!=DataflowLoadRelaxed(queue.control->produced)
		|| ticket-DataflowLoadAcquire(queue.control->consumed)>=queue.layout->pages)
		return nullptr;
	return queue.local+(ticket%queue.layout->pages)*queue.layout->pageStride;
}

inline bool DataflowQueuePublishBack(DataflowQueueView& queue,uint32_t ticket)
{
	if (!DataflowQueueValid(queue) || DataflowQueueFailure(*queue.control)
		|| DataflowLoadRelaxed(queue.control->produced)!=ticket
		|| ticket-DataflowLoadAcquire(queue.control->consumed)>=queue.layout->pages)
		return false;
	DataflowStoreRelease(queue.control->produced,ticket+1);
	return true;
}

inline bool DataflowQueueWaitFront(DataflowQueueView& queue,uint32_t& ticket,
	uint8_t*& page)
{
	if (!DataflowQueueValid(queue)) return false;
	for (unsigned attempt=0;attempt<DataflowQueuePollLimit;attempt++) {
		if (DataflowQueueFailure(*queue.control)) return false;
		const uint32_t consumed=DataflowLoadRelaxed(queue.control->consumed);
		const uint32_t produced=DataflowLoadAcquire(queue.control->produced);
		const uint32_t available=produced-consumed;
		if (available>queue.layout->pages) return false;
		if (available) {
			ticket=consumed;
			page=queue.local+(ticket%queue.layout->pages)*queue.layout->pageStride;
			return true;
		}
	}
	return false;
}

inline bool DataflowQueuePopFront(DataflowQueueView& queue,uint32_t ticket)
{
	if (!DataflowQueueValid(queue) || DataflowQueueFailure(*queue.control)
		|| DataflowLoadRelaxed(queue.control->consumed)!=ticket) return false;
	const uint32_t produced=DataflowLoadAcquire(queue.control->produced);
	if (produced-ticket==0 || produced-ticket>queue.layout->pages) return false;
	DataflowStoreRelease(queue.control->consumed,ticket+1);
	return true;
}

inline bool DataflowCoreHeaderValid(const DataflowCoreInvocation& program)
{
	return program.Valid();
}

inline bool DataflowBindQueues(const DataflowCoreInvocation& program,
	DataflowQueueControl* controls,uint8_t* local,DataflowQueueView (&queues)[3])
{
	if (!controls || !local) return false;
	for (unsigned i=0;i<DataflowCircularBufferCount;i++) {
		const auto& buffer=program.circularBuffer[i];
		if (buffer.address<DataflowWeightBufferAddress
			|| buffer.address>=DataflowLocalBufferEnd
			|| buffer.bytes>DataflowLocalBufferEnd-buffer.address) return false;
		queues[i]={&program.circularBuffer[i],&controls[i],
			local+(buffer.address-DataflowWeightBufferAddress)};
		if (!DataflowQueueValid(queues[i])) return false;
	}
	return true;
}

struct DataflowDeviceContext {
	const DataflowCoreInvocation* program;
	DataflowQueueView queues[DataflowCircularBufferCount];
	uint8_t* activationCache;
};

inline void DataflowSignalLocalFailure(unsigned reason)
{
	auto* controls=reinterpret_cast<DataflowQueueControl*>(
		static_cast<uintptr_t>(DataflowQueueStateAddress));
	for (unsigned i=0;i<DataflowCircularBufferCount;i++)
		DataflowStoreRoleFailure(controls[i],reason);
}

inline bool DataflowBindLocalContext(DataflowDeviceContext& context,unsigned failureReason)
{
	DataflowDeviceContext candidate;
	candidate.program=reinterpret_cast<const DataflowCoreInvocation*>(
		static_cast<uintptr_t>(DataflowProgramAddress));
	candidate.activationCache=reinterpret_cast<uint8_t*>(
		static_cast<uintptr_t>(DataflowActivationCacheAddress));
	for (unsigned i=0;i<DataflowCircularBufferCount;i++)
		candidate.queues[i]={nullptr,nullptr,nullptr};
	if (!candidate.program || !candidate.activationCache
		|| !DataflowBindQueues(*candidate.program,
			reinterpret_cast<DataflowQueueControl*>(
				static_cast<uintptr_t>(DataflowQueueStateAddress)),
			reinterpret_cast<uint8_t*>(static_cast<uintptr_t>(DataflowWeightBufferAddress)),
			candidate.queues)
		|| !DataflowCoreHeaderValid(*candidate.program)) {
		DataflowSignalLocalFailure(failureReason);
		return false;
	}
	context=candidate;
	return true;
}

inline unsigned DataflowActivationChunk(unsigned type,unsigned block)
{
	return type==8?block/8:block;
}

// Packed activations remain immutable for the complete projection. The
// projection worker can read a chunk directly instead of copying it to an
// intermediate L1 page for every output group and weight block.
inline const uint8_t* DataflowPackedActivationPage(const uint8_t* cache,
	unsigned type,unsigned inputs,unsigned block)
{
	if (!cache || !PioneerQwen35Projection::Supports(type,inputs)
		|| !inputs || inputs%DataflowActivationChunkInputs) return nullptr;
	const unsigned blocks=inputs/(type==8?32U:DataflowActivationChunkInputs);
	if (block>=blocks) return nullptr;
	const unsigned chunk=DataflowActivationChunk(type,block);
	const unsigned offset=chunk*DataflowInputPageBytes;
	if (offset>DataflowActivationCacheBytes-DataflowInputPageBytes) return nullptr;
	return cache+offset;
}

// Q5_K weights use the Q8_K activation format, just like Q4_K/Q6_K. Keep
// the projection weight type out of the Q8_0-vs-Q8_K activation decision.
inline unsigned DataflowActivationPackingType(unsigned projectionType)
{
	return projectionType==13?12:projectionType;
}

template<class Ops>
bool DataflowPackActivationCache(const DataflowCoreInvocation& program,
	DataflowQueueView& inputs,uint8_t* activationCache,Ops& ops)
{
	if (!DataflowQueueValid(inputs) || !activationCache
		|| program.inputs%DataflowActivationChunkInputs
		|| program.inputs>DataflowActivationCacheBytes/sizeof(float)
		|| DataflowLoadRelaxed(inputs.control->produced)
		|| DataflowLoadRelaxed(inputs.control->consumed)
		|| DataflowQueueFailure(*inputs.control)
		|| inputs.layout->pageBytes!=DataflowInputPageBytes) return false;
	const unsigned chunks=program.inputs/DataflowActivationChunkInputs;
	if (!chunks || uint64_t(chunks)*DataflowInputPageBytes>DataflowActivationCacheBytes)
		return false;
	uint8_t* scratch=DataflowQueueWritePage(inputs,0);
	if (!scratch) return false;
	const uintptr_t cacheStart=reinterpret_cast<uintptr_t>(activationCache);
	const uintptr_t scratchStart=reinterpret_cast<uintptr_t>(scratch);
	if (cacheStart>UINTPTR_MAX-DataflowActivationCacheBytes
		|| scratchStart>UINTPTR_MAX-DataflowInputPageBytes
		|| !(cacheStart+DataflowActivationCacheBytes<=scratchStart
			|| scratchStart+DataflowInputPageBytes<=cacheStart)) return false;
	for (unsigned chunk=0;chunk<chunks;chunk++) {
		if (!ops.PackActivation(DataflowActivationPackingType(program.type),
			program.inputs,chunk,
			reinterpret_cast<const float*>(activationCache),scratch,DataflowInputPageBytes))
			return false;
		for (unsigned word=0;word<DataflowInputPageBytes/4;word++)
			reinterpret_cast<uint32_t*>(activationCache)
				[chunk*(DataflowInputPageBytes/4)+word]
				=reinterpret_cast<const uint32_t*>(scratch)[word];
		DataflowFence();
	}
	return true;
}

// Reader role: fetch one shard's activation vector into local SRAM, then
// stream each weight/activation tile pair through the two input circular
// buffers. Ops are device-side NoC and quantization primitives.
template<class Ops>
bool PioneerQwen35DataflowReader(const DataflowCoreInvocation& program,
	DataflowQueueView& weights,DataflowQueueView& inputs,
	uint8_t* activationCache,Ops& ops)
{
	if (!DataflowCoreHeaderValid(program) || !DataflowQueueValid(weights)
		|| !DataflowQueueValid(inputs) || !activationCache) {
		DataflowQueueFail(weights,DataflowFailureReader);
		DataflowQueueFail(inputs,DataflowFailureReader);
		return false;
	}
	if (!ops.ReadActivation(program.input,program.inputRows,activationCache,
		program.inputRows*sizeof(float))) {
		DataflowQueueFail(weights,DataflowFailureReader);
		DataflowQueueFail(inputs,DataflowFailureReader);
		return false;
	}
	auto failReader=[&]() {
		DataflowQueueFail(weights,DataflowFailureReader);
		DataflowQueueFail(inputs,DataflowFailureReader);
		return false;
	};
	// Quantize each 256-float activation chunk once per core. Its 576-byte
	// representation compacts the 1024-byte FP32 source in this local cache;
	// subsequent output-row groups reuse the packed chunks.
	if (!DataflowPackActivationCache(program,inputs,activationCache,ops))
		return failReader();
	const auto& assignment=program.assignment;
	const unsigned tileBytes=program.circularBuffer[0].pageBytes;
	for (unsigned segmentIndex=0;segmentIndex<assignment.segmentCount;segmentIndex++) {
		const auto& segment=assignment.weight[segmentIndex];
		for (unsigned group=0;group<segment.groups;group++) {
			const uint64_t groupBytes=uint64_t(16)*program.weights.rowBytes;
			const uint64_t groupOffset=uint64_t(group)*groupBytes;
			for (unsigned block=0;block<program.blocks;block++) {
				uint32_t weightTicket,inputTicket;
				if (!DataflowQueueReserveBack(weights,weightTicket)
					|| !DataflowQueueReserveBack(inputs,inputTicket)
					|| weightTicket!=inputTicket) goto failed;
				uint8_t* weightPage=DataflowQueueWritePage(weights,weightTicket);
				uint8_t* inputPage=DataflowQueueWritePage(inputs,inputTicket);
				const unsigned activationChunk=DataflowActivationChunk(program.type,block);
				const uint64_t blockOffset=uint64_t(block)*tileBytes;
				const uint64_t address=uint64_t(segment.address)+groupOffset+blockOffset;
				if (!weightPage || !inputPage || address>UINT32_MAX
					|| groupOffset>segment.bytes || groupBytes>segment.bytes-groupOffset
					|| blockOffset>groupBytes || tileBytes>groupBytes-blockOffset
					|| activationChunk>=program.inputs/DataflowActivationChunkInputs
					|| !ops.ReadWeight(segment.bank,uint32_t(address),weightPage,tileBytes))
					goto failed;
				for (unsigned word=0;word<DataflowInputPageBytes/4;word++)
					reinterpret_cast<uint32_t*>(inputPage)[word]
						=reinterpret_cast<const uint32_t*>(activationCache)
							[activationChunk*(DataflowInputPageBytes/4)+word];
				DataflowFence();
				if (!DataflowQueuePublishBack(weights,weightTicket)
					|| !DataflowQueuePublishBack(inputs,inputTicket)) goto failed;
			}
		}
	}
	return true;
failed:
	return failReader();
}

// Compute role: the only numeric projection operation is supplied as the
// Tensix matrix-engine primitive. No host or scalar dot-product callback is
// provided by this interface.
template<class Ops>
bool PioneerQwen35DataflowCompute(const DataflowCoreInvocation& program,
	DataflowQueueView& weights,DataflowQueueView& inputs,
	DataflowQueueView& outputs,Ops& ops)
{
	static_assert(Ops::TensixMatrixEngine,
		"Qwen35 projection compute requires the Tensix matrix engine");
	if (!DataflowCoreHeaderValid(program) || !DataflowQueueValid(weights)
		|| !DataflowQueueValid(inputs) || !DataflowQueueValid(outputs)
		|| !ops.Bind(program)) {
		DataflowQueueFail(weights,DataflowFailureCompute);
		DataflowQueueFail(inputs,DataflowFailureCompute);
		DataflowQueueFail(outputs,DataflowFailureCompute);
		return false;
	}
	const auto& assignment=program.assignment;
	for (unsigned segmentIndex=0;segmentIndex<assignment.segmentCount;segmentIndex++) {
		const auto& segment=assignment.weight[segmentIndex];
		for (unsigned group=0;group<segment.groups;group++) {
			if (!ops.TensixBeginOutput(program.type)) goto failed;
			for (unsigned block=0;block<program.blocks;block++) {
				uint32_t weightTicket,inputTicket;
				uint8_t *weightPage=nullptr,*inputPage=nullptr;
				if (!DataflowQueueWaitFront(weights,weightTicket,weightPage)
					|| !DataflowQueueWaitFront(inputs,inputTicket,inputPage)
					|| weightTicket!=inputTicket || !weightPage || !inputPage
					|| !ops.TensixMatmul(program.type,block,weightPage,inputPage)
					|| !DataflowQueuePopFront(weights,weightTicket)
					|| !DataflowQueuePopFront(inputs,inputTicket)) goto failed;
			}
			uint32_t outputTicket;
			if (!DataflowQueueReserveBack(outputs,outputTicket)) goto failed;
			uint8_t* outputPage=DataflowQueueWritePage(outputs,outputTicket);
			if (!outputPage || !ops.TensixFinishOutput(outputPage,
				program.circularBuffer[2].pageBytes)
				|| !DataflowQueuePublishBack(outputs,outputTicket)) goto failed;
		}
	}
	return true;
failed:
	DataflowQueueFail(weights,DataflowFailureCompute);
	DataflowQueueFail(inputs,DataflowFailureCompute);
	DataflowQueueFail(outputs,DataflowFailureCompute);
	return false;
}

// Writer role: stream each completed 16-row output page directly to its
// sharded device-memory destination. There is no ROOT/host gather step.
template<class Ops>
bool PioneerQwen35DataflowWriter(const DataflowCoreInvocation& program,
	DataflowQueueView& outputs,Ops& ops)
{
	if (!DataflowCoreHeaderValid(program) || !DataflowQueueValid(outputs)) {
		DataflowQueueFail(outputs,DataflowFailureWriter);
		return false;
	}
	const auto& assignment=program.assignment;
	for (unsigned segmentIndex=0;segmentIndex<assignment.segmentCount;segmentIndex++) {
		const auto& segment=assignment.output[segmentIndex];
		for (unsigned group=0;group<segment.rows/16;group++) {
			uint32_t ticket;uint8_t* page=nullptr;
			const uint64_t address=uint64_t(segment.address)+uint64_t(group)*DataflowOutputPageBytes;
			if (!DataflowQueueWaitFront(outputs,ticket,page) || !page
				|| address>UINT32_MAX
				|| !ops.WriteOutput(segment.bank,uint32_t(address),page,
					DataflowOutputPageBytes)
				|| !DataflowQueuePopFront(outputs,ticket)) {
				DataflowQueueFail(outputs,DataflowFailureWriter);
				return false;
			}
		}
	}
	return true;
}

template<class Ops>
bool PioneerQwen35DataflowReaderEntry(Ops& ops)
{
	DataflowDeviceContext context;
	return DataflowBindLocalContext(context,DataflowFailureReader)
		&& PioneerQwen35DataflowReader(*context.program,context.queues[0],
			context.queues[1],context.activationCache,ops);
}

template<class Ops>
bool PioneerQwen35DataflowComputeEntry(Ops& ops)
{
	static_assert(Ops::TensixMatrixEngine,
		"Qwen35 compute image must own the Tensix matrix engine");
	DataflowDeviceContext context;
	return DataflowBindLocalContext(context,DataflowFailureCompute)
		&& PioneerQwen35DataflowCompute(*context.program,context.queues[0],
			context.queues[1],context.queues[2],ops);
}

template<class Ops>
bool PioneerQwen35DataflowWriterEntry(Ops& ops)
{
	DataflowDeviceContext context;
	return DataflowBindLocalContext(context,DataflowFailureWriter)
		&& PioneerQwen35DataflowWriter(*context.program,context.queues[2],ops);
}

} // namespace PioneerQwen35

#endif
