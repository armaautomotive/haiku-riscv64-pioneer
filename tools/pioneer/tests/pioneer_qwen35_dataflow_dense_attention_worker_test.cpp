/* SPDX-License-Identifier: MIT */
#define PIONEER_Q35_OPERATOR_TEST
#include "../PioneerQwen35Operators.h"
#include "../PioneerQwen35DataflowBlackholeDenseAttentionWorker.h"

#include <assert.h>
#include <cstring>

using namespace PioneerQwen35;

static const unsigned TestRoutes[2][Banks]={
	{786,978,1170,785,977,1169,1361},
	{786,978,1170,785,977,1169,1361}
};

static float Sample(unsigned index,unsigned salt)
{
	const int centered=int((index*17+salt*11)%97)-48;
	return float(centered)*0.0007f;
}

static void FlatRows(RowSpace& space,unsigned rows,unsigned rowBytes,
	unsigned bank,uint32_t address,unsigned unit)
{
	space={};
	space.rows=rows;
	space.rowBytes=rowBytes;
	space.unit=unit;
	space.count=1;
	space.parts[0]={bank,address,0,rows};
}

struct MockAttentionIO {
	const DataflowDenseAttentionInvocation* invocation=nullptr;
	float* qg;
	float* keys;
	float* values;
	float* queryNorm;
	float* keyNorm;
	float* rope;
	uint16_t* kv;
	float* context;
	unsigned position,contextLength;

	bool Bind(const DataflowDenseAttentionInvocation& candidate)
	{
		invocation=&candidate;
		return candidate.Valid();
	}

	bool ReadProjectedRows(unsigned first,float* output) const
	{
		if (!invocation || !output) return false;
		const float* source=nullptr;
		if (invocation->program.mode==DataflowDenseAttentionPackKey)
			source=keys;
		else if (invocation->program.mode==DataflowDenseAttentionPackValue)
			source=values;
		else
			source=qg;
		const unsigned limit=invocation->program.mode==DataflowDenseAttentionRun
			?ConvChannels:DeltaInner;
		if (first>=limit || DenseHeadWidth>limit-first) return false;
		std::memcpy(output,source+first,DenseHeadWidth*sizeof(float));
		return true;
	}

	bool ReadNorm(bool query,float* output) const
	{
		if (!output) return false;
		std::memcpy(output,query?queryNorm:keyNorm,
			DenseHeadWidth*sizeof(float));
		return true;
	}

	bool ReadRope(float* output) const
	{
		if (!output) return false;
		std::memcpy(output,rope,Rotary*sizeof(float));
		return true;
	}

	bool WriteKv(unsigned head,bool value,const uint32_t* packed) const
	{
		if (!packed || head>=DenseKeyHeads) return false;
		const unsigned row=head+(value?DenseKeyHeads:0U);
		std::memcpy(kv+(row*contextLength+position)*DenseHeadWidth,
			packed,DataflowDenseAttentionHeadBytes);
		return true;
	}

	bool ReadKvTile(unsigned head,bool value,unsigned first,
		unsigned positions,float* output) const
	{
		if (!output || head>=DenseKeyHeads || first>position
			|| positions>position-first+1) return false;
		const unsigned row=head+(value?DenseKeyHeads:0U);
		std::memcpy(output,kv+(row*contextLength+first)*DenseHeadWidth,
			positions*DataflowDenseAttentionHeadBytes);
		return true;
	}

	bool WriteContext(unsigned firstRow,const float* input) const
	{
		if (!input || firstRow>=DenseHeads*DenseHeadWidth
			|| DenseHeadWidth>DenseHeads*DenseHeadWidth-firstRow) return false;
		std::memcpy(context+firstRow,input,DenseHeadWidth*sizeof(float));
		return true;
	}
};

static DataflowDenseAttentionProgram Program(unsigned mode)
{
	DataflowDenseAttentionProgram program{};
	program.layer=3;
	program.position=2;
	program.context=4;
	program.mode=mode;
	for (unsigned noc=0;noc<2;noc++) for (unsigned bank=0;bank<Banks;bank++)
		program.bankRoute[noc][bank]=TestRoutes[noc][bank];
	FlatRows(program.qkv,ConvChannels,sizeof(float),0,0x10000000,16);
	FlatRows(program.gate,DeltaInner,sizeof(float),0,0x10010000,16);
	FlatRows(program.contextOutput,Ffn,sizeof(float),0,0x10020000,16);
	FlatRows(program.kv,2*DenseKeyHeads,
		program.context*DenseHeadWidth*sizeof(uint16_t),1,0x20000000,1);
	program.queryNorm.type=0;
	program.queryNorm.inputs=DenseHeadWidth;
	FlatRows(program.queryNorm.place,1,DenseHeadWidth*sizeof(float),
		2,0x30000000,1);
	program.keyNorm.type=0;
	program.keyNorm.inputs=DenseHeadWidth;
	FlatRows(program.keyNorm.place,1,DenseHeadWidth*sizeof(float),
		3,0x40000000,1);
	program.rope={0,0x10030000,
		unsigned(program.context*Rotary*sizeof(float))};
	return program;
}

int main()
{
	constexpr unsigned contextLength=4;
	const unsigned position=2;
	float qg[ConvChannels],keyProjection[DenseKeyHeads*DenseHeadWidth];
	float valueProjection[DenseKeyHeads*DenseHeadWidth];
	float queryNorm[DenseHeadWidth],keyNorm[DenseHeadWidth],rope[Rotary];
	uint16_t kv[2*DenseKeyHeads*contextLength*DenseHeadWidth]{};
	float contextOutput[DenseHeads*DenseHeadWidth]{};
	for (unsigned i=0;i<ConvChannels;i++) qg[i]=Sample(i,1);
	for (unsigned i=0;i<DenseKeyHeads*DenseHeadWidth;i++) {
		keyProjection[i]=Sample(i,2);
		valueProjection[i]=Sample(i,3);
	}
	for (unsigned i=0;i<DenseHeadWidth;i++) {
		queryNorm[i]=0.75f+float(i%23)*0.002f;
		keyNorm[i]=0.8f+float(i%19)*0.002f;
	}
	for (unsigned i=0;i<Rotary;i++) rope[i]=float(int(i%13)-6)*0.025f;
	// Seed prior positions with exact packed half K/V rows.
	for (unsigned head=0;head<DenseKeyHeads;head++)
		for (unsigned at=0;at<position;at++)
			for (unsigned lane=0;lane<DenseHeadWidth;lane++) {
				kv[((head*contextLength+at)*DenseHeadWidth)+lane]
					=PioneerAttentionToHalf(Sample(lane,head+at+5));
				kv[(((head+DenseKeyHeads)*contextLength+at)*DenseHeadWidth)+lane]
					=PioneerAttentionToHalf(Sample(lane,head+at+9));
			}

	float queryLocal[DenseHeadWidth],gateLocal[DenseHeadWidth];
	float normLocal[DenseHeadWidth],ropeLocal[Rotary];
	float outputLocal[DenseHeadWidth];
	float keyTile[DataflowDenseAttentionTilePositions*DenseHeadWidth];
	float valueTile[DataflowDenseAttentionTilePositions*DenseHeadWidth];
	const unsigned modes[3]={DataflowDenseAttentionPackKey,
		DataflowDenseAttentionPackValue,DataflowDenseAttentionRun};
	for (unsigned mode:modes) {
		DataflowDenseAttentionProgram program=Program(mode);
		assert(program.Valid());
		const unsigned count=mode==DataflowDenseAttentionRun
			?DenseHeads:DenseKeyHeads;
		for (unsigned core=0;core<DataflowCoreCount;core++) {
			DataflowDenseAttentionInvocation invocation{};
			assert(DataflowDenseAttentionProgramBuilder::BindCore(program,core,
				invocation));
			MockAttentionIO io{nullptr,qg,keyProjection,valueProjection,
				queryNorm,keyNorm,rope,kv,contextOutput,position,contextLength};
			assert(PioneerQwen35DataflowDenseAttentionWorker(invocation,
				keyTile,valueTile,queryLocal,gateLocal,normLocal,ropeLocal,
				outputLocal,io));
		}
		assert(count==(mode==DataflowDenseAttentionRun
			?DenseHeads:DenseKeyHeads));
	}

	for (unsigned head=0;head<DenseKeyHeads;head++) {
		float key[DenseHeadWidth];
		uint32_t expected[DenseHeadWidth/2];
		std::memcpy(key,keyProjection+head*DenseHeadWidth,sizeof(key));
		assert(Rms<DenseHeadWidth>(key,keyNorm,key));
		assert(Rotate(key,rope));
		assert(PackHead(key,expected));
		assert(!std::memcmp(expected,
			kv+(head*contextLength+position)*DenseHeadWidth,
			DataflowDenseAttentionHeadBytes));
		assert(PackHead(valueProjection+head*DenseHeadWidth,expected));
		assert(!std::memcmp(expected,
			kv+((head+DenseKeyHeads)*contextLength+position)*DenseHeadWidth,
			DataflowDenseAttentionHeadBytes));
	}
	for (unsigned head=0;head<DenseHeads;head++) {
		float query[DenseHeadWidth],expected[DenseHeadWidth];
		uint32_t packedKey[DenseHeadWidth/2],packedValue[DenseHeadWidth/2];
		const float* qgHead=qg+2*head*DenseHeadWidth;
		std::memcpy(query,qgHead,sizeof(query));
		assert(Rms<DenseHeadWidth>(query,queryNorm,query));
		assert(Rotate(query,rope));
		AttentionRow row{expected};
		const unsigned kvHead=head/(DenseHeads/DenseKeyHeads);
		for (unsigned at=0;at<=position;at++) {
			std::memcpy(packedKey,
				kv+(kvHead*contextLength+at)*DenseHeadWidth,
				DataflowDenseAttentionHeadBytes);
			std::memcpy(packedValue,
				kv+((kvHead+DenseKeyHeads)*contextLength+at)*DenseHeadWidth,
				DataflowDenseAttentionHeadBytes);
			assert(row.Add(query,packedKey,packedValue));
		}
		assert(row.Finish(qgHead+DenseHeadWidth));
		assert(!std::memcmp(expected,
			contextOutput+head*DenseHeadWidth,sizeof(expected)));
	}
	return 0;
}
