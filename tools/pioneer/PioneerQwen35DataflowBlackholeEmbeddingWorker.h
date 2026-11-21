/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_BLACKHOLE_EMBEDDING_WORKER_H
#define PIONEER_QWEN35_DATAFLOW_BLACKHOLE_EMBEDDING_WORKER_H

#include "PioneerQwen35DataflowBlackholeIO.h"
#include "PioneerQwen35DataflowEmbeddingProgram.h"

namespace PioneerQwen35 {

// Device worker for one immutable token-embedding shard. Each of the 120
// BRISC workers reads only the Q6_K blocks containing its assigned panels,
// unpacks 16 lanes locally, and writes its disjoint activation rows to GDDR.
// The host supplies no embedding values and performs no inference math.
template<class IO>
bool PioneerQwen35DataflowEmbedWorker(
	const DataflowEmbeddingInvocation& program,uint8_t* weightTile,
	float* outputPage,IO& io,unsigned* failure=nullptr,
	unsigned* failureDetail=nullptr,uint32_t* failureStatus=nullptr)
{
	if (failure) *failure=0;
	if (failureDetail) *failureDetail=0;
	if (failureStatus) *failureStatus=0;
	auto fail=[&](unsigned reason,unsigned detail=0,uint32_t status=0) {
		if (failure) *failure=reason;
		if (failureDetail) *failureDetail=detail;
		if (failureStatus) *failureStatus=status;
		return false;
	};
	if (!program.Valid() || !weightTile || !outputPage)
		return fail(DataflowFailureEmbeddingInvocation);
	uint32_t previousBlock=UINT32_MAX;
	for (unsigned i=0;i<program.segmentCount;i++) {
		const auto& output=program.segment[i];
		const unsigned element=output.firstRow;
		const unsigned block=element/256;
		const unsigned panel=(element%256)/DataflowQ6KEmbeddingValuesPerPanel;
		const uint64_t weightAddress=uint64_t(program.weightBase)
			+uint64_t(block)*DataflowQ6KEmbeddingTileBytes;
		if (weightAddress>UINT32_MAX)
			return fail(DataflowFailureEmbeddingInvocation);
		if (block!=previousBlock) {
			DataflowNocDiagnostic diagnostic{};
			if (!io.ReadWeight(program.weightBank,uint32_t(weightAddress),
				weightTile,DataflowQ6KEmbeddingTileBytes,&diagnostic))
				return fail(DataflowFailureEmbeddingWeightRead,
					DataflowNocDiagnosticCode(diagnostic),diagnostic.value);
		}
		if (!DataflowDecodeQ6KEmbeddingPanel(weightTile,
			DataflowQ6KEmbeddingTileBytes,program.lane,panel,outputPage,
			DataflowQ6KEmbeddingValuesPerPanel))
			return fail(DataflowFailureEmbeddingUnpack);
		if (!io.WriteOutput(output.bank,output.address,
				reinterpret_cast<const uint8_t*>(outputPage),
				DataflowQ6KEmbeddingValuesPerPanel*sizeof(float)))
			return fail(DataflowFailureEmbeddingOutputWrite);
		previousBlock=block;
	}
	return true;
}

class DataflowBlackholeEmbeddingOps {
	const DataflowEmbeddingInvocation* fProgram=nullptr;
	DataflowBlackholeNoc<0> fReader;
	DataflowBlackholeNoc<1> fWriter;

public:
	bool Bind(const DataflowEmbeddingInvocation& program)
	{
		fProgram=nullptr;
		if (!program.Valid()) return false;
		fProgram=&program;
		return true;
	}

	bool ReadWeight(unsigned bank,uint32_t address,uint8_t* output,
		unsigned bytes,DataflowNocDiagnostic* detail=nullptr) const
	{
		if (detail) *detail={};
		if (!fProgram || bank!=fProgram->weightBank
			|| bytes!=DataflowQ6KEmbeddingTileBytes) {
			if (detail) detail->stage=DataflowNocTransferWeightBinding;
			return false;
		}
		for (unsigned i=0;i<fProgram->segmentCount;i++) {
			const unsigned block=fProgram->segment[i].firstRow/256;
			const uint64_t expected=uint64_t(fProgram->weightBase)
				+uint64_t(block)*DataflowQ6KEmbeddingTileBytes;
			if (expected==address)
				return fReader.Read(address,fProgram->bankRoute[0][bank],
					output,bytes,detail);
		}
		if (detail) detail->stage=DataflowNocTransferWeightBinding;
		return false;
	}

	bool WriteOutput(unsigned bank,uint32_t address,const uint8_t* input,
		unsigned bytes) const
	{
		if (!fProgram || bytes!=DataflowQ6KEmbeddingValuesPerPanel*sizeof(float))
			return false;
		for (unsigned i=0;i<fProgram->segmentCount;i++) {
			const auto& output=fProgram->segment[i];
			if (output.bank==bank && output.address==address
				&& output.bytes==bytes)
				return fWriter.Write(address,fProgram->bankRoute[1][bank],
				input,bytes);
		}
		return false;
	}
};

} // namespace PioneerQwen35

#endif
