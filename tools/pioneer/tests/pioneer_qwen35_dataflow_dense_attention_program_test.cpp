/* SPDX-License-Identifier: MIT */
#include "../PioneerQwen35DataflowAuthorization.h"
#include "../PioneerQwen35DataflowOwner.h"

#include <assert.h>
#include <cstring>

using namespace PioneerQwen35;

static const unsigned TestRoutes[2][Banks]={
	{786,978,1170,785,977,1169,1361},
	{786,978,1170,785,977,1169,1361}
};

struct DenseAttentionOwnerMock {
	DataflowWorkerSnapshot snapshots[DataflowCoreCount]{};
	DataflowDenseAttentionInvocation invocation[DataflowCoreCount]{};
	unsigned writes=0,audits=0,acknowledgements=0,commits=0,active=0;

	DenseAttentionOwnerMock()
	{
		for (unsigned core=0;core<DataflowCoreCount;core++)
			snapshots[core].state=DataflowWorkerReady;
	}

	bool ReadWorkerSnapshot(unsigned core,DataflowWorkerSnapshot& result)
	{
		if (core>=DataflowCoreCount) return false;
		result=snapshots[core];
		return true;
	}
	bool CoreReady(unsigned,const DataflowWorkerSnapshot&) { return true; }
	bool AuthorizeDenseAttention(const DataflowDenseAttentionProgram& program)
		{ return program.Valid(); }
	bool BeginDenseAttention(const DataflowDenseAttentionProgram& program)
		{ return program.Valid(); }
	bool WriteDenseAttentionInvocation(unsigned core,
		const DataflowDenseAttentionInvocation& value)
	{
		if (core>=DataflowCoreCount || !value.Valid() || value.core!=core)
			return false;
		invocation[core]=value;
		writes++;
		active+=value.headCount!=0;
		return true;
	}
	bool DenseAttentionInvocationMatches(unsigned core,
		const DataflowDenseAttentionInvocation& value) const
	{
		return core<DataflowCoreCount
			&& std::memcmp(&invocation[core],&value,sizeof(value))==0;
	}
	bool PublishCommand(unsigned core,unsigned sequence)
	{
		if (core>=DataflowCoreCount || !sequence) return false;
		auto& snapshot=snapshots[core];
		snapshot.commandSequence=sequence;
		snapshot.completedSequence=sequence;
		snapshot.state=DataflowWorkerComplete;
		snapshot.failure=0;
		snapshot.phase=2;
		return true;
	}
	bool WaitForCompletion(unsigned core,unsigned sequence,
		DataflowWorkerSnapshot& result)
	{
		return core<DataflowCoreCount
			&& snapshots[core].commandSequence==sequence
			&& snapshots[core].completedSequence==sequence
			&&(result=snapshots[core],true);
	}
	bool AuditDenseAttention(const DataflowDenseAttentionProgram& program,
		unsigned sequence,
		const DataflowWorkerSnapshot (&receipts)[DataflowCoreCount])
	{
		if (!program.Valid()) return false;
		unsigned completed=0;
		for (unsigned core=0;core<DataflowCoreCount;core++) {
			DataflowDenseAttentionInvocation expected;
			if (receipts[core].commandSequence!=sequence
				|| receipts[core].completedSequence!=sequence
				|| receipts[core].state!=DataflowWorkerComplete
				|| !DataflowDenseAttentionProgramBuilder::BindCore(program,core,
					expected)
				|| std::memcmp(&invocation[core],&expected,sizeof(expected)))
				return false;
			completed+=expected.headCount!=0;
		}
		if (completed!=(program.mode==DataflowDenseAttentionRun
			?DenseHeads:DenseKeyHeads)) return false;
		audits++;
		return true;
	}
	bool Acknowledge(unsigned core,unsigned sequence)
	{
		if (core>=DataflowCoreCount
			|| snapshots[core].commandSequence!=sequence) return false;
		auto& snapshot=snapshots[core];
		snapshot.acknowledgedSequence=sequence;
		snapshot.state=DataflowWorkerReady;
		snapshot.phase=0;
		acknowledgements++;
		return true;
	}
	bool CompleteDenseAttention(const DataflowDenseAttentionProgram& program,
		unsigned sequence)
	{
		if (!program.Valid() || !sequence) return false;
		commits++;
		return true;
	}
};

static void Rows(RowSpace& space,unsigned rows,unsigned rowBytes,
	uint32_t base,unsigned unit=16)
{
	space={};
	space.rows=rows;
	space.rowBytes=rowBytes;
	space.unit=unit;
	if (unit==1) {
		space.count=1;
		space.parts[0]={0,base,0,rows};
		return;
	}
	const unsigned groups=rows/16;
	unsigned first=0;
	for (unsigned bank=0;bank<Banks;bank++) {
		const unsigned count=groups/Banks+unsigned(bank<groups%Banks);
		if (!count) continue;
		space.parts[space.count++]={bank,base+bank*0x01000000,first*16,count*16};
		first+=count;
	}
}

static DeviceLayout Fixture(DataflowGraphWorkspace& workspace,Range& rope)
{
	DeviceLayout manifest{};
	manifest.magic=LayoutMagic;
	manifest.version=LayoutVersion;
	manifest.context=128;
	Rows(manifest.activation[0],Ffn,sizeof(float),0x04000000);
	Rows(manifest.activation[1],Ffn,sizeof(float),0x08000000);
	Rows(workspace.space[0],Hidden,sizeof(float),0x0c000000);
	Rows(workspace.space[1],Ffn,sizeof(float),0x10000000);
	Rows(workspace.space[DataflowGraphRecurrentQkvWorkspaceIndex],ConvChannels,
		sizeof(float),0x14000000);
	Rows(workspace.space[DataflowGraphRecurrentGateWorkspaceIndex],DeltaInner,
		sizeof(float),0x18000000);
	Rows(workspace.space[DataflowGraphRecurrentAlphaWorkspaceIndex],DeltaHeads,
		sizeof(float),0x1c000000);
	Rows(workspace.space[DataflowGraphRecurrentBetaWorkspaceIndex],DeltaHeads,
		sizeof(float),0x20000000);
	const unsigned normRoles[2]={QueryNorm,KeyNorm};
	for (unsigned role:normRoles) {
		TensorSpace& weight=manifest.layers[3].weights[role];
		weight.type=0;
		weight.inputs=DenseHeadWidth;
		Rows(weight.place,1,DenseHeadWidth*sizeof(float),
			role==QueryNorm?0x24000000:0x25000000,1);
	}
	const unsigned projectionRoles[4]={QueryAndGate,Key,Value,AttentionOut};
	const unsigned projectionRows[4]={8192,1024,1024,Hidden};
	const unsigned projectionInputs[4]={Hidden,Hidden,Hidden,
		DenseHeads*DenseHeadWidth};
	const uint32_t projectionBases[4]={
		0x2c000000,0x30000000,0x34000000,0x38000000};
	for (unsigned index=0;index<4;index++) {
		TensorSpace& weight=manifest.layers[3].weights[projectionRoles[index]];
		weight.type=12;
		weight.inputs=projectionInputs[index];
		Rows(weight.place,projectionRows[index],
			PioneerQwen35Projection::TileBytes(weight.type)
				*(weight.inputs/256)/16,projectionBases[index]);
	}
	Rows(manifest.layers[3].kv,2*DenseKeyHeads,
		manifest.context*DenseHeadWidth*sizeof(uint16_t),0x28000000,1);
	rope={0,0x2a000000,unsigned(manifest.context*Rotary*sizeof(float))};
	return manifest;
}

int main()
{
	DataflowGraphWorkspace workspace{};
	Range rope{};
	const DeviceLayout manifest=Fixture(workspace,rope);
	assert(workspace.Valid(manifest));
	const unsigned modes[3]={DataflowDenseAttentionPackKey,
		DataflowDenseAttentionPackValue,DataflowDenseAttentionRun};
	for (unsigned mode:modes) {
		DataflowDenseAttentionProgram program{};
		assert(DataflowDenseAttentionProgramBuilder::Build(manifest,workspace,
			rope,3,17,mode,TestRoutes,program));
		assert(program.Valid() && program.position==17 && program.context==128);
		unsigned active=0;
		for (unsigned core=0;core<DataflowCoreCount;core++) {
			DataflowDenseAttentionInvocation invocation{};
			assert(DataflowDenseAttentionProgramBuilder::BindCore(program,core,
				invocation));
			active+=invocation.headCount!=0;
		}
		assert(active==(mode==DataflowDenseAttentionRun
			?DenseHeads:DenseKeyHeads));
	}
	DataflowDenseAttentionProgram invalid{};
	assert(!DataflowDenseAttentionProgramBuilder::Build(manifest,workspace,
		rope,2,17,DataflowDenseAttentionRun,TestRoutes,invalid));
	assert(!DataflowDenseAttentionProgramBuilder::Build(manifest,workspace,
		rope,3,manifest.context,DataflowDenseAttentionRun,TestRoutes,invalid));
	Range shiftedRope=rope;
	shiftedRope.address+=64;
	assert(DataflowDenseAttentionProgramBuilder::Build(manifest,workspace,
		shiftedRope,3,17,DataflowDenseAttentionRun,TestRoutes,invalid));
	assert(invalid.rope.address==shiftedRope.address);
	invalid.kv.parts[0].address++;
	assert(!invalid.Valid());

	DataflowProjectionAuthority authority;
	assert(authority.BindValidatedModel(manifest,workspace,TestRoutes,rope));
	assert(authority.MarkActivationInitialized(0,Hidden));
	assert(!authority.BuildDenseAttention(3,0,
		DataflowDenseAttentionPackKey,invalid));
	DataflowProjectionProgram projection{};
	assert(authority.BuildDenseProjection(3,QueryAndGate,0,projection));
	assert(authority.Begin(projection) && authority.Complete(projection));
	assert(authority.BuildDenseProjection(3,Key,0,projection));
	assert(authority.Begin(projection) && authority.Complete(projection));
	DataflowDenseAttentionProgram operation{};
	assert(authority.BuildDenseAttention(3,0,
		DataflowDenseAttentionPackKey,operation));
	DataflowDenseAttentionProgram shifted=operation;
	shifted.rope.address+=64;
	assert(!authority.AuthorizeDenseAttention(shifted));
	assert(authority.BeginDenseAttention(operation)
		&& authority.PendingDenseAttentionMatches(operation));
	assert(authority.CompleteDenseAttention(operation));
	assert(authority.BuildDenseProjection(3,Value,0,projection));
	assert(authority.Begin(projection) && authority.Complete(projection));
	assert(authority.BuildDenseAttention(3,0,
		DataflowDenseAttentionPackValue,operation));
	assert(authority.BeginDenseAttention(operation)
		&& authority.CompleteDenseAttention(operation));
	assert(authority.BuildDenseAttention(3,0,
		DataflowDenseAttentionRun,operation));
	assert(authority.BeginDenseAttention(operation)
		&& authority.CompleteDenseAttention(operation));
	assert(!authority.BuildDenseAttention(3,0,
		DataflowDenseAttentionRun,operation));
	assert(authority.BuildDenseAttentionOutput(3,1,projection));
	assert(authority.Begin(projection) && authority.Complete(projection));
	assert(authority.ActivationReady(1)
		&& authority.ActivationRows(1)==Hidden);

	DataflowDenseAttentionProgram ownerProgram{};
	assert(DataflowDenseAttentionProgramBuilder::Build(manifest,workspace,rope,
		3,0,DataflowDenseAttentionRun,TestRoutes,ownerProgram));
	DataflowProjectionOwner owner;
	DenseAttentionOwnerMock ownerIO;
	assert(owner.Initialize(ownerIO)==DataflowOwnerComplete);
	assert(owner.RunDenseAttention(ownerProgram,ownerIO)==DataflowOwnerComplete);
	assert(owner.NextSequence()==1 && ownerIO.writes==DataflowCoreCount
		&& ownerIO.active==DenseHeads && ownerIO.audits==1
		&& ownerIO.acknowledgements==DataflowCoreCount && ownerIO.commits==1);
	return 0;
}
