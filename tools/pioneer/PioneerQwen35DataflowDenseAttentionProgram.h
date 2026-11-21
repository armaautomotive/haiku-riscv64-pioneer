/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_DENSE_ATTENTION_PROGRAM_H
#define PIONEER_QWEN35_DATAFLOW_DENSE_ATTENTION_PROGRAM_H

#include "PioneerQwen35DataflowGraphWorkspace.h"
#include "PioneerQwen35DataflowProgram.h"

namespace PioneerQwen35 {

enum : unsigned {
	DataflowDenseAttentionMagic=0x31414435,
	DataflowDenseAttentionVersion=1,
	DataflowDenseAttentionPackKey=1,
	DataflowDenseAttentionPackValue=2,
	DataflowDenseAttentionRun=3,
	DataflowDenseAttentionKvHeadsPerCore=1,
	DataflowDenseAttentionQueryHeadsPerCore=1,
	DataflowDenseAttentionPackCoreCount=DenseKeyHeads,
	DataflowDenseAttentionRunCoreCount=DenseHeads,
	DataflowDenseAttentionHeadFloats=DenseHeadWidth*sizeof(float),
	DataflowDenseAttentionHeadBytes=DenseHeadWidth*sizeof(uint16_t)
};

struct DataflowDenseAttentionProgram {
	unsigned layer,position,context,mode;
	unsigned bankRoute[2][Banks];
	RowSpace qkv,gate,contextOutput,kv;
	TensorSpace queryNorm,keyNorm;
	Range rope;

	static bool AlignedRange(const Range& range,unsigned bytes)
	{
		return range.bank<Banks && range.bytes==bytes
			&& range.address>=BankBase && range.address<BankLimit
			&& bytes<=BankLimit-range.address && !(range.address&63)
			&& !(bytes&63);
	}

	bool ShapeValid() const
	{
		const unsigned normBytes=DenseHeadWidth*sizeof(float);
		if (layer>=Layers || !Dense(layer) || !context || context>MaxPositions
			|| position>=context
			|| (mode!=DataflowDenseAttentionPackKey
				&& mode!=DataflowDenseAttentionPackValue
				&& mode!=DataflowDenseAttentionRun)
			|| !DataflowBankRoutesValid(bankRoute)
			|| !qkv.Valid() || qkv.rows!=ConvChannels
			|| qkv.rowBytes!=sizeof(float) || qkv.unit!=16
			|| !gate.Valid() || gate.rows!=DeltaInner
			|| gate.rowBytes!=sizeof(float) || gate.unit!=16
			|| !contextOutput.Valid() || contextOutput.rows!=Ffn
			|| contextOutput.rowBytes!=sizeof(float) || contextOutput.unit!=16
			|| !kv.Valid() || kv.rows!=2*DenseKeyHeads
			|| kv.rowBytes!=context*DenseHeadWidth*sizeof(uint16_t)
			|| kv.unit!=1
			|| queryNorm.type!=0 || queryNorm.inputs!=DenseHeadWidth
			|| !queryNorm.place.Valid() || queryNorm.place.rows!=1
			|| queryNorm.place.rowBytes!=normBytes || queryNorm.place.unit!=1
			|| keyNorm.type!=0 || keyNorm.inputs!=DenseHeadWidth
			|| !keyNorm.place.Valid() || keyNorm.place.rows!=1
			|| keyNorm.place.rowBytes!=normBytes || keyNorm.place.unit!=1
			|| rope.bank!=0 || rope.bytes!=context*Rotary*sizeof(float)
			|| rope.address<BankBase || rope.address>=BankLimit
			|| rope.bytes>BankLimit-rope.address || (rope.address&63)
			|| !DataflowGraphWorkspace::Disjoint(qkv,gate)
			|| !DataflowGraphWorkspace::Disjoint(qkv,contextOutput)
			|| !DataflowGraphWorkspace::Disjoint(gate,contextOutput)
			|| !DataflowGraphWorkspace::Disjoint(contextOutput,kv)
			|| !DataflowGraphWorkspace::Disjoint(qkv,kv)
			|| !DataflowGraphWorkspace::Disjoint(gate,kv)
			|| !DataflowGraphWorkspace::Disjoint(qkv,queryNorm.place)
			|| !DataflowGraphWorkspace::Disjoint(qkv,keyNorm.place)
			|| !DataflowGraphWorkspace::Disjoint(gate,queryNorm.place)
			|| !DataflowGraphWorkspace::Disjoint(gate,keyNorm.place)
			|| !DataflowGraphWorkspace::Disjoint(contextOutput,queryNorm.place)
			|| !DataflowGraphWorkspace::Disjoint(contextOutput,keyNorm.place)
			|| !DataflowGraphWorkspace::Disjoint(kv,queryNorm.place)
			|| !DataflowGraphWorkspace::Disjoint(kv,keyNorm.place)) return false;
		const unsigned ropeOffset=position*Rotary*sizeof(float);
		Range positionRope{rope.bank,rope.address+ropeOffset,
			Rotary*sizeof(float)};
		if (!AlignedRange(positionRope,Rotary*sizeof(float))) return false;
		for (unsigned head=0;head<DenseKeyHeads;head++) {
			Range keyRange{},valueRange{};
			if (!kv.Resolve(head,1,position*DataflowDenseAttentionHeadBytes,
				DataflowDenseAttentionHeadBytes,keyRange)
				|| !kv.Resolve(head+DenseKeyHeads,1,
					position*DataflowDenseAttentionHeadBytes,
					DataflowDenseAttentionHeadBytes,valueRange)
				|| !AlignedRange(keyRange,DataflowDenseAttentionHeadBytes)
				|| !AlignedRange(valueRange,DataflowDenseAttentionHeadBytes)) return false;
		}
		return true;
	}

	bool Valid() const
	{
		// ShapeValid checks the current position's range for each KV head.
		// RowSpace::Valid plus the checked context bound prove earlier slices
		// are contained; do not scan every prior token in the hot host path.
		return ShapeValid();
	}
};

inline void DataflowDenseAttentionClear(DataflowDenseAttentionProgram& program)
{
	static_assert(sizeof(program)%sizeof(uint32_t)==0,
		"dense attention descriptor clears in native words");
	uint32_t* words=reinterpret_cast<uint32_t*>(&program);
	for (unsigned i=0;i<sizeof(program)/sizeof(uint32_t);i++) words[i]=0;
}

struct DataflowDenseAttentionInvocation {
	unsigned magic,version,core,firstHead,headCount;
	DataflowDenseAttentionProgram program;

	bool Valid() const
	{
		if (magic!=DataflowDenseAttentionMagic
			|| version!=DataflowDenseAttentionVersion
			|| core>=DataflowCoreCount || !program.ShapeValid()) return false;
		const unsigned count=program.mode==DataflowDenseAttentionRun
			?DataflowDenseAttentionRunCoreCount:DataflowDenseAttentionPackCoreCount;
		const unsigned limit=program.mode==DataflowDenseAttentionRun
			?DenseHeads:DenseKeyHeads;
		const unsigned expected=core<count?1U:0U;
		const unsigned first=core<count?core:limit;
		return headCount==expected && firstHead==first
			&& firstHead<=limit && headCount<=limit-firstHead;
	}
};

class DataflowDenseAttentionProgramBuilder {
public:
	static bool Build(const DeviceLayout& manifest,
		const DataflowGraphWorkspace& workspace,const Range& rope,
		unsigned layer,unsigned position,unsigned mode,
		const unsigned (&bankRoute)[2][Banks],
		DataflowDenseAttentionProgram& result)
	{
		DataflowDenseAttentionClear(result);
		if (manifest.magic!=LayoutMagic || manifest.version!=LayoutVersion
			|| !manifest.context || layer>=Layers || !Dense(layer)
			|| position>=manifest.context || position>=MaxPositions
			|| !workspace.Valid(manifest) || !DataflowBankRoutesValid(bankRoute))
			return false;
		const TensorSpace* queryNorm=manifest.Weight(layer,QueryNorm);
		const TensorSpace* keyNorm=manifest.Weight(layer,KeyNorm);
		if (!queryNorm || !keyNorm) return false;
		DataflowDenseAttentionProgram candidate;
		candidate.layer=layer;
		candidate.position=position;
		candidate.context=manifest.context;
		candidate.mode=mode;
		for (unsigned noc=0;noc<2;noc++) for (unsigned bank=0;bank<Banks;bank++)
			candidate.bankRoute[noc][bank]=bankRoute[noc][bank];
		candidate.qkv=workspace.space[DataflowGraphRecurrentQkvWorkspaceIndex];
		candidate.gate=workspace.space[DataflowGraphRecurrentGateWorkspaceIndex];
		candidate.contextOutput=workspace.space[DataflowGraphUpWorkspaceIndex];
		candidate.kv=manifest.layers[layer].kv;
		candidate.queryNorm=*queryNorm;
		candidate.keyNorm=*keyNorm;
		candidate.rope=rope;
		if (!candidate.Valid()) return false;
		result=candidate;
		return true;
	}

	static bool BindCore(const DataflowDenseAttentionProgram& program,
		unsigned core,DataflowDenseAttentionInvocation& result)
	{
		if (!program.Valid() || core>=DataflowCoreCount) return false;
		DataflowDenseAttentionInvocation candidate;
		candidate.magic=DataflowDenseAttentionMagic;
		candidate.version=DataflowDenseAttentionVersion;
		candidate.core=core;
		if (program.mode==DataflowDenseAttentionRun) {
			candidate.headCount=core<DenseHeads?1U:0U;
			candidate.firstHead=core<DenseHeads?core:DenseHeads;
		} else {
			candidate.headCount=core<DenseKeyHeads?1U:0U;
			candidate.firstHead=core<DenseKeyHeads?core:DenseKeyHeads;
		}
		candidate.program=program;
		if (!candidate.Valid()) return false;
		result=candidate;
		return true;
	}
};

} // namespace PioneerQwen35

#endif
