/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_RECURRENT_LAYER_SEQUENCE_H
#define PIONEER_QWEN35_DATAFLOW_RECURRENT_LAYER_SEQUENCE_H

#include "PioneerQwen35DataflowFfnSequence.h"

namespace PioneerQwen35 {

enum DataflowRecurrentLayerSequenceResult {
	DataflowRecurrentLayerComplete=0,
	DataflowRecurrentLayerInvalidRequest=1,
	DataflowRecurrentSaveBuildFailed=2,
	DataflowRecurrentSaveRunFailed=3,
	DataflowRecurrentNormBuildFailed=4,
	DataflowRecurrentNormRunFailed=5,
	DataflowRecurrentQkvBuildFailed=6,
	DataflowRecurrentQkvRunFailed=7,
	DataflowRecurrentGateBuildFailed=8,
	DataflowRecurrentGateRunFailed=9,
	DataflowRecurrentAlphaBuildFailed=10,
	DataflowRecurrentAlphaRunFailed=11,
	DataflowRecurrentBetaBuildFailed=12,
	DataflowRecurrentBetaRunFailed=13,
	DataflowRecurrentConvBuildFailed=14,
	DataflowRecurrentConvRunFailed=15,
	DataflowRecurrentDeltaBuildFailed=16,
	DataflowRecurrentDeltaRunFailed=17,
	DataflowRecurrentOutBuildFailed=18,
	DataflowRecurrentOutRunFailed=19,
	DataflowRecurrentAddBuildFailed=20,
	DataflowRecurrentAddRunFailed=21,
	DataflowRecurrentFfnFailed=22,
	DataflowRecurrentFfnStageBase=32
};

static_assert(DataflowRecurrentFfnStageBase+DataflowFfnAddRunFailed<256,
	"recurrent layer stage fits its driver ABI field");

// Execute one complete recurrent decoder layer with checked resident-worker
// commands. This sequences attention norm, QKV/gate/Alpha/Beta Tensix
// projections, recurrent convolution and delta state, RecurrentOut, residual
// add, and the existing feed-forward sequence. It performs no tensor math.
class DataflowRecurrentLayerSequence {
public:
	template<class Authority,class Service,class IO>
	static DataflowRecurrentLayerSequenceResult Run(unsigned layer,
		unsigned position,unsigned inputSlot,Authority& authority,
		Service& service,IO& io,unsigned& outputSlot,unsigned& stage,
		DataflowProjectionProgram& projectionScratch)
	{
		outputSlot=ActivationBuffers;
		stage=DataflowRecurrentLayerInvalidRequest;
		if (layer>=Layers || Dense(layer) || position>=MaxPositions
			|| inputSlot>=ActivationBuffers)
			return DataflowRecurrentLayerInvalidRequest;
		const unsigned normalizedSlot=inputSlot^1U;

		DataflowResidualProgram residualSave{};
		if (!authority.BuildResidual(DataflowResidualSave,inputSlot,residualSave))
			return static_cast<DataflowRecurrentLayerSequenceResult>(
				stage=DataflowRecurrentSaveBuildFailed);
		if (service.RunResidual(residualSave,io)!=0)
			return static_cast<DataflowRecurrentLayerSequenceResult>(
				stage=DataflowRecurrentSaveRunFailed);

		DataflowRmsProgram norm{};
		if (!authority.BuildRms(layer,AttentionNorm,inputSlot,normalizedSlot,norm))
			return static_cast<DataflowRecurrentLayerSequenceResult>(
				stage=DataflowRecurrentNormBuildFailed);
		if (service.RunRms(norm,io)!=0)
			return static_cast<DataflowRecurrentLayerSequenceResult>(
				stage=DataflowRecurrentNormRunFailed);

		const unsigned projectionRoles[4]={RecurrentQkv,RecurrentGate,Alpha,Beta};
		const unsigned projectionSlots[4]={DataflowGraphRecurrentQkvOutputSlot,
			DataflowGraphRecurrentGateOutputSlot,
			DataflowGraphRecurrentAlphaOutputSlot,
			DataflowGraphRecurrentBetaOutputSlot};
		const unsigned buildStages[4]={DataflowRecurrentQkvBuildFailed,
			DataflowRecurrentGateBuildFailed,DataflowRecurrentAlphaBuildFailed,
			DataflowRecurrentBetaBuildFailed};
		const unsigned runStages[4]={DataflowRecurrentQkvRunFailed,
			DataflowRecurrentGateRunFailed,DataflowRecurrentAlphaRunFailed,
			DataflowRecurrentBetaRunFailed};
		for (unsigned i=0;i<4;i++) {
			DataflowProjectionProgram& projection=projectionScratch;
			if (!authority.BuildProjection(layer,projectionRoles[i],normalizedSlot,
				projectionSlots[i],projection))
				return static_cast<DataflowRecurrentLayerSequenceResult>(
					stage=buildStages[i]);
			if (service.RunProjection(projection,io)!=0)
				return static_cast<DataflowRecurrentLayerSequenceResult>(
					stage=runStages[i]);
		}

		DataflowConvProgram conv{};
		if (!authority.BuildConv(layer,position,conv))
			return static_cast<DataflowRecurrentLayerSequenceResult>(
				stage=DataflowRecurrentConvBuildFailed);
		if (service.RunConv(conv,io)!=0)
			return static_cast<DataflowRecurrentLayerSequenceResult>(
				stage=DataflowRecurrentConvRunFailed);

		DataflowDeltaProgram delta{};
		if (!authority.BuildDelta(layer,position,delta))
			return static_cast<DataflowRecurrentLayerSequenceResult>(
				stage=DataflowRecurrentDeltaBuildFailed);
		if (service.RunDelta(delta,io)!=0)
			return static_cast<DataflowRecurrentLayerSequenceResult>(
				stage=DataflowRecurrentDeltaRunFailed);

		DataflowProjectionProgram& recurrentOut=projectionScratch;
		if (!authority.BuildRecurrentOutput(layer,normalizedSlot,recurrentOut))
			return static_cast<DataflowRecurrentLayerSequenceResult>(
				stage=DataflowRecurrentOutBuildFailed);
		if (service.RunProjection(recurrentOut,io)!=0)
			return static_cast<DataflowRecurrentLayerSequenceResult>(
				stage=DataflowRecurrentOutRunFailed);

		DataflowResidualProgram residualAdd{};
		if (!authority.BuildResidual(DataflowResidualAdd,normalizedSlot,residualAdd))
			return static_cast<DataflowRecurrentLayerSequenceResult>(
				stage=DataflowRecurrentAddBuildFailed);
		if (service.RunResidual(residualAdd,io)!=0)
			return static_cast<DataflowRecurrentLayerSequenceResult>(
				stage=DataflowRecurrentAddRunFailed);

		unsigned ffnOutput=ActivationBuffers;
		const DataflowFfnSequenceResult ffn=DataflowFfnSequence::Run(layer,
			normalizedSlot,authority,service,io,ffnOutput,projectionScratch);
		if (ffn!=DataflowFfnSequenceComplete)
			return static_cast<DataflowRecurrentLayerSequenceResult>(
				stage=DataflowRecurrentFfnStageBase+unsigned(ffn));
		outputSlot=ffnOutput;
		stage=DataflowRecurrentLayerComplete;
		return DataflowRecurrentLayerComplete;
	}
};

} // namespace PioneerQwen35

#endif
