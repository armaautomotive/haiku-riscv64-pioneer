/* SPDX-License-Identifier: MIT */
#include <cassert>
#include <cstdint>
#include <cstring>

#include "../PioneerAttentionMath.h"
#include "../PioneerQwen35DataflowBlackholeEmbeddingWorker.h"
#include "../PioneerQwen35Embedding.h"

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

struct MockEmbeddingIO {
	unsigned weightBank,weightBase,outputBank,outputBase,lane;
	const uint8_t (*tiles)[DataflowQ6KEmbeddingTileBytes];
	float* result;
	unsigned reads=0,writes=0;
	bool failRead=false,failWrite=false;

	bool ReadWeight(unsigned bank,uint32_t address,uint8_t* output,unsigned bytes,
		DataflowNocDiagnostic* detail=nullptr)
	{
		if (detail) *detail=failRead
			?DataflowNocDiagnostic{DataflowNocTransferCounterTimeout,0x80,1,7}
			:DataflowNocDiagnostic{};
		if (failRead || bank!=weightBank || bytes!=DataflowQ6KEmbeddingTileBytes
			|| address<weightBase) return false;
		const unsigned offset=address-weightBase;
		if (offset%DataflowQ6KEmbeddingTileBytes) return false;
		const unsigned block=offset/DataflowQ6KEmbeddingTileBytes;
		if (block>=DataflowEmbeddingTileCount) return false;
		memcpy(output,tiles[block],bytes);
		reads++;
		return true;
	}

	bool WriteOutput(unsigned bank,uint32_t address,const uint8_t* input,
		unsigned bytes)
	{
		if (failWrite || bank!=outputBank || address<outputBase || bytes!=64
			|| ((address-outputBase)&63)) return false;
		const unsigned first=(address-outputBase)/sizeof(float);
		if (first>=Hidden || Hidden-first<16) return false;
		memcpy(result+first,input,bytes);
		writes++;
		return true;
	}
};

int main()
{
	DeviceLayout model=MakeLayout();
	constexpr unsigned Token=35359;
	DataflowEmbeddingProgram program{};
	assert(DataflowEmbeddingProgramBuilder::Build(model,Token,0,
		TestBankRoutes,program));
	uint8_t tiles[DataflowEmbeddingTileCount][DataflowQ6KEmbeddingTileBytes];
	for (unsigned block=0;block<DataflowEmbeddingTileCount;block++) {
		for (unsigned i=0;i<DataflowQ6KEmbeddingTileBytes;i++)
			tiles[block][i]=uint8_t(block*37U+i*73U+(i>>3)*19U+11U);
		for (unsigned lane=0;lane<16;lane++) {
			const uint16_t half=uint16_t(0x3000U+(lane*0x31U));
			tiles[block][208*16+lane]=uint8_t(half);
			tiles[block][209*16+lane]=uint8_t(half>>8);
		}
	}
	float expected[Hidden],decoded[256];
	for (unsigned block=0;block<DataflowEmbeddingTileCount;block++) {
		assert(PioneerQwen35Embedding::DecodeBlock(tiles[block],
			DataflowQ6KEmbeddingTileBytes,program.lane,decoded,256));
		memcpy(expected+block*256,decoded,sizeof(decoded));
	}
	float actual[Hidden];
	for (float& value:actual) value=0;
	for (unsigned core=0;core<DataflowCoreCount;core++) {
		DataflowEmbeddingInvocation invocation{};
		assert(DataflowEmbeddingProgramBuilder::BindCore(program,core,
			invocation));
		uint8_t tileScratch[DataflowQ6KEmbeddingTileBytes];
		float outputScratch[DataflowQ6KEmbeddingValuesPerPanel];
		MockEmbeddingIO io={program.weightBank,program.weightBase,
			program.output.parts[0].bank,program.output.parts[0].address,
			program.lane,tiles,actual,0,0};
		assert(PioneerQwen35DataflowEmbedWorker(invocation,tileScratch,
			outputScratch,io));
		assert(io.writes==invocation.segmentCount);
	}
	for (unsigned i=0;i<Hidden;i++)
		assert(PioneerAttentionBits(actual[i])==PioneerAttentionBits(expected[i]));

	DataflowEmbeddingInvocation goodInvocation{};
	assert(DataflowEmbeddingProgramBuilder::BindCore(program,0,goodInvocation));
	DataflowEmbeddingInvocation invalid=goodInvocation;
	invalid.segment[0].address+=64;
	uint8_t tileScratch[DataflowQ6KEmbeddingTileBytes];
	float outputScratch[DataflowQ6KEmbeddingValuesPerPanel];
	MockEmbeddingIO io={program.weightBank,program.weightBase,
		program.output.parts[0].bank,program.output.parts[0].address,
		program.lane,tiles,actual,0,0};
	assert(!PioneerQwen35DataflowEmbedWorker(invalid,tileScratch,
		outputScratch,io));
	unsigned failure=0;
	assert(!PioneerQwen35DataflowEmbedWorker(invalid,tileScratch,
		outputScratch,io,&failure));
	assert(failure==DataflowFailureEmbeddingInvocation);
	io.failRead=true;
	unsigned failureDetail=0;
	uint32_t failureStatus=0,failureExpected=0;
	assert(!PioneerQwen35DataflowEmbedWorker(goodInvocation,tileScratch,
		outputScratch,io,&failure,&failureDetail,&failureStatus,&failureExpected));
	assert(failure==DataflowFailureEmbeddingWeightRead
		&& failureDetail==DataflowNocDiagnosticCode(
			DataflowNocDiagnostic{DataflowNocTransferCounterTimeout,0x80,1,7})
		&& failureStatus==1 && failureExpected==7);
	io.failRead=false;
	io.failWrite=true;
	assert(!PioneerQwen35DataflowEmbedWorker(goodInvocation,tileScratch,
		outputScratch,io,&failure));
	assert(failure==DataflowFailureEmbeddingOutputWrite);
	io.failWrite=false;
	assert(!PioneerQwen35DataflowEmbedWorker(goodInvocation,nullptr,
		outputScratch,io,&failure));
	assert(failure==DataflowFailureEmbeddingInvocation);
	return 0;
}
