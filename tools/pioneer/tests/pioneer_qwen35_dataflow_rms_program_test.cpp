/* SPDX-License-Identifier: MIT */
#include <cassert>

#include "../PioneerQwen35DataflowRmsProgram.h"

using namespace PioneerQwen35;

static const unsigned TestBankRoutes[2][Banks]={
	{786,978,1170,785,977,1169,1361},
	{786,978,1170,785,977,1169,1361}
};

static void Rows(RowSpace& space,unsigned rows,uint32_t base)
{
	space={};
	space.rows=rows;
	space.rowBytes=sizeof(float);
	space.count=Banks;
	space.unit=16;
	const unsigned groups=rows/16;
	const unsigned baseGroups=groups/Banks,extra=groups%Banks;
	unsigned first=0;
	for (unsigned bank=0;bank<Banks;bank++) {
		const unsigned count=baseGroups+unsigned(bank<extra);
		space.parts[bank]={bank,base+bank*0x01000000,first*16,count*16};
		first+=count;
	}
}

static DeviceLayout MakeLayout()
{
	DeviceLayout model{};
	model.magic=LayoutMagic;
	model.version=LayoutVersion;
	model.context=64;
	TensorSpace& norm=model.layers[0].weights[AttentionNorm];
	norm.type=0;
	norm.inputs=Hidden;
	norm.place.rows=1;
	norm.place.rowBytes=Hidden*sizeof(float);
	norm.place.count=1;
	norm.place.unit=1;
	norm.place.parts[0]={1,BankBase+0x10000000,0,1};
	Rows(model.activation[0],Ffn,BankBase+0x20000000);
	Rows(model.activation[1],Ffn,BankBase+0x30000000);
	assert(norm.place.Valid() && model.activation[0].Valid()
		&& model.activation[1].Valid());
	return model;
}

int main()
{
	const DeviceLayout model=MakeLayout();
	DataflowRmsProgram program{};
	assert(DataflowRmsProgramBuilder::Build(model,0,AttentionNorm,0,1,
		TestBankRoutes,program));
	assert(program.Valid() && program.scratchRow>=Hidden
		&& program.scratchRow+16<=Ffn);
	bool covered[DataflowRmsGroups]={};
	unsigned total=0;
	for (unsigned core=0;core<DataflowCoreCount;core++) {
		DataflowRmsInvocation invocation{};
		assert(DataflowRmsProgramBuilder::BindCore(program,core,invocation));
		assert(invocation.Valid() && invocation.core==core);
		for (unsigned i=0;i<invocation.segmentCount;i++) {
			const unsigned group=invocation.segment[i].firstRow/16;
			assert(group<DataflowRmsGroups && !covered[group]);
			covered[group]=true;
			total++;
		}
	}
	assert(total==DataflowRmsGroups);
	for (bool value:covered) assert(value);

	assert(!DataflowRmsProgramBuilder::Build(model,0,Gate,0,1,
		TestBankRoutes,program));
	assert(!DataflowRmsProgramBuilder::Build(model,0,AttentionNorm,0,0,
		TestBankRoutes,program));
	DeviceLayout invalid=MakeLayout();
	invalid.layers[0].weights[AttentionNorm].type=12;
	assert(!DataflowRmsProgramBuilder::Build(invalid,0,AttentionNorm,0,1,
		TestBankRoutes,program));
	return 0;
}
