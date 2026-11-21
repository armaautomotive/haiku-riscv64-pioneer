/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_AUTHORIZATION_H
#define PIONEER_QWEN35_DATAFLOW_AUTHORIZATION_H

#include "PioneerQwen35DataflowProgram.h"

namespace PioneerQwen35 {

// Bind only after ValidateManifest has checked the pinned packed plan,
// initialized-page ledger, and after the adapter has audited both NoC route
// tables. The authority keeps an immutable layout copy so later caller-side
// mutations cannot widen a projection's address space.
class DataflowProjectionAuthority {
	DeviceLayout fManifest{};
	unsigned fRoutes[2][Banks]{};
	bool fBound=false;
	unsigned fActivationRows[ActivationBuffers]{};
	bool fPending=false;
	unsigned fPendingOutput=ActivationBuffers;

	bool Slots(const DataflowProjectionProgram& program,unsigned& input,
		unsigned& output) const
	{
		input=output=ActivationBuffers;
		for (unsigned slot=0;slot<ActivationBuffers;slot++) {
			if (DataflowRowSpaceMatches(fManifest.activation[slot],program.input))
				input=slot;
			if (DataflowRowSpaceMatches(fManifest.activation[slot],program.output))
				output=slot;
		}
		return input<ActivationBuffers && output<ActivationBuffers
			&& input!=output;
	}

public:
	DataflowProjectionAuthority()=default;
	DataflowProjectionAuthority(const DataflowProjectionAuthority&)=delete;
	DataflowProjectionAuthority& operator=(const DataflowProjectionAuthority&)=delete;

	bool Bound() const { return fBound; }
	bool Pending() const { return fPending; }
	const DeviceLayout* Manifest() const { return fBound?&fManifest:nullptr; }
	bool ActivationReady(unsigned slot) const
	{
		return slot<ActivationBuffers && fActivationRows[slot]!=0;
	}
	unsigned ActivationRows(unsigned slot) const
	{
		return slot<ActivationBuffers?fActivationRows[slot]:0;
	}

	bool BuildProjection(unsigned layer,unsigned role,unsigned inputSlot,
		unsigned outputSlot,DataflowProjectionProgram& program) const
	{
		if (!fBound || inputSlot>=ActivationBuffers
			|| outputSlot>=ActivationBuffers || inputSlot==outputSlot
			|| layer>Layers || role>=LayerRoles) return false;
		const TensorSpace* weight=fManifest.Weight(layer,role);
		if (!weight || !DataflowProgramBuilder::Build(*weight,
			fManifest.activation[inputSlot],weight->inputs,
			fManifest.activation[outputSlot],fRoutes,program)) return false;
		return Authorize(program);
	}

	bool BindValidatedModel(const DeviceLayout& manifest,
		const unsigned (&auditedRoutes)[2][Banks])
	{
		if (fBound || manifest.magic!=LayoutMagic
			|| manifest.version!=LayoutVersion || !manifest.context
			|| !DataflowBankRoutesValid(auditedRoutes)) return false;
		fManifest=manifest;
		for (unsigned noc=0;noc<2;noc++) for (unsigned bank=0;bank<Banks;bank++)
			fRoutes[noc][bank]=auditedRoutes[noc][bank];
		fBound=true;
		return true;
	}

	// The caller may seed a buffer only after its activation bytes have been
	// written and read back under the model session's physical IO proof.
	bool MarkActivationInitialized(unsigned slot)
	{
		return slot<ActivationBuffers && fBound
			&& MarkActivationInitialized(slot,fManifest.activation[slot].rows);
	}

	bool MarkActivationInitialized(unsigned slot,unsigned rows)
	{
		if (!fBound || fPending || slot>=ActivationBuffers || !rows
			|| rows>fManifest.activation[slot].rows || (rows&15)
			|| !fManifest.activation[slot].Valid()) return false;
		fActivationRows[slot]=rows;
		return true;
	}

	bool InvalidateActivation(unsigned slot)
	{
		if (!fBound || fPending || slot>=ActivationBuffers) return false;
		fActivationRows[slot]=0;
		return true;
	}

	bool Authorize(const DataflowProjectionProgram& program) const
	{
		if (!fBound || fPending
			|| !DataflowProjectionMatchesLayout(fManifest,fRoutes,program)) return false;
		unsigned input=ActivationBuffers,output=ActivationBuffers;
		return Slots(program,input,output)
			&& fActivationRows[input]>=program.inputRows;
	}

	// Invalidate the destination before descriptors are staged. Any error from
	// this point leaves its previous contents unusable until a clean projection
	// audit and receipt commit the new activation.
	bool Begin(const DataflowProjectionProgram& program)
	{
		if (!Authorize(program)) return false;
		unsigned input=ActivationBuffers,output=ActivationBuffers;
		if (!Slots(program,input,output)) return false;
		fActivationRows[output]=0;
		fPending=true;
		fPendingOutput=output;
		return true;
	}

	bool Complete(const DataflowProjectionProgram& program)
	{
		if (!fBound || !fPending
			|| !DataflowProjectionMatchesLayout(fManifest,fRoutes,program)) return false;
		unsigned input=ActivationBuffers,output=ActivationBuffers;
		if (!Slots(program,input,output) || output!=fPendingOutput
			|| fActivationRows[input]<program.inputRows
			|| program.outputRows>fManifest.activation[output].rows) return false;
		fActivationRows[output]=program.outputRows;
		fPending=false;
		fPendingOutput=ActivationBuffers;
		return true;
	}

	// Used only after a physical fleet audit has proved every command either
	// remained unpublished or reached a terminal receipt and was acknowledged.
	// The destination stays invalid after an aborted projection.
	bool AbortPendingProjection()
	{
		if (!fBound || !fPending || fPendingOutput>=ActivationBuffers) return false;
		fActivationRows[fPendingOutput]=0;
		fPending=false;
		fPendingOutput=ActivationBuffers;
		return true;
	}
};

} // namespace PioneerQwen35

#endif
