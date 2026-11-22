/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_GRAPH_GENERATION_H
#define PIONEER_QWEN35_DATAFLOW_GRAPH_GENERATION_H

#include "PioneerQwen35DataflowGenerationSequence.h"
#include "PioneerQwen35DataflowGraphSession.h"

#ifdef PIONEER_QWEN35_DEVICE_DISPATCH
namespace PioneerQwen35 {

// Host orchestration only: each graph computes its own output on the device.
// The first selected token belongs to prompt time; decode time covers only
// subsequent generated tokens, matching the client throughput calculation.
class DataflowGraphGeneration {
	static uint64_t Elapsed(uint64_t start,uint64_t end) { return end>=start?end-start:0; }
public:
	template<class Owner,class Timer>
	static DataflowGenerationSequenceResult Run(const unsigned* prompt,unsigned promptCount,
		unsigned maxNewTokens,unsigned eosToken,unsigned context,unsigned polls,Owner& owner,
		unsigned* output,unsigned& outputCount,unsigned& stage,Timer& timer,
		uint64_t& promptMicros,uint64_t& decodeMicros)
	{
		outputCount=0;stage=DataflowGenerationSequenceInvalidRequest;promptMicros=decodeMicros=0;
		if (!prompt || !output || !promptCount || !maxNewTokens || eosToken>Vocabulary
			|| !context || context>MaxPositions || promptCount>context
			|| maxNewTokens>context-promptCount+1 || !polls || polls>DataflowGraphRunMaxPolls)
			return DataflowGenerationSequenceInvalidRequest;
		for (unsigned i=0;i<promptCount;i++) if (prompt[i]>=Vocabulary)
			return DataflowGenerationSequenceInvalidRequest;
		const uint64_t promptStart=timer.NowMicros();
		DataflowArgmaxResult result{};unsigned graphStage=0;
		auto run=[&](unsigned position,unsigned token,bool select) {
			const auto status=owner.Run(position,token,select,polls,result,graphStage);
			if (status!=DataflowGraphSessionComplete || (select && !DataflowArgmaxResultValid(result))) {
				const unsigned detail=graphStage?graphStage:(0x8000U|unsigned(status));
				stage=(position<<16)|(detail&0xffffU);return false;
			}
			return true;
		};
		for (unsigned position=0;position+1<promptCount;position++) {
			if (!run(position,prompt[position],false)) {
				promptMicros=Elapsed(promptStart,timer.NowMicros());return DataflowGenerationSequenceDecoderFailed;
			}
		}
		if (!run(promptCount-1,prompt[promptCount-1],true)) {
			promptMicros=Elapsed(promptStart,timer.NowMicros());return DataflowGenerationSequenceDecoderFailed;
		}
		output[0]=result.token;outputCount=1;promptMicros=Elapsed(promptStart,timer.NowMicros());
		if (result.token==eosToken || maxNewTokens==1) {
			stage=DataflowGenerationSequenceComplete;return DataflowGenerationSequenceComplete;
		}
		const uint64_t decodeStart=timer.NowMicros();
		for (unsigned generated=1;generated<maxNewTokens;generated++) {
			const unsigned position=promptCount+generated-1;
			if (!run(position,output[generated-1],true)) {
				decodeMicros=Elapsed(decodeStart,timer.NowMicros());return DataflowGenerationSequenceDecoderFailed;
			}
			output[generated]=result.token;outputCount=generated+1;
			if (result.token==eosToken) break;
		}
		decodeMicros=Elapsed(decodeStart,timer.NowMicros());
		stage=DataflowGenerationSequenceComplete;return DataflowGenerationSequenceComplete;
	}
};
}
#endif
#endif
