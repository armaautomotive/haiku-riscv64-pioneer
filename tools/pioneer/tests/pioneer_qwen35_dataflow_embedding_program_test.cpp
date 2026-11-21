/* SPDX-License-Identifier: MIT */
#include <cassert>

#include "../PioneerQwen35DataflowEmbeddingProgram.h"

using namespace PioneerQwen35;

static const unsigned TestBankRoutes[2][Banks]={
	{786,978,1170,785,977,1169,1361},
	{786,978,1170,785,977,1169,1361}
};

static DeviceLayout MakeLayout()
{
	DeviceLayout model{};
	model.magic=LayoutMagic;
	model.version=LayoutVersion;
	model.context=64;
	model.tied.type=14;
	model.tied.inputs=Hidden;
	auto& weights=model.tied.place;
	weights.rows=Vocabulary;
	weights.rowBytes=2120;
	weights.count=Banks;
	weights.unit=16;
	const unsigned groups=Vocabulary/16;
	const unsigned base=groups/Banks,extra=groups%Banks;
	unsigned firstRow=0;
	for (unsigned bank=0;bank<Banks;bank++) {
		const unsigned rows=(base+(bank<extra))*16;
		weights.parts[bank]={bank,BankBase,firstRow,rows};
		firstRow+=rows;
	}
	auto& output=model.activation[0];
	output.rows=9216;
	output.rowBytes=sizeof(float);
	output.count=1;
	output.unit=16;
	output.parts[0]={6,BankBase+0x20000000,0,output.rows};
	auto& second=model.activation[1];
	second.rows=9216;
	second.rowBytes=sizeof(float);
	second.count=1;
	second.unit=16;
	second.parts[0]={5,BankBase+0x20000000,0,second.rows};
	assert(weights.Valid() && output.Valid() && second.Valid());
	return model;
}

int main()
{
	const DeviceLayout model=MakeLayout();
	const unsigned tokens[]={0U,15U,16U,65535U,Vocabulary-1};
	for (unsigned token:tokens) {
		DataflowEmbeddingProgram program{};
		assert(DataflowEmbeddingProgramBuilder::Build(model,token,0,
			TestBankRoutes,program));
		assert(program.Valid() && program.token==token
			&& program.lane==(token&15)
			&& program.outputSlot==0);
		bool covered[DataflowEmbeddingGroups]={};
		unsigned total=0;
		for (unsigned core=0;core<DataflowCoreCount;core++) {
			DataflowEmbeddingInvocation invocation{};
			assert(DataflowEmbeddingProgramBuilder::BindCore(program,core,
				invocation));
			assert(invocation.Valid() && invocation.core==core
				&& invocation.token==token);
			for (unsigned segment=0;segment<invocation.segmentCount;segment++) {
				const unsigned group=invocation.segment[segment].firstRow/16;
				assert(group<DataflowEmbeddingGroups && !covered[group]);
				covered[group]=true;
				total++;
			}
		}
		assert(total==DataflowEmbeddingGroups);
		for (bool value:covered) assert(value);
	}

	DataflowEmbeddingProgram program{};
	assert(!DataflowEmbeddingProgramBuilder::Build(model,Vocabulary,0,
		TestBankRoutes,program));
	assert(!DataflowEmbeddingProgramBuilder::Build(model,1,ActivationBuffers,
		TestBankRoutes,program));
	unsigned duplicateRoutes[2][Banks];
	for (unsigned noc=0;noc<2;noc++) for (unsigned bank=0;bank<Banks;bank++)
		duplicateRoutes[noc][bank]=TestBankRoutes[noc][bank];
	duplicateRoutes[0][1]=duplicateRoutes[0][0];
	assert(!DataflowEmbeddingProgramBuilder::Build(model,1,0,
		duplicateRoutes,program));

	DeviceLayout invalid=MakeLayout();
	invalid.tied.place.rowBytes--;
	assert(!DataflowEmbeddingProgramBuilder::Build(invalid,1,0,
		TestBankRoutes,program));
	return 0;
}
