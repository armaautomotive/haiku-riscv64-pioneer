/* SPDX-License-Identifier: MIT */
#include <assert.h>
#include <cstring>

#include "../PioneerQwen35DataflowAuthorization.h"
#include "../PioneerQwen35DataflowOwner.h"

using namespace PioneerQwen35;

static const unsigned TestRoutes[2][Banks]={
	{786,978,1170,785,977,1169,1361},
	{786,978,1170,785,977,1169,1361}
};

static RowSpace MakeRows(unsigned rows,unsigned rowBytes,unsigned unit,
	unsigned base)
{
	RowSpace result{};
	result.rows=rows;
	result.rowBytes=rowBytes;
	result.unit=unit;
	const unsigned groups=rows/16;
	unsigned first=0;
	for (unsigned bank=0;bank<Banks;bank++) {
		const unsigned count=groups/Banks+unsigned(bank<groups%Banks);
		if (!count) continue;
		result.parts[result.count++]={bank,base+bank*0x01000000,first*16,count*16};
		first+=count;
	}
	assert(result.Valid());
	return result;
}

static RowSpace MakeVector(unsigned rows,unsigned rowBytes,unsigned base)
{
	RowSpace result{};
	result.rows=rows;
	result.rowBytes=rowBytes;
	result.unit=1;
	result.count=1;
	result.parts[0]={0,base,0,rows};
	assert(result.Valid());
	return result;
}

static TensorSpace MakeProjection(unsigned type,unsigned inputs,unsigned rows,
	unsigned base)
{
	TensorSpace result{};
	result.type=type;
	result.inputs=inputs;
	const unsigned tileBytes=PioneerQwen35Projection::TileBytes(type);
	const unsigned blocks=inputs>>(type==8?5:8);
	result.place=MakeRows(rows,blocks*tileBytes/16,16,base);
	return result;
}

static TensorSpace MakeScalarVector(unsigned base)
{
	TensorSpace result{};
	result.type=0;
	result.inputs=DeltaHeads;
	result.place=MakeVector(1,DeltaHeads*sizeof(float),base);
	return result;
}

static TensorSpace MakeNormVector(unsigned base)
{
	TensorSpace result{};
	result.type=0;
	result.inputs=DeltaWidth;
	result.place=MakeVector(1,DeltaWidth*sizeof(float),base);
	return result;
}

struct DeltaOwnerMockIO {
	DataflowProjectionAuthority& authority;
	DataflowWorkerSnapshot snapshot[DataflowCoreCount]{};
	DataflowDeltaInvocation invocation[DataflowCoreCount]{};
	bool written[DataflowCoreCount]{};
	unsigned auditCount=0;

	explicit DeltaOwnerMockIO(DataflowProjectionAuthority& modelAuthority)
		:authority(modelAuthority)
	{
		for (unsigned core=0;core<DataflowCoreCount;core++)
			snapshot[core].state=DataflowWorkerReady;
	}

	bool ReadWorkerSnapshot(unsigned core,DataflowWorkerSnapshot& result) const
	{
		if (core>=DataflowCoreCount) return false;
		result=snapshot[core];
		return true;
	}

	bool CoreReady(unsigned core,const DataflowWorkerSnapshot& state) const
		{ return core<DataflowCoreCount && DataflowWorkerSnapshotReady(state); }
	bool AuthorizeDelta(const DataflowDeltaProgram& program) const
		{ return authority.AuthorizeDelta(program); }
	bool BeginDelta(const DataflowDeltaProgram& program)
		{ return authority.BeginDelta(program); }

	bool WriteDeltaInvocation(unsigned core,const DataflowDeltaInvocation& item)
	{
		if (core>=DataflowCoreCount || !item.Valid() || item.core!=core) return false;
		invocation[core]=item;
		written[core]=true;
		return true;
	}

	bool DeltaInvocationMatches(unsigned core,const DataflowDeltaInvocation& item) const
	{
		return core<DataflowCoreCount && written[core]
			&& memcmp(&invocation[core],&item,sizeof(item))==0;
	}

	bool PublishCommand(unsigned core,unsigned sequence)
	{
		if (core>=DataflowCoreCount || !written[core] || !sequence) return false;
		snapshot[core]={sequence,snapshot[core].acknowledgedSequence,
			sequence,DataflowWorkerComplete,0,2};
		return true;
	}

	bool WaitForCompletion(unsigned core,unsigned sequence,
		DataflowWorkerSnapshot& result) const
	{
		if (core>=DataflowCoreCount || snapshot[core].completedSequence!=sequence)
			return false;
		result=snapshot[core];
		return true;
	}

	bool AuditDelta(const DataflowDeltaProgram& program,unsigned sequence,
		const DataflowWorkerSnapshot (&receipts)[DataflowCoreCount])
	{
		bool covered[DeltaHeads]{};
		for (unsigned core=0;core<DataflowCoreCount;core++) {
			DataflowDeltaInvocation expected{};
			if (!written[core] || receipts[core].commandSequence!=sequence
				|| receipts[core].completedSequence!=sequence
				|| receipts[core].state!=DataflowWorkerComplete
				|| receipts[core].failure || receipts[core].phase!=2
				|| !DataflowDeltaProgramBuilder::BindCore(program,core,expected)
				|| memcmp(&expected,&invocation[core],sizeof(expected))!=0) return false;
			for (unsigned head=expected.firstHead;
				head<expected.firstHead+expected.headCount;head++) {
				if (covered[head]) return false;
				covered[head]=true;
			}
		}
		for (unsigned head=0;head<DeltaHeads;head++)
			if (!covered[head]) return false;
		auditCount++;
		return true;
	}

	bool Acknowledge(unsigned core,unsigned sequence)
	{
		if (core>=DataflowCoreCount
			|| snapshot[core].completedSequence!=sequence
			|| snapshot[core].state!=DataflowWorkerComplete) return false;
		snapshot[core].acknowledgedSequence=sequence;
		snapshot[core].state=DataflowWorkerReady;
		snapshot[core].phase=0;
		return true;
	}

	bool CompleteDelta(const DataflowDeltaProgram& program,unsigned sequence)
		{ return sequence && authority.CompleteDelta(program); }
};

static void MarkProjection(DataflowProjectionAuthority& authority,
	unsigned role,unsigned outputSlot)
{
	DataflowProjectionProgram projection{};
	assert(authority.BuildProjection(0,role,0,outputSlot,projection));
	assert(authority.Begin(projection));
	assert(authority.Complete(projection));
}

int main()
{
	DeviceLayout manifest{};
	manifest.magic=LayoutMagic;
	manifest.version=LayoutVersion;
	manifest.context=8;
	manifest.activation[0]=MakeRows(Ffn,sizeof(float),16,0x08000000);
	manifest.activation[1]=MakeRows(Ffn,sizeof(float),16,0x09000000);
	manifest.layers[0].weights[RecurrentQkv]=MakeProjection(13,Hidden,
		ConvChannels,0x11000000);
	manifest.layers[0].weights[RecurrentGate]=MakeProjection(13,Hidden,
		DeltaInner,0x12000000);
	manifest.layers[0].weights[RecurrentOut]=MakeProjection(13,DeltaInner,
		Hidden,0x12500000);
	manifest.layers[0].weights[Alpha]=MakeProjection(13,Hidden,DeltaHeads,
		0x13000000);
	manifest.layers[0].weights[Beta]=MakeProjection(13,Hidden,DeltaHeads,
		0x14000000);
	manifest.layers[0].weights[ConvWeights].type=0;
	manifest.layers[0].weights[ConvWeights].inputs=4;
	manifest.layers[0].weights[ConvWeights].place=MakeRows(ConvChannels,
		4*sizeof(float),1,0x15000000);
	manifest.layers[0].weights[NegativeA]=MakeScalarVector(0x16000000);
	manifest.layers[0].weights[DeltaBias]=MakeScalarVector(0x17000000);
	manifest.layers[0].weights[DeltaNorm]=MakeNormVector(0x18000000);
	manifest.layers[0].conv[0]=MakeRows(ConvChannels,3*sizeof(float),1,
		0x20000000);
	manifest.layers[0].conv[1]=MakeRows(ConvChannels,3*sizeof(float),1,
		0x30000000);
	manifest.layers[0].delta[0]=MakeRows(DeltaHeads,
		DeltaStateWords*sizeof(float),1,0x40000000);
	manifest.layers[0].delta[1]=MakeRows(DeltaHeads,
		DeltaStateWords*sizeof(float),1,0x50000000);

	DataflowGraphWorkspace workspace{};
	workspace.space[DataflowGraphResidualWorkspaceIndex]=
		MakeRows(Hidden,sizeof(float),16,0x60000000);
	workspace.space[DataflowGraphUpWorkspaceIndex]=
		MakeRows(Ffn,sizeof(float),16,0x61000000);
	workspace.space[DataflowGraphRecurrentQkvWorkspaceIndex]=
		MakeRows(ConvChannels,sizeof(float),16,0x62000000);
	workspace.space[DataflowGraphRecurrentGateWorkspaceIndex]=
		MakeRows(DeltaInner,sizeof(float),16,0x63000000);
	workspace.space[DataflowGraphRecurrentAlphaWorkspaceIndex]=
		MakeRows(DeltaHeads,sizeof(float),16,0x64000000);
	workspace.space[DataflowGraphRecurrentBetaWorkspaceIndex]=
		MakeRows(DeltaHeads,sizeof(float),16,0x65000000);
	assert(workspace.Valid(manifest));

	DataflowProjectionAuthority authority;
	assert(authority.BindValidatedModel(manifest,workspace,TestRoutes));
	assert(authority.MarkActivationInitialized(0));
	MarkProjection(authority,RecurrentQkv,DataflowGraphRecurrentQkvOutputSlot);
	MarkProjection(authority,RecurrentGate,DataflowGraphRecurrentGateOutputSlot);
	MarkProjection(authority,Alpha,DataflowGraphRecurrentAlphaOutputSlot);
	MarkProjection(authority,Beta,DataflowGraphRecurrentBetaOutputSlot);
	DataflowConvProgram conv{};
	assert(authority.BuildConv(0,0,conv));
	assert(authority.BeginConv(conv) && authority.CompleteConv(conv));

	DataflowDeltaProgram delta{};
	assert(authority.BuildDelta(0,0,delta));
	assert(delta.Valid());
	unsigned headCount=0;
	for (unsigned core=0;core<DataflowCoreCount;core++) {
		DataflowDeltaInvocation invocation{};
		assert(DataflowDeltaProgramBuilder::BindCore(delta,core,invocation));
		assert(invocation.Valid());
		assert(invocation.headCount==(core<DeltaHeads?1U:0U));
		headCount+=invocation.headCount;
	}
	assert(headCount==DeltaHeads);

	DataflowProjectionOwner owner;
	DeltaOwnerMockIO ownerIO(authority);
	assert(owner.Initialize(ownerIO)==DataflowOwnerComplete);
	assert(owner.RunDelta(delta,ownerIO)==DataflowOwnerComplete);
	assert(owner.NextSequence()==1 && ownerIO.auditCount==1);
	assert(authority.GraphWorkspaceReady(DataflowGraphRecurrentQkvWorkspaceIndex));
	assert(!authority.GraphWorkspaceReady(DataflowGraphRecurrentAlphaWorkspaceIndex));
	assert(!authority.GraphWorkspaceReady(DataflowGraphRecurrentBetaWorkspaceIndex));
	DataflowDeltaProgram repeated{};
	assert(!authority.BuildDelta(0,0,repeated));
	assert(authority.DeltaNextPosition(0)==1);

	DataflowProjectionProgram recurrentOut{};
	assert(authority.BuildRecurrentOutput(0,1,recurrentOut));
	assert(recurrentOut.inputs==DeltaInner && recurrentOut.inputRows==DeltaInner
		&& recurrentOut.inputBase==DataflowProjectionRecurrentVBase
		&& recurrentOut.outputRows==Hidden && recurrentOut.activeCores==120);
	unsigned outputSlot=ActivationBuffers,modelLayer=Layers;
	assert(DataflowProjectionMatchesRecurrentOutput(manifest,workspace,
		TestRoutes,recurrentOut,&outputSlot,&modelLayer));
	assert(outputSlot==1 && modelLayer==0);
	DataflowProjectionProgram wrongVOffset=recurrentOut;
	wrongVOffset.inputBase-=16;
	assert(wrongVOffset.Valid());
	assert(!DataflowProjectionMatchesRecurrentOutput(manifest,workspace,
		TestRoutes,wrongVOffset));
	assert(!authority.Begin(wrongVOffset));
	unsigned activeCores=0;
	for (unsigned core=0;core<DataflowCoreCount;core++) {
		DataflowCoreInvocation invocation{};
		assert(DataflowProgramBuilder::BindCore(recurrentOut,core,invocation));
		assert(invocation.inputBase==DataflowProjectionRecurrentVBase);
		activeCores+=invocation.assignment.active;
	}
	assert(activeCores==DataflowCoreCount);
	assert(authority.Begin(recurrentOut));
	assert(authority.Pending() && !authority.ActivationReady(1)
		&& authority.PendingProjectionMatches(recurrentOut));
	assert(authority.Complete(recurrentOut));
	assert(!authority.Pending() && authority.ActivationRows(1)==Hidden
		&& !authority.GraphWorkspaceReady(DataflowGraphRecurrentQkvWorkspaceIndex));
	assert(!authority.BuildRecurrentOutput(0,1,recurrentOut));
	return 0;
}
