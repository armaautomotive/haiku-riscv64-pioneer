/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_PROGRAM_H
#define PIONEER_QWEN35_DATAFLOW_PROGRAM_H

#include "PioneerQwen35DeviceLayout.h"
#include "PioneerQwen35DataflowNocProtocol.h"
#include "PioneerQwen35DataflowBarrierLayout.h"
#include "PioneerQwen35DataflowGraphWorkspace.h"
#include "PioneerQwen35ProgramSchedule.h"

namespace PioneerQwen35 {

enum : unsigned {
	DataflowProgramMagic=0x31504635,
	DataflowProgramVersion=3,
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
	DataflowAffinePanelScratchAddress=0x91000,
	DataflowAffineCoefficientAddress=0x91200,
	DataflowAffineMinimumAddress=0x91240,
	DataflowAffineMetadataBytes=16*sizeof(uint32_t),
	DataflowConvHistoryBufferAddress=0x93000,
	DataflowConvNextHistoryBufferAddress=0x93400,
	DataflowConvHistoryBufferBytes=16*3*sizeof(float),
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
	DataflowWorkerMailboxVersion=3,
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
	DataflowProjectionRecurrentVBase=2*DeltaKeyHeads*DeltaWidth,
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
	&& DataflowAffinePanelScratchAddress>=DataflowQ5PanelScratchAddress
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
		<=DataflowConvHistoryBufferAddress
	&& DataflowConvHistoryBufferAddress+DataflowConvHistoryBufferBytes
		<=DataflowConvNextHistoryBufferAddress
	&& DataflowConvNextHistoryBufferAddress+DataflowConvHistoryBufferBytes
		<=DataflowWeightBufferAddress+DataflowWeightBufferPageStride
	&& DataflowWeightBufferAddress+DataflowWeightBufferPageStride
		+DataflowMaxWeightPageBytes<=DataflowLocalBufferEnd,
	"all dataflow L1 pages and reserved affine metadata have disjoint ranges");
static_assert(unsigned(DataflowCoreCount)==unsigned(DataflowBarrierWorkers)
	&& DataflowWorkerMailboxAddress+DataflowWorkerMailboxBytes==DataflowBarrierOutbox,
	"the barrier follows the mailbox and includes every Tensix worker");

struct DataflowCircularBuffer {
	unsigned id,address,pageBytes,pages,bytes,pageStride;
};

// Host and one resident TRISC0 worker share this fixed 64-byte command/receipt
// record. The host owns commandSequence and acknowledgedSequence; the worker
// owns completedSequence, state, failure, and phase. During a failed embedding
// weight read, reserved[0..5] preserve the request address, NoC stage/index and
// target route, observed and expected counters, counter-match/initiator masks,
// transfer size, and outstanding transaction-ID slots until the host
// acknowledges failure. Argmax also owns all six reserved words for its
// result. The fixed 64-byte record does not grow.
struct alignas(32) DataflowWorkerMailbox {
	uint32_t magic,version;
	volatile uint32_t commandSequence,acknowledgedSequence,completedSequence;
	volatile uint32_t state,failure,phase;
	volatile uint32_t shutdownSequence;
	uint32_t reserved[6];
	uint32_t logicalCore;
};
static_assert(sizeof(DataflowWorkerMailbox)==DataflowWorkerMailboxBytes,
	"the resident worker mailbox is one fixed pair of 32-byte records");

inline uint32_t DataflowPackEmbeddingFailureDetailRoute(unsigned detail,
	unsigned route)
{
	return ((route&0xfffU)<<20)|(detail&0xffffU);
}

inline unsigned DataflowEmbeddingFailureRoute(uint32_t packed)
{
	return (packed>>20)&0xfffU;
}

inline unsigned DataflowEmbeddingFailureDetail(uint32_t packed)
{
	return packed&0xffffU;
}

inline uint32_t DataflowPackEmbeddingFailureNoCStateBytes(unsigned state,
	unsigned bytes)
{
	return (state&0xfffU)|((bytes&0xfffffU)<<12);
}

inline unsigned DataflowEmbeddingFailureNoCState(uint32_t packed)
{
	return packed&0xfffU;
}

inline unsigned DataflowEmbeddingFailureBytes(uint32_t packed)
{
	return (packed>>12)&0xfffffU;
}

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
	unsigned inputRows,inputBase,inputFragmentCount,outputFragmentCount;
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
			|| !activeCores || activeCores>DataflowCoreCount || inputRows!=inputs
			|| (inputBase&15) || inputBase>input.rows
			|| inputRows>input.rows-inputBase
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
			if (!assignment.active) {
				if (assignment.groups || assignment.segmentCount) return false;
				continue;
			}
			if (!assignment.groups || !assignment.segmentCount
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
		return active==activeCores && totalGroups==groups;
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
		&& program.inputBase==0 && program.outputRows==program.weights.rows;
}

// Graph projections target exact model roles and their matching manifest-bound
// scratch row spaces. Up and recurrent QKV/gate/Alpha/Beta are admitted;
// unrelated roles, activation aliases, and unreserved output ranges are rejected.
inline bool DataflowProjectionMatchesGraphWorkspace(const DeviceLayout& manifest,
	const DataflowGraphWorkspace& workspace,
	const unsigned (&auditedRoutes)[2][Banks],
	const DataflowProjectionProgram& program,unsigned* inputSlot=nullptr,
	unsigned* workspaceIndex=nullptr,unsigned* modelLayer=nullptr,
	unsigned* modelRole=nullptr)
{
	unsigned targetIndex=0;
	if (manifest.magic!=LayoutMagic || manifest.version!=LayoutVersion
		|| !manifest.context || !workspace.Valid(manifest) || !program.Valid()
		|| !DataflowBankRoutesValid(auditedRoutes)) return false;
	for (unsigned index=DataflowGraphUpWorkspaceIndex;
		index<DataflowGraphWorkspaceBufferCount;index++) {
		if (program.outputRows==DataflowGraphWorkspaceRows(index)
			&& DataflowRowSpaceMatches(workspace.space[index],program.output)) {
			targetIndex=index;
			break;
		}
	}
	if (targetIndex<DataflowGraphUpWorkspaceIndex || program.inputBase!=0
		|| program.inputs!=Hidden || program.inputRows!=Hidden) return false;
	for (unsigned noc=0;noc<2;noc++) for (unsigned bank=0;bank<Banks;bank++)
		if (program.bankRoute[noc][bank]!=auditedRoutes[noc][bank]) return false;

	unsigned foundInput=ActivationBuffers;
	for (unsigned slot=0;slot<ActivationBuffers;slot++)
		if (DataflowRowSpaceMatches(manifest.activation[slot],program.input))
			foundInput=slot;
	if (foundInput>=ActivationBuffers) return false;

	unsigned requiredRole=0;
	if (!DataflowGraphProjectionRole(0,targetIndex,requiredRole)) return false;
	bool modelWeight=false;
	for (unsigned layer=0;layer<Layers;layer++) {
		if (!DataflowGraphProjectionRole(layer,targetIndex,requiredRole)) continue;
		const TensorSpace* expected=manifest.Weight(layer,requiredRole);
		if (expected && expected->type==program.type
			&& expected->inputs==program.inputs
			&& expected->place.rows==program.outputRows
			&& DataflowRowSpaceMatches(expected->place,program.weights)) {
			modelWeight=true;
			if (modelLayer) *modelLayer=layer;
			if (modelRole) *modelRole=requiredRole;
			break;
		}
	}
	if (!modelWeight) return false;
	if (inputSlot) *inputSlot=foundInput;
	if (workspaceIndex) *workspaceIndex=targetIndex;
	return true;
}

// Dense Q/K/V projections use the existing recurrent sidecars as temporary
// scratch. QueryAndGate owns the 8192-row QKV region; Key and Value reuse the
// 4096-row gate region in sequence before being packed into the persistent KV
// cache. Their destinations remain exact workspace bindings, never arbitrary
// caller supplied ranges.
inline bool DataflowProjectionMatchesDenseGraphWorkspace(
	const DeviceLayout& manifest,const DataflowGraphWorkspace& workspace,
	const unsigned (&auditedRoutes)[2][Banks],
	const DataflowProjectionProgram& program,unsigned* inputSlot=nullptr,
	unsigned* workspaceIndex=nullptr,unsigned* modelLayer=nullptr,
	unsigned* modelRole=nullptr)
{
	if (manifest.magic!=LayoutMagic || manifest.version!=LayoutVersion
		|| !manifest.context || !workspace.Valid(manifest) || !program.Valid()
		|| !DataflowBankRoutesValid(auditedRoutes) || program.inputBase!=0
		|| program.inputs!=Hidden || program.inputRows!=Hidden) return false;
	unsigned foundWorkspace=DataflowGraphWorkspaceBufferCount;
	for (unsigned index=DataflowGraphRecurrentQkvWorkspaceIndex;
		index<=DataflowGraphRecurrentGateWorkspaceIndex;index++)
		if (DataflowRowSpaceMatches(workspace.space[index],program.output))
			foundWorkspace=index;
	if (foundWorkspace>=DataflowGraphWorkspaceBufferCount) return false;
	unsigned expectedWorkspace=DataflowGraphWorkspaceBufferCount;
	unsigned foundRole=LayerRoles;
	for (unsigned roleIndex=0;roleIndex<3;roleIndex++) {
		const unsigned role=roleIndex==0?QueryAndGate:roleIndex==1?Key:Value;
		unsigned target=DataflowGraphWorkspaceBufferCount;
		if (!DataflowDenseProjectionTarget(role,target) || target!=foundWorkspace)
			continue;
		for (unsigned layer=0;layer<Layers;layer++) {
			if (!Dense(layer)) continue;
			const TensorSpace* expected=manifest.Weight(layer,role);
			if (expected && expected->type==program.type
				&& expected->inputs==program.inputs
				&& expected->place.rows==program.outputRows
				&& DataflowRowSpaceMatches(expected->place,program.weights)) {
				expectedWorkspace=target;
				foundRole=role;
				if (modelLayer) *modelLayer=layer;
				break;
			}
		}
		if (expectedWorkspace<DataflowGraphWorkspaceBufferCount) break;
	}
	if (expectedWorkspace!=foundWorkspace || foundRole>=LayerRoles) return false;
	unsigned foundInput=ActivationBuffers;
	for (unsigned slot=0;slot<ActivationBuffers;slot++)
		if (DataflowRowSpaceMatches(manifest.activation[slot],program.input))
			foundInput=slot;
	if (foundInput>=ActivationBuffers) return false;
	for (unsigned noc=0;noc<2;noc++) for (unsigned bank=0;bank<Banks;bank++)
		if (program.bankRoute[noc][bank]!=auditedRoutes[noc][bank]) return false;
	if (inputSlot) *inputSlot=foundInput;
	if (workspaceIndex) *workspaceIndex=foundWorkspace;
	if (modelRole) *modelRole=foundRole;
	return true;
}

// AttentionOut is the dense projection that consumes the context written by
// the attention worker. Its input is the first 4096 rows of graph Up scratch,
// which keeps the concurrent query workers from overwriting Q/G source panels.
inline bool DataflowProjectionMatchesDenseAttentionOutput(
	const DeviceLayout& manifest,const DataflowGraphWorkspace& workspace,
	const unsigned (&auditedRoutes)[2][Banks],
	const DataflowProjectionProgram& program,unsigned* outputSlot=nullptr,
	unsigned* modelLayer=nullptr)
{
	if (manifest.magic!=LayoutMagic || manifest.version!=LayoutVersion
		|| !manifest.context || !workspace.Valid(manifest) || !program.Valid()
		|| !DataflowBankRoutesValid(auditedRoutes) || program.inputBase!=0
		|| program.inputs!=DenseHeads*DenseHeadWidth
		|| program.inputRows!=DenseHeads*DenseHeadWidth
		|| program.inputRows>program.input.rows
		|| !DataflowRowSpaceMatches(
			workspace.space[DataflowGraphUpWorkspaceIndex],program.input))
		return false;
	unsigned foundOutput=ActivationBuffers;
	for (unsigned slot=0;slot<ActivationBuffers;slot++)
		if (DataflowRowSpaceMatches(manifest.activation[slot],program.output))
			foundOutput=slot;
	if (foundOutput>=ActivationBuffers) return false;
	for (unsigned noc=0;noc<2;noc++) for (unsigned bank=0;bank<Banks;bank++)
		if (program.bankRoute[noc][bank]!=auditedRoutes[noc][bank]) return false;
	for (unsigned layer=0;layer<Layers;layer++) {
		if (!Dense(layer)) continue;
		const TensorSpace* expected=manifest.Weight(layer,AttentionOut);
		if (expected && expected->type==program.type
			&& expected->inputs==program.inputs
			&& expected->place.rows==program.outputRows
			&& DataflowRowSpaceMatches(expected->place,program.weights)) {
			if (outputSlot) *outputSlot=foundOutput;
			if (modelLayer) *modelLayer=layer;
			return true;
		}
	}
	return false;
}

// The final tied output head consumes the final-normalized hidden activation
// and writes one FP32 logit per vocabulary row into its reserved graph buffer.
// Its 15,520 output groups require all 120 Tensix cores.
inline bool DataflowProjectionMatchesTiedOutput(const DeviceLayout& manifest,
	const DataflowGraphWorkspace& workspace,
	const unsigned (&auditedRoutes)[2][Banks],
	const DataflowProjectionProgram& program,unsigned* inputSlot=nullptr)
{
	if (manifest.magic!=LayoutMagic || manifest.version!=LayoutVersion
		|| !manifest.context || !workspace.Valid(manifest) || !program.Valid()
		|| !DataflowBankRoutesValid(auditedRoutes)
		|| program.inputs!=Hidden || program.inputRows!=Hidden
		|| program.inputBase!=0 || program.outputRows!=Vocabulary
		|| program.activeCores!=DataflowCoreCount
		|| !DataflowRowSpaceMatches(
			workspace.space[DataflowGraphVocabularyWorkspaceIndex],program.output))
		return false;
	const TensorSpace* expected=manifest.Weight(Layers,TiedOutput);
	if (!expected || expected->type!=program.type || expected->inputs!=Hidden
		|| expected->place.rows!=Vocabulary
		|| !DataflowRowSpaceMatches(expected->place,program.weights)) return false;
	unsigned foundInput=ActivationBuffers;
	for (unsigned slot=0;slot<ActivationBuffers;slot++)
		if (DataflowRowSpaceMatches(manifest.activation[slot],program.input))
			foundInput=slot;
	if (foundInput>=ActivationBuffers) return false;
	for (unsigned noc=0;noc<2;noc++) for (unsigned bank=0;bank<Banks;bank++)
		if (program.bankRoute[noc][bank]!=auditedRoutes[noc][bank]) return false;
	if (inputSlot) *inputSlot=foundInput;
	return true;
}

// RecurrentOut consumes only the V channels retained in a recurrent QKV
// workspace. Keep the complete QKV RowSpace as the source so its fragment map
// remains valid, and express the V row offset in the projection descriptor.
inline bool DataflowProjectionMatchesRecurrentOutput(const DeviceLayout& manifest,
	const DataflowGraphWorkspace& workspace,
	const unsigned (&auditedRoutes)[2][Banks],
	const DataflowProjectionProgram& program,unsigned* outputSlot=nullptr,
	unsigned* modelLayer=nullptr)
{
	if (manifest.magic!=LayoutMagic || manifest.version!=LayoutVersion
		|| !manifest.context || !workspace.Valid(manifest) || !program.Valid()
		|| !DataflowBankRoutesValid(auditedRoutes)
		|| program.inputs!=DeltaInner || program.inputRows!=DeltaInner
		|| program.inputBase!=DataflowProjectionRecurrentVBase
		|| program.outputRows!=Hidden
		|| !DataflowRowSpaceMatches(
			workspace.space[DataflowGraphRecurrentQkvWorkspaceIndex],program.input))
		return false;
	for (unsigned noc=0;noc<2;noc++) for (unsigned bank=0;bank<Banks;bank++)
		if (program.bankRoute[noc][bank]!=auditedRoutes[noc][bank]) return false;
	unsigned foundOutput=ActivationBuffers;
	for (unsigned slot=0;slot<ActivationBuffers;slot++)
		if (DataflowRowSpaceMatches(manifest.activation[slot],program.output))
			foundOutput=slot;
	if (foundOutput>=ActivationBuffers) return false;
	for (unsigned layer=0;layer<Layers;layer++) {
		if (Dense(layer)) continue;
		const TensorSpace* expected=manifest.Weight(layer,RecurrentOut);
		if (expected && expected->type==program.type
			&& expected->inputs==program.inputs
			&& expected->place.rows==program.outputRows
			&& DataflowRowSpaceMatches(expected->place,program.weights)) {
			if (outputSlot) *outputSlot=foundOutput;
			if (modelLayer) *modelLayer=layer;
			return true;
		}
	}
	return false;
}

// Compact per-core invocation copied into local L1. The host builder keeps the
// full 120-core schedule; each device core receives only its own assignment.
struct DataflowCoreInvocation {
	unsigned magic,version,core,type,inputs,outputRows,groups,blocks,activeCores,kernelRoles;
	unsigned inputRows,inputBase,inputFragmentCount,outputFragmentCount;
	unsigned bankRoute[2][Banks];
	DataflowCircularBuffer circularBuffer[DataflowCircularBufferCount];
	RowSpace weights,input,output;
	DataflowCoreProgram assignment;

	bool Valid() const
	{
		if (magic!=DataflowProgramMagic || version!=DataflowProgramVersion
			|| core>=DataflowCoreCount || !PioneerQwen35Projection::Supports(type,inputs)
			|| !outputRows || (outputRows&15) || groups!=outputRows/16
			|| blocks!=(inputs>>(type==8?5:8)) || !activeCores
			|| activeCores>DataflowCoreCount
			|| kernelRoles!=DataflowAllRoles || inputRows!=inputs
			|| (inputBase&15) || inputBase>input.rows
			|| inputRows>input.rows-inputBase
			|| !inputFragmentCount || inputFragmentCount>Banks
			|| !outputFragmentCount || outputFragmentCount>Banks
			|| !DataflowBankRoutesValid(bankRoute)
			|| !weights.Valid() || weights.rows!=outputRows || weights.unit!=16
			|| weights.rowBytes*16!=PioneerQwen35Projection::TileBytes(type)*blocks
			|| !input.Valid() || input.rows<inputRows || input.rowBytes!=sizeof(float)
			|| input.unit!=16 || !output.Valid() || output.rows<outputRows
			|| output.rowBytes!=sizeof(float) || output.unit!=16
			|| inputFragmentCount!=input.count || outputFragmentCount!=output.count
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
		if (!assignment.active)
			return !assignment.groups && !assignment.segmentCount;
		if (!assignment.groups || !assignment.segmentCount
			|| assignment.segmentCount>DataflowMaxSegments) return false;
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
		candidate.inputBase=program.inputBase;
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
		const unsigned (&bankRoute)[2][Banks],DataflowProjectionProgram& result,
		PioneerQ35ProjectionProgram& schedule,unsigned inputBase=0)
	{
		result={};
		struct ResultGuard {
			DataflowProjectionProgram& result;
			bool keep=false;
			~ResultGuard() { if (!keep) result={}; }
		} resultGuard{result};
		if (!PioneerQwen35Projection::Supports(weight.type,weight.inputs)
			|| inputRows!=weight.inputs || !input.Valid() || !output.Valid()
			|| (inputBase&15) || inputBase>input.rows
			|| inputRows>input.rows-inputBase
			|| input.rowBytes!=sizeof(float) || input.unit!=16
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
		if (!PioneerQ35BuildProjectionProgram(fragments,weight.place.count,schedule))
			return false;

		DataflowProjectionProgram& candidate=result;
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
		candidate.inputBase=inputBase;
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
			if (!assignment.active) {
				if (scheduled.segmentCount) return false;
				continue;
			}
			if (!scheduled.segmentCount
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
		resultGuard.keep=true;
		return true;
	}

	static bool Build(const TensorSpace& weight,const RowSpace& input,
		unsigned inputRows,const RowSpace& output,
		const unsigned (&bankRoute)[2][Banks],DataflowProjectionProgram& result,
		unsigned inputBase=0)
	{
		PioneerQ35ProjectionProgram schedule{};
		return Build(weight,input,inputRows,output,bankRoute,result,schedule,
			inputBase);
	}
};

} // namespace PioneerQwen35

#endif
