/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_DENSE_LAYER_SEQUENCE_H
#define PIONEER_QWEN35_DATAFLOW_DENSE_LAYER_SEQUENCE_H

#include "PioneerQwen35DataflowRecurrentLayerSequence.h"

namespace PioneerQwen35 {

enum DataflowDenseLayerSequenceResult {
	DataflowDenseLayerComplete=0,
	DataflowDenseLayerInvalidRequest=1,
	DataflowDenseSaveBuildFailed=2,
	DataflowDenseSaveRunFailed=3,	DataflowDenseNormBuildFailed=4,
	DataflowDenseNormRunFailed=5,	DataflowDenseQueryBuildFailed=6,
	DataflowDenseQueryRunFailed=7,	DataflowDenseKeyBuildFailed=8,
	DataflowDenseKeyRunFailed=9,	DataflowDensePackKeyBuildFailed=10,
	DataflowDensePackKeyRunFailed=11,	DataflowDenseValueBuildFailed=12,
	DataflowDenseValueRunFailed=13,	DataflowDensePackValueBuildFailed=14,
	DataflowDensePackValueRunFailed=15,	DataflowDenseAttentionBuildFailed=16,
	DataflowDenseAttentionRunFailed=17,	DataflowDenseOutBuildFailed=18,
	DataflowDenseOutRunFailed=19,	DataflowDenseAddBuildFailed=20,
	DataflowDenseAddRunFailed=21,	DataflowDenseFfnFailed=22,
	DataflowDenseFfnStageBase=32
};

static_assert(DataflowDenseFfnStageBase+DataflowFfnAddRunFailed<256,
	"dense layer stage fits its driver ABI field");

// Compose the dense decoder layer from resident all-core Tensix projections
// and resident BRISC attention workers. Projection math stays on Tensix;
// workers only normalize/rotate heads, append the KV cache, and compute
// grouped-query online attention.
class DataflowDenseLayerSequence {
public:
	template<class Authority,class Service,class IO>
	static DataflowDenseLayerSequenceResult Run(unsigned layer,unsigned position,
		unsigned inputSlot,Authority& authority,Service& service,IO& io,
		unsigned& outputSlot,unsigned& stage)
	{
		outputSlot=ActivationBuffers;
		stage=DataflowDenseLayerInvalidRequest;
		if (layer>=Layers || !Dense(layer) || position>=MaxPositions
			|| inputSlot>=ActivationBuffers)
			return DataflowDenseLayerInvalidRequest;
		const unsigned normalizedSlot=inputSlot^1U;

		DataflowResidualProgram residualSave{};
		if (!authority.BuildResidual(DataflowResidualSave,inputSlot,residualSave))
			return static_cast<DataflowDenseLayerSequenceResult>(
				stage=DataflowDenseSaveBuildFailed);
		if (service.RunResidual(residualSave,io)!=0)
			return static_cast<DataflowDenseLayerSequenceResult>(
				stage=DataflowDenseSaveRunFailed);

		DataflowRmsProgram norm{};
		if (!authority.BuildRms(layer,AttentionNorm,inputSlot,normalizedSlot,norm))
			return static_cast<DataflowDenseLayerSequenceResult>(
				stage=DataflowDenseNormBuildFailed);
		if (service.RunRms(norm,io)!=0)
			return static_cast<DataflowDenseLayerSequenceResult>(
				stage=DataflowDenseNormRunFailed);

		const unsigned roles[3]={QueryAndGate,Key,Value};
		const unsigned buildStages[3]={DataflowDenseQueryBuildFailed,
			DataflowDenseKeyBuildFailed,DataflowDenseValueBuildFailed};
		const unsigned runStages[3]={DataflowDenseQueryRunFailed,
			DataflowDenseKeyRunFailed,DataflowDenseValueRunFailed};
		for (unsigned index=0;index<3;index++) {
			DataflowProjectionProgram projection{};
			if (!authority.BuildDenseProjection(layer,roles[index],normalizedSlot,
				projection))
				return static_cast<DataflowDenseLayerSequenceResult>(
					stage=buildStages[index]);
			if (service.RunProjection(projection,io)!=0)
				return static_cast<DataflowDenseLayerSequenceResult>(
					stage=runStages[index]);
			if (index==1 || index==2) {
				const unsigned mode=index==1?DataflowDenseAttentionPackKey
					:DataflowDenseAttentionPackValue;
				DataflowDenseAttentionProgram pack{};
				if (!authority.BuildDenseAttention(layer,position,mode,pack))
					return static_cast<DataflowDenseLayerSequenceResult>(
						stage=index==1?DataflowDensePackKeyBuildFailed
							:DataflowDensePackValueBuildFailed);
				if (service.RunDenseAttention(pack,io)!=0)
					return static_cast<DataflowDenseLayerSequenceResult>(
						stage=index==1?DataflowDensePackKeyRunFailed
							:DataflowDensePackValueRunFailed);
			}
		}

		DataflowDenseAttentionProgram attention{};
		if (!authority.BuildDenseAttention(layer,position,
			DataflowDenseAttentionRun,attention))
			return static_cast<DataflowDenseLayerSequenceResult>(
				stage=DataflowDenseAttentionBuildFailed);
		if (service.RunDenseAttention(attention,io)!=0)
			return static_cast<DataflowDenseLayerSequenceResult>(
				stage=DataflowDenseAttentionRunFailed);

		DataflowProjectionProgram attentionOut{};
		if (!authority.BuildDenseAttentionOutput(layer,normalizedSlot,attentionOut))
			return static_cast<DataflowDenseLayerSequenceResult>(
				stage=DataflowDenseOutBuildFailed);
		if (service.RunProjection(attentionOut,io)!=0)
			return static_cast<DataflowDenseLayerSequenceResult>(
				stage=DataflowDenseOutRunFailed);

		DataflowResidualProgram residualAdd{};
		if (!authority.BuildResidual(DataflowResidualAdd,normalizedSlot,residualAdd))
			return static_cast<DataflowDenseLayerSequenceResult>(
				stage=DataflowDenseAddBuildFailed);
		if (service.RunResidual(residualAdd,io)!=0)
			return static_cast<DataflowDenseLayerSequenceResult>(
				stage=DataflowDenseAddRunFailed);

		unsigned ffnOutput=ActivationBuffers;
		const DataflowFfnSequenceResult ffn=DataflowFfnSequence::Run(layer,
			normalizedSlot,authority,service,io,ffnOutput);
		if (ffn!=DataflowFfnSequenceComplete)
			return static_cast<DataflowDenseLayerSequenceResult>(
				stage=DataflowDenseFfnStageBase+unsigned(ffn));
		outputSlot=ffnOutput;
		stage=DataflowDenseLayerComplete;
		return DataflowDenseLayerComplete;
	}
};

} // namespace PioneerQwen35

#endif
