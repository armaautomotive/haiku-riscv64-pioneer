/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_DELTA_PROGRAM_H
#define PIONEER_QWEN35_DATAFLOW_DELTA_PROGRAM_H

#include "PioneerQwen35DataflowGraphWorkspace.h"
#include "PioneerQwen35DataflowProgram.h"

#if defined(__GNUC__) && !defined(__clang__)
#define PIONEER_QWEN35_DELTA_PROGRAM_SIZE_OPT __attribute__((optimize("Os")))
#else
#define PIONEER_QWEN35_DELTA_PROGRAM_SIZE_OPT
#endif

namespace PioneerQwen35 {

enum : unsigned {
	DataflowDeltaMagic=0x314c4435,
	DataflowDeltaVersion=1,
	DataflowDeltaPanelsPerHead=DeltaWidth/16,
	DataflowDeltaPanelCount=DeltaHeads*DataflowDeltaPanelsPerHead,
	DataflowDeltaBaseHeads=DeltaHeads/DataflowCoreCount,
	DataflowDeltaRemainderHeads=DeltaHeads%DataflowCoreCount,
	DataflowDeltaQChannels=DeltaKeyHeads*DeltaWidth,
	DataflowDeltaKChannels=DeltaKeyHeads*DeltaWidth,
	DataflowDeltaVBase=DataflowDeltaQChannels+DataflowDeltaKChannels,
	DataflowDeltaPanelBytes=16*sizeof(float),
	DataflowDeltaHeadsPanelBytes=16*sizeof(float),
	DataflowDeltaStatePanelBytes=16*DeltaWidth*sizeof(float),
	DataflowDeltaHeadStateBytes=DeltaStateWords*sizeof(float)
};

constexpr unsigned DataflowDeltaFirstHead(unsigned core)
{
	return core*DataflowDeltaBaseHeads
		+(core<DataflowDeltaRemainderHeads?core:DataflowDeltaRemainderHeads);
}

constexpr unsigned DataflowDeltaHeadsForCore(unsigned core)
{
	return DataflowDeltaBaseHeads
		+unsigned(core<DataflowDeltaRemainderHeads);
}

struct DataflowDeltaProgram {
	unsigned layer,position;
	unsigned bankRoute[2][Banks];
	RowSpace qkv,gate,alpha,beta,stateRead,stateWrite;
	TensorSpace negativeA,deltaBias,deltaNorm;

	PIONEER_QWEN35_DELTA_PROGRAM_SIZE_OPT bool ShapeValid() const
	{
		const RowSpace& negA=negativeA.place;
		const RowSpace& bias=deltaBias.place;
		const RowSpace& norm=deltaNorm.place;
		if (layer>=Layers || Dense(layer) || position>=MaxPositions
			|| !DataflowBankRoutesValid(bankRoute)
			|| !qkv.Valid() || qkv.rows!=ConvChannels
			|| qkv.rowBytes!=sizeof(float) || qkv.unit!=16
			|| !gate.Valid() || gate.rows!=DeltaInner
			|| gate.rowBytes!=sizeof(float) || gate.unit!=16
			|| !alpha.Valid() || alpha.rows!=DeltaHeads
			|| alpha.rowBytes!=sizeof(float) || alpha.unit!=16
			|| !beta.Valid() || beta.rows!=DeltaHeads
			|| beta.rowBytes!=sizeof(float) || beta.unit!=16
			|| negativeA.type!=0 || negativeA.inputs!=DeltaHeads
			|| !negA.Valid() || negA.rows!=1
			|| negA.rowBytes!=DeltaHeads*sizeof(float) || negA.unit!=1
			|| deltaBias.type!=0 || deltaBias.inputs!=DeltaHeads
			|| !bias.Valid() || bias.rows!=1
			|| bias.rowBytes!=DeltaHeads*sizeof(float) || bias.unit!=1
			|| deltaNorm.type!=0 || deltaNorm.inputs!=DeltaWidth
			|| !norm.Valid() || norm.rows!=1
			|| norm.rowBytes!=DeltaWidth*sizeof(float) || norm.unit!=1
			|| !stateRead.Valid() || stateRead.rows!=DeltaHeads
			|| stateRead.rowBytes!=DataflowDeltaHeadStateBytes
			|| stateRead.unit!=1 || !stateWrite.Valid()
			|| stateWrite.rows!=DeltaHeads
			|| stateWrite.rowBytes!=DataflowDeltaHeadStateBytes
			|| stateWrite.unit!=1
			|| !DataflowGraphWorkspace::Disjoint(stateRead,stateWrite)
			|| !DataflowGraphWorkspace::Disjoint(qkv,gate)
			|| !DataflowGraphWorkspace::Disjoint(qkv,alpha)
			|| !DataflowGraphWorkspace::Disjoint(qkv,beta)
			|| !DataflowGraphWorkspace::Disjoint(gate,alpha)
			|| !DataflowGraphWorkspace::Disjoint(gate,beta)
			|| !DataflowGraphWorkspace::Disjoint(alpha,beta)
			|| !DataflowGraphWorkspace::Disjoint(qkv,stateRead)
			|| !DataflowGraphWorkspace::Disjoint(qkv,stateWrite)
			|| !DataflowGraphWorkspace::Disjoint(gate,stateRead)
			|| !DataflowGraphWorkspace::Disjoint(gate,stateWrite)
			|| !DataflowGraphWorkspace::Disjoint(alpha,stateRead)
			|| !DataflowGraphWorkspace::Disjoint(alpha,stateWrite)
			|| !DataflowGraphWorkspace::Disjoint(beta,stateRead)
			|| !DataflowGraphWorkspace::Disjoint(beta,stateWrite)
			|| !DataflowGraphWorkspace::Disjoint(negA,qkv)
			|| !DataflowGraphWorkspace::Disjoint(negA,gate)
			|| !DataflowGraphWorkspace::Disjoint(negA,alpha)
			|| !DataflowGraphWorkspace::Disjoint(negA,beta)
			|| !DataflowGraphWorkspace::Disjoint(negA,stateRead)
			|| !DataflowGraphWorkspace::Disjoint(negA,stateWrite)
			|| !DataflowGraphWorkspace::Disjoint(bias,qkv)
			|| !DataflowGraphWorkspace::Disjoint(bias,gate)
			|| !DataflowGraphWorkspace::Disjoint(bias,alpha)
			|| !DataflowGraphWorkspace::Disjoint(bias,beta)
			|| !DataflowGraphWorkspace::Disjoint(bias,stateRead)
			|| !DataflowGraphWorkspace::Disjoint(bias,stateWrite)
			|| !DataflowGraphWorkspace::Disjoint(negA,bias)
			|| !DataflowGraphWorkspace::Disjoint(norm,qkv)
			|| !DataflowGraphWorkspace::Disjoint(norm,gate)
			|| !DataflowGraphWorkspace::Disjoint(norm,alpha)
			|| !DataflowGraphWorkspace::Disjoint(norm,beta)
			|| !DataflowGraphWorkspace::Disjoint(norm,stateRead)
			|| !DataflowGraphWorkspace::Disjoint(norm,stateWrite)
			|| !DataflowGraphWorkspace::Disjoint(norm,negA)
			|| !DataflowGraphWorkspace::Disjoint(norm,bias)) return false;
		for (unsigned firstHead=0;firstHead<DeltaHeads;firstHead+=16) {
			Range alphaRange,betaRange;
			if (!alpha.Resolve(firstHead,16,0,DataflowDeltaHeadsPanelBytes,alphaRange)
				|| !beta.Resolve(firstHead,16,0,DataflowDeltaHeadsPanelBytes,betaRange)
				|| !AlignedRange(alphaRange,DataflowDeltaHeadsPanelBytes)
				|| !AlignedRange(betaRange,DataflowDeltaHeadsPanelBytes)) return false;
		}
		Range negativeARange,deltaBiasRange,deltaNormRange;
		if (!negativeA.place.Resolve(0,1,0,DeltaHeads*sizeof(float),negativeARange)
			|| !deltaBias.place.Resolve(0,1,0,DeltaHeads*sizeof(float),deltaBiasRange)
			|| !deltaNorm.place.Resolve(0,1,0,DeltaWidth*sizeof(float),deltaNormRange)
			|| !AlignedRange(negativeARange,DeltaHeads*sizeof(float))
			|| !AlignedRange(deltaBiasRange,DeltaHeads*sizeof(float))
			|| !AlignedRange(deltaNormRange,DeltaWidth*sizeof(float))) return false;
		return true;
	}

	PIONEER_QWEN35_DELTA_PROGRAM_SIZE_OPT static bool
	AlignedRange(const Range& range,unsigned bytes)
	{
		return range.bank<Banks && range.bytes==bytes
			&& range.address>=BankBase && range.address<BankLimit
			&& bytes<=BankLimit-range.address && !(range.address&63)
			&& !(bytes&63);
	}

	PIONEER_QWEN35_DELTA_PROGRAM_SIZE_OPT bool
	PanelRangesValid(unsigned panel) const
	{
		if (panel>=DataflowDeltaPanelCount) return false;
		const unsigned head=panel/DataflowDeltaPanelsPerHead;
		const unsigned subPanel=panel%DataflowDeltaPanelsPerHead;
		const unsigned valueRow=head*DeltaWidth+subPanel*16;
		Range valueRange,stateIn,stateOut;
		if (!qkv.Resolve(DataflowDeltaVBase+valueRow,16,0,
				DataflowDeltaPanelBytes,valueRange)
			|| !stateRead.Resolve(head,1,
				subPanel*16*DeltaWidth*sizeof(float),
				DataflowDeltaStatePanelBytes,stateIn)
			|| !stateWrite.Resolve(head,1,
				subPanel*16*DeltaWidth*sizeof(float),
				DataflowDeltaStatePanelBytes,stateOut)
			|| !AlignedRange(valueRange,DataflowDeltaPanelBytes)
			|| !AlignedRange(stateIn,DataflowDeltaStatePanelBytes)
			|| !AlignedRange(stateOut,DataflowDeltaStatePanelBytes)) return false;
		return true;
	}

	bool Valid() const
	{
		if (!ShapeValid()) return false;
		for (unsigned panel=0;panel<DataflowDeltaPanelCount;panel++)
			if (!PanelRangesValid(panel)) return false;
		return true;
	}
};

struct DataflowDeltaInvocation {
	unsigned magic,version,core,firstHead,headCount;
	DataflowDeltaProgram program;

	bool Valid() const
	{
		if (magic!=DataflowDeltaMagic || version!=DataflowDeltaVersion
			|| core>=DataflowCoreCount || !program.ShapeValid()
			|| firstHead!=DataflowDeltaFirstHead(core)
			|| headCount!=DataflowDeltaHeadsForCore(core)
			|| headCount>DeltaHeads-firstHead) return false;
		for (unsigned head=firstHead;head<firstHead+headCount;head++)
			for (unsigned panel=0;panel<DataflowDeltaPanelsPerHead;panel++)
				if (!program.PanelRangesValid(head*DataflowDeltaPanelsPerHead+panel))
					return false;
		return true;
	}
};

class DataflowDeltaProgramBuilder {
public:
	static bool Build(const DeviceLayout& manifest,
		const DataflowGraphWorkspace& workspace,unsigned layer,unsigned position,
		const unsigned (&bankRoute)[2][Banks],DataflowDeltaProgram& result)
	{
		result={};
		if (manifest.magic!=LayoutMagic || manifest.version!=LayoutVersion
			|| !manifest.context || position>=manifest.context
			|| position>=MaxPositions || layer>=Layers || Dense(layer)
			|| !workspace.Valid(manifest) || !DataflowBankRoutesValid(bankRoute))
			return false;
		const TensorSpace* negativeA=manifest.Weight(layer,NegativeA);
		const TensorSpace* deltaBias=manifest.Weight(layer,DeltaBias);
		const TensorSpace* deltaNorm=manifest.Weight(layer,DeltaNorm);
		if (!negativeA || !deltaBias || !deltaNorm || negativeA->type!=0
			|| negativeA->inputs!=DeltaHeads || negativeA->place.rows!=1
			|| negativeA->place.rowBytes!=DeltaHeads*sizeof(float)
			|| negativeA->place.unit!=1 || deltaBias->type!=0
			|| deltaBias->inputs!=DeltaHeads || deltaBias->place.rows!=1
			|| deltaBias->place.rowBytes!=DeltaHeads*sizeof(float)
			|| deltaBias->place.unit!=1 || deltaNorm->type!=0
			|| deltaNorm->inputs!=DeltaWidth || deltaNorm->place.rows!=1
			|| deltaNorm->place.rowBytes!=DeltaWidth*sizeof(float)
			|| deltaNorm->place.unit!=1) return false;
		DataflowDeltaProgram candidate{};
		candidate.layer=layer;
		candidate.position=position;
		for (unsigned noc=0;noc<2;noc++) for (unsigned bank=0;bank<Banks;bank++)
			candidate.bankRoute[noc][bank]=bankRoute[noc][bank];
		candidate.qkv=workspace.space[DataflowGraphRecurrentQkvWorkspaceIndex];
		candidate.gate=workspace.space[DataflowGraphRecurrentGateWorkspaceIndex];
		candidate.alpha=workspace.space[DataflowGraphRecurrentAlphaWorkspaceIndex];
		candidate.beta=workspace.space[DataflowGraphRecurrentBetaWorkspaceIndex];
		candidate.negativeA=*negativeA;
		candidate.deltaBias=*deltaBias;
		candidate.deltaNorm=*deltaNorm;
		candidate.stateRead=manifest.layers[layer].delta[position&1];
		candidate.stateWrite=manifest.layers[layer].delta[(position&1)^1];
		if (!candidate.Valid()) return false;
		result=candidate;
		return true;
	}

	static bool BindCore(const DataflowDeltaProgram& program,unsigned core,
		DataflowDeltaInvocation& result)
	{
		if (!program.Valid() || core>=DataflowCoreCount) return false;
		DataflowDeltaInvocation candidate{};
		candidate.magic=DataflowDeltaMagic;
		candidate.version=DataflowDeltaVersion;
		candidate.core=core;
		candidate.firstHead=DataflowDeltaFirstHead(core);
		candidate.headCount=DataflowDeltaHeadsForCore(core);
		candidate.program=program;
		if (!candidate.Valid()) return false;
		result=candidate;
		return true;
	}
};

static_assert(DataflowDeltaPanelCount==256 && DataflowDeltaBaseHeads==0
	&& DataflowDeltaRemainderHeads==unsigned(DeltaHeads),
	"the recurrent head owner keeps each RMS normalization group on one worker");
static_assert(sizeof(DataflowDeltaInvocation)<=DataflowProgramRegionBytes,
	"the recurrent state invocation fits the resident L1 command slot");

} // namespace PioneerQwen35

#undef PIONEER_QWEN35_DELTA_PROGRAM_SIZE_OPT

#endif
