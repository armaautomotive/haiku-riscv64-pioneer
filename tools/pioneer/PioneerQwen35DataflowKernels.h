/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_KERNELS_H
#define PIONEER_QWEN35_DATAFLOW_KERNELS_H

#include "PioneerQwen35DataflowProgram.h"

namespace PioneerQwen35 {

enum : unsigned {
	DataflowQueueControlBytes=16,
	DataflowQueuePollLimit=0x01000000,
	DataflowFailureReader=1,
	DataflowFailureCompute=2,
	DataflowFailureWriter=3,
	DataflowFailureQueue=4
};

struct DataflowQueueControl {
	volatile uint32_t produced,consumed,failure,reserved;
};

struct DataflowQueueView {
	const DataflowCircularBuffer* layout;
	DataflowQueueControl* control;
	uint8_t* local;
};

inline void DataflowFence()
{
	__atomic_thread_fence(__ATOMIC_SEQ_CST);
}

inline bool DataflowQueueValid(const DataflowQueueView& queue)
{
	if (!queue.layout || !queue.control || !queue.local
		|| queue.layout->pages!=DataflowCircularBufferPages
		|| !queue.layout->pageBytes || (queue.layout->pageBytes&63)
		|| queue.layout->pageBytes>UINT32_MAX/queue.layout->pages
		|| queue.layout->bytes!=queue.layout->pageBytes*queue.layout->pages
		|| queue.layout->address<DataflowWeightBufferAddress
		|| queue.layout->address>=DataflowLocalBufferEnd
		|| queue.layout->bytes>DataflowLocalBufferEnd-queue.layout->address)
		return false;
	return true;
}

inline void DataflowQueueFail(DataflowQueueView& queue,unsigned reason)
{
	if (!queue.control) return;
	if (!queue.control->failure) queue.control->failure=reason?reason:DataflowFailureQueue;
	DataflowFence();
}

inline bool DataflowQueueReserveBack(DataflowQueueView& queue,uint32_t& ticket)
{
	if (!DataflowQueueValid(queue)) return false;
	for (unsigned attempt=0;attempt<DataflowQueuePollLimit;attempt++) {
		if (queue.control->failure) return false;
		const uint32_t produced=queue.control->produced;
		const uint32_t consumed=queue.control->consumed;
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
	if (!DataflowQueueValid(queue) || ticket!=queue.control->produced
		|| ticket-queue.control->consumed>=queue.layout->pages) return nullptr;
	return queue.local+(ticket%queue.layout->pages)*queue.layout->pageBytes;
}

inline bool DataflowQueuePublishBack(DataflowQueueView& queue,uint32_t ticket)
{
	if (!DataflowQueueValid(queue) || queue.control->failure
		|| queue.control->produced!=ticket
		|| ticket-queue.control->consumed>=queue.layout->pages) return false;
	DataflowFence();
	queue.control->produced=ticket+1;
	DataflowFence();
	return true;
}

inline bool DataflowQueueWaitFront(DataflowQueueView& queue,uint32_t& ticket,
	uint8_t*& page)
{
	if (!DataflowQueueValid(queue)) return false;
	for (unsigned attempt=0;attempt<DataflowQueuePollLimit;attempt++) {
		if (queue.control->failure) return false;
		const uint32_t consumed=queue.control->consumed;
		const uint32_t produced=queue.control->produced;
		const uint32_t available=produced-consumed;
		if (available>queue.layout->pages) return false;
		if (available) {
			ticket=consumed;
			DataflowFence();
			page=queue.local+(ticket%queue.layout->pages)*queue.layout->pageBytes;
			return true;
		}
	}
	return false;
}

inline bool DataflowQueuePopFront(DataflowQueueView& queue,uint32_t ticket)
{
	if (!DataflowQueueValid(queue) || queue.control->failure
		|| queue.control->consumed!=ticket
		|| queue.control->produced-ticket==0
		|| queue.control->produced-ticket>queue.layout->pages) return false;
	DataflowFence();
	queue.control->consumed=ticket+1;
	DataflowFence();
	return true;
}

inline bool DataflowCoreHeaderValid(const DataflowProjectionProgram& program,
	unsigned core)
{
	if (program.magic!=DataflowProgramMagic || program.version!=DataflowProgramVersion)
		return false;
	if (program.kernelRoles!=DataflowAllRoles || core>=DataflowCoreCount
		|| !PioneerQwen35Projection::Supports(program.type,program.inputs)
		|| program.blocks!=(program.inputs>>(program.type==8?5:8))
		|| program.activeCores!=DataflowCoreCount || !program.core[core].active
		|| !program.core[core].segmentCount
		|| program.core[core].segmentCount>DataflowMaxSegments
		|| !program.weights.Valid() || !program.input.Valid() || !program.output.Valid()
		|| program.weights.rows!=program.outputRows || program.weights.unit!=16
		|| program.weights.rowBytes*16
			!=PioneerQwen35Projection::TileBytes(program.type)*program.blocks
		|| program.outputRows!=program.groups*16) return false;
	const auto& assignment=program.core[core];
	unsigned groups=0;
	for (unsigned i=0;i<assignment.segmentCount;i++) {
		const auto& weight=assignment.weight[i];
		const auto& output=assignment.output[i];
		if (!weight.groups || weight.bank>=Banks || !weight.address
			|| weight.firstGroup>=program.groups || weight.groups>program.groups-weight.firstGroup
			|| uint64_t(weight.bytes)!=uint64_t(weight.groups)*16*program.weights.rowBytes
			|| weight.address<BankBase || weight.address>=BankLimit
			|| weight.bytes>BankLimit-weight.address || !output.rows
			|| (output.firstRow|output.rows)&15 || output.bank>=Banks
			|| output.firstRow!=weight.firstGroup*16
			|| output.rows!=weight.groups*16
			|| output.bytes!=output.rows*sizeof(float)
			|| output.address<BankBase || output.address>=BankLimit
			|| output.bytes>BankLimit-output.address || groups>UINT32_MAX-weight.groups)
			return false;
		Range expectedWeight{},expectedOutput{};
		if (!program.weights.Resolve(output.firstRow,output.rows,0,weight.bytes,expectedWeight)
			|| expectedWeight.bank!=weight.bank || expectedWeight.address!=weight.address
			|| !program.output.Resolve(output.firstRow,output.rows,0,output.bytes,expectedOutput)
			|| expectedOutput.bank!=output.bank || expectedOutput.address!=output.address)
			return false;
		groups+=weight.groups;
	}
	return groups==assignment.groups;
}

inline bool DataflowBindQueues(const DataflowProjectionProgram& program,
	DataflowQueueControl* controls,uint8_t* local,DataflowQueueView (&queues)[3])
{
	if (!controls || !local) return false;
	for (unsigned i=0;i<DataflowCircularBufferCount;i++) {
		if (program.circularBuffer[i].address<DataflowWeightBufferAddress) return false;
		queues[i]={&program.circularBuffer[i],&controls[i],
			local+(program.circularBuffer[i].address-DataflowWeightBufferAddress)};
		if (!DataflowQueueValid(queues[i])) return false;
	}
	return true;
}

// Reader role: fetch one shard's activation vector into local SRAM, then
// stream each weight/activation tile pair through the two input circular
// buffers. Ops are device-side NoC and quantization primitives.
template<class Ops>
bool PioneerQwen35DataflowReader(const DataflowProjectionProgram& program,
	unsigned core,DataflowQueueView& weights,DataflowQueueView& inputs,
	uint8_t* activationCache,Ops& ops)
{
	if (!DataflowCoreHeaderValid(program,core) || !DataflowQueueValid(weights)
		|| !DataflowQueueValid(inputs) || !activationCache) return false;
	if (!ops.ReadActivation(program.input,program.inputRows,activationCache,
		program.inputRows*sizeof(float))) {
		DataflowQueueFail(weights,DataflowFailureReader);
		DataflowQueueFail(inputs,DataflowFailureReader);
		return false;
	}
	const auto& assignment=program.core[core];
	const unsigned tileBytes=program.circularBuffer[0].pageBytes;
	for (unsigned segmentIndex=0;segmentIndex<assignment.segmentCount;segmentIndex++) {
		const auto& segment=assignment.weight[segmentIndex];
		for (unsigned group=0;group<segment.groups;group++) {
			for (unsigned block=0;block<program.blocks;block++) {
				uint32_t weightTicket,inputTicket;
				if (!DataflowQueueReserveBack(weights,weightTicket)
					|| !DataflowQueueReserveBack(inputs,inputTicket)
					|| weightTicket!=inputTicket) goto failed;
				uint8_t* weightPage=DataflowQueueWritePage(weights,weightTicket);
				uint8_t* inputPage=DataflowQueueWritePage(inputs,inputTicket);
				const uint64_t address=uint64_t(segment.address)
					+uint64_t(group)*program.weights.rowBytes
					+uint64_t(block)*tileBytes;
				if (!weightPage || !inputPage || address>UINT32_MAX
					|| tileBytes>segment.bytes-uint64_t(group)*program.weights.rowBytes
					|| uint64_t(block)*tileBytes+tileBytes>program.weights.rowBytes
					|| !ops.ReadWeight(segment.bank,uint32_t(address),weightPage,tileBytes)
					|| !ops.PackActivation(program.type,program.inputs,block,
						reinterpret_cast<const float*>(activationCache),inputPage,
						program.circularBuffer[1].pageBytes)) goto failed;
				if (!DataflowQueuePublishBack(weights,weightTicket)
					|| !DataflowQueuePublishBack(inputs,inputTicket)) goto failed;
			}
		}
	}
	return true;
failed:
	DataflowQueueFail(weights,DataflowFailureReader);
	DataflowQueueFail(inputs,DataflowFailureReader);
	return false;
}

// Compute role: the only numeric projection operation is supplied as the
// Tensix matrix-engine primitive. No host or scalar dot-product callback is
// provided by this interface.
template<class Ops>
bool PioneerQwen35DataflowCompute(const DataflowProjectionProgram& program,
	unsigned core,DataflowQueueView& weights,DataflowQueueView& inputs,
	DataflowQueueView& outputs,Ops& ops)
{
	static_assert(Ops::TensixMatrixEngine,
		"Qwen35 projection compute requires the Tensix matrix engine");
	if (!DataflowCoreHeaderValid(program,core) || !DataflowQueueValid(weights)
		|| !DataflowQueueValid(inputs) || !DataflowQueueValid(outputs)) return false;
	const auto& assignment=program.core[core];
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
					|| !ops.TensixMatmul(program.type,weightPage,inputPage)
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
bool PioneerQwen35DataflowWriter(const DataflowProjectionProgram& program,
	unsigned core,DataflowQueueView& outputs,Ops& ops)
{
	if (!DataflowCoreHeaderValid(program,core) || !DataflowQueueValid(outputs)) return false;
	const auto& assignment=program.core[core];
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

} // namespace PioneerQwen35

#endif
