/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_EMBEDDING_PROGRAM_H
#define PIONEER_QWEN35_DATAFLOW_EMBEDDING_PROGRAM_H

#include "PioneerQwen35DataflowEmbedding.h"
#include "PioneerQwen35DataflowProgram.h"

namespace PioneerQwen35 {

enum : unsigned {
	DataflowEmbeddingMagic=0x31455035,
	DataflowEmbeddingVersion=1,
	DataflowEmbeddingGroups=Hidden/16,
	DataflowEmbeddingTileCount=Hidden/256,
	DataflowEmbeddingMaxCoreSegments=2,
	DataflowEmbeddingSplitCores=DataflowEmbeddingGroups-DataflowCoreCount
};

template<class T>
inline void DataflowEmbeddingZeroWords(T& value)
{
	static_assert(sizeof(T)%sizeof(uint32_t)==0,
		"device embedding records contain whole words");
	uint32_t* words=reinterpret_cast<uint32_t*>(&value);
	for (unsigned i=0;i<sizeof(T)/sizeof(uint32_t);i++) words[i]=0;
}

struct DataflowEmbeddingProgram {
	unsigned token,lane,outputSlot,weightBank,weightBase;
	unsigned bankRoute[2][Banks];
	RowSpace output;

	bool Valid() const
	{
		if (token>=Vocabulary || lane!=(token&15)
			|| outputSlot>=ActivationBuffers || weightBank>=Banks
			|| weightBase<BankBase || weightBase>=BankLimit
			|| uint64_t(DataflowEmbeddingTileCount)
				*DataflowQ6KEmbeddingTileBytes>BankLimit-weightBase
			|| !DataflowBankRoutesValid(bankRoute) || !output.Valid()
			|| output.rows<Hidden || output.rowBytes!=sizeof(float)
			|| output.unit!=16) return false;
		return true;
	}
};

struct DataflowEmbeddingInvocation {
	unsigned magic,version,core,token,lane,weightBank,weightBase,segmentCount;
	unsigned bankRoute[2][Banks];
	RowSpace output;
	DataflowOutputSegment segment[DataflowEmbeddingMaxCoreSegments];

	bool Valid() const
	{
		if (magic!=DataflowEmbeddingMagic || version!=DataflowEmbeddingVersion
			|| core>=DataflowCoreCount || token>=Vocabulary || lane!=(token&15)
			|| weightBank>=Banks || weightBase<BankBase || weightBase>=BankLimit
			|| uint64_t(DataflowEmbeddingTileCount)*DataflowQ6KEmbeddingTileBytes
				>BankLimit-weightBase || !DataflowBankRoutesValid(bankRoute)
			|| !output.Valid() || output.rows<Hidden
			|| output.rowBytes!=sizeof(float) || output.unit!=16
			|| segmentCount!=(core<DataflowEmbeddingSplitCores?2U:1U))
			return false;
		const unsigned firstGroup=core<DataflowEmbeddingSplitCores
			?core*2:core+DataflowEmbeddingSplitCores;
		for (unsigned i=0;i<segmentCount;i++) {
			const auto& outputPart=segment[i];
			const unsigned row=(firstGroup+i)*16;
			Range expected{};
			if (row>=Hidden || !output.Resolve(row,16,0,16*sizeof(float),expected)
				|| outputPart.bank!=expected.bank
				|| outputPart.address!=expected.address
				|| outputPart.firstRow!=row || outputPart.rows!=16
				|| outputPart.bytes!=16*sizeof(float)) return false;
		}
		for (unsigned i=segmentCount;i<DataflowEmbeddingMaxCoreSegments;i++)
			if (segment[i].bank || segment[i].address || segment[i].firstRow
				|| segment[i].rows || segment[i].bytes) return false;
		return true;
	}
};

class DataflowEmbeddingProgramBuilder {
public:
	static bool Build(const DeviceLayout& manifest,unsigned token,
		unsigned outputSlot,const unsigned (&bankRoute)[2][Banks],
		DataflowEmbeddingProgram& result)
	{
		DataflowEmbeddingZeroWords(result);
		if (manifest.magic!=LayoutMagic || manifest.version!=LayoutVersion
			|| !manifest.context || token>=Vocabulary
			|| outputSlot>=ActivationBuffers
			|| manifest.tied.type!=14 || manifest.tied.inputs!=Hidden
			|| manifest.tied.place.rows!=Vocabulary
			|| manifest.tied.place.rowBytes!=2120
			|| manifest.tied.place.unit!=16
			|| !DataflowBankRoutesValid(bankRoute)) return false;
		Range embedding{};
		unsigned lane=0;
		const RowSpace& output=manifest.activation[outputSlot];
		if (!manifest.ResolveEmbedding(token,embedding,lane)
			|| embedding.bytes!=DataflowEmbeddingTileCount
				*DataflowQ6KEmbeddingTileBytes
			|| uint64_t(embedding.address)+embedding.bytes>BankLimit
			|| !output.Valid() || output.rows<Hidden
			|| output.rowBytes!=sizeof(float) || output.unit!=16) return false;
		DataflowEmbeddingProgram candidate;
		DataflowEmbeddingZeroWords(candidate);
		candidate.token=token;
		candidate.lane=lane;
		candidate.outputSlot=outputSlot;
		candidate.weightBank=embedding.bank;
		candidate.weightBase=embedding.address;
		for (unsigned noc=0;noc<2;noc++) for (unsigned bank=0;bank<Banks;bank++)
			candidate.bankRoute[noc][bank]=bankRoute[noc][bank];
		candidate.output=output;
		if (!candidate.Valid()) return false;
		result=candidate;
		return true;
	}

	static bool BindCore(const DataflowEmbeddingProgram& program,unsigned core,
		DataflowEmbeddingInvocation& result)
	{
		if (!program.Valid() || core>=DataflowCoreCount) return false;
		DataflowEmbeddingInvocation candidate;
		DataflowEmbeddingZeroWords(candidate);
		candidate.magic=DataflowEmbeddingMagic;
		candidate.version=DataflowEmbeddingVersion;
		candidate.core=core;
		candidate.token=program.token;
		candidate.lane=program.lane;
		candidate.weightBank=program.weightBank;
		candidate.weightBase=program.weightBase;
		candidate.segmentCount=core<DataflowEmbeddingSplitCores?2:1;
		for (unsigned noc=0;noc<2;noc++) for (unsigned bank=0;bank<Banks;bank++)
			candidate.bankRoute[noc][bank]=program.bankRoute[noc][bank];
		candidate.output=program.output;
		const unsigned firstGroup=core<DataflowEmbeddingSplitCores
			?core*2:core+DataflowEmbeddingSplitCores;
		for (unsigned i=0;i<candidate.segmentCount;i++) {
			const unsigned firstRow=(firstGroup+i)*16;
			Range destination{};
			if (!program.output.Resolve(firstRow,16,0,16*sizeof(float),destination))
				return false;
			candidate.segment[i]={destination.bank,destination.address,
				firstRow,16,16*sizeof(float)};
		}
		if (!candidate.Valid()) return false;
		result=candidate;
		return true;
	}
};

static_assert(sizeof(DataflowEmbeddingInvocation)<=DataflowProgramRegionBytes,
	"the token lookup invocation fits the resident L1 command slot");

} // namespace PioneerQwen35

#endif
