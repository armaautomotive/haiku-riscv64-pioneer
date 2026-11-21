/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_GENERATION_SEQUENCE_H
#define PIONEER_QWEN35_DATAFLOW_GENERATION_SEQUENCE_H

#include "PioneerQwen35DataflowDecoderSequence.h"

namespace PioneerQwen35 {

enum DataflowGenerationSequenceResult {
	DataflowGenerationSequenceComplete=0,
	DataflowGenerationSequenceInvalidRequest=1,
	DataflowGenerationSequenceDecoderFailed=2
};

class DataflowGenerationSequence {
public:
	template<class Authority,class Service,class IO,class Timer>
	static DataflowGenerationSequenceResult Run(const unsigned* promptTokens,
		unsigned promptCount,unsigned maxNewTokens,unsigned eosToken,
		unsigned inputSlot,Authority& authority,Service& service,IO& io,
		unsigned* outputTokens,unsigned& outputCount,unsigned& stage,
		Timer& timer,uint64_t& promptTimeMicros,uint64_t& decodeTimeMicros)
	{
		outputCount=0;
		stage=DataflowGenerationSequenceInvalidRequest;
		promptTimeMicros=0;
		decodeTimeMicros=0;
		if (!promptTokens || !promptCount || !maxNewTokens || !outputTokens
			|| inputSlot>=ActivationBuffers || eosToken>Vocabulary
			|| promptCount>MaxPositions
			|| maxNewTokens>MaxPositions-promptCount+1)
			return DataflowGenerationSequenceInvalidRequest;
		for (unsigned i=0;i<promptCount;i++)
			if (promptTokens[i]>=Vocabulary)
				return DataflowGenerationSequenceInvalidRequest;

		const uint64_t promptStart=timer.NowMicros();
		unsigned nextToken=Vocabulary,decoderStage=0;
		for (unsigned position=0;position+1<promptCount;position++) {
			const DataflowDecoderSequenceResult result=
				DataflowDecoderSequence::Run(position,promptTokens[position],inputSlot,
					authority,service,io,nextToken,decoderStage,false);
			if (result!=DataflowDecoderSequenceComplete) {
				stage=(position<<16)|(decoderStage&0xffffU);
				promptTimeMicros=timer.NowMicros()-promptStart;
				return DataflowGenerationSequenceDecoderFailed;
			}
		}

		const unsigned lastPromptPosition=promptCount-1;
		DataflowDecoderSequenceResult result=DataflowDecoderSequence::Run(
			lastPromptPosition,promptTokens[lastPromptPosition],inputSlot,
			authority,service,io,nextToken,decoderStage);
		if (result!=DataflowDecoderSequenceComplete) {
			stage=(lastPromptPosition<<16)|(decoderStage&0xffffU);
			promptTimeMicros=timer.NowMicros()-promptStart;
			return DataflowGenerationSequenceDecoderFailed;
		}
		outputTokens[0]=nextToken;
		outputCount=1;
		promptTimeMicros=timer.NowMicros()-promptStart;
		if (nextToken==eosToken) {
			stage=DataflowGenerationSequenceComplete;
			return DataflowGenerationSequenceComplete;
		}
		if (maxNewTokens==1) {
			stage=DataflowGenerationSequenceComplete;
			return DataflowGenerationSequenceComplete;
		}

		const uint64_t decodeStart=timer.NowMicros();
		for (unsigned generated=1;generated<maxNewTokens;generated++) {
			const unsigned position=promptCount+generated-1;
			result=DataflowDecoderSequence::Run(position,
				outputTokens[generated-1],inputSlot,authority,service,io,
				nextToken,decoderStage);
			if (result!=DataflowDecoderSequenceComplete) {
				stage=(position<<16)|(decoderStage&0xffffU);
				decodeTimeMicros=timer.NowMicros()-decodeStart;
				return DataflowGenerationSequenceDecoderFailed;
			}
			outputTokens[generated]=nextToken;
			outputCount=generated+1;
			if (nextToken==eosToken) break;
		}
		decodeTimeMicros=timer.NowMicros()-decodeStart;
		stage=DataflowGenerationSequenceComplete;
		return DataflowGenerationSequenceComplete;
	}
};

} // namespace PioneerQwen35

#endif
