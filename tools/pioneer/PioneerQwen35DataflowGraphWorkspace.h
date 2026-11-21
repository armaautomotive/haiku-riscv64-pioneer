/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_GRAPH_WORKSPACE_H
#define PIONEER_QWEN35_DATAFLOW_GRAPH_WORKSPACE_H

#include "PioneerQwen35ActivationPlan.h"
#include "PioneerQwen35DeviceLayout.h"

namespace PioneerQwen35 {

enum : unsigned {
	DataflowGraphResidualWorkspaceIndex=0,
	DataflowGraphUpWorkspaceIndex=1,
	DataflowGraphUpOutputSlot=ActivationBuffers
};

// The model manifest keeps its two established Ffn-sized ping-pong buffers.
// This rewrite sidecar reserves two additional tensors needed to execute one
// FFN safely: preserve the residual while the ping-pong buffers hold gate and
// normalized/down values, and retain the up projection while gate is activated.
struct DataflowGraphWorkspace {
	RowSpace space[2]; // residual[Hidden], up[Ffn]

	bool Valid(const DeviceLayout& manifest) const
	{
		if (manifest.magic!=LayoutMagic || manifest.version!=LayoutVersion
			|| !manifest.context || !space[0].Valid() || !space[1].Valid()
			|| space[0].rows!=Hidden || space[1].rows!=Ffn
			|| space[0].rowBytes!=sizeof(float) || space[1].rowBytes!=sizeof(float)
			|| space[0].unit!=16 || space[1].unit!=16) return false;
		for (unsigned i=0;i<ActivationBuffers;i++) {
			if (!manifest.activation[i].Valid()
				|| !Disjoint(space[0],manifest.activation[i])
				|| !Disjoint(space[1],manifest.activation[i])) return false;
		}
		return Disjoint(space[0],space[1]);
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

		PioneerQ35ActivationBuffer residual{},up{};
		if (!allocator.AllocateRows(Hidden,sizeof(float),residual)
			|| !allocator.AllocateRows(Ffn,sizeof(float),up)
			|| !allocator.Valid(residual) || !allocator.Valid(up)
			|| !DataflowActivationBufferToRows(residual,result.space[0])
			|| !DataflowActivationBufferToRows(up,result.space[1])
			|| !result.Valid(manifest)) {
			result={};
			return false;
		}
		return true;
	}
};

static_assert(sizeof(DataflowGraphWorkspace)==2*sizeof(RowSpace),
	"graph scratch is a fixed pair of manifest-bound row spaces");

} // namespace PioneerQwen35

#endif
