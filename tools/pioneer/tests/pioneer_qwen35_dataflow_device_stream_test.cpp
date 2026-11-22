/* SPDX-License-Identifier: MIT */
#include "../PioneerQwen35DataflowStreamPlan.h"
#include <cassert>
#include <cstdio>
#include <cstring>
using namespace PioneerQwen35;

struct FakeStreamIO {
	DataflowStreamRecord record{};
	unsigned expectedBank=2,expectedAddress=0x08000000,expectedRoute=3,calls=0;
	bool fail=false,corrupt=false;
	bool ReadStream(unsigned bank,unsigned address,unsigned route,uint8_t* out,unsigned bytes) {
		calls++;
		if (fail || bank!=expectedBank || address!=expectedAddress || route!=expectedRoute
			|| bytes!=sizeof(record)) return false;
		std::memcpy(out,&record,sizeof(record));
		if (corrupt) out[128]^=1;
		return true;
	}
};

struct FakePackedWorkspace {
	uint32_t bytes=64*1024*1024;
	bool Workspace(unsigned bank,uint32_t& address,uint32_t& size) const {
		if (bank>=Banks) return false;
		address=0x60000000;size=bytes;return true;
	}
};

static void CheckStreamPlan(const unsigned (&routes)[2][Banks])
{
	FakePackedWorkspace packed;
	static DeviceLayout manifest{};manifest.magic=LayoutMagic;manifest.version=LayoutVersion;manifest.context=64;
	const unsigned ropeBytes=manifest.context*Rotary*sizeof(float);
	const Range rope{0,0x60000000,ropeBytes};
	PioneerQwen35ActivationPlan allocator;
	assert(allocator.InitializeFromPacked(packed,(ropeBytes+4095)&~4095U));
	for (unsigned slot=0;slot<ActivationBuffers;slot++) {
		PioneerQ35ActivationBuffer buffer{};
		assert(allocator.AllocateRows(Ffn,sizeof(float),buffer)
			&& DataflowActivationBufferToRows(buffer,manifest.activation[slot]));
	}
	DataflowGraphWorkspace graph{};
	assert(DataflowGraphWorkspaceBuilder::Build(packed,manifest,graph));
	static DataflowStreamPlan plan;
	assert(DataflowStreamPlanBuilder::Build(packed,manifest,graph,rope,routes,99,548,plan)
		&& DataflowStreamPlanBuilder::Validate(packed,manifest,graph,rope,routes,plan));
	for (unsigned core=0;core<DataflowCoreCount;core++) {
		assert(plan.lease[core].bytes==548*DataflowStreamRecordBytes
			&& DataflowStreamBindingMatches(plan.binding[core],plan.lease[core],routes,core));
		for (unsigned prior=0;prior<core;prior++) if (plan.lease[prior].bank==plan.lease[core].bank)
			assert(uint64_t(plan.lease[prior].address)+plan.lease[prior].bytes<=plan.lease[core].address);
	}
	const uint32_t original=plan.binding[7].base;plan.binding[7].base+=4096;
	assert(!DataflowStreamPlanBuilder::Validate(packed,manifest,graph,rope,routes,plan));
	plan.binding[7].base=original;
	const uint32_t originalActivation=manifest.activation[0].parts[0].address;
	manifest.activation[0].parts[0].address=rope.address;
	assert(!DataflowStreamPlanBuilder::Validate(packed,manifest,graph,rope,routes,plan));
	manifest.activation[0].parts[0].address=originalActivation;
	plan.lease[7].bytes-=4096;
	assert(!DataflowStreamPlanBuilder::Validate(packed,manifest,graph,rope,routes,plan));
	plan.lease[7].bytes+=4096;
	Range badRope=rope;badRope.address+=4096;
	assert(!DataflowStreamPlanBuilder::Validate(packed,manifest,graph,badRope,routes,plan));
	FakePackedWorkspace tooSmall;tooSmall.bytes=1024*1024;
	assert(!DataflowStreamPlanBuilder::Build(tooSmall,manifest,graph,rope,routes,99,548,plan) && !plan.ready);
	assert(!DataflowStreamPlanBuilder::Build(packed,manifest,graph,rope,routes,0,548,plan) && !plan.ready);
}

static DataflowEmbeddingInvocation MakeEmbedding(unsigned core)
{
	DataflowEmbeddingProgram program{};
	program.token=15;program.lane=15;program.outputSlot=0;
	program.weightBank=0;program.weightBase=BankBase;
	for (unsigned n=0;n<2;n++) for (unsigned b=0;b<Banks;b++) program.bankRoute[n][b]=1+n*16+b;
	program.output.rows=Hidden;program.output.rowBytes=4;program.output.count=1;program.output.unit=16;
	program.output.parts[0]={1,0x10000000,0,Hidden};
	assert(program.Valid());
	DataflowEmbeddingInvocation invocation{};
	assert(DataflowEmbeddingProgramBuilder::BindCore(program,core,invocation));
	return invocation;
}

static void StripeRows(RowSpace& space,unsigned rows,unsigned rowBytes,unsigned base)
{
	space={};space.rows=rows;space.rowBytes=rowBytes;space.count=Banks;space.unit=16;
	const unsigned groups=rows/16;unsigned first=0;
	for (unsigned bank=0;bank<Banks;bank++) {
		const unsigned part=(groups/Banks+unsigned(bank<groups%Banks))*16;
		space.parts[bank]={bank,base+bank*0x01000000,first,part};first+=part;
	}
}

static DataflowCoreInvocation MakeProjection(unsigned core,const unsigned (&routes)[2][Banks])
{
	TensorSpace weight{};weight.type=12;weight.inputs=Hidden;
	StripeRows(weight.place,Ffn,Hidden/256*(PioneerQwen35Projection::TileBytes(12)/16),0x10000000);
	RowSpace input{},output{};StripeRows(input,Ffn,4,0x30000000);StripeRows(output,Ffn,4,0x50000000);
	static DataflowProjectionProgram program;
	assert(DataflowProgramBuilder::Build(weight,input,Hidden,output,routes,program));
	DataflowCoreInvocation invocation{};
	assert(DataflowProgramBuilder::BindCore(program,core,invocation));return invocation;
}

int main()
{
	const struct { unsigned magic,bytes; } kinds[]={
		{DataflowProgramMagic,sizeof(DataflowCoreInvocation)},
		{DataflowEmbeddingMagic,sizeof(DataflowEmbeddingInvocation)},
		{DataflowRmsMagic,sizeof(DataflowRmsInvocation)},
		{DataflowSiluMagic,sizeof(DataflowSiluInvocation)},
		{DataflowResidualMagic,sizeof(DataflowResidualInvocation)},
		{DataflowConvMagic,sizeof(DataflowConvInvocation)},
		{DataflowDeltaMagic,sizeof(DataflowDeltaInvocation)},
		{DataflowDenseAttentionMagic,sizeof(DataflowDenseAttentionInvocation)},
		{DataflowArgmaxMagic,sizeof(DataflowArgmaxInvocation)}
	};
	for (const auto& kind:kinds) assert(DataflowStreamInvocationBytes(kind.magic)==kind.bytes);
	assert(!DataflowStreamInvocationBytes(0));
	unsigned routes[2][Banks]{};
	for (unsigned n=0;n<2;n++) for (unsigned b=0;b<Banks;b++) routes[n][b]=1+n*16+b;
	CheckStreamPlan(routes);
	DataflowStreamBinding binding{DataflowStreamBindingMagic,DataflowStreamVersion,17,7,2,3,0x08000000,2};
	Range lease{2,0x08000000,4096};
	assert(binding.Valid() && DataflowStreamBindingMatches(binding,lease,routes,7));
	assert(!DataflowStreamBindingMatches(binding,lease,routes,8));
	Range tooSmall=lease;tooSmall.bytes-=1;
	assert(!DataflowStreamBindingMatches(binding,tooSmall,routes,7));
	Range wrongBank=lease;wrongBank.bank=3;
	assert(!DataflowStreamBindingMatches(binding,wrongBank,routes,7));
	DataflowStreamBinding bad=binding;bad.route=4;
	assert(!DataflowStreamBindingMatches(bad,lease,routes,7));
	bad=binding;bad.base++;
	assert(!bad.Valid());
	bad=binding;bad.count=DataflowStreamMaxRecords+1;
	assert(!bad.Valid());
	bad=binding;bad.base=BankLimit-DataflowStreamRecordBytes;bad.count=2;
	assert(!bad.Valid());

	const auto invocation=MakeEmbedding(binding.core);
	FakeStreamIO io;
	assert(DataflowStreamEncode(invocation,17,1,io.record));
	assert(DataflowStreamRecordStructureValid(io.record,17,1,7));
	assert(!DataflowStreamRecordStructureValid(io.record,18,1,7));
	assert(!DataflowStreamRecordStructureValid(io.record,17,2,7));
	assert(!DataflowStreamRecordStructureValid(io.record,17,1,8));
	DataflowStreamRecord badRecord=io.record;badRecord.payload[10]^=1;
	assert(!DataflowStreamRecordStructureValid(badRecord,17,1,7));
	badRecord=io.record;badRecord.payload[DataflowStreamPayloadWords-1]=1;
	assert(!DataflowStreamRecordStructureValid(badRecord,17,1,7));
	badRecord=io.record;badRecord.header[8]=1;
	assert(!DataflowStreamRecordStructureValid(badRecord,17,1,7));
	badRecord=io.record;badRecord.header[6]-=4;
	assert(!DataflowStreamRecordStructureValid(badRecord,17,1,7));
	badRecord=io.record;badRecord.payload[2]=8;
	badRecord.header[7]=DataflowStreamChecksum(badRecord.payload,badRecord.header[6]/4);
	assert(!DataflowStreamRecordStructureValid(badRecord,17,1,7));
	auto invalid=invocation;invalid.core=DataflowCoreCount;
	assert(!DataflowStreamEncode(invalid,17,1,badRecord));

	DataflowStreamRecord staging{};alignas(64) uint32_t output[DataflowProgramRegionBytes/4];
	for (auto& word:output) word=0xfeedface;
	assert(DataflowStreamFetch(io,binding,lease,routes,7,0,staging,output,sizeof(output)) && io.calls==1);
	assert(std::memcmp(output,&invocation,sizeof(invocation))==0
		&& output[sizeof(invocation)/4]==0xfeedface);
	for (auto& word:output) word=0xfeedface;
	io.corrupt=true;
	assert(!DataflowStreamFetch(io,binding,lease,routes,7,0,staging,output,sizeof(output)));
	assert(output[0]==0);
	for (unsigned i=1;i<sizeof(output)/sizeof(output[0]);i++) assert(output[i]==0xfeedface);
	output[0]=0xfeedface;
	io.corrupt=false;io.fail=true;
	assert(!DataflowStreamFetch(io,binding,lease,routes,7,0,staging,output,sizeof(output)));
	assert(output[0]==0);
	for (unsigned i=1;i<sizeof(output)/sizeof(output[0]);i++) assert(output[i]==0xfeedface);
	const unsigned calls=io.calls;
	output[0]=0xfeedface;
	assert(!DataflowStreamFetch(io,binding,lease,routes,7,2,staging,output,sizeof(output)) && io.calls==calls);
	assert(!DataflowStreamFetch(io,binding,lease,routes,7,0,staging,output,16) && io.calls==calls);
	assert(!DataflowStreamFetch(io,binding,tooSmall,routes,7,0,staging,output,sizeof(output)) && io.calls==calls);
	assert(!DataflowStreamFetch(io,binding,lease,routes,8,0,staging,output,sizeof(output)) && io.calls==calls);
	assert(output[0]==0xfeedface);
	const auto projection=MakeProjection(7,routes);
	assert(DataflowStreamEncode(projection,17,2,io.record));
	io.fail=false;io.expectedAddress=binding.base+DataflowStreamRecordBytes;
	assert(DataflowStreamFetch(io,binding,lease,routes,7,1,staging,output,sizeof(output))
		&& std::memcmp(output,&projection,sizeof(projection))==0);
	std::puts("PASS device descriptor stream: lease bounds, core/epoch/step, corruption, NoC failure; SIMULATED ONLY");
}
