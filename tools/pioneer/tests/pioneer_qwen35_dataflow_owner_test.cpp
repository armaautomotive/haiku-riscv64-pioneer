/* SPDX-License-Identifier: MIT */
#include <cassert>
#include <cstdint>
#include <cstdio>

#include "../PioneerQwen35DataflowOwner.h"
#include "../PioneerQwen35DataflowBlackholeWorker.h"

using namespace PioneerQwen35;

static const unsigned TestRoutes[2][Banks]={
	{786,978,1170,785,977,1169,1361},
	{786,978,1170,785,977,1169,1361}
};

static void Rows(RowSpace& space,unsigned rows,unsigned rowBytes,uint32_t base)
{
	space={};
	space.rows=rows;
	space.rowBytes=rowBytes;
	space.unit=16;
	const unsigned groups=rows/16;
	unsigned first=0;
	for (unsigned bank=0;bank<Banks;bank++) {
		const unsigned count=groups/Banks+unsigned(bank<groups%Banks);
		space.parts[bank]={bank,base+bank*0x01000000,first*16,count*16};
		space.count++;
		first+=count;
	}
}

static uint32_t Signature(const DataflowCoreInvocation& invocation)
{
	const auto* bytes=reinterpret_cast<const uint8_t*>(&invocation);
	uint32_t value=2166136261U;
	for (unsigned i=0;i<sizeof(invocation);i++) value=(value^bytes[i])*16777619U;
	return value;
}

static uint32_t Signature(const DataflowSiluInvocation& invocation)
{
	const auto* bytes=reinterpret_cast<const uint8_t*>(&invocation);
	uint32_t value=2166136261U;
	for (unsigned i=0;i<sizeof(invocation);i++) value=(value^bytes[i])*16777619U;
	return value;
}

struct MockOwnerIO {
	DataflowWorkerSnapshot snapshot[DataflowCoreCount]{};
	uint32_t invocationSignature[DataflowCoreCount]{};
	uint32_t siluInvocationSignature[DataflowCoreCount]{};
	unsigned writes=0,publishes=0,waits=0,audits=0,acknowledgements=0;
	unsigned begins=0,completes=0,aborts=0;
	unsigned failPublishCore=UINT32_MAX;
	bool ambiguousPublishFailure=true,recoveryAllowed=true,programAuthorized=true;
	bool failBegin=false,failComplete=false;

	MockOwnerIO()
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

	bool CoreReady(unsigned core,const DataflowWorkerSnapshot& observed) const
	{
		unsigned routeCore=UINT32_MAX;
		return core<DataflowCoreCount && DataflowWorkerSnapshotReady(observed)
			&& DataflowBlackholeLocalCore(DataflowBlackholeRouteForCore(core),routeCore)
			&& routeCore==core;
	}

	bool AuthorizeProjection(const DataflowProjectionProgram& program) const
	{
		return programAuthorized && program.Valid();
	}
	bool BeginProjection(const DataflowProjectionProgram& program)
	{
		begins++;
		return !failBegin && program.Valid();
	}
	bool CompleteProjection(const DataflowProjectionProgram& program,unsigned sequence)
	{
		completes++;
		return !failComplete && sequence && program.Valid();
	}
	bool AuthorizeSilu(const DataflowSiluProgram& program) const
	{
		return programAuthorized && program.Valid();
	}
	bool BeginSilu(const DataflowSiluProgram& program)
	{
		begins++;
		return !failBegin && program.Valid();
	}
	bool CompleteSilu(const DataflowSiluProgram& program,unsigned sequence)
	{
		completes++;
		return !failComplete && sequence && program.Valid();
	}

	bool WriteInvocation(unsigned core,const DataflowCoreInvocation& invocation)
	{
		if (core>=DataflowCoreCount || invocation.core!=core || !invocation.Valid())
			return false;
		invocationSignature[core]=Signature(invocation);
		writes++;
		return true;
	}

	bool InvocationMatches(unsigned core,const DataflowCoreInvocation& invocation) const
	{
		return core<DataflowCoreCount && invocation.core==core
			&& invocationSignature[core]==Signature(invocation);
	}
	bool WriteSiluInvocation(unsigned core,const DataflowSiluInvocation& invocation)
	{
		if (core>=DataflowCoreCount || invocation.core!=core || !invocation.Valid())
			return false;
		siluInvocationSignature[core]=Signature(invocation);
		writes++;
		return true;
	}
	bool SiluInvocationMatches(unsigned core,const DataflowSiluInvocation& invocation) const
	{
		return core<DataflowCoreCount && invocation.core==core
			&& siluInvocationSignature[core]==Signature(invocation);
	}

	bool PublishCommand(unsigned core,unsigned sequence)
	{
		if (core>=DataflowCoreCount) return false;
		publishes++;
		snapshot[core].commandSequence=sequence;
		if (core==failPublishCore) {
			if (!ambiguousPublishFailure) snapshot[core].commandSequence=0;
			return false;
		}
		return true;
	}

	bool WaitForCompletion(unsigned core,unsigned sequence,
		DataflowWorkerSnapshot& result)
	{
		if (core>=DataflowCoreCount || snapshot[core].commandSequence!=sequence)
			return false;
		snapshot[core].completedSequence=sequence;
		snapshot[core].state=DataflowWorkerComplete;
		snapshot[core].failure=0;
		snapshot[core].phase=2;
		result=snapshot[core];
		waits++;
		return true;
	}

	bool AuditProjection(const DataflowProjectionProgram& program,unsigned sequence,
		const DataflowWorkerSnapshot (&receipts)[DataflowCoreCount])
	{
		if (!program.Valid()) return false;
		for (unsigned core=0;core<DataflowCoreCount;core++)
			if (receipts[core].state!=DataflowWorkerComplete
				|| receipts[core].commandSequence!=sequence
				|| receipts[core].completedSequence!=sequence) return false;
		audits++;
		return true;
	}

	bool AuditSilu(const DataflowSiluProgram& program,unsigned sequence,
		const DataflowWorkerSnapshot (&receipts)[DataflowCoreCount])
	{
		if (!program.Valid()) return false;
		for (unsigned core=0;core<DataflowCoreCount;core++)
			if (receipts[core].state!=DataflowWorkerComplete
				|| receipts[core].commandSequence!=sequence
				|| receipts[core].completedSequence!=sequence) return false;
		audits++;
		return true;
	}

	bool Acknowledge(unsigned core,unsigned sequence)
	{
		if (core>=DataflowCoreCount
			|| snapshot[core].completedSequence!=sequence
			|| (snapshot[core].state!=DataflowWorkerComplete
				&& snapshot[core].state!=DataflowWorkerFailed)) return false;
		snapshot[core].acknowledgedSequence=sequence;
		snapshot[core].state=DataflowWorkerReady;
		snapshot[core].failure=0;
		snapshot[core].phase=0;
		acknowledgements++;
		return true;
	}

	bool AuditAndRecoverAll(unsigned sequence,
		DataflowWorkerSnapshot (&)[DataflowCoreCount])
	{
		if (!recoveryAllowed) return false;
		for (unsigned core=0;core<DataflowCoreCount;core++) {
			auto& state=snapshot[core];
			if (state.commandSequence==sequence
				&& state.completedSequence<sequence) {
				state.completedSequence=sequence;
				state.state=DataflowWorkerFailed;
				state.failure=DataflowFailureReader;
				state.phase=5;
			}
		}
		return true;
	}
	bool AbortPendingProjection() { aborts++; return true; }
};

int main()
{
	TensorSpace weight{};
	weight.type=13;
	weight.inputs=Hidden;
	Rows(weight.place,Ffn,Hidden/256
		*(PioneerQwen35Projection::TileBytes(13)/16),0x08000000);
	RowSpace input{},output{};
	Rows(input,Ffn,sizeof(float),0x04000000);
	Rows(output,Ffn,sizeof(float),0x06000000);
	DataflowProjectionProgram program{};
	assert(DataflowProgramBuilder::Build(weight,input,Hidden,output,
		TestRoutes,program));

	MockOwnerIO io{};
	DataflowProjectionOwner owner;
	assert(owner.Initialize(io)==DataflowOwnerComplete);
	assert(owner.NextSequence()==0 && owner.Initialized());
	io.programAuthorized=false;
	assert(owner.RunProjection(program,io)==DataflowOwnerProgramUnauthorized);
	assert(io.writes==0 && io.publishes==0 && !io.begins && !io.completes
		&& owner.NextSequence()==0 && !owner.Faulted());
	io.programAuthorized=true;
	assert(owner.RunProjection(program,io)==DataflowOwnerComplete);
	assert(owner.NextSequence()==1 && !owner.Faulted()
		&& io.begins==1 && io.completes==1);
	assert(io.writes==DataflowCoreCount && io.publishes==DataflowCoreCount
		&& io.waits==DataflowCoreCount && io.audits==1
		&& io.acknowledgements==DataflowCoreCount);

	DeviceLayout siluManifest{};
	siluManifest.magic=LayoutMagic;
	siluManifest.version=LayoutVersion;
	siluManifest.context=128;
	Rows(siluManifest.activation[0],Ffn,sizeof(float),0x04000000);
	Rows(siluManifest.activation[1],Ffn,sizeof(float),0x06000000);
	DataflowGraphWorkspace siluWorkspace{};
	Rows(siluWorkspace.space[DataflowGraphResidualWorkspaceIndex],Hidden,
		sizeof(float),0x20000000);
	Rows(siluWorkspace.space[DataflowGraphUpWorkspaceIndex],Ffn,
		sizeof(float),0x30000000);
	assert(siluWorkspace.Valid(siluManifest));
	DataflowSiluProgram siluProgram{};
	assert(DataflowSiluProgramBuilder::Build(siluManifest,siluWorkspace,0,
		TestRoutes,siluProgram));
	MockOwnerIO siluIO{};
	DataflowProjectionOwner siluOwner;
	assert(siluOwner.Initialize(siluIO)==DataflowOwnerComplete);
	assert(siluOwner.RunSilu(siluProgram,siluIO)==DataflowOwnerComplete);
	assert(siluOwner.NextSequence()==1 && !siluOwner.Faulted()
		&& siluIO.begins==1 && siluIO.completes==1
		&& siluIO.writes==DataflowCoreCount
		&& siluIO.publishes==DataflowCoreCount
		&& siluIO.waits==DataflowCoreCount
		&& siluIO.audits==1
		&& siluIO.acknowledgements==DataflowCoreCount);
	for (unsigned core=0;core<DataflowCoreCount;core++) {
		DataflowSiluInvocation expected{};
		assert(DataflowSiluProgramBuilder::BindCore(siluProgram,core,expected)
			&& siluIO.SiluInvocationMatches(core,expected));
	}
	MockOwnerIO beginFailureIO{};
	DataflowProjectionOwner beginFailureOwner;
	assert(beginFailureOwner.Initialize(beginFailureIO)==DataflowOwnerComplete);
	beginFailureIO.failBegin=true;
	assert(beginFailureOwner.RunProjection(program,beginFailureIO)
		==DataflowOwnerProjectionBeginFailed);
	assert(!beginFailureIO.writes && !beginFailureIO.publishes
		&& beginFailureOwner.NextSequence()==0 && !beginFailureOwner.Faulted());
	for (unsigned core=0;core<DataflowCoreCount;core++)
		assert(DataflowWorkerSnapshotReady(io.snapshot[core])
			&& io.snapshot[core].completedSequence==1);

	io.failPublishCore=17;
	assert(owner.RunProjection(program,io)==DataflowOwnerCommandPublishFailed);
	assert(owner.Faulted() && owner.NextSequence()==1);
	assert(owner.Recover(io)==DataflowOwnerRecovered);
	assert(!owner.Faulted() && owner.NextSequence()==2 && io.aborts==1);
	for (unsigned core=0;core<DataflowCoreCount;core++)
		assert(DataflowWorkerSnapshotReady(io.snapshot[core]));

	io.failPublishCore=UINT32_MAX;
	assert(owner.RunProjection(program,io)==DataflowOwnerComplete);
	assert(owner.NextSequence()==3 && io.audits==2);
	for (unsigned core=0;core<DataflowCoreCount;core++)
		assert(io.snapshot[core].completedSequence==3);

	std::puts("PASS Qwen35 owner stages 120 shards, publishes only after readback, and recovers partial launch sequence gaps");
	return 0;
}
