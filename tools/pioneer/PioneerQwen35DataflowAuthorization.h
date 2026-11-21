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
	bool fActivationReady[ActivationBuffers]{};
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
	bool ActivationReady(unsigned slot) const
	{
		return slot<ActivationBuffers && fActivationReady[slot];
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
		if (!fBound || fPending || slot>=ActivationBuffers
			|| !fManifest.activation[slot].Valid()) return false;
		fActivationReady[slot]=true;
		return true;
	}

	bool Authorize(const DataflowProjectionProgram& program) const
	{
		if (!fBound || fPending
			|| !DataflowProjectionMatchesLayout(fManifest,fRoutes,program)) return false;
		unsigned input=ActivationBuffers,output=ActivationBuffers;
		return Slots(program,input,output) && fActivationReady[input];
	}

	// Invalidate the destination before descriptors are staged. Any error from
	// this point leaves its previous contents unusable until a clean projection
	// audit and receipt commit the new activation.
	bool Begin(const DataflowProjectionProgram& program)
	{
		if (!Authorize(program)) return false;
		unsigned input=ActivationBuffers,output=ActivationBuffers;
		if (!Slots(program,input,output)) return false;
		fActivationReady[output]=false;
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
			|| !fActivationReady[input]) return false;
		fActivationReady[output]=true;
		fPending=false;
		fPendingOutput=ActivationBuffers;
		return true;
	}
};

} // namespace PioneerQwen35

#endif
