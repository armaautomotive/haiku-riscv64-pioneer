/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_PROGRAM_H
#define PIONEER_QWEN35_DATAFLOW_PROGRAM_H

#include "PioneerQwen35DeviceLayout.h"
#include "PioneerQwen35ProgramSchedule.h"

namespace PioneerQwen35 {

enum : unsigned {
	DataflowProgramMagic=0x31504635,
	DataflowProgramVersion=2,
	DataflowCoreCount=120,
	DataflowMaxSegments=13,
	DataflowCircularBufferCount=3,
	DataflowCircularBufferPages=2,
	DataflowReaderRole=1U<<0,
	DataflowComputeRole=1U<<1,
	DataflowWriterRole=1U<<2,
	DataflowAllRoles=DataflowReaderRole|DataflowComputeRole|DataflowWriterRole,
	DataflowLocalBufferStart=0x80000,
	DataflowWeightBufferAddress=0x90000,
	DataflowWeightBufferPageStride=0x8000,
	DataflowInputBufferAddress=0x92000,
	DataflowOutputBufferAddress=0x92800,
	DataflowActivationCacheAddress=0x80000,
	DataflowActivationCacheBytes=9216*sizeof(float),
	DataflowQueueStateAddress=0x89000,
	DataflowQueueStateBytes=3*32,
	DataflowAffineCoefficientAddress=0x91200,
	DataflowAffineMinimumAddress=0x91240,
	DataflowAffineMetadataBytes=16*sizeof(uint32_t),
	DataflowQ5PanelScratchAddress=0x90fc0,
	DataflowQ5PanelTileBytes=576,
	DataflowQ5PanelScratchBytes=704,
	// 0xa0000..0xa03ff is the existing Tensix destination-init source window.
	// Keep the immutable invocation beyond that window and below root scratch.
	DataflowProgramAddress=0xaa000,
	DataflowProgramRegionBytes=0x1000,
	DataflowProgramRegionEnd=DataflowProgramAddress+DataflowProgramRegionBytes,
	DataflowWorkerMailboxAddress=DataflowProgramRegionEnd,
	DataflowWorkerMailboxBytes=64,
	DataflowWorkerMailboxMagic=0x314d4435,
	DataflowWorkerMailboxVersion=2,
	DataflowWorkerShutdownRequest=0x53544f50,
	DataflowWorkerSequenceLimit=0x7ffffffe,
	DataflowWorkerStarting=0,
	DataflowWorkerReady=1,
	DataflowWorkerRunning=2,
	DataflowWorkerComplete=3,
	DataflowWorkerFailed=4,
	DataflowWorkerStopped=5,
	DataflowWorkerFatal=6,
	DataflowActivationChunkInputs=256,
	DataflowInputPageBytes=576,
	DataflowOutputPageBytes=64,
	DataflowMaxWeightPageBytes=3392,
	DataflowLocalBufferEnd=0xa0000
};

inline bool DataflowBankRoutesValid(const unsigned (&routes)[2][Banks])
{
	for (unsigned noc=0;noc<2;noc++) for (unsigned bank=0;bank<Banks;bank++) {
		if (!routes[noc][bank] || routes[noc][bank]>0xfff) return false;
		for (unsigned prior=0;prior<bank;prior++)
			if (routes[noc][prior]==routes[noc][bank]) return false;
	}
	return true;
}

static_assert((PioneerQwen35Projection::MaxInputs/DataflowActivationChunkInputs)
	*DataflowInputPageBytes<=DataflowActivationCacheBytes,
	"packed projection activations fit in the local activation cache");
static_assert(DataflowActivationCacheAddress+DataflowActivationCacheBytes
	<=DataflowQueueStateAddress
	&& DataflowQueueStateAddress+DataflowQueueStateBytes<=DataflowWeightBufferAddress
	&& DataflowWeightBufferAddress+DataflowMaxWeightPageBytes
		<=DataflowQ5PanelScratchAddress
	&& DataflowQ5PanelScratchAddress+DataflowQ5PanelTileBytes
		==DataflowAffineCoefficientAddress
	&& DataflowAffineCoefficientAddress+DataflowAffineMetadataBytes
		==DataflowAffineMinimumAddress
	&& DataflowAffineMinimumAddress+DataflowAffineMetadataBytes
		==DataflowQ5PanelScratchAddress+DataflowQ5PanelScratchBytes
	&& DataflowAffineMinimumAddress+DataflowAffineMetadataBytes
		<=DataflowInputBufferAddress
	&& DataflowInputBufferAddress+DataflowInputPageBytes*DataflowCircularBufferPages
		<=DataflowOutputBufferAddress
	&& DataflowOutputBufferAddress+DataflowOutputPageBytes*DataflowCircularBufferPages
		<=DataflowWeightBufferAddress+DataflowWeightBufferPageStride
	&& DataflowWeightBufferAddress+DataflowWeightBufferPageStride
		+DataflowMaxWeightPageBytes<=DataflowLocalBufferEnd,
	"all dataflow L1 pages and reserved affine metadata have disjoint ranges");

struct DataflowCircularBuffer {
	unsigned id,address,pageBytes,pages,bytes,pageStride;
};

// Host and one resident BRISC worker share this fixed 64-byte command/receipt
// record. The host owns commandSequence and acknowledgedSequence; the worker
// owns completedSequence, state, failure, and phase.
struct alignas(32) DataflowWorkerMailbox {
	uint32_t magic,version;
	volatile uint32_t commandSequence,acknowledgedSequence,completedSequence;
	volatile uint32_t state,failure,phase;
	volatile uint32_t shutdownSequence;
	uint32_t reserved[7];
};
static_assert(sizeof(DataflowWorkerMailbox)==DataflowWorkerMailboxBytes,
	"the resident worker mailbox is one fixed pair of 32-byte records");

inline bool DataflowLocalRangeValid(unsigned address,unsigned bytes)
{
	return address>=DataflowLocalBufferStart && address<DataflowLocalBufferEnd
		&& bytes<=DataflowLocalBufferEnd-address;
}

inline bool DataflowLocalRangesOverlap(unsigned first,unsigned firstBytes,
	unsigned second,unsigned secondBytes)
{
	return first<uint64_t(second)+secondBytes
		&& second<uint64_t(first)+firstBytes;
}

inline bool DataflowLocalCircularBufferValid(const DataflowCircularBuffer& buffer)
{
	if (!buffer.address || (buffer.address&63) || !buffer.pageBytes
		|| (buffer.pageBytes&63) || buffer.pages!=DataflowCircularBufferPages
		|| buffer.pageBytes>UINT32_MAX/buffer.pages
		|| buffer.bytes!=buffer.pageBytes*buffer.pages
		|| buffer.pageStride<buffer.pageBytes || (buffer.pageStride&63)
		|| buffer.pageStride>UINT32_MAX/(buffer.pages-1)) return false;
	for (unsigned page=0;page<buffer.pages;page++) {
		const uint64_t address=uint64_t(buffer.address)+uint64_t(page)*buffer.pageStride;
		if (address>UINT32_MAX
			|| !DataflowLocalRangeValid(unsigned(address),buffer.pageBytes)) return false;
		const unsigned localAddress=unsigned(address);
		if (DataflowLocalRangesOverlap(localAddress,buffer.pageBytes,
				DataflowActivationCacheAddress,DataflowActivationCacheBytes)
			|| DataflowLocalRangesOverlap(localAddress,buffer.pageBytes,
				DataflowQ5PanelScratchAddress,DataflowQ5PanelScratchBytes)
			|| DataflowLocalRangesOverlap(localAddress,buffer.pageBytes,
				DataflowQueueStateAddress,DataflowQueueStateBytes)
			|| DataflowLocalRangesOverlap(localAddress,buffer.pageBytes,
				DataflowAffineCoefficientAddress,DataflowAffineMetadataBytes)
			|| DataflowLocalRangesOverlap(localAddress,buffer.pageBytes,
				DataflowAffineMinimumAddress,DataflowAffineMetadataBytes))
			return false;
		for (unsigned prior=0;prior<page;prior++) {
			const uint64_t priorAddress=uint64_t(buffer.address)
				+uint64_t(prior)*buffer.pageStride;
			if (DataflowLocalRangesOverlap(localAddress,buffer.pageBytes,
				unsigned(priorAddress),buffer.pageBytes)) return false;
		}
	}
	return true;
}

inline bool DataflowLocalCircularBuffersValid(
	const DataflowCircularBuffer (&buffers)[DataflowCircularBufferCount])
{
	if (!DataflowLocalRangeValid(DataflowActivationCacheAddress,
			DataflowActivationCacheBytes)
		|| !DataflowLocalRangeValid(DataflowQueueStateAddress,DataflowQueueStateBytes)
		|| !DataflowLocalRangeValid(DataflowAffineCoefficientAddress,
			DataflowAffineMetadataBytes)
		|| !DataflowLocalRangeValid(DataflowAffineMinimumAddress,
			DataflowAffineMetadataBytes)
		|| !DataflowLocalRangeValid(DataflowQ5PanelScratchAddress,
			DataflowQ5PanelScratchBytes)
		|| DataflowLocalRangesOverlap(DataflowActivationCacheAddress,
			DataflowActivationCacheBytes,DataflowQueueStateAddress,
			DataflowQueueStateBytes)
		|| DataflowLocalRangesOverlap(DataflowActivationCacheAddress,
			DataflowActivationCacheBytes,DataflowAffineCoefficientAddress,
			DataflowAffineMetadataBytes)
		|| DataflowLocalRangesOverlap(DataflowActivationCacheAddress,
			DataflowActivationCacheBytes,DataflowAffineMinimumAddress,
			DataflowAffineMetadataBytes)
		|| DataflowLocalRangesOverlap(DataflowActivationCacheAddress,
			DataflowActivationCacheBytes,DataflowQ5PanelScratchAddress,
			DataflowQ5PanelScratchBytes)
		|| DataflowLocalRangesOverlap(DataflowQueueStateAddress,
			DataflowQueueStateBytes,DataflowAffineCoefficientAddress,
			DataflowAffineMetadataBytes)
		|| DataflowLocalRangesOverlap(DataflowQueueStateAddress,
			DataflowQueueStateBytes,DataflowAffineMinimumAddress,
			DataflowAffineMetadataBytes)
		|| DataflowLocalRangesOverlap(DataflowQueueStateAddress,
			DataflowQueueStateBytes,DataflowQ5PanelScratchAddress,
			DataflowQ5PanelScratchBytes)
		|| DataflowLocalRangesOverlap(DataflowAffineCoefficientAddress,
			DataflowAffineMetadataBytes,DataflowAffineMinimumAddress,
			DataflowAffineMetadataBytes)) return false;
	for (unsigned c=0;c<DataflowCircularBufferCount;c++) {
		if (!DataflowLocalCircularBufferValid(buffers[c])) return false;
		for (unsigned other=0;other<c;other++) {
			for (unsigned page=0;page<buffers[c].pages;page++) {
				const unsigned address=buffers[c].address+page*buffers[c].pageStride;
				for (unsigned prior=0;prior<buffers[other].pages;prior++) {
					const unsigned priorAddress=buffers[other].address
						+prior*buffers[other].pageStride;
					if (DataflowLocalRangesOverlap(address,buffers[c].pageBytes,
							priorAddress,buffers[other].pageBytes)) return false;
				}
			}
		}
	}
	return true;
}

struct DataflowWeightSegment {
	unsigned bank,address,firstGroup,groups,bytes;
};

struct DataflowOutputSegment {
	unsigned bank,address,firstRow,rows,bytes;
};

struct DataflowCoreProgram {
	unsigned active,segmentCount,groups;
	DataflowWeightSegment weight[DataflowMaxSegments];
	DataflowOutputSegment output[DataflowMaxSegments];
};

// Host-side program metadata only. Numeric work belongs to the reader,
// compute, and writer device roles; no callback here performs inference.
struct DataflowProjectionProgram {
	unsigned magic,version,type,inputs,outputRows,groups,blocks,activeCores,kernelRoles;
	unsigned inputRows,inputFragmentCount,outputFragmentCount;
	// Copied from the validated resident-bank authority; logical bank indices
	// alone are insufficient to issue a device NoC transfer.
	unsigned bankRoute[2][Banks];
	DataflowCircularBuffer circularBuffer[DataflowCircularBufferCount];
	RowSpace weights,input,output;
	DataflowCoreProgram core[DataflowCoreCount];

	bool Valid() const
	{
		if (magic!=DataflowProgramMagic || version!=DataflowProgramVersion
			|| kernelRoles!=DataflowAllRoles || !PioneerQwen35Projection::Supports(type,inputs)
			|| !outputRows || (outputRows&15) || groups!=outputRows/16
			|| activeCores!=DataflowCoreCount || inputRows!=inputs
			|| !inputFragmentCount || inputFragmentCount>Banks
			|| !outputFragmentCount || outputFragmentCount>Banks
			|| !DataflowBankRoutesValid(bankRoute)
			|| !input.Valid() || !output.Valid() || input.rows<inputRows
			|| input.rowBytes!=sizeof(float) || input.unit!=16
			|| output.rows<outputRows || output.rowBytes!=sizeof(float)
			|| output.unit!=16) return false;
		const unsigned tileBytes=PioneerQwen35Projection::TileBytes(type);
		if (!tileBytes || !blocks || !circularBuffer[0].bytes
			|| !circularBuffer[1].bytes || !circularBuffer[2].bytes) return false;
		if (!weights.Valid() || weights.rows!=outputRows || weights.unit!=16
			|| weights.rowBytes*16!=tileBytes*blocks) return false;
		if (!DataflowLocalCircularBuffersValid(circularBuffer)) return false;
		for (unsigned c=0;c<DataflowCircularBufferCount;c++) {
			const auto& buffer=circularBuffer[c];
			const unsigned address=c==0?DataflowWeightBufferAddress
				:c==1?DataflowInputBufferAddress:DataflowOutputBufferAddress;
			if (buffer.id!=(c==2?16U:c) || buffer.address!=address || (buffer.address&63)
				|| !buffer.pageBytes || buffer.pages!=DataflowCircularBufferPages
				|| buffer.bytes!=buffer.pageBytes*buffer.pages
				|| buffer.pageStride!=(c==0?DataflowWeightBufferPageStride:buffer.pageBytes)
				|| !DataflowLocalCircularBufferValid(buffer)
				|| buffer.address<DataflowWeightBufferAddress
				|| buffer.address>=DataflowLocalBufferEnd) return false;
		}
		if (circularBuffer[0].pageBytes!=tileBytes
			|| circularBuffer[1].pageBytes!=DataflowInputPageBytes
			|| circularBuffer[2].pageBytes!=DataflowOutputPageBytes) return false;
		unsigned active=0,totalGroups=0,nextGroup=0;
		for (unsigned c=0;c<DataflowCoreCount;c++) {
			const auto& assignment=core[c];
			if (!assignment.active || !assignment.groups || !assignment.segmentCount
				|| assignment.segmentCount>DataflowMaxSegments) return false;
			active++;
			unsigned coreGroups=0;
			for (unsigned s=0;s<assignment.segmentCount;s++) {
				const auto& weight=assignment.weight[s];
				const auto& outputPart=assignment.output[s];
				if (weight.bank>=Banks || outputPart.bank>=Banks || !weight.address
					|| !weight.groups || weight.firstGroup!=nextGroup || weight.firstGroup>groups
					|| weight.groups>groups-weight.firstGroup
					|| weight.bytes!=weight.groups*16*weights.rowBytes
					|| outputPart.firstRow!=weight.firstGroup*16
					|| outputPart.rows!=weight.groups*16
					|| outputPart.bytes!=outputPart.rows*sizeof(float)) return false;
				Range expectedWeight{},expectedOutput{};
				if (!weights.Resolve(outputPart.firstRow,outputPart.rows,0,weight.bytes,expectedWeight)
					|| expectedWeight.bank!=weight.bank || expectedWeight.address!=weight.address
					|| expectedWeight.bytes!=weight.bytes
					|| !output.Resolve(outputPart.firstRow,outputPart.rows,0,outputPart.bytes,expectedOutput)
					|| expectedOutput.bank!=outputPart.bank || expectedOutput.address!=outputPart.address
					|| expectedOutput.bytes!=outputPart.bytes) return false;
				coreGroups+=weight.groups;
				nextGroup+=weight.groups;
			}
			if (coreGroups!=assignment.groups || coreGroups>groups-totalGroups) return false;
			totalGroups+=coreGroups;
		}
		return active==DataflowCoreCount && totalGroups==groups;
	}
};

inline bool DataflowRowSpaceMatches(const RowSpace& expected,
	const RowSpace& observed)
{
	if (!expected.Valid() || !observed.Valid() || expected.rows!=observed.rows
		|| expected.rowBytes!=observed.rowBytes || expected.count!=observed.count
		|| expected.unit!=observed.unit) return false;
	for (unsigned i=0;i<expected.count;i++) {
		const auto& a=expected.parts[i];
		const auto& b=observed.parts[i];
		if (a.bank!=b.bank || a.address!=b.address || a.firstRow!=b.firstRow
			|| a.rows!=b.rows) return false;
	}
	return true;
}

// The caller must first validate the complete manifest against the pinned
// model plan and initialized-page ledger, and must supply routes from a
// completed physical route audit. This check then restricts a syntactically
// valid program to one exact model tensor and two distinct declared
// activation buffers.
inline bool DataflowProjectionMatchesLayout(const DeviceLayout& manifest,
	const unsigned (&auditedRoutes)[2][Banks],
	const DataflowProjectionProgram& program)
{
	if (manifest.magic!=LayoutMagic || manifest.version!=LayoutVersion
		|| !manifest.context || !program.Valid()
		|| !DataflowBankRoutesValid(auditedRoutes)) return false;
	for (unsigned noc=0;noc<2;noc++) for (unsigned bank=0;bank<Banks;bank++)
		if (program.bankRoute[noc][bank]!=auditedRoutes[noc][bank]) return false;

	bool modelWeight=false;
	for (unsigned layer=0;layer<=Layers;layer++) {
		for (unsigned role=0;role<LayerRoles;role++) {
			const TensorSpace* expected=manifest.Weight(layer,role);
			if (!expected || expected->type!=program.type
				|| expected->inputs!=program.inputs
				|| expected->place.rows!=program.outputRows) continue;
			if (DataflowRowSpaceMatches(expected->place,program.weights)) {
				modelWeight=true;
				break;
			}
		}
		if (modelWeight) break;
	}
	if (!modelWeight) return false;

	unsigned inputSlot=ActivationBuffers,outputSlot=ActivationBuffers;
	for (unsigned slot=0;slot<ActivationBuffers;slot++) {
		if (DataflowRowSpaceMatches(manifest.activation[slot],program.input))
			inputSlot=slot;
		if (DataflowRowSpaceMatches(manifest.activation[slot],program.output))
			outputSlot=slot;
	}
	return inputSlot<ActivationBuffers && outputSlot<ActivationBuffers
		&& inputSlot!=outputSlot && program.inputRows==program.inputs
		&& program.outputRows==program.weights.rows;
}

// Compact per-core invocation copied into local L1. The host builder keeps the
// full 120-core schedule; each device core receives only its own assignment.
struct DataflowCoreInvocation {
	unsigned magic,version,core,type,inputs,outputRows,groups,blocks,activeCores,kernelRoles;
	unsigned inputRows,inputFragmentCount,outputFragmentCount;
	unsigned bankRoute[2][Banks];
	DataflowCircularBuffer circularBuffer[DataflowCircularBufferCount];
	RowSpace weights,input,output;
	DataflowCoreProgram assignment;

	bool Valid() const
	{
		if (magic!=DataflowProgramMagic || version!=DataflowProgramVersion
			|| core>=DataflowCoreCount || !PioneerQwen35Projection::Supports(type,inputs)
			|| !outputRows || (outputRows&15) || groups!=outputRows/16
			|| blocks!=(inputs>>(type==8?5:8)) || activeCores!=DataflowCoreCount
			|| kernelRoles!=DataflowAllRoles || inputRows!=inputs
			|| !inputFragmentCount || inputFragmentCount>Banks
			|| !outputFragmentCount || outputFragmentCount>Banks
			|| !DataflowBankRoutesValid(bankRoute)
			|| !weights.Valid() || weights.rows!=outputRows || weights.unit!=16
			|| weights.rowBytes*16!=PioneerQwen35Projection::TileBytes(type)*blocks
			|| !input.Valid() || input.rows<inputRows || input.rowBytes!=sizeof(float)
			|| input.unit!=16 || !output.Valid() || output.rows<outputRows
			|| output.rowBytes!=sizeof(float) || output.unit!=16
			|| inputFragmentCount!=input.count || outputFragmentCount!=output.count
			|| !assignment.active || !assignment.groups || !assignment.segmentCount
			|| assignment.segmentCount>DataflowMaxSegments
			|| !DataflowLocalCircularBuffersValid(circularBuffer)) return false;
		for (unsigned c=0;c<DataflowCircularBufferCount;c++) {
			const auto& buffer=circularBuffer[c];
			const unsigned address=c==0?DataflowWeightBufferAddress
				:c==1?DataflowInputBufferAddress:DataflowOutputBufferAddress;
			const unsigned pageBytes=c==0?PioneerQwen35Projection::TileBytes(type)
				:c==1?DataflowInputPageBytes:DataflowOutputPageBytes;
			if (buffer.id!=(c==2?16U:c) || buffer.address!=address
				|| buffer.pageBytes!=pageBytes || buffer.pages!=DataflowCircularBufferPages
				|| buffer.bytes!=pageBytes*DataflowCircularBufferPages
				|| buffer.pageStride!=(c==0?DataflowWeightBufferPageStride:pageBytes)
				|| !DataflowLocalCircularBufferValid(buffer)) return false;
		}
		unsigned groupsSeen=0,nextGroup=assignment.weight[0].firstGroup;
		for (unsigned i=0;i<assignment.segmentCount;i++) {
			const auto& weight=assignment.weight[i];
			const auto& outputPart=assignment.output[i];
			if (!weight.groups || weight.firstGroup!=nextGroup || weight.bank>=Banks
				|| weight.firstGroup>=groups || weight.groups>groups-weight.firstGroup
				|| weight.bytes!=uint64_t(weight.groups)*16*weights.rowBytes
				|| weight.address<BankBase || weight.address>=BankLimit
				|| weight.bytes>BankLimit-weight.address || outputPart.bank>=Banks
				|| outputPart.firstRow!=weight.firstGroup*16
				|| outputPart.rows!=weight.groups*16
				|| outputPart.bytes!=outputPart.rows*sizeof(float)
				|| outputPart.address<BankBase || outputPart.address>=BankLimit
				|| outputPart.bytes>BankLimit-outputPart.address) return false;
			Range expectedWeight{},expectedOutput{};
			if (!weights.Resolve(outputPart.firstRow,outputPart.rows,0,weight.bytes,expectedWeight)
				|| expectedWeight.bank!=weight.bank || expectedWeight.address!=weight.address
				|| !output.Resolve(outputPart.firstRow,outputPart.rows,0,outputPart.bytes,expectedOutput)
				|| expectedOutput.bank!=outputPart.bank
				|| expectedOutput.address!=outputPart.address) return false;
			nextGroup+=weight.groups;
			groupsSeen+=weight.groups;
		}
		return groupsSeen==assignment.groups;
	}
};
static_assert(DataflowProgramAddress>=DataflowLocalBufferEnd
	&& DataflowProgramAddress>=0xa0400 && DataflowProgramRegionEnd<=0xb0000
	&& sizeof(DataflowCoreInvocation)<=DataflowProgramRegionBytes
	&& DataflowWorkerMailboxAddress==DataflowProgramRegionEnd
	&& DataflowWorkerMailboxAddress+DataflowWorkerMailboxBytes<=0xb0000,
	"the per-core invocation fits outside local queues and reserved Tensix scratch");

class DataflowProgramBuilder {
	static bool Add(unsigned& total,unsigned value)
	{
		if (value>0xffffffffU-total) return false;
		total+=value;
		return true;
	}

public:
	static bool BindCore(const DataflowProjectionProgram& program,unsigned core,
		DataflowCoreInvocation& result)
	{
		if (core>=DataflowCoreCount || !program.Valid()) return false;
		DataflowCoreInvocation candidate{};
		candidate.magic=program.magic;
		candidate.version=program.version;
		candidate.core=core;
		candidate.type=program.type;
		candidate.inputs=program.inputs;
		candidate.outputRows=program.outputRows;
		candidate.groups=program.groups;
		candidate.blocks=program.blocks;
		candidate.activeCores=program.activeCores;
		candidate.kernelRoles=program.kernelRoles;
		candidate.inputRows=program.inputRows;
		candidate.inputFragmentCount=program.inputFragmentCount;
		candidate.outputFragmentCount=program.outputFragmentCount;
		for (unsigned noc=0;noc<2;noc++) for (unsigned bank=0;bank<Banks;bank++)
			candidate.bankRoute[noc][bank]=program.bankRoute[noc][bank];
		for (unsigned i=0;i<DataflowCircularBufferCount;i++)
			candidate.circularBuffer[i]=program.circularBuffer[i];
		candidate.weights=program.weights;
		candidate.input=program.input;
		candidate.output=program.output;
		candidate.assignment=program.core[core];
		if (!candidate.Valid()) return false;
		result=candidate;
		return true;
	}

	static bool Build(const TensorSpace& weight,const RowSpace& input,
		unsigned inputRows,const RowSpace& output,
		const unsigned (&bankRoute)[2][Banks],DataflowProjectionProgram& result)
	{
		result={};
		if (!PioneerQwen35Projection::Supports(weight.type,weight.inputs)
			|| inputRows!=weight.inputs || !input.Valid() || !output.Valid()
			|| input.rows<inputRows || input.rowBytes!=sizeof(float) || input.unit!=16
			|| output.rows<weight.place.rows || output.rowBytes!=sizeof(float)
			|| !DataflowBankRoutesValid(bankRoute)
			|| output.unit!=16 || !weight.place.Valid() || weight.place.unit!=16
			|| !weight.place.rows || (weight.place.rows&15)) return false;
		const unsigned tileBytes=PioneerQwen35Projection::TileBytes(weight.type);
		const unsigned blocks=weight.inputs>>(weight.type==8?5:8);
		if (!tileBytes || !blocks || weight.place.rowBytes*16!=tileBytes*blocks) return false;

		PioneerQ35WorkFragment fragments[Banks]={};
		for (unsigned f=0;f<weight.place.count;f++) {
			const auto& part=weight.place.parts[f];
			if ((part.firstRow|part.rows)&15) return false;
			fragments[f]={part.firstRow/16,part.rows/16};
		}
		PioneerQ35ProjectionProgram schedule{};
		if (!PioneerQ35BuildProjectionProgram(fragments,weight.place.count,schedule)
			|| schedule.activeCores!=DataflowCoreCount) return false;

		DataflowProjectionProgram candidate{};
		candidate.magic=DataflowProgramMagic;
		candidate.version=DataflowProgramVersion;
		candidate.type=weight.type;
		candidate.inputs=weight.inputs;
		candidate.outputRows=weight.place.rows;
		candidate.groups=schedule.groups;
		candidate.blocks=blocks;
		candidate.activeCores=schedule.activeCores;
		candidate.kernelRoles=DataflowAllRoles;
		candidate.inputRows=inputRows;
		candidate.inputFragmentCount=input.count;
		candidate.outputFragmentCount=output.count;
		for (unsigned noc=0;noc<2;noc++) for (unsigned bank=0;bank<Banks;bank++)
			candidate.bankRoute[noc][bank]=bankRoute[noc][bank];
		candidate.weights=weight.place;
		candidate.input=input;
		candidate.output=output;
		candidate.circularBuffer[0]={0,DataflowWeightBufferAddress,tileBytes,
			DataflowCircularBufferPages,tileBytes*DataflowCircularBufferPages,
			DataflowWeightBufferPageStride};
		candidate.circularBuffer[1]={1,DataflowInputBufferAddress,DataflowInputPageBytes,
			DataflowCircularBufferPages,DataflowInputPageBytes*DataflowCircularBufferPages,
			DataflowInputPageBytes};
		candidate.circularBuffer[2]={16,DataflowOutputBufferAddress,DataflowOutputPageBytes,
			DataflowCircularBufferPages,DataflowOutputPageBytes*DataflowCircularBufferPages,
			DataflowOutputPageBytes};
		for (unsigned core=0;core<DataflowCoreCount;core++) {
			const auto& scheduled=schedule.core[core];
			auto& assignment=candidate.core[core];
			assignment.active=scheduled.groups!=0;
			assignment.groups=scheduled.groups;
			if (!assignment.active || !scheduled.segmentCount
				|| scheduled.segmentCount>PioneerQ35ProgramMaxFragments) return false;
			for (unsigned s=0;s<scheduled.segmentCount;s++) {
				const auto& segment=scheduled.segment[s];
				if (segment.fragment>=weight.place.count) return false;
				const auto& part=weight.place.parts[segment.fragment];
				if (segment.firstGroup<part.firstRow/16
					|| segment.groups>part.rows/16-(segment.firstGroup-part.firstRow/16))
					return false;
				unsigned firstGroup=segment.firstGroup,remaining=segment.groups;
				while (remaining) {
					unsigned outputPartIndex=output.count;
					for (unsigned p=0;p<output.count;p++) {
						const auto& target=output.parts[p];
						if (firstGroup*16>=target.firstRow
							&& firstGroup*16<target.firstRow+target.rows) {
							outputPartIndex=p; break;
						}
					}
					if (outputPartIndex==output.count
						|| assignment.segmentCount>=DataflowMaxSegments) return false;
					const auto& target=output.parts[outputPartIndex];
					const unsigned available=(target.firstRow+target.rows-firstGroup*16)/16;
					const unsigned segmentGroups=remaining<available?remaining:available;
					const unsigned firstRow=firstGroup*16,rows=segmentGroups*16;
					const uint64_t source=uint64_t(part.address)
						+uint64_t(firstRow-part.firstRow)*weight.place.rowBytes;
					const unsigned bytes=rows*weight.place.rowBytes;
					Range destination{};
					if (!segmentGroups || source>UINT32_MAX || bytes>BankLimit-source
						|| !output.Resolve(firstRow,rows,0,rows*sizeof(float),destination)
						|| destination.bank!=target.bank || destination.bank>=Banks
						|| !destination.bytes) return false;
					const unsigned outputSegment=assignment.segmentCount++;
					assignment.weight[outputSegment]={part.bank,unsigned(source),firstGroup,
						segmentGroups,bytes};
					assignment.output[outputSegment]={destination.bank,destination.address,
						firstRow,rows,destination.bytes};
					firstGroup+=segmentGroups;
					remaining-=segmentGroups;
				}
			}
		}
		if (!candidate.Valid()) return false;
		result=candidate;
		return true;
	}
};

} // namespace PioneerQwen35

#endif
