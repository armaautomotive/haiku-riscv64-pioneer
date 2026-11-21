/* SPDX-License-Identifier: MIT */
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>

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
		if (!count) continue;
		space.parts[space.count++]={bank,base+bank*0x01000000,first*16,count*16};
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

static uint32_t Signature(const DataflowResidualInvocation& invocation)
{
	const auto* bytes=reinterpret_cast<const uint8_t*>(&invocation);
	uint32_t value=2166136261U;
	for (unsigned i=0;i<sizeof(invocation);i++) value=(value^bytes[i])*16777619U;
	return value;
}

static uint32_t Signature(const DataflowArgmaxInvocation& invocation)
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
	uint32_t residualInvocationSignature[DataflowCoreCount]{};
	uint32_t argmaxInvocationSignature[DataflowCoreCount]{};
	unsigned writes=0,publishes=0,waits=0,audits=0,acknowledgements=0;
	unsigned begins=0,completes=0,aborts=0,completedToken=Vocabulary;
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
	bool AuthorizeResidual(const DataflowResidualProgram& program) const
	{
		return programAuthorized && program.Valid();
	}
	bool BeginResidual(const DataflowResidualProgram& program)
	{
		begins++;
		return !failBegin && program.Valid();
	}
	bool CompleteResidual(const DataflowResidualProgram& program,unsigned sequence)
	{
		completes++;
		return !failComplete && sequence && program.Valid();
	}
	bool AuthorizeArgmax(const DataflowArgmaxProgram& program) const
		{ return programAuthorized && program.Valid(); }
	bool BeginArgmax(const DataflowArgmaxProgram& program)
		{ begins++; return !failBegin && program.Valid(); }
	bool WriteArgmaxInvocation(unsigned core,
		const DataflowArgmaxInvocation& invocation)
	{
		if (core>=DataflowCoreCount || invocation.core!=core || !invocation.Valid())
			return false;
		argmaxInvocationSignature[core]=Signature(invocation);
		writes++;
		return true;
	}
	bool ArgmaxInvocationMatches(unsigned core,
		const DataflowArgmaxInvocation& invocation) const
	{
		return core<DataflowCoreCount && invocation.core==core
			&& argmaxInvocationSignature[core]==Signature(invocation);
	}
	bool ReadArgmaxResult(unsigned sequence,DataflowArgmaxResult& result) const
	{
		const unsigned scoreBits=0x40e00000U;
		result={12345,scoreBits,sequence,
			DataflowArgmaxResultChecksum(12345,scoreBits,sequence)};
		return DataflowArgmaxResultValid(result);
	}
	bool AuditArgmax(const DataflowArgmaxProgram& program,unsigned sequence,
		const DataflowWorkerSnapshot (&receipts)[DataflowCoreCount],
		const DataflowArgmaxResult& result)
	{
		if (!program.Valid() || !DataflowArgmaxResultValid(result)) return false;
		for (unsigned core=0;core<DataflowCoreCount;core++)
			if (receipts[core].state!=DataflowWorkerComplete
				|| receipts[core].commandSequence!=sequence
				|| receipts[core].completedSequence!=sequence) return false;
		audits++;
		return true;
	}
	bool CompleteArgmax(const DataflowArgmaxProgram& program,unsigned sequence,
		unsigned token)
	{
		completes++;
		completedToken=token;
		return !failComplete && sequence && program.Valid() && token<Vocabulary;
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
	bool WriteResidualInvocation(unsigned core,const DataflowResidualInvocation& invocation)
	{
		if (core>=DataflowCoreCount || invocation.core!=core || !invocation.Valid())
			return false;
		residualInvocationSignature[core]=Signature(invocation);
		writes++;
		return true;
	}
	bool ResidualInvocationMatches(unsigned core,
		const DataflowResidualInvocation& invocation) const
	{
		return core<DataflowCoreCount && invocation.core==core
			&& residualInvocationSignature[core]==Signature(invocation);
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

	bool AuditResidual(const DataflowResidualProgram& program,unsigned sequence,
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

	DataflowArgmaxProgram argmaxProgram{};
	argmaxProgram.magic=DataflowArgmaxMagic;
	argmaxProgram.version=DataflowArgmaxVersion;
	for (unsigned noc=0;noc<2;noc++) for (unsigned bank=0;bank<Banks;bank++)
		argmaxProgram.bankRoute[noc][bank]=TestRoutes[noc][bank];
	argmaxProgram.logits.rows=Vocabulary;
	argmaxProgram.logits.rowBytes=sizeof(float);
	argmaxProgram.logits.count=1;
	argmaxProgram.logits.unit=16;
	argmaxProgram.logits.parts[0]={0,BankBase+0x10000000,0,Vocabulary};
	assert(argmaxProgram.Valid());
	MockOwnerIO argmaxIO{};
	DataflowProjectionOwner argmaxOwner;
	assert(argmaxOwner.Initialize(argmaxIO)==DataflowOwnerComplete);
	unsigned selectedToken=Vocabulary;
	assert(argmaxOwner.RunArgmax(argmaxProgram,selectedToken,argmaxIO)
		==DataflowOwnerComplete);
	assert(selectedToken==12345 && argmaxIO.completedToken==12345
		&& argmaxOwner.NextSequence()==1 && !argmaxOwner.Faulted()
		&& argmaxIO.begins==1 && argmaxIO.completes==1
		&& argmaxIO.writes==DataflowCoreCount
		&& argmaxIO.publishes==DataflowCoreCount
		&& argmaxIO.waits==DataflowCoreCount
		&& argmaxIO.audits==1
		&& argmaxIO.acknowledgements==DataflowCoreCount);
	unsigned activeArgmax=0;
	for (unsigned core=0;core<DataflowCoreCount;core++) {
		DataflowArgmaxInvocation invocation;
		DataflowArgmaxZeroWords(invocation);
		assert(DataflowArgmaxProgramBuilder::BindCore(argmaxProgram,core,invocation)
			&& argmaxIO.ArgmaxInvocationMatches(core,invocation));
		activeArgmax+=invocation.active;
	}
	assert(activeArgmax==1);
	TensorSpace smallWeight{};
	smallWeight.type=13;
	smallWeight.inputs=Hidden;
	Rows(smallWeight.place,DeltaHeads,
		PioneerQwen35Projection::TileBytes(smallWeight.type)*(Hidden/256)/16,
		0x18000000);
	RowSpace smallInput{},smallOutput{};
	Rows(smallInput,Hidden,sizeof(float),0x19000000);
	Rows(smallOutput,DeltaHeads,sizeof(float),0x1a000000);
	DataflowProjectionProgram smallProgram{};
	assert(DataflowProgramBuilder::Build(smallWeight,smallInput,Hidden,
		smallOutput,TestRoutes,smallProgram));
	assert(smallProgram.activeCores==2);
	MockOwnerIO smallIO{};
	DataflowProjectionOwner smallOwner;
	assert(smallOwner.Initialize(smallIO)==DataflowOwnerComplete);
	assert(smallOwner.RunProjection(smallProgram,smallIO)==DataflowOwnerComplete);
	assert(smallOwner.NextSequence()==1
		&& smallIO.writes==DataflowCoreCount
		&& smallIO.publishes==DataflowCoreCount
		&& smallIO.waits==DataflowCoreCount
		&& smallIO.acknowledgements==DataflowCoreCount);
	unsigned smallActive=0;
	for (unsigned core=0;core<DataflowCoreCount;core++) {
		DataflowCoreInvocation invocation{};
		assert(DataflowProgramBuilder::BindCore(smallProgram,core,invocation));
		smallActive+=invocation.assignment.active;
		assert(smallIO.InvocationMatches(core,invocation));
	}
	assert(smallActive==2);

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
	Rows(siluWorkspace.space[DataflowGraphRecurrentQkvWorkspaceIndex],ConvChannels,
		sizeof(float),0x40000000);
	Rows(siluWorkspace.space[DataflowGraphRecurrentGateWorkspaceIndex],DeltaInner,
		sizeof(float),0x50000000);
	Rows(siluWorkspace.space[DataflowGraphRecurrentAlphaWorkspaceIndex],DeltaHeads,
		sizeof(float),0x60000000);
	Rows(siluWorkspace.space[DataflowGraphRecurrentBetaWorkspaceIndex],DeltaHeads,
		sizeof(float),0x70000000);
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
	DataflowResidualProgram residualProgram{};
	assert(DataflowResidualProgramBuilder::Build(siluManifest,siluWorkspace,
		DataflowResidualSave,0,TestRoutes,residualProgram));
	MockOwnerIO residualIO{};
	DataflowProjectionOwner residualOwner;
	assert(residualOwner.Initialize(residualIO)==DataflowOwnerComplete);
	assert(residualOwner.RunResidual(residualProgram,residualIO)
		==DataflowOwnerComplete);
	residualProgram.mode=DataflowResidualAdd;
	assert(residualProgram.Valid());
	assert(residualOwner.RunResidual(residualProgram,residualIO)
		==DataflowOwnerComplete);
	assert(residualOwner.NextSequence()==2 && !residualOwner.Faulted()
		&& residualIO.begins==2 && residualIO.completes==2
		&& residualIO.writes==2*DataflowCoreCount
		&& residualIO.publishes==2*DataflowCoreCount
		&& residualIO.waits==2*DataflowCoreCount
		&& residualIO.audits==2
		&& residualIO.acknowledgements==2*DataflowCoreCount);
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
