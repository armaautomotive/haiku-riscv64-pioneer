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
	unsigned* failureDetail=nullptr,uint32_t* failureStatus=nullptr,
	uint32_t* failureExpected=nullptr,uint32_t* failureNoCState=nullptr,
	uint32_t* failureOutstandingIdMask=nullptr,
	uint32_t* failureAddress=nullptr,uint32_t* failureRoute=nullptr,
	uint32_t* failureBytes=nullptr)
{
	if (failure) *failure=0;
	if (failureDetail) *failureDetail=0;
	if (failureStatus) *failureStatus=0;
	if (failureExpected) *failureExpected=0;
	if (failureNoCState) *failureNoCState=0;
	if (failureOutstandingIdMask) *failureOutstandingIdMask=0;
	if (failureAddress) *failureAddress=0;
	if (failureRoute) *failureRoute=0;
	if (failureBytes) *failureBytes=0;
	auto fail=[&](unsigned reason,unsigned detail=0,uint32_t status=0,
		uint32_t expected=0,uint32_t noCState=0,uint32_t outstandingIdMask=0,
		uint32_t address=0,uint32_t route=0,uint32_t bytes=0) {
		if (failure) *failure=reason;
		if (failureDetail) *failureDetail=detail;
		if (failureStatus) *failureStatus=status;
		if (failureExpected) *failureExpected=expected;
		if (failureNoCState) *failureNoCState=noCState;
		if (failureOutstandingIdMask) *failureOutstandingIdMask=outstandingIdMask;
		if (failureAddress) *failureAddress=address;
		if (failureRoute) *failureRoute=route;
		if (failureBytes) *failureBytes=bytes;
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
					DataflowNocDiagnosticCode(diagnostic),diagnostic.value,
				diagnostic.expected,
				diagnostic.counterMatchMask
					|((diagnostic.initiatorBusyMask&0xfU)<<8),
				diagnostic.outstandingIdMask,diagnostic.remoteAddress,
				diagnostic.route,diagnostic.bytes);
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
			if (expected==address) {
				const unsigned route=fProgram->bankRoute[0][bank];
				const bool okay=fReader.Read(address,route,output,bytes,detail);
				if (!okay && detail) {
					detail->remoteAddress=address;
					detail->route=route;
					detail->bytes=bytes;
				}
				return okay;
			}
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
