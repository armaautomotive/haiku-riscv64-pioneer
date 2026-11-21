/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_GRAPH_WORKSPACE_H
#define PIONEER_QWEN35_DATAFLOW_GRAPH_WORKSPACE_H

#include "PioneerQwen35ActivationPlan.h"
#include "PioneerQwen35DeviceLayout.h"

namespace PioneerQwen35 {

enum : unsigned {
	DataflowGraphResidualWorkspaceIndex=0,
	DataflowGraphUpWorkspaceIndex=1,
	DataflowGraphRecurrentQkvWorkspaceIndex=2,
	DataflowGraphRecurrentGateWorkspaceIndex=3,
	DataflowGraphRecurrentAlphaWorkspaceIndex=4,
	DataflowGraphRecurrentBetaWorkspaceIndex=5,
	DataflowGraphVocabularyWorkspaceIndex=6,
	DataflowGraphWorkspaceBufferCount=7,
	DataflowGraphUpOutputSlot=ActivationBuffers,
	DataflowGraphRecurrentQkvOutputSlot=DataflowGraphUpOutputSlot+1,
	DataflowGraphRecurrentGateOutputSlot=DataflowGraphUpOutputSlot+2,
	DataflowGraphRecurrentAlphaOutputSlot=DataflowGraphUpOutputSlot+3,
	DataflowGraphRecurrentBetaOutputSlot=DataflowGraphUpOutputSlot+4,
	DataflowGraphVocabularyOutputSlot=DataflowGraphUpOutputSlot+5
};

inline bool DataflowGraphProjectionTarget(unsigned outputSlot,
	unsigned& workspaceIndex)
{
	if (outputSlot<DataflowGraphUpOutputSlot
		|| outputSlot>DataflowGraphVocabularyOutputSlot) return false;
	workspaceIndex=DataflowGraphUpWorkspaceIndex
		+outputSlot-DataflowGraphUpOutputSlot;
	return workspaceIndex<DataflowGraphWorkspaceBufferCount;
}

inline bool DataflowDenseProjectionTarget(unsigned role,unsigned& workspaceIndex)
{
	if (role==QueryAndGate) {
		workspaceIndex=DataflowGraphRecurrentQkvWorkspaceIndex;
		return true;
	}
	if (role==Key || role==Value) {
		workspaceIndex=DataflowGraphRecurrentGateWorkspaceIndex;
		return true;
	}
	return false;
}

inline unsigned DataflowGraphProjectionOutputSlot(unsigned workspaceIndex)
{
	return workspaceIndex>=DataflowGraphUpWorkspaceIndex
		&& workspaceIndex<DataflowGraphWorkspaceBufferCount
		?DataflowGraphUpOutputSlot+workspaceIndex-DataflowGraphUpWorkspaceIndex
		:~0U;
}

inline bool DataflowGraphProjectionRole(unsigned layer,unsigned workspaceIndex,
	unsigned& role)
{
	if (layer>=Layers) return false;
	if (workspaceIndex==DataflowGraphUpWorkspaceIndex) {
		role=Up;
		return true;
	}
	if (Dense(layer)) return false;
	if (workspaceIndex==DataflowGraphRecurrentQkvWorkspaceIndex) {
		role=RecurrentQkv;
		return true;
	}
	if (workspaceIndex==DataflowGraphRecurrentGateWorkspaceIndex) {
		role=RecurrentGate;
		return true;
	}
	if (workspaceIndex==DataflowGraphRecurrentAlphaWorkspaceIndex) {
		role=Alpha;
		return true;
	}
	if (workspaceIndex==DataflowGraphRecurrentBetaWorkspaceIndex) {
		role=Beta;
		return true;
	}
	return false;
}

inline unsigned DataflowGraphWorkspaceRows(unsigned index)
{
	static constexpr unsigned rows[DataflowGraphWorkspaceBufferCount]={
		Hidden,Ffn,ConvChannels,DeltaInner,DeltaHeads,DeltaHeads,Vocabulary};
	return index<DataflowGraphWorkspaceBufferCount?rows[index]:0;
}

// The manifest keeps its two established Ffn-sized ping-pong buffers. This
// sidecar adds residual/Up scratch for the FFN, retained recurrent QKV, gate,
// alpha, and beta projections for one Gated DeltaNet layer, and one vocabulary
// logits row space for the tied output projection.
struct DataflowGraphWorkspace {
	RowSpace space[DataflowGraphWorkspaceBufferCount];

	bool Valid(const DeviceLayout& manifest) const
	{
		if (manifest.magic!=LayoutMagic || manifest.version!=LayoutVersion
			|| !manifest.context) return false;
		for (unsigned i=0;i<DataflowGraphWorkspaceBufferCount;i++) {
			if (i==DataflowGraphVocabularyWorkspaceIndex
				&& !space[i].rows && !space[i].rowBytes && !space[i].count
				&& !space[i].unit) continue;
			if (!space[i].Valid()
				|| space[i].rows!=DataflowGraphWorkspaceRows(i)
				|| space[i].rowBytes!=sizeof(float) || space[i].unit!=16) return false;
			for (unsigned prior=0;prior<i;prior++)
				if (!Disjoint(space[prior],space[i])) return false;
		}
		for (unsigned i=0;i<ActivationBuffers;i++) {
			if (!manifest.activation[i].Valid()) return false;
			for (unsigned scratch=0;scratch<DataflowGraphWorkspaceBufferCount;scratch++) {
				if (scratch==DataflowGraphVocabularyWorkspaceIndex
					&& !space[scratch].rows && !space[scratch].rowBytes
					&& !space[scratch].count && !space[scratch].unit) continue;
				if (!Disjoint(space[scratch],manifest.activation[i])) return false;
			}
		}
		return true;
	}

	static bool Disjoint(const RowSpace& a,const RowSpace& b)
	{
		if (!a.Valid() || !b.Valid()) return false;
		for (unsigned i=0;i<a.count;i++) for (unsigned j=0;j<b.count;j++) {
			const Fragment& x=a.parts[i];
			const Fragment& y=b.parts[j];
			if (x.bank!=y.bank) continue;
			const uint64_t xEnd=uint64_t(x.address)+uint64_t(x.rows)*a.rowBytes;
			const uint64_t yEnd=uint64_t(y.address)+uint64_t(y.rows)*b.rowBytes;
			if (x.address<yEnd && y.address<xEnd) return false;
		}
		return true;
	}
};

inline bool DataflowActivationBufferToRows(const PioneerQ35ActivationBuffer& source,
	RowSpace& destination)
{
	if (!source.rows || !source.rowBytes || !source.fragmentCount
		|| source.fragmentCount>Banks) return false;
	destination={};
	destination.rows=source.rows;
	destination.rowBytes=source.rowBytes;
	destination.unit=16;
	destination.count=source.fragmentCount;
	for (unsigned i=0;i<source.fragmentCount;i++) {
		const PioneerQ35ActivationFragment& part=source.fragments[i];
		destination.parts[i]={part.bank,part.address,part.firstRow,part.rows};
	}
	return destination.Valid();
}

inline bool DataflowRowSpacesEqual(const RowSpace& a,const RowSpace& b)
{
	if (a.rows!=b.rows || a.rowBytes!=b.rowBytes || a.count!=b.count
		|| a.unit!=b.unit) return false;
	for (unsigned i=0;i<a.count;i++) {
		const Fragment& x=a.parts[i];
		const Fragment& y=b.parts[i];
		if (x.bank!=y.bank || x.address!=y.address
			|| x.firstRow!=y.firstRow || x.rows!=y.rows) return false;
	}
	return true;
}

class DataflowGraphWorkspaceBuilder {
public:
	template<class PackedPlan>
	static bool Build(const PackedPlan& packed,const DeviceLayout& manifest,
		DataflowGraphWorkspace& result)
	{
		result={};
		if (manifest.magic!=LayoutMagic || manifest.version!=LayoutVersion
			|| !manifest.context || manifest.context>MaxPositions) return false;
		const uint64_t ropeBytes=uint64_t(manifest.context)*Rotary*sizeof(float);
		const uint64_t ropeReservation=(ropeBytes+4095)&~UINT64_C(4095);
		if (ropeReservation>UINT32_MAX) return false;

		PioneerQwen35ActivationPlan allocator;
		if (!allocator.InitializeFromPacked(packed,uint32_t(ropeReservation))) return false;
		for (unsigned slot=0;slot<ActivationBuffers;slot++) {
			PioneerQ35ActivationBuffer existing{};
			RowSpace rows{};
			if (!allocator.AllocateRows(Ffn,sizeof(float),existing)
				|| !DataflowActivationBufferToRows(existing,rows)
				|| !DataflowRowSpacesEqual(rows,manifest.activation[slot])) return false;
		}

		PioneerQ35ActivationBuffer scratch[DataflowGraphWorkspaceBufferCount]{};
		for (unsigned i=0;i<DataflowGraphWorkspaceBufferCount;i++) {
			if (!allocator.AllocateRows(DataflowGraphWorkspaceRows(i),sizeof(float),
					scratch[i]) || !allocator.Valid(scratch[i])
				|| !DataflowActivationBufferToRows(scratch[i],result.space[i])) {
				result={};
				return false;
			}
		}
		if (!result.Valid(manifest)) {
			result={};
			return false;
		}
		return true;
	}
};

static_assert(sizeof(DataflowGraphWorkspace)
	==DataflowGraphWorkspaceBufferCount*sizeof(RowSpace),
	"graph scratch is a fixed manifest-bound set of row spaces");

} // namespace PioneerQwen35

#endif
