/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_AUTHORIZATION_H
#define PIONEER_QWEN35_DATAFLOW_AUTHORIZATION_H

#include "PioneerQwen35DataflowProgram.h"
#include "PioneerQwen35DataflowEmbeddingProgram.h"
#include "PioneerQwen35DataflowRmsProgram.h"
#include "PioneerQwen35DataflowGraphWorkspace.h"
#include "PioneerQwen35DataflowSiluProgram.h"
#include "PioneerQwen35DataflowResidualProgram.h"
#include "PioneerQwen35DataflowConvProgram.h"
#include "PioneerQwen35DataflowDeltaProgram.h"
#include "PioneerQwen35DataflowDenseAttentionProgram.h"
#include "PioneerQwen35DataflowArgmaxProgram.h"

namespace PioneerQwen35 {

// Bind only after ValidateManifest has checked the pinned packed plan,
// initialized-page ledger, and after the adapter has audited both NoC route
// tables. The authority keeps an immutable layout copy so later caller-side
// mutations cannot widen a projection's address space.
class DataflowProjectionAuthority {
	enum : unsigned {
		PendingNone=0,PendingProjection=1,PendingEmbedding=2,PendingRms=3,
		PendingGraphProjection=4,PendingSilu=5,PendingResidual=6,PendingConv=7,
		PendingDelta=8,PendingRecurrentOut=9,PendingDenseGraphProjection=10,
		PendingDenseAttention=11,PendingDenseAttentionOut=12,
		PendingTiedOutput=13,PendingArgmax=14
	};
	DeviceLayout fManifest{};
	DataflowGraphWorkspace fGraphWorkspace{};
	mutable PioneerQ35ProjectionProgram fProjectionScheduleScratch{};
	Range fRope{};
	unsigned fRoutes[2][Banks]{};
	bool fBound=false;
	unsigned fActivationRows[ActivationBuffers]{};
	bool fGraphWorkspaceReady[DataflowGraphWorkspaceBufferCount]{};
	unsigned fGraphWorkspaceLayer[DataflowGraphWorkspaceBufferCount]{};
	unsigned fGraphWorkspaceRole[DataflowGraphWorkspaceBufferCount]{};
	bool fDenseKeyStored[Layers]{};
	bool fDenseValueStored[Layers]{};
	unsigned fDenseNextPosition[Layers]{};
	unsigned fConvNextPosition[Layers]{};
	unsigned fDeltaNextPosition[Layers]{};
	bool fGraphResidualReady=false;
	bool fFinalNormReady=false;
	unsigned fFinalNormSlot=ActivationBuffers;
	bool fVocabularyReady=false;
	bool fPending=false;
	unsigned fPendingOutput=ActivationBuffers;
	unsigned fPendingKind=PendingNone;
	unsigned fPendingResidualMode=0;
	unsigned fPendingLayer=~0U,fPendingPosition=0;
	unsigned fPendingRole=LayerRoles;

	void InvalidateVocabulary()
	{
		const unsigned vocabulary=DataflowGraphVocabularyWorkspaceIndex;
		fVocabularyReady=false;
		fGraphWorkspaceReady[vocabulary]=false;
		fGraphWorkspaceLayer[vocabulary]=~0U;
		fGraphWorkspaceRole[vocabulary]=LayerRoles;
	}

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

	bool ResidualMatches(const DataflowResidualProgram& program) const
	{
		if (!fBound || !program.Valid()) return false;
		DataflowResidualProgram expected{};
		if (!DataflowResidualProgramBuilder::Build(fManifest,fGraphWorkspace,
			program.mode,program.activationSlot,fRoutes,expected)
			|| !DataflowRowSpaceMatches(expected.activation,program.activation)
			|| !DataflowRowSpaceMatches(expected.residual,program.residual)) return false;
		for (unsigned noc=0;noc<2;noc++) for (unsigned bank=0;bank<Banks;bank++)
			if (program.bankRoute[noc][bank]!=expected.bankRoute[noc][bank])
				return false;
		return true;
	}

	bool ConvMatches(const DataflowConvProgram& program) const
	{
		if (!fBound || !program.Valid()) return false;
		DataflowConvProgram expected{};
		if (!DataflowConvProgramBuilder::Build(fManifest,fGraphWorkspace,
			program.layer,program.position,fRoutes,expected)
			|| program.layer!=expected.layer || program.position!=expected.position
			|| !DataflowRowSpaceMatches(program.qkv,expected.qkv)
			|| !DataflowRowSpaceMatches(program.historyRead,expected.historyRead)
			|| !DataflowRowSpaceMatches(program.historyWrite,expected.historyWrite)
			|| program.weights.type!=expected.weights.type
			|| program.weights.inputs!=expected.weights.inputs
			|| !DataflowRowSpaceMatches(program.weights.place,expected.weights.place))
			return false;
		for (unsigned noc=0;noc<2;noc++) for (unsigned bank=0;bank<Banks;bank++)
			if (program.bankRoute[noc][bank]!=expected.bankRoute[noc][bank])
				return false;
		return true;
	}

	bool DeltaMatches(const DataflowDeltaProgram& program) const
	{
		if (!fBound || !program.Valid()) return false;
		DataflowDeltaProgram expected{};
		if (!DataflowDeltaProgramBuilder::Build(fManifest,fGraphWorkspace,
			program.layer,program.position,fRoutes,expected)
			|| program.layer!=expected.layer || program.position!=expected.position
			|| !DataflowRowSpaceMatches(program.qkv,expected.qkv)
			|| !DataflowRowSpaceMatches(program.gate,expected.gate)
			|| !DataflowRowSpaceMatches(program.alpha,expected.alpha)
			|| !DataflowRowSpaceMatches(program.beta,expected.beta)
			|| !DataflowRowSpaceMatches(program.stateRead,expected.stateRead)
			|| !DataflowRowSpaceMatches(program.stateWrite,expected.stateWrite)
			|| program.negativeA.type!=expected.negativeA.type
			|| program.negativeA.inputs!=expected.negativeA.inputs
			|| !DataflowRowSpaceMatches(program.negativeA.place,
				expected.negativeA.place)
			|| program.deltaBias.type!=expected.deltaBias.type
			|| program.deltaBias.inputs!=expected.deltaBias.inputs
			|| !DataflowRowSpaceMatches(program.deltaBias.place,
				expected.deltaBias.place)
			|| program.deltaNorm.type!=expected.deltaNorm.type
			|| program.deltaNorm.inputs!=expected.deltaNorm.inputs
			|| !DataflowRowSpaceMatches(program.deltaNorm.place,
				expected.deltaNorm.place)) return false;
		for (unsigned noc=0;noc<2;noc++) for (unsigned bank=0;bank<Banks;bank++)
			if (program.bankRoute[noc][bank]!=expected.bankRoute[noc][bank])
				return false;
		return true;
	}

	bool DenseAttentionMatches(const DataflowDenseAttentionProgram& program) const
	{
		if (!fBound || !program.Valid()) return false;
		DataflowDenseAttentionProgram expected{};
		if (!DataflowDenseAttentionProgramBuilder::Build(fManifest,
			fGraphWorkspace,fRope,program.layer,program.position,program.mode,
			fRoutes,expected)
			|| program.context!=expected.context
			|| program.layer!=expected.layer || program.position!=expected.position
			|| program.mode!=expected.mode
			|| program.rope.bank!=expected.rope.bank
			|| program.rope.address!=expected.rope.address
			|| program.rope.bytes!=expected.rope.bytes
			|| !DataflowRowSpaceMatches(expected.qkv,program.qkv)
			|| !DataflowRowSpaceMatches(expected.gate,program.gate)
			|| !DataflowRowSpaceMatches(expected.contextOutput,
				program.contextOutput)
			|| !DataflowRowSpaceMatches(expected.kv,program.kv)
			|| program.queryNorm.type!=expected.queryNorm.type
			|| program.queryNorm.inputs!=expected.queryNorm.inputs
			|| !DataflowRowSpaceMatches(expected.queryNorm.place,
				program.queryNorm.place)
			|| program.keyNorm.type!=expected.keyNorm.type
			|| program.keyNorm.inputs!=expected.keyNorm.inputs
			|| !DataflowRowSpaceMatches(expected.keyNorm.place,
				program.keyNorm.place)) return false;
		for (unsigned noc=0;noc<2;noc++) for (unsigned bank=0;bank<Banks;bank++)
			if (program.bankRoute[noc][bank]!=expected.bankRoute[noc][bank])
				return false;
		return true;
	}

	bool RecurrentOutputMatches(const DataflowProjectionProgram& program,
		unsigned* outputSlot=nullptr,unsigned* modelLayer=nullptr) const
	{
		unsigned output=ActivationBuffers,layer=Layers;
		if (!fBound || !DataflowProjectionMatchesRecurrentOutput(fManifest,
			fGraphWorkspace,fRoutes,program,&output,&layer)
			|| layer>=Layers || fDeltaNextPosition[layer]==0
			|| fConvNextPosition[layer]!=fDeltaNextPosition[layer]
			|| !GraphWorkspaceReady(DataflowGraphRecurrentQkvWorkspaceIndex)
			|| GraphWorkspaceLayer(DataflowGraphRecurrentQkvWorkspaceIndex)!=layer)
			return false;
		if (GraphWorkspaceRole(DataflowGraphRecurrentQkvWorkspaceIndex)!=RecurrentQkv)
			return false;
		if (outputSlot) *outputSlot=output;
		if (modelLayer) *modelLayer=layer;
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
	bool GraphUpReady() const
		{ return GraphWorkspaceReady(DataflowGraphUpWorkspaceIndex); }
	bool GraphWorkspaceReady(unsigned index) const
		{ return fBound && index<DataflowGraphWorkspaceBufferCount
			&& fGraphWorkspaceReady[index]; }
	unsigned GraphWorkspaceLayer(unsigned index) const
		{ return GraphWorkspaceReady(index)?fGraphWorkspaceLayer[index]:~0U; }
	unsigned GraphWorkspaceRole(unsigned index) const
		{ return GraphWorkspaceReady(index)?fGraphWorkspaceRole[index]:LayerRoles; }
	bool GraphResidualReady() const { return fBound && fGraphResidualReady; }
	bool VocabularyReady() const { return fBound && fVocabularyReady
		&& GraphWorkspaceReady(DataflowGraphVocabularyWorkspaceIndex); }

	// Starts a new decode request without clearing resident state storage.
	// Position zero operators are required to ignore the previous request's
	// ping-pong cells, then successful recurrent steps advance each layer.
	bool ResetRecurrentState()
	{
		if (!fBound || fPending) return false;
		for (unsigned layer=0;layer<Layers;layer++)
			fConvNextPosition[layer]=fDeltaNextPosition[layer]=0;
		for (unsigned layer=0;layer<Layers;layer++) {
			fDenseNextPosition[layer]=0;
			fDenseKeyStored[layer]=false;
			fDenseValueStored[layer]=false;
		}
		fFinalNormReady=false;
		fFinalNormSlot=ActivationBuffers;
		InvalidateVocabulary();
		for (unsigned i=DataflowGraphRecurrentQkvWorkspaceIndex;
			i<DataflowGraphWorkspaceBufferCount;i++) {
			fGraphWorkspaceReady[i]=false;
			fGraphWorkspaceLayer[i]=~0U;
			fGraphWorkspaceRole[i]=LayerRoles;
		}
		return true;
	}

	unsigned ConvNextPosition(unsigned layer) const
	{
		return fBound && layer<Layers?fConvNextPosition[layer]:~0U;
	}

	unsigned DeltaNextPosition(unsigned layer) const
	{
		return fBound && layer<Layers?fDeltaNextPosition[layer]:~0U;
	}

	bool BuildConv(unsigned layer,unsigned position,
		DataflowConvProgram& program) const
	{
		const unsigned qkv=DataflowGraphRecurrentQkvWorkspaceIndex;
		if (!fBound || fPending || layer>=Layers
			|| position!=fConvNextPosition[layer]
			|| !GraphWorkspaceReady(qkv) || GraphWorkspaceLayer(qkv)!=layer
			|| GraphWorkspaceRole(qkv)!=RecurrentQkv
			|| !DataflowConvProgramBuilder::Build(fManifest,fGraphWorkspace,
				layer,position,fRoutes,program)) return false;
		return AuthorizeConv(program);
	}

	bool AuthorizeConv(const DataflowConvProgram& program) const
	{
		const unsigned qkv=DataflowGraphRecurrentQkvWorkspaceIndex;
		return fBound && !fPending && program.layer<Layers
			&& program.position==fConvNextPosition[program.layer]
			&& GraphWorkspaceReady(qkv)
			&& GraphWorkspaceLayer(qkv)==program.layer
			&& GraphWorkspaceRole(qkv)==RecurrentQkv
			&& ConvMatches(program);
	}

	bool BeginConv(const DataflowConvProgram& program)
	{
		if (!AuthorizeConv(program)) return false;
		fFinalNormReady=false;
		fFinalNormSlot=ActivationBuffers;
		InvalidateVocabulary();
		fGraphWorkspaceReady[DataflowGraphRecurrentQkvWorkspaceIndex]=false;
		fGraphWorkspaceRole[DataflowGraphRecurrentQkvWorkspaceIndex]=LayerRoles;
		fPending=true;
		fPendingOutput=DataflowGraphRecurrentQkvOutputSlot;
		fPendingKind=PendingConv;
		fPendingLayer=program.layer;
		fPendingPosition=program.position;
		return true;
	}

	bool PendingConvMatches(const DataflowConvProgram& program) const
	{
		return fBound && fPending && fPendingKind==PendingConv
			&& fPendingOutput==DataflowGraphRecurrentQkvOutputSlot
			&& program.layer==fPendingLayer && program.position==fPendingPosition
			&& ConvMatches(program);
	}

	bool CompleteConv(const DataflowConvProgram& program)
	{
		if (!PendingConvMatches(program)) return false;
		const unsigned qkv=DataflowGraphRecurrentQkvWorkspaceIndex;
		fGraphWorkspaceLayer[qkv]=program.layer;
		fGraphWorkspaceRole[qkv]=RecurrentQkv;
		fGraphWorkspaceReady[qkv]=true;
		fConvNextPosition[program.layer]=program.position+1;
		fPending=false;
		fPendingOutput=ActivationBuffers;
		fPendingKind=PendingNone;
		fPendingLayer=~0U;
		fPendingPosition=0;
		return true;
	}

	bool BuildDelta(unsigned layer,unsigned position,DataflowDeltaProgram& program) const
	{
		const unsigned qkv=DataflowGraphRecurrentQkvWorkspaceIndex;
		const unsigned gate=DataflowGraphRecurrentGateWorkspaceIndex;
		const unsigned alpha=DataflowGraphRecurrentAlphaWorkspaceIndex;
		const unsigned beta=DataflowGraphRecurrentBetaWorkspaceIndex;
		if (!fBound || fPending || layer>=Layers
			|| position!=fDeltaNextPosition[layer]
			|| fConvNextPosition[layer]!=position+1
			|| !GraphWorkspaceReady(qkv) || GraphWorkspaceLayer(qkv)!=layer
			|| GraphWorkspaceRole(qkv)!=RecurrentQkv
			|| !GraphWorkspaceReady(gate) || GraphWorkspaceLayer(gate)!=layer
			|| GraphWorkspaceRole(gate)!=RecurrentGate
			|| !GraphWorkspaceReady(alpha) || GraphWorkspaceLayer(alpha)!=layer
			|| GraphWorkspaceRole(alpha)!=Alpha
			|| !GraphWorkspaceReady(beta) || GraphWorkspaceLayer(beta)!=layer
			|| GraphWorkspaceRole(beta)!=Beta
			|| !DataflowDeltaProgramBuilder::Build(fManifest,fGraphWorkspace,
				layer,position,fRoutes,program)) return false;
		return AuthorizeDelta(program);
	}

	bool AuthorizeDelta(const DataflowDeltaProgram& program) const
	{
		if (!fBound || fPending || program.layer>=Layers
			|| program.position!=fDeltaNextPosition[program.layer]
			|| fConvNextPosition[program.layer]!=program.position+1) return false;
		const unsigned qkv=DataflowGraphRecurrentQkvWorkspaceIndex;
		const unsigned gate=DataflowGraphRecurrentGateWorkspaceIndex;
		const unsigned alpha=DataflowGraphRecurrentAlphaWorkspaceIndex;
		const unsigned beta=DataflowGraphRecurrentBetaWorkspaceIndex;
		return GraphWorkspaceReady(qkv) && GraphWorkspaceLayer(qkv)==program.layer
			&& GraphWorkspaceRole(qkv)==RecurrentQkv
			&& GraphWorkspaceReady(gate) && GraphWorkspaceLayer(gate)==program.layer
			&& GraphWorkspaceRole(gate)==RecurrentGate
			&& GraphWorkspaceReady(alpha) && GraphWorkspaceLayer(alpha)==program.layer
			&& GraphWorkspaceRole(alpha)==Alpha
			&& GraphWorkspaceReady(beta) && GraphWorkspaceLayer(beta)==program.layer
			&& GraphWorkspaceRole(beta)==Beta
			&& DeltaMatches(program);
	}

	bool BeginDelta(const DataflowDeltaProgram& program)
	{
		if (!AuthorizeDelta(program)) return false;
		fFinalNormReady=false;
		fFinalNormSlot=ActivationBuffers;
		InvalidateVocabulary();
		fGraphWorkspaceReady[DataflowGraphRecurrentQkvWorkspaceIndex]=false;
		fGraphWorkspaceReady[DataflowGraphRecurrentAlphaWorkspaceIndex]=false;
		fGraphWorkspaceReady[DataflowGraphRecurrentBetaWorkspaceIndex]=false;
		fGraphWorkspaceRole[DataflowGraphRecurrentQkvWorkspaceIndex]=LayerRoles;
		fGraphWorkspaceRole[DataflowGraphRecurrentAlphaWorkspaceIndex]=LayerRoles;
		fGraphWorkspaceRole[DataflowGraphRecurrentBetaWorkspaceIndex]=LayerRoles;
		fPending=true;
		fPendingOutput=DataflowGraphRecurrentQkvOutputSlot;
		fPendingKind=PendingDelta;
		fPendingLayer=program.layer;
		fPendingPosition=program.position;
		return true;
	}

	bool PendingDeltaMatches(const DataflowDeltaProgram& program) const
	{
		return fBound && fPending && fPendingKind==PendingDelta
			&& fPendingOutput==DataflowGraphRecurrentQkvOutputSlot
			&& program.layer==fPendingLayer && program.position==fPendingPosition
			&& DeltaMatches(program);
	}

	bool CompleteDelta(const DataflowDeltaProgram& program)
	{
		if (!PendingDeltaMatches(program)) return false;
		const unsigned qkv=DataflowGraphRecurrentQkvWorkspaceIndex;
		fGraphWorkspaceLayer[qkv]=program.layer;
		fGraphWorkspaceRole[qkv]=RecurrentQkv;
		fGraphWorkspaceReady[qkv]=true;
		fGraphWorkspaceReady[DataflowGraphRecurrentAlphaWorkspaceIndex]=false;
		fGraphWorkspaceReady[DataflowGraphRecurrentBetaWorkspaceIndex]=false;
		fGraphWorkspaceRole[DataflowGraphRecurrentAlphaWorkspaceIndex]=LayerRoles;
		fGraphWorkspaceRole[DataflowGraphRecurrentBetaWorkspaceIndex]=LayerRoles;
		fGraphWorkspaceLayer[DataflowGraphRecurrentAlphaWorkspaceIndex]=~0U;
		fGraphWorkspaceLayer[DataflowGraphRecurrentBetaWorkspaceIndex]=~0U;
		fDeltaNextPosition[program.layer]=program.position+1;
		fPending=false;
		fPendingOutput=ActivationBuffers;
		fPendingKind=PendingNone;
		fPendingLayer=~0U;
		fPendingPosition=0;
		return true;
	}

	bool BuildProjection(unsigned layer,unsigned role,unsigned inputSlot,
		unsigned outputSlot,DataflowProjectionProgram& program) const
	{
		if (!fBound || inputSlot>=ActivationBuffers
			|| outputSlot>DataflowGraphVocabularyOutputSlot
			|| inputSlot==outputSlot || layer>Layers || role>=LayerRoles)
			return false;
		if (outputSlot==DataflowGraphVocabularyOutputSlot) {
			if (layer!=Layers || role!=TiedOutput) return false;
		} else if (outputSlot>=DataflowGraphUpOutputSlot) {
			unsigned targetIndex=0,expectedRole=0;
			if (!DataflowGraphProjectionTarget(outputSlot,targetIndex)
				|| !DataflowGraphProjectionRole(layer,targetIndex,expectedRole)
				|| role!=expectedRole) return false;
		}
		const TensorSpace* weight=fManifest.Weight(layer,role);
		unsigned workspaceIndex=0;
		const bool graphOutput=outputSlot>=DataflowGraphUpOutputSlot;
		if (graphOutput
			&& !DataflowGraphProjectionTarget(outputSlot,workspaceIndex)) return false;
		const RowSpace* output=graphOutput
			?&fGraphWorkspace.space[workspaceIndex]
			:outputSlot<ActivationBuffers?&fManifest.activation[outputSlot]:nullptr;
		if (!weight || !DataflowProgramBuilder::Build(*weight,
			fManifest.activation[inputSlot],weight->inputs,
			*output,fRoutes,program,fProjectionScheduleScratch)) return false;
		return Authorize(program);
	}

	bool BuildArgmax(DataflowArgmaxProgram& program) const
	{
		if (!fBound || fPending || !VocabularyReady()) return false;
		return DataflowArgmaxProgramBuilder::Build(fManifest,fGraphWorkspace,
			fRoutes,program) && AuthorizeArgmax(program);
	}

	bool AuthorizeArgmax(const DataflowArgmaxProgram& program) const
	{
		return fBound && !fPending && VocabularyReady()
			&& DataflowArgmaxMatchesWorkspace(fManifest,fGraphWorkspace,
				fRoutes,program);
	}

	bool BeginArgmax(const DataflowArgmaxProgram& program)
	{
		if (!AuthorizeArgmax(program)) return false;
		fPending=true;
		fPendingOutput=DataflowGraphVocabularyOutputSlot;
		fPendingKind=PendingArgmax;
		return true;
	}

	bool PendingArgmaxMatches(const DataflowArgmaxProgram& program) const
	{
		return fBound && fPending && fPendingKind==PendingArgmax
			&& fPendingOutput==DataflowGraphVocabularyOutputSlot
			&& VocabularyReady()
			&& DataflowArgmaxMatchesWorkspace(fManifest,fGraphWorkspace,
				fRoutes,program);
	}

	bool CompleteArgmax(const DataflowArgmaxProgram& program,unsigned token)
	{
		if (token>=Vocabulary || !PendingArgmaxMatches(program)) return false;
		InvalidateVocabulary();
		fPending=false;
		fPendingOutput=ActivationBuffers;
		fPendingKind=PendingNone;
		fPendingResidualMode=0;
		fPendingLayer=~0U;
		fPendingPosition=0;
		fPendingRole=LayerRoles;
		return true;
	}

	bool BuildDenseProjection(unsigned layer,unsigned role,unsigned inputSlot,
		DataflowProjectionProgram& program) const
	{
		unsigned workspaceIndex=DataflowGraphWorkspaceBufferCount;
		if (!fBound || fPending || layer>=Layers || !Dense(layer)
			|| inputSlot>=ActivationBuffers || fActivationRows[inputSlot]<Hidden
			|| !DataflowDenseProjectionTarget(role,workspaceIndex)) return false;
		const TensorSpace* weight=fManifest.Weight(layer,role);
		if (!weight || !DataflowProgramBuilder::Build(*weight,
			fManifest.activation[inputSlot],weight->inputs,
			fGraphWorkspace.space[workspaceIndex],fRoutes,program,
			fProjectionScheduleScratch)) return false;
		return Authorize(program);
	}

	bool BuildDenseAttention(unsigned layer,unsigned position,unsigned mode,
		DataflowDenseAttentionProgram& program) const
	{
		if (!fBound || fPending || layer>=Layers || !Dense(layer)
			|| position!=fDenseNextPosition[layer]
			|| !DataflowDenseAttentionProgramBuilder::Build(fManifest,
				fGraphWorkspace,fRope,layer,position,mode,fRoutes,program)) return false;
		return AuthorizeDenseAttention(program);
	}

	bool BuildDenseAttentionOutput(unsigned layer,unsigned outputSlot,
		DataflowProjectionProgram& program) const
	{
		const unsigned context=DataflowGraphUpWorkspaceIndex;
		if (!fBound || fPending || layer>=Layers || !Dense(layer)
			|| outputSlot>=ActivationBuffers
			|| !GraphWorkspaceReady(context) || GraphWorkspaceLayer(context)!=layer
			|| GraphWorkspaceRole(context)!=AttentionOut) return false;
		const TensorSpace* weight=fManifest.Weight(layer,AttentionOut);
		if (!weight || !DataflowProgramBuilder::Build(*weight,
			fGraphWorkspace.space[context],DenseHeads*DenseHeadWidth,
			fManifest.activation[outputSlot],fRoutes,program,
			fProjectionScheduleScratch)) return false;
		return Authorize(program);
	}

	bool AuthorizeDenseAttention(
		const DataflowDenseAttentionProgram& program) const
	{
		if (!fBound || fPending || !DenseAttentionMatches(program)
			|| program.position!=fDenseNextPosition[program.layer]) return false;
		const unsigned qkv=DataflowGraphRecurrentQkvWorkspaceIndex;
		const unsigned gate=DataflowGraphRecurrentGateWorkspaceIndex;
		if (!GraphWorkspaceReady(qkv)
			|| GraphWorkspaceLayer(qkv)!=program.layer
			|| GraphWorkspaceRole(qkv)!=QueryAndGate) return false;
		if (program.mode==DataflowDenseAttentionPackKey)
			return GraphWorkspaceReady(gate)
				&& GraphWorkspaceLayer(gate)==program.layer
				&& GraphWorkspaceRole(gate)==Key
				&& !fDenseKeyStored[program.layer];
		if (program.mode==DataflowDenseAttentionPackValue)
			return GraphWorkspaceReady(gate)
				&& GraphWorkspaceLayer(gate)==program.layer
				&& GraphWorkspaceRole(gate)==Value
				&& fDenseKeyStored[program.layer]
				&& !fDenseValueStored[program.layer];
		return program.mode==DataflowDenseAttentionRun
			&& GraphWorkspaceReady(gate)
			&& GraphWorkspaceLayer(gate)==program.layer
			&& GraphWorkspaceRole(gate)==Value
			&& fDenseKeyStored[program.layer]
			&& fDenseValueStored[program.layer];
	}

	bool BeginDenseAttention(const DataflowDenseAttentionProgram& program)
	{
		if (!AuthorizeDenseAttention(program)) return false;
		fFinalNormReady=false;
		fFinalNormSlot=ActivationBuffers;
		InvalidateVocabulary();
		fPending=true;
		fPendingOutput=program.mode==DataflowDenseAttentionRun
			?DataflowGraphRecurrentQkvOutputSlot
			:DataflowGraphRecurrentGateOutputSlot;
		fPendingKind=PendingDenseAttention;
		fPendingLayer=program.layer;
		fPendingPosition=program.position;
		fPendingRole=program.mode;
		if (program.mode==DataflowDenseAttentionRun) {
			const unsigned context=DataflowGraphUpWorkspaceIndex;
			fGraphWorkspaceReady[context]=false;
			fGraphWorkspaceLayer[context]=~0U;
			fGraphWorkspaceRole[context]=LayerRoles;
		} else {
			const unsigned gate=DataflowGraphRecurrentGateWorkspaceIndex;
			fGraphWorkspaceReady[gate]=false;
			fGraphWorkspaceLayer[gate]=~0U;
			fGraphWorkspaceRole[gate]=LayerRoles;
		}
		return true;
	}

	bool PendingDenseAttentionMatches(
		const DataflowDenseAttentionProgram& program) const
	{
		return fBound && fPending && fPendingKind==PendingDenseAttention
			&& fPendingLayer==program.layer
			&& fPendingPosition==program.position && fPendingRole==program.mode
			&& fPendingOutput==(program.mode==DataflowDenseAttentionRun
				?DataflowGraphRecurrentQkvOutputSlot
				:DataflowGraphRecurrentGateOutputSlot)
			&& DenseAttentionMatches(program);
	}

	bool CompleteDenseAttention(
		const DataflowDenseAttentionProgram& program)
	{
		if (!PendingDenseAttentionMatches(program)) return false;
		if (program.mode==DataflowDenseAttentionPackKey) {
			const unsigned gate=DataflowGraphRecurrentGateWorkspaceIndex;
			fGraphWorkspaceReady[gate]=true;
			fGraphWorkspaceLayer[gate]=program.layer;
			fGraphWorkspaceRole[gate]=Key;
			fDenseKeyStored[program.layer]=true;
		} else if (program.mode==DataflowDenseAttentionPackValue) {
			const unsigned gate=DataflowGraphRecurrentGateWorkspaceIndex;
			fGraphWorkspaceReady[gate]=true;
			fGraphWorkspaceLayer[gate]=program.layer;
			fGraphWorkspaceRole[gate]=Value;
			fDenseValueStored[program.layer]=true;
		} else {
			const unsigned context=DataflowGraphUpWorkspaceIndex;
			fGraphWorkspaceReady[context]=true;
			fGraphWorkspaceLayer[context]=program.layer;
			fGraphWorkspaceRole[context]=AttentionOut;
			fDenseNextPosition[program.layer]=program.position+1;
		}
		fPending=false;
		fPendingOutput=ActivationBuffers;
		fPendingKind=PendingNone;
		fPendingLayer=~0U;
		fPendingPosition=0;
		fPendingRole=LayerRoles;
		return true;
	}

	bool BuildRecurrentOutput(unsigned layer,unsigned outputSlot,
		DataflowProjectionProgram& program) const
	{
		if (!fBound || fPending || layer>=Layers || Dense(layer)
			|| outputSlot>=ActivationBuffers || fDeltaNextPosition[layer]==0
			|| fConvNextPosition[layer]!=fDeltaNextPosition[layer]
			|| !GraphWorkspaceReady(DataflowGraphRecurrentQkvWorkspaceIndex)
			|| GraphWorkspaceLayer(DataflowGraphRecurrentQkvWorkspaceIndex)!=layer)
			return false;
		const TensorSpace* weight=fManifest.Weight(layer,RecurrentOut);
		if (!weight || !DataflowProgramBuilder::Build(*weight,
			fGraphWorkspace.space[DataflowGraphRecurrentQkvWorkspaceIndex],
			DeltaInner,fManifest.activation[outputSlot],fRoutes,program,
			fProjectionScheduleScratch,DataflowProjectionRecurrentVBase)) return false;
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
			|| !GraphUpReady() || fActivationRows[gateSlot]<Ffn
			|| !DataflowSiluProgramBuilder::Build(fManifest,fGraphWorkspace,
				gateSlot,fRoutes,program)) return false;
		return AuthorizeSilu(program);
	}

	bool AuthorizeSilu(const DataflowSiluProgram& program) const
	{
		return fBound && !fPending && GraphUpReady()
			&& program.gateSlot<ActivationBuffers
			&& fActivationRows[program.gateSlot]>=Ffn
			&& SiluMatches(program);
	}

	bool BeginSilu(const DataflowSiluProgram& program)
	{
		if (!AuthorizeSilu(program)) return false;
		fFinalNormReady=false;
		fFinalNormSlot=ActivationBuffers;
		InvalidateVocabulary();
		fActivationRows[program.gateSlot]=0;
		fGraphWorkspaceReady[DataflowGraphUpWorkspaceIndex]=false;
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

	bool BuildResidual(unsigned mode,unsigned activationSlot,
		DataflowResidualProgram& program) const
	{
		if (!fBound || fPending || activationSlot>=ActivationBuffers
			|| fActivationRows[activationSlot]<Hidden
			|| (mode!=DataflowResidualSave && mode!=DataflowResidualAdd)
			|| (mode==DataflowResidualAdd && !fGraphResidualReady)
			|| !DataflowResidualProgramBuilder::Build(fManifest,fGraphWorkspace,
				mode,activationSlot,fRoutes,program)) return false;
		return AuthorizeResidual(program);
	}

	bool AuthorizeResidual(const DataflowResidualProgram& program) const
	{
		return fBound && !fPending
			&& program.activationSlot<ActivationBuffers
			&& fActivationRows[program.activationSlot]>=Hidden
			&& (program.mode==DataflowResidualSave
				|| (program.mode==DataflowResidualAdd && fGraphResidualReady))
			&& ResidualMatches(program);
	}

	bool BeginResidual(const DataflowResidualProgram& program)
	{
		if (!AuthorizeResidual(program)) return false;
		fFinalNormReady=false;
		fFinalNormSlot=ActivationBuffers;
		InvalidateVocabulary();
		fGraphResidualReady=false;
		if (program.mode==DataflowResidualAdd)
			fActivationRows[program.activationSlot]=0;
		fPending=true;
		fPendingOutput=program.activationSlot;
		fPendingKind=PendingResidual;
		fPendingResidualMode=program.mode;
		return true;
	}

	bool PendingResidualMatches(const DataflowResidualProgram& program) const
	{
		return fBound && fPending && fPendingKind==PendingResidual
			&& program.activationSlot==fPendingOutput
			&& program.mode==fPendingResidualMode && ResidualMatches(program);
	}

	bool CompleteResidual(const DataflowResidualProgram& program)
	{
		if (!PendingResidualMatches(program)) return false;
		if (program.mode==DataflowResidualSave)
			fGraphResidualReady=true;
		else
			fActivationRows[program.activationSlot]=Hidden;
		fPending=false;
		fPendingOutput=ActivationBuffers;
		fPendingKind=PendingNone;
		fPendingResidualMode=0;
		return true;
	}

	bool BindValidatedModel(const DeviceLayout& manifest,
		const DataflowGraphWorkspace& graphWorkspace,
		const unsigned (&auditedRoutes)[2][Banks],Range rope={})
	{
		if (fBound || manifest.magic!=LayoutMagic
			|| manifest.version!=LayoutVersion || !manifest.context
			|| !DataflowBankRoutesValid(auditedRoutes)
			|| !graphWorkspace.Valid(manifest)) return false;
		fManifest=manifest;
		fGraphWorkspace=graphWorkspace;
		fRope=rope;
		for (unsigned noc=0;noc<2;noc++) for (unsigned bank=0;bank<Banks;bank++)
			fRoutes[noc][bank]=auditedRoutes[noc][bank];
		for (unsigned i=0;i<DataflowGraphWorkspaceBufferCount;i++) {
			fGraphWorkspaceReady[i]=false;
			fGraphWorkspaceLayer[i]=~0U;
			fGraphWorkspaceRole[i]=LayerRoles;
		}
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
		unsigned tiedInput=ActivationBuffers;
		if (DataflowProjectionMatchesTiedOutput(fManifest,fGraphWorkspace,
			fRoutes,program,&tiedInput))
			return fFinalNormReady && tiedInput==fFinalNormSlot
				&& fActivationRows[tiedInput]>=Hidden;
		if (RecurrentOutputMatches(program)) return true;
		unsigned denseOutput=ActivationBuffers,denseOutputLayer=Layers;
		if (DataflowProjectionMatchesDenseAttentionOutput(fManifest,
			fGraphWorkspace,fRoutes,program,&denseOutput,&denseOutputLayer)) {
			const unsigned context=DataflowGraphUpWorkspaceIndex;
			return GraphWorkspaceReady(context)
				&& GraphWorkspaceLayer(context)==denseOutputLayer
				&& GraphWorkspaceRole(context)==AttentionOut
				&& program.outputRows<=fManifest.activation[denseOutput].rows;
		}
		unsigned denseInput=ActivationBuffers,denseWorkspace=0;
		unsigned denseLayer=Layers,denseRole=LayerRoles;
		if (DataflowProjectionMatchesDenseGraphWorkspace(fManifest,fGraphWorkspace,
			fRoutes,program,&denseInput,&denseWorkspace,&denseLayer,&denseRole)) {
			if (fActivationRows[denseInput]<program.inputRows) return false;
			const unsigned qkv=DataflowGraphRecurrentQkvWorkspaceIndex;
			const unsigned gate=DataflowGraphRecurrentGateWorkspaceIndex;
			if (denseRole==QueryAndGate) return true;
			if (denseRole==Key)
				return GraphWorkspaceReady(qkv)
					&& GraphWorkspaceLayer(qkv)==denseLayer
					&& GraphWorkspaceRole(qkv)==QueryAndGate
					&& !fDenseKeyStored[denseLayer];
			if (denseRole==Value)
				return GraphWorkspaceReady(qkv)
					&& GraphWorkspaceLayer(qkv)==denseLayer
					&& GraphWorkspaceRole(qkv)==QueryAndGate
					&& fDenseKeyStored[denseLayer]
					&& GraphWorkspaceReady(gate)
					&& GraphWorkspaceLayer(gate)==denseLayer
					&& GraphWorkspaceRole(gate)==Key
					&& !fDenseValueStored[denseLayer];
			return false;
		}
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
		unsigned tiedInput=ActivationBuffers;
		if (DataflowProjectionMatchesTiedOutput(fManifest,fGraphWorkspace,
			fRoutes,program,&tiedInput)) {
			const unsigned vocabulary=DataflowGraphVocabularyWorkspaceIndex;
			fFinalNormReady=false;
			fFinalNormSlot=ActivationBuffers;
			fVocabularyReady=false;
			fGraphWorkspaceReady[vocabulary]=false;
			fGraphWorkspaceLayer[vocabulary]=~0U;
			fGraphWorkspaceRole[vocabulary]=LayerRoles;
			fPending=true;
			fPendingOutput=DataflowGraphVocabularyOutputSlot;
			fPendingKind=PendingTiedOutput;
			fPendingLayer=Layers;
			fPendingPosition=tiedInput;
			return true;
		}
		fFinalNormReady=false;
		fFinalNormSlot=ActivationBuffers;
		InvalidateVocabulary();
		unsigned recurrentOutput=ActivationBuffers,recurrentLayer=Layers;
		if (RecurrentOutputMatches(program,&recurrentOutput,&recurrentLayer)) {
			fActivationRows[recurrentOutput]=0;
			fPendingOutput=recurrentOutput;
			fPendingKind=PendingRecurrentOut;
			fPendingLayer=recurrentLayer;
			fPendingPosition=fDeltaNextPosition[recurrentLayer]-1;
		} else if (DataflowProjectionMatchesDenseAttentionOutput(fManifest,
			fGraphWorkspace,fRoutes,program,&recurrentOutput,&recurrentLayer)) {
			const unsigned context=DataflowGraphUpWorkspaceIndex;
			fActivationRows[recurrentOutput]=0;
			fGraphWorkspaceReady[context]=false;
			fGraphWorkspaceLayer[context]=~0U;
			fGraphWorkspaceRole[context]=LayerRoles;
			fPendingOutput=recurrentOutput;
			fPendingKind=PendingDenseAttentionOut;
			fPendingLayer=recurrentLayer;
		} else {
			unsigned input=ActivationBuffers,workspaceIndex=0,layer=Layers,role=LayerRoles;
			if (DataflowProjectionMatchesDenseGraphWorkspace(fManifest,
				fGraphWorkspace,fRoutes,program,&input,&workspaceIndex,&layer,&role)) {
				fGraphWorkspaceReady[workspaceIndex]=false;
				fGraphWorkspaceLayer[workspaceIndex]=~0U;
				fGraphWorkspaceRole[workspaceIndex]=LayerRoles;
				fPendingOutput=DataflowGraphProjectionOutputSlot(workspaceIndex);
				fPendingKind=PendingDenseGraphProjection;
				fPendingLayer=layer;
				fPendingRole=role;
			} else if (DataflowProjectionMatchesLayout(fManifest,fRoutes,program)) {
				unsigned input=ActivationBuffers,output=ActivationBuffers;
				if (!Slots(program,input,output)) return false;
				fActivationRows[output]=0;
				fPendingOutput=output;
				fPendingKind=PendingProjection;
			} else {
				unsigned workspaceIndex=0;
				if (!DataflowProjectionMatchesGraphWorkspace(fManifest,fGraphWorkspace,
					fRoutes,program,nullptr,&workspaceIndex)) return false;
				fGraphWorkspaceReady[workspaceIndex]=false;
				fGraphWorkspaceLayer[workspaceIndex]=~0U;
				fGraphWorkspaceRole[workspaceIndex]=LayerRoles;
				fPendingOutput=DataflowGraphProjectionOutputSlot(workspaceIndex);
				fPendingKind=PendingGraphProjection;
			}
		}
		fPending=true;
		return true;
	}

	bool PendingProjectionMatches(const DataflowProjectionProgram& program) const
	{
		if (!fBound || !fPending) return false;
		if (fPendingKind==PendingTiedOutput) {
			unsigned input=ActivationBuffers;
			return fPendingOutput==DataflowGraphVocabularyOutputSlot
				&& DataflowProjectionMatchesTiedOutput(fManifest,fGraphWorkspace,
					fRoutes,program,&input)
				&& input==fPendingPosition && input<ActivationBuffers
				&& fActivationRows[input]>=Hidden;
		}
		if (fPendingKind==PendingRecurrentOut) {
			unsigned output=ActivationBuffers,layer=Layers;
			return RecurrentOutputMatches(program,&output,&layer)
				&& output==fPendingOutput && layer==fPendingLayer
				&& fDeltaNextPosition[layer]-1==fPendingPosition;
		}
		if (fPendingKind==PendingDenseAttentionOut) {
			unsigned output=ActivationBuffers,layer=Layers;
			return DataflowProjectionMatchesDenseAttentionOutput(fManifest,
				fGraphWorkspace,fRoutes,program,&output,&layer)
				&& output==fPendingOutput && layer==fPendingLayer
				&& fActivationRows[output]==0;
		}
		if (fPendingKind==PendingProjection) {
			unsigned input=ActivationBuffers,output=ActivationBuffers;
			return DataflowProjectionMatchesLayout(fManifest,fRoutes,program)
				&& Slots(program,input,output) && output==fPendingOutput
				&& fActivationRows[input]>=program.inputRows;
		}
		if (fPendingKind==PendingDenseGraphProjection) {
			unsigned input=ActivationBuffers,workspaceIndex=0,layer=Layers;
			unsigned role=LayerRoles;
			if (!DataflowProjectionMatchesDenseGraphWorkspace(fManifest,
				fGraphWorkspace,fRoutes,program,&input,&workspaceIndex,&layer,&role))
				return false;
			return fPendingOutput==DataflowGraphProjectionOutputSlot(workspaceIndex)
				&& layer==fPendingLayer && role==fPendingRole
				&& fActivationRows[input]>=program.inputRows;
		}
		unsigned input=ActivationBuffers,workspaceIndex=0;
		return fPendingKind==PendingGraphProjection
			&& DataflowProjectionMatchesGraphWorkspace(fManifest,fGraphWorkspace,
				fRoutes,program,&input,&workspaceIndex)
			&& fPendingOutput==DataflowGraphProjectionOutputSlot(workspaceIndex)
			&& fActivationRows[input]>=program.inputRows;
	}

	bool BeginEmbedding(const DataflowEmbeddingProgram& program)
	{
		if (!AuthorizeEmbedding(program)
			|| program.outputSlot>=ActivationBuffers) return false;
		fFinalNormReady=false;
		fFinalNormSlot=ActivationBuffers;
		InvalidateVocabulary();
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
		fFinalNormReady=false;
		fFinalNormSlot=ActivationBuffers;
		InvalidateVocabulary();
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
		if (fPendingKind==PendingTiedOutput) {
			const unsigned vocabulary=DataflowGraphVocabularyWorkspaceIndex;
			fGraphWorkspaceReady[vocabulary]=true;
			fGraphWorkspaceLayer[vocabulary]=Layers;
			fGraphWorkspaceRole[vocabulary]=TiedOutput;
			fVocabularyReady=true;
		} else if (fPendingKind==PendingRecurrentOut) {
			if (fPendingOutput>=ActivationBuffers
				|| program.outputRows>fManifest.activation[fPendingOutput].rows)
				return false;
			fActivationRows[fPendingOutput]=program.outputRows;
			fGraphWorkspaceReady[DataflowGraphRecurrentQkvWorkspaceIndex]=false;
			fGraphWorkspaceLayer[DataflowGraphRecurrentQkvWorkspaceIndex]=~0U;
			fGraphWorkspaceRole[DataflowGraphRecurrentQkvWorkspaceIndex]=LayerRoles;
		} else if (fPendingKind==PendingDenseAttentionOut) {
			unsigned output=ActivationBuffers,layer=Layers;
			if (!DataflowProjectionMatchesDenseAttentionOutput(fManifest,
				fGraphWorkspace,fRoutes,program,&output,&layer)
				|| output!=fPendingOutput || layer!=fPendingLayer
				|| program.outputRows>fManifest.activation[output].rows)
				return false;
			fActivationRows[output]=program.outputRows;
		} else if (fPendingKind==PendingDenseGraphProjection) {
			unsigned input=ActivationBuffers,workspaceIndex=0,layer=Layers;
			unsigned role=LayerRoles;
			if (!DataflowProjectionMatchesDenseGraphWorkspace(fManifest,
				fGraphWorkspace,fRoutes,program,&input,&workspaceIndex,&layer,&role)
				|| fPendingOutput!=DataflowGraphProjectionOutputSlot(workspaceIndex)
				|| layer!=fPendingLayer || role!=fPendingRole
				|| fActivationRows[input]<program.inputRows) return false;
			fGraphWorkspaceReady[workspaceIndex]=true;
			fGraphWorkspaceLayer[workspaceIndex]=layer;
			fGraphWorkspaceRole[workspaceIndex]=role;
			if (role==QueryAndGate) {
				fDenseKeyStored[layer]=false;
				fDenseValueStored[layer]=false;
			} else if (role==Key)
				fDenseKeyStored[layer]=false;
			else if (role==Value)
				fDenseValueStored[layer]=false;
		} else if (fPendingKind==PendingGraphProjection) {
			unsigned input=ActivationBuffers,workspaceIndex=0,modelLayer=0,modelRole=LayerRoles;
			if (!DataflowProjectionMatchesGraphWorkspace(fManifest,fGraphWorkspace,
				fRoutes,program,&input,&workspaceIndex,&modelLayer,&modelRole)
				|| fPendingOutput!=DataflowGraphProjectionOutputSlot(workspaceIndex)
				|| fActivationRows[input]<program.inputRows) return false;
			fGraphWorkspaceReady[workspaceIndex]=true;
			fGraphWorkspaceLayer[workspaceIndex]=modelLayer;
			fGraphWorkspaceRole[workspaceIndex]=modelRole;
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
		fPendingLayer=~0U;
		fPendingPosition=0;
		fPendingRole=LayerRoles;
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
		if (program.layer==Layers && program.role==FinalNorm) {
			fFinalNormReady=true;
			fFinalNormSlot=program.outputSlot;
		}
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
			unsigned workspaceIndex=0;
			if (!DataflowGraphProjectionTarget(fPendingOutput,workspaceIndex))
				return false;
			fGraphWorkspaceReady[workspaceIndex]=false;
		} else if (fPendingKind==PendingSilu) {
			if (fPendingOutput>=ActivationBuffers) return false;
			fActivationRows[fPendingOutput]=0;
			fGraphWorkspaceReady[DataflowGraphUpWorkspaceIndex]=false;
		} else if (fPendingKind==PendingEmbedding) {
			if (fPendingOutput>=ActivationBuffers) return false;
			fActivationRows[fPendingOutput]=0;
			InvalidateVocabulary();
		} else if (fPendingKind==PendingResidual) {
			if (fPendingOutput>=ActivationBuffers) return false;
			if (fPendingResidualMode==DataflowResidualAdd)
				fActivationRows[fPendingOutput]=0;
			fGraphResidualReady=false;
		} else if (fPendingKind==PendingConv) {
			if (fPendingOutput!=DataflowGraphRecurrentQkvOutputSlot
				|| fPendingLayer>=Layers) return false;
			fGraphWorkspaceReady[DataflowGraphRecurrentQkvWorkspaceIndex]=false;
			fGraphWorkspaceLayer[DataflowGraphRecurrentQkvWorkspaceIndex]=~0U;
			fGraphWorkspaceRole[DataflowGraphRecurrentQkvWorkspaceIndex]=LayerRoles;
		} else if (fPendingKind==PendingDelta) {
			if (fPendingOutput!=DataflowGraphRecurrentQkvOutputSlot
				|| fPendingLayer>=Layers) return false;
			fGraphWorkspaceReady[DataflowGraphRecurrentQkvWorkspaceIndex]=false;
			fGraphWorkspaceRole[DataflowGraphRecurrentQkvWorkspaceIndex]=LayerRoles;
			fGraphWorkspaceReady[DataflowGraphRecurrentAlphaWorkspaceIndex]=false;
			fGraphWorkspaceReady[DataflowGraphRecurrentBetaWorkspaceIndex]=false;
			fGraphWorkspaceRole[DataflowGraphRecurrentAlphaWorkspaceIndex]=LayerRoles;
			fGraphWorkspaceRole[DataflowGraphRecurrentBetaWorkspaceIndex]=LayerRoles;
			fGraphWorkspaceLayer[DataflowGraphRecurrentAlphaWorkspaceIndex]=~0U;
			fGraphWorkspaceLayer[DataflowGraphRecurrentBetaWorkspaceIndex]=~0U;
		} else if (fPendingKind==PendingRecurrentOut) {
			if (fPendingOutput>=ActivationBuffers || fPendingLayer>=Layers)
				return false;
			fActivationRows[fPendingOutput]=0;
			fGraphWorkspaceReady[DataflowGraphRecurrentQkvWorkspaceIndex]=false;
			fGraphWorkspaceLayer[DataflowGraphRecurrentQkvWorkspaceIndex]=~0U;
			fGraphWorkspaceRole[DataflowGraphRecurrentQkvWorkspaceIndex]=LayerRoles;
		} else if (fPendingKind==PendingDenseAttentionOut) {
			if (fPendingOutput>=ActivationBuffers || fPendingLayer>=Layers)
				return false;
			fActivationRows[fPendingOutput]=0;
			fGraphWorkspaceReady[DataflowGraphUpWorkspaceIndex]=false;
			fGraphWorkspaceLayer[DataflowGraphUpWorkspaceIndex]=~0U;
			fGraphWorkspaceRole[DataflowGraphUpWorkspaceIndex]=LayerRoles;
		} else if (fPendingKind==PendingTiedOutput) {
			const unsigned vocabulary=DataflowGraphVocabularyWorkspaceIndex;
			fGraphWorkspaceReady[vocabulary]=false;
			fGraphWorkspaceLayer[vocabulary]=~0U;
			fGraphWorkspaceRole[vocabulary]=LayerRoles;
			fVocabularyReady=false;
		} else if (fPendingKind==PendingArgmax) {
			InvalidateVocabulary();
		} else if (fPendingKind==PendingDenseAttention) {
			unsigned workspaceIndex=0;
			if (fPendingLayer>=Layers
				|| (fPendingRole!=DataflowDenseAttentionRun
					&& fPendingRole!=DataflowDenseAttentionPackKey
					&& fPendingRole!=DataflowDenseAttentionPackValue)
				|| !DataflowGraphProjectionTarget(fPendingOutput,workspaceIndex)
				|| workspaceIndex>=DataflowGraphWorkspaceBufferCount) return false;
			fGraphWorkspaceReady[workspaceIndex]=false;
			fGraphWorkspaceLayer[workspaceIndex]=~0U;
			fGraphWorkspaceRole[workspaceIndex]=LayerRoles;
		} else if (fPendingKind==PendingDenseGraphProjection) {
			unsigned workspaceIndex=0;
			if (fPendingOutput<DataflowGraphUpOutputSlot
				|| !DataflowGraphProjectionTarget(fPendingOutput,workspaceIndex)
				|| workspaceIndex>=DataflowGraphWorkspaceBufferCount) return false;
			fGraphWorkspaceReady[workspaceIndex]=false;
			fGraphWorkspaceLayer[workspaceIndex]=~0U;
			fGraphWorkspaceRole[workspaceIndex]=LayerRoles;
		} else {
			if (fPendingKind!=PendingProjection || fPendingOutput>=ActivationBuffers)
				return false;
			fActivationRows[fPendingOutput]=0;
		}
		fPending=false;
		fPendingOutput=ActivationBuffers;
		fPendingKind=PendingNone;
		fPendingResidualMode=0;
		fPendingLayer=~0U;
		fPendingPosition=0;
		fPendingRole=LayerRoles;
		fPendingRole=LayerRoles;
		return true;
	}
};

} // namespace PioneerQwen35

#endif
