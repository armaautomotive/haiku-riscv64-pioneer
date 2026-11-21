/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_DECODER_SEQUENCE_H
#define PIONEER_QWEN35_DATAFLOW_DECODER_SEQUENCE_H

#include "PioneerQwen35DataflowDenseLayerSequence.h"
#include "PioneerQwen35DataflowArgmaxProgram.h"

namespace PioneerQwen35 {

enum DataflowDecoderSequenceResult {
	DataflowDecoderSequenceComplete=0,
	DataflowDecoderSequenceInvalidRequest=1,
	DataflowDecoderResetFailed=2,
	DataflowDecoderEmbeddingBuildFailed=3,
	DataflowDecoderEmbeddingRunFailed=4,
	DataflowDecoderLayerFailed=5,
	DataflowDecoderFinalNormBuildFailed=6,
	DataflowDecoderFinalNormRunFailed=7,
	DataflowDecoderOutputBuildFailed=8,
	DataflowDecoderOutputRunFailed=9,
	DataflowDecoderArgmaxBuildFailed=10,
	DataflowDecoderArgmaxRunFailed=11
};

// Execute one token position across the complete mixed Qwen3.5 decoder stack.
// Each layer sequence delegates tensor work to the resident all-core service.
// This coordinator only selects the layer type and preserves the first stage
// failure; it performs no model arithmetic.
class DataflowDecoderSequence {
public:
	template<class Authority,class Service,class IO>
	static DataflowDecoderSequenceResult Run(unsigned position,unsigned token,
		unsigned inputSlot,Authority& authority,Service& service,IO& io,
		unsigned& outputToken,unsigned& stage,bool selectNextToken=true)
	{
		outputToken=Vocabulary;
		stage=DataflowDecoderSequenceInvalidRequest;
		if (position>=MaxPositions || token>=Vocabulary
			|| inputSlot>=ActivationBuffers) return DataflowDecoderSequenceInvalidRequest;
		if (position==0 && !authority.ResetRecurrentState())
			return static_cast<DataflowDecoderSequenceResult>(
				stage=DataflowDecoderResetFailed);

		DataflowEmbeddingProgram embedding{};
		if (!authority.BuildEmbedding(token,inputSlot,embedding))
			return static_cast<DataflowDecoderSequenceResult>(
				stage=DataflowDecoderEmbeddingBuildFailed);
		if (service.EmbedToken(embedding,io)!=0)
			return static_cast<DataflowDecoderSequenceResult>(
				stage=DataflowDecoderEmbeddingRunFailed);

		unsigned currentSlot=inputSlot;
		for (unsigned layer=0;layer<Layers;layer++) {
			unsigned nextSlot=ActivationBuffers,layerStage=0;
			if (Dense(layer)) {
				const DataflowDenseLayerSequenceResult result=
					DataflowDenseLayerSequence::Run(layer,position,currentSlot,
						authority,service,io,nextSlot,layerStage);
				if (result!=DataflowDenseLayerComplete) {
					stage=(layer<<8)|(layerStage&0xffU);
					return DataflowDecoderLayerFailed;
				}
			} else {
				const DataflowRecurrentLayerSequenceResult result=
					DataflowRecurrentLayerSequence::Run(layer,position,currentSlot,
						authority,service,io,nextSlot,layerStage);
				if (result!=DataflowRecurrentLayerComplete) {
					stage=(layer<<8)|(layerStage&0xffU);
					return DataflowDecoderLayerFailed;
				}
			}
			if (nextSlot>=ActivationBuffers) {
				stage=(layer<<8)|0xffU;
				return DataflowDecoderLayerFailed;
			}
			currentSlot=nextSlot;
		}
		if (!selectNextToken) {
			stage=DataflowDecoderSequenceComplete;
			return DataflowDecoderSequenceComplete;
		}

		DataflowRmsProgram finalNorm{};
		const unsigned normalizedSlot=currentSlot^1U;
		if (!authority.BuildRms(Layers,FinalNorm,currentSlot,normalizedSlot,
			finalNorm))
			return static_cast<DataflowDecoderSequenceResult>(
				stage=DataflowDecoderFinalNormBuildFailed);
		if (service.RunRms(finalNorm,io)!=0)
			return static_cast<DataflowDecoderSequenceResult>(
				stage=DataflowDecoderFinalNormRunFailed);

		DataflowProjectionProgram tiedOutput{};
		if (!authority.BuildProjection(Layers,TiedOutput,normalizedSlot,
			DataflowGraphVocabularyOutputSlot,tiedOutput))
			return static_cast<DataflowDecoderSequenceResult>(
				stage=DataflowDecoderOutputBuildFailed);
		if (service.RunProjection(tiedOutput,io)!=0)
			return static_cast<DataflowDecoderSequenceResult>(
				stage=DataflowDecoderOutputRunFailed);

		DataflowArgmaxProgram argmax;
		DataflowArgmaxZeroWords(argmax);
		if (!authority.BuildArgmax(argmax))
			return static_cast<DataflowDecoderSequenceResult>(
				stage=DataflowDecoderArgmaxBuildFailed);
		if (service.RunArgmax(argmax,outputToken,io)!=0) {
			outputToken=Vocabulary;
			return static_cast<DataflowDecoderSequenceResult>(
				stage=DataflowDecoderArgmaxRunFailed);
		}
		if (outputToken>=Vocabulary) {
			outputToken=Vocabulary;
			return static_cast<DataflowDecoderSequenceResult>(
				stage=DataflowDecoderArgmaxRunFailed);
		}

		stage=DataflowDecoderSequenceComplete;
		return DataflowDecoderSequenceComplete;
	}
};

} // namespace PioneerQwen35

#endif
