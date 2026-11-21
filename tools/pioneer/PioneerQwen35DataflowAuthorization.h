/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_AUTHORIZATION_H
#define PIONEER_QWEN35_DATAFLOW_AUTHORIZATION_H

#include "PioneerQwen35DataflowProgram.h"
#include "PioneerQwen35DataflowEmbeddingProgram.h"
#include "PioneerQwen35DataflowRmsProgram.h"
#include "PioneerQwen35DataflowGraphWorkspace.h"
#include "PioneerQwen35DataflowSiluProgram.h"

namespace PioneerQwen35 {

// Bind only after ValidateManifest has checked the pinned packed plan,
// initialized-page ledger, and after the adapter has audited both NoC route
// tables. The authority keeps an immutable layout copy so later caller-side
// mutations cannot widen a projection's address space.
class DataflowProjectionAuthority {
	enum : unsigned {
		PendingNone=0,PendingProjection=1,PendingEmbedding=2,PendingRms=3,
		PendingGraphProjection=4,PendingSilu=5
	};
	DeviceLayout fManifest{};
	DataflowGraphWorkspace fGraphWorkspace{};
	unsigned fRoutes[2][Banks]{};
	bool fBound=false;
	unsigned fActivationRows[ActivationBuffers]{};
	bool fGraphUpReady=false;
	bool fPending=false;
	unsigned fPendingOutput=ActivationBuffers;
	unsigned fPendingKind=PendingNone;

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

	bool EmbeddingMatches(const DataflowEmbeddingProgram& program) const
	{
		if (!fBound || !program.Valid()) return false;
		DataflowEmbeddingProgram expected{};
		if (!DataflowEmbeddingProgramBuilder::Build(fManifest,program.token,
			program.outputSlot,fRoutes,expected)
			|| program.lane!=expected.lane
			|| program.weightBank!=expected.weightBank
			|| program.weightBase!=expected.weightBase
			|| !DataflowRowSpaceMatches(expected.output,program.output)) return false;
		for (unsigned noc=0;noc<2;noc++) for (unsigned bank=0;bank<Banks;bank++)
			if (program.bankRoute[noc][bank]!=expected.bankRoute[noc][bank])
				return false;
		return true;
	}

	bool RmsMatches(const DataflowRmsProgram& program) const
	{
		if (!fBound || !program.Valid()) return false;
		DataflowRmsProgram expected;
		DataflowRmsZeroWords(expected);
		if (!DataflowRmsProgramBuilder::Build(fManifest,program.layer,
			program.role,program.inputSlot,program.outputSlot,fRoutes,expected)
			|| program.scratchRow!=expected.scratchRow
			|| !DataflowRowSpaceMatches(expected.input,program.input)
			|| !DataflowRowSpaceMatches(expected.weight,program.weight)
			|| !DataflowRowSpaceMatches(expected.output,program.output)) return false;
		for (unsigned noc=0;noc<2;noc++) for (unsigned bank=0;bank<Banks;bank++)
			if (program.bankRoute[noc][bank]!=expected.bankRoute[noc][bank])
				return false;
		return true;
	}

	bool SiluMatches(const DataflowSiluProgram& program) const
	{
		if (!fBound || !program.Valid()) return false;
		DataflowSiluProgram expected{};
		if (!DataflowSiluProgramBuilder::Build(fManifest,fGraphWorkspace,
			program.gateSlot,fRoutes,expected)
			|| !DataflowRowSpaceMatches(expected.gate,program.gate)
			|| !DataflowRowSpaceMatches(expected.up,program.up)) return false;
		for (unsigned noc=0;noc<2;noc++) for (unsigned bank=0;bank<Banks;bank++)
			if (program.bankRoute[noc][bank]!=expected.bankRoute[noc][bank])
				return false;
		return true;
	}

public:
	DataflowProjectionAuthority()=default;
	DataflowProjectionAuthority(const DataflowProjectionAuthority&)=delete;
	DataflowProjectionAuthority& operator=(const DataflowProjectionAuthority&)=delete;

	bool Bound() const { return fBound; }
	bool Pending() const { return fPending; }
	const DeviceLayout* Manifest() const { return fBound?&fManifest:nullptr; }
	const DataflowGraphWorkspace* GraphWorkspace() const
		{ return fBound?&fGraphWorkspace:nullptr; }
	bool ActivationReady(unsigned slot) const
	{
		return slot<ActivationBuffers && fActivationRows[slot]!=0;
	}
	unsigned ActivationRows(unsigned slot) const
	{
		return slot<ActivationBuffers?fActivationRows[slot]:0;
	}
	bool GraphUpReady() const { return fBound && fGraphUpReady; }

	bool BuildProjection(unsigned layer,unsigned role,unsigned inputSlot,
		unsigned outputSlot,DataflowProjectionProgram& program) const
	{
		if (!fBound || inputSlot>=ActivationBuffers
			|| outputSlot>DataflowGraphUpOutputSlot || inputSlot==outputSlot
			|| layer>Layers || role>=LayerRoles
			|| (outputSlot==DataflowGraphUpOutputSlot
				&& (layer>=Layers || role!=Up))) return false;
		const TensorSpace* weight=fManifest.Weight(layer,role);
		const RowSpace* output=outputSlot==DataflowGraphUpOutputSlot
			?&fGraphWorkspace.space[DataflowGraphUpWorkspaceIndex]
			:outputSlot<ActivationBuffers?&fManifest.activation[outputSlot]:nullptr;
		if (!weight || !DataflowProgramBuilder::Build(*weight,
			fManifest.activation[inputSlot],weight->inputs,
			*output,fRoutes,program)) return false;
		return Authorize(program);
	}

	bool BuildEmbedding(unsigned token,unsigned outputSlot,
		DataflowEmbeddingProgram& program) const
	{
		if (!fBound || fPending
			|| !DataflowEmbeddingProgramBuilder::Build(fManifest,token,
				outputSlot,fRoutes,program)) return false;
		return AuthorizeEmbedding(program);
	}

	bool AuthorizeEmbedding(const DataflowEmbeddingProgram& program) const
	{
		return fBound && !fPending && EmbeddingMatches(program);
	}

	bool BuildRms(unsigned layer,unsigned role,unsigned inputSlot,
		unsigned outputSlot,DataflowRmsProgram& program) const
	{
		if (!fBound || fPending || inputSlot>=ActivationBuffers
			|| fActivationRows[inputSlot]<Hidden
			|| !DataflowRmsProgramBuilder::Build(fManifest,layer,role,inputSlot,
				outputSlot,fRoutes,program)) return false;
		return AuthorizeRms(program);
	}

	bool AuthorizeRms(const DataflowRmsProgram& program) const
	{
		return fBound && !fPending && program.inputSlot<ActivationBuffers
			&& fActivationRows[program.inputSlot]>=Hidden && RmsMatches(program);
	}

	bool BuildSilu(unsigned gateSlot,DataflowSiluProgram& program) const
	{
		if (!fBound || fPending || gateSlot>=ActivationBuffers
			|| !fGraphUpReady || fActivationRows[gateSlot]<Ffn
			|| !DataflowSiluProgramBuilder::Build(fManifest,fGraphWorkspace,
				gateSlot,fRoutes,program)) return false;
		return AuthorizeSilu(program);
	}

	bool AuthorizeSilu(const DataflowSiluProgram& program) const
	{
		return fBound && !fPending && fGraphUpReady
			&& program.gateSlot<ActivationBuffers
			&& fActivationRows[program.gateSlot]>=Ffn
			&& SiluMatches(program);
	}

	bool BeginSilu(const DataflowSiluProgram& program)
	{
		if (!AuthorizeSilu(program)) return false;
		fActivationRows[program.gateSlot]=0;
		fGraphUpReady=false;
		fPending=true;
		fPendingOutput=program.gateSlot;
		fPendingKind=PendingSilu;
		return true;
	}

	bool PendingSiluMatches(const DataflowSiluProgram& program) const
	{
		return fBound && fPending && fPendingKind==PendingSilu
			&& program.gateSlot==fPendingOutput && SiluMatches(program);
	}

	bool CompleteSilu(const DataflowSiluProgram& program)
	{
		if (!PendingSiluMatches(program)) return false;
		fActivationRows[program.gateSlot]=Ffn;
		fPending=false;
		fPendingOutput=ActivationBuffers;
		fPendingKind=PendingNone;
		return true;
	}

	bool BindValidatedModel(const DeviceLayout& manifest,
		const DataflowGraphWorkspace& graphWorkspace,
		const unsigned (&auditedRoutes)[2][Banks])
	{
		if (fBound || manifest.magic!=LayoutMagic
			|| manifest.version!=LayoutVersion || !manifest.context
			|| !DataflowBankRoutesValid(auditedRoutes)
			|| !graphWorkspace.Valid(manifest)) return false;
		fManifest=manifest;
		fGraphWorkspace=graphWorkspace;
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
		if (!fBound || fPending) return false;
		if (DataflowProjectionMatchesLayout(fManifest,fRoutes,program)) {
			unsigned input=ActivationBuffers,output=ActivationBuffers;
			return Slots(program,input,output)
				&& fActivationRows[input]>=program.inputRows;
		}
		unsigned input=ActivationBuffers;
		return DataflowProjectionMatchesGraphWorkspace(fManifest,fGraphWorkspace,
			fRoutes,program,&input) && fActivationRows[input]>=program.inputRows;
	}

	// Invalidate the destination before descriptors are staged. Any error from
	// this point leaves its previous contents unusable until a clean projection
	// audit and receipt commit the new activation.
	bool Begin(const DataflowProjectionProgram& program)
	{
		if (!Authorize(program)) return false;
		if (DataflowProjectionMatchesLayout(fManifest,fRoutes,program)) {
			unsigned input=ActivationBuffers,output=ActivationBuffers;
			if (!Slots(program,input,output)) return false;
			fActivationRows[output]=0;
			fPendingOutput=output;
			fPendingKind=PendingProjection;
		} else {
			if (!DataflowProjectionMatchesGraphWorkspace(fManifest,fGraphWorkspace,
				fRoutes,program)) return false;
			fGraphUpReady=false;
			fPendingOutput=DataflowGraphUpOutputSlot;
			fPendingKind=PendingGraphProjection;
		}
		fPending=true;
		return true;
	}

	bool PendingProjectionMatches(const DataflowProjectionProgram& program) const
	{
		if (!fBound || !fPending) return false;
		if (fPendingKind==PendingProjection) {
			unsigned input=ActivationBuffers,output=ActivationBuffers;
			return DataflowProjectionMatchesLayout(fManifest,fRoutes,program)
				&& Slots(program,input,output) && output==fPendingOutput
				&& fActivationRows[input]>=program.inputRows;
		}
		unsigned input=ActivationBuffers;
		return fPendingKind==PendingGraphProjection
			&& fPendingOutput==DataflowGraphUpOutputSlot
			&& DataflowProjectionMatchesGraphWorkspace(fManifest,fGraphWorkspace,
				fRoutes,program,&input)
			&& fActivationRows[input]>=program.inputRows;
	}

	bool BeginEmbedding(const DataflowEmbeddingProgram& program)
	{
		if (!AuthorizeEmbedding(program)
			|| program.outputSlot>=ActivationBuffers) return false;
		fActivationRows[program.outputSlot]=0;
		fPending=true;
		fPendingOutput=program.outputSlot;
		fPendingKind=PendingEmbedding;
		return true;
	}

	bool BeginRms(const DataflowRmsProgram& program)
	{
		if (!AuthorizeRms(program)
			|| program.outputSlot>=ActivationBuffers) return false;
		fActivationRows[program.outputSlot]=0;
		fPending=true;
		fPendingOutput=program.outputSlot;
		fPendingKind=PendingRms;
		return true;
	}

	bool PendingEmbeddingMatches(const DataflowEmbeddingProgram& program) const
	{
		return fPending && fPendingKind==PendingEmbedding
			&& program.outputSlot==fPendingOutput
			&& EmbeddingMatches(program);
	}

	bool PendingRmsMatches(const DataflowRmsProgram& program) const
	{
		return fPending && fPendingKind==PendingRms
			&& program.outputSlot==fPendingOutput && RmsMatches(program);
	}

	bool Complete(const DataflowProjectionProgram& program)
	{
		if (!PendingProjectionMatches(program)) return false;
		if (fPendingKind==PendingGraphProjection) {
			unsigned input=ActivationBuffers;
			if (!DataflowProjectionMatchesGraphWorkspace(fManifest,fGraphWorkspace,
				fRoutes,program,&input)
				|| fPendingOutput!=DataflowGraphUpOutputSlot
				|| fActivationRows[input]<program.inputRows) return false;
			fGraphUpReady=true;
		} else {
			unsigned input=ActivationBuffers,output=ActivationBuffers;
			if (!Slots(program,input,output) || output!=fPendingOutput
				|| fActivationRows[input]<program.inputRows
				|| program.outputRows>fManifest.activation[output].rows) return false;
			fActivationRows[output]=program.outputRows;
		}
		fPending=false;
		fPendingOutput=ActivationBuffers;
		fPendingKind=PendingNone;
		return true;
	}

	bool CompleteEmbedding(const DataflowEmbeddingProgram& program)
	{
		if (!fBound || !fPending || fPendingKind!=PendingEmbedding
			|| !EmbeddingMatches(program)
			|| program.outputSlot!=fPendingOutput
			|| !DataflowRowSpaceMatches(fManifest.activation[program.outputSlot],
				program.output)) return false;
		fActivationRows[program.outputSlot]=Hidden;
		fPending=false;
		fPendingOutput=ActivationBuffers;
		fPendingKind=PendingNone;
		return true;
	}

	bool CompleteRms(const DataflowRmsProgram& program)
	{
		if (!fBound || !fPending || fPendingKind!=PendingRms
			|| !RmsMatches(program) || program.outputSlot!=fPendingOutput
			|| program.output.rows>fManifest.activation[program.outputSlot].rows)
			return false;
		fActivationRows[program.outputSlot]=Hidden;
		fPending=false;
		fPendingOutput=ActivationBuffers;
		fPendingKind=PendingNone;
		return true;
	}

	// Used only after a physical fleet audit has proved every command either
	// remained unpublished or reached a terminal receipt and was acknowledged.
	// The destination stays invalid after an aborted projection.
	bool AbortPendingProjection()
	{
		if (!fBound || !fPending) return false;
		if (fPendingKind==PendingGraphProjection) {
			if (fPendingOutput!=DataflowGraphUpOutputSlot) return false;
			fGraphUpReady=false;
		} else if (fPendingKind==PendingSilu) {
			if (fPendingOutput>=ActivationBuffers) return false;
			fActivationRows[fPendingOutput]=0;
			fGraphUpReady=false;
		} else {
			if (fPendingKind!=PendingProjection || fPendingOutput>=ActivationBuffers)
				return false;
			fActivationRows[fPendingOutput]=0;
		}
		fPending=false;
		fPendingOutput=ActivationBuffers;
		fPendingKind=PendingNone;
		return true;
	}
};

} // namespace PioneerQwen35

#endif
