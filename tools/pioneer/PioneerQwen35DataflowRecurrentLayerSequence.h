/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_RECURRENT_LAYER_SEQUENCE_H
#define PIONEER_QWEN35_DATAFLOW_RECURRENT_LAYER_SEQUENCE_H

#include "PioneerQwen35DataflowFfnSequence.h"
#ifdef PIONEER_QWEN35_PAIRED_RECURRENCE
#include <new>
#endif

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
	DataflowRecurrentPairRunFailed=23,
	DataflowRecurrentTripleRunFailed=24,
	DataflowRecurrentFfnStageBase=32
};

static_assert(DataflowRecurrentFfnStageBase+DataflowFfnAddRunFailed<256,
	"recurrent layer stage fits its driver ABI field");

// Execute one complete recurrent decoder layer with checked resident-worker
// commands. This sequences attention norm, QKV/gate/Alpha/Beta Tensix
// projections, recurrent convolution and delta state, RecurrentOut, residual
// add, and the existing feed-forward sequence. It performs no tensor math.
class DataflowRecurrentLayerSequence {
#ifdef PIONEER_QWEN35_PAIRED_RECURRENCE
#ifdef PIONEER_QWEN35_FUSED_RECURRENCE
	template<class Authority,class Service,class IO>
	static __attribute__((noinline)) auto TryTriple(unsigned layer,unsigned normalizedSlot,Authority& authority,
		Service& service,IO& io,DataflowProjectionProgram& first,int)
		-> decltype(service.RunIndependentProjectionTriple(first,first,first,io),int())
	{
		auto* alpha=new(std::nothrow) DataflowProjectionProgram;
		auto* beta=new(std::nothrow) DataflowProjectionProgram;
		if (!alpha || !beta) { delete alpha;delete beta;return DataflowRecurrentTripleRunFailed; }
		int result=DataflowRecurrentLayerComplete;
		if (!authority.BuildProjection(layer,Alpha,normalizedSlot,DataflowGraphRecurrentAlphaOutputSlot,*alpha))
			result=DataflowRecurrentAlphaBuildFailed;
		else if (!authority.BuildProjection(layer,Beta,normalizedSlot,DataflowGraphRecurrentBetaOutputSlot,*beta))
			result=DataflowRecurrentBetaBuildFailed;
		else if (alpha->activeCores+beta->activeCores>=DataflowCoreCount
			|| !authority.BuildProjection(layer,RecurrentQkv,normalizedSlot,
				DataflowGraphRecurrentQkvOutputSlot,first,DataflowCoreCount-alpha->activeCores-beta->activeCores))
			result=DataflowRecurrentQkvBuildFailed;
		else if (service.RunIndependentProjectionTriple(first,*alpha,*beta,io)!=0)
			result=DataflowRecurrentTripleRunFailed;
		delete beta;delete alpha;return result;
	}
	template<class Authority,class Service,class IO>
	static int TryTriple(unsigned,unsigned,Authority&,Service&,IO&,DataflowProjectionProgram&,long)
		{ return -1; }
#endif
	template<class Authority,class Service,class IO>
	static __attribute__((noinline)) auto TryPair(unsigned layer,unsigned normalizedSlot,Authority& authority,
		Service& service,IO& io,DataflowProjectionProgram& first,int)
		-> decltype(service.RunIndependentProjectionPair(first,first,io),int())
	{
		auto* second=new(std::nothrow) DataflowProjectionProgram;
		if (!second) return DataflowRecurrentBetaBuildFailed;
		int result=DataflowRecurrentLayerComplete;
		if (!authority.BuildProjection(layer,Alpha,normalizedSlot,
			DataflowGraphRecurrentAlphaOutputSlot,first)) result=DataflowRecurrentAlphaBuildFailed;
		else if (!authority.BuildProjection(layer,Beta,normalizedSlot,
			DataflowGraphRecurrentBetaOutputSlot,*second)) result=DataflowRecurrentBetaBuildFailed;
		else if (service.RunIndependentProjectionPair(first,*second,io)!=0)
			result=DataflowRecurrentPairRunFailed;
		delete second;
		return result;
	}
	template<class Authority,class Service,class IO>
	static int TryPair(unsigned,unsigned,Authority&,Service&,IO&,DataflowProjectionProgram&,long)
		{ return -1; }
#endif
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
#ifndef PIONEER_QWEN35_FUSED_RMS_SAVE_COMPILE_ONLY
		if (service.RunResidual(residualSave,io)!=0)
			return static_cast<DataflowRecurrentLayerSequenceResult>(
				stage=DataflowRecurrentSaveRunFailed);
#endif

		DataflowRmsProgram norm{};
		if (!authority.BuildRms(layer,AttentionNorm,inputSlot,normalizedSlot,norm))
			return static_cast<DataflowRecurrentLayerSequenceResult>(
				stage=DataflowRecurrentNormBuildFailed);
#ifdef PIONEER_QWEN35_FUSED_RMS_SAVE_COMPILE_ONLY
		if (service.RunRmsSave(norm,residualSave,io)!=0)
#else
		if (service.RunRms(norm,io)!=0)
#endif
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
		unsigned projectionCount=4;
#ifdef PIONEER_QWEN35_PAIRED_RECURRENCE
		projectionCount=2;
#endif
#ifdef PIONEER_QWEN35_FUSED_RECURRENCE
		const int tripleResult=TryTriple(layer,normalizedSlot,authority,service,io,projectionScratch,0);
		const bool fused=tripleResult>=0;
		if (fused && tripleResult!=DataflowRecurrentLayerComplete)
			return static_cast<DataflowRecurrentLayerSequenceResult>(stage=unsigned(tripleResult));
#endif
		for (unsigned i=0;i<projectionCount;i++) {
#ifdef PIONEER_QWEN35_FUSED_RECURRENCE
			if (fused && i==0) continue;
#endif
			DataflowProjectionProgram& projection=projectionScratch;
			if (!authority.BuildProjection(layer,projectionRoles[i],normalizedSlot,
				projectionSlots[i],projection))
				return static_cast<DataflowRecurrentLayerSequenceResult>(
					stage=buildStages[i]);
			if (service.RunProjection(projection,io)!=0)
				return static_cast<DataflowRecurrentLayerSequenceResult>(
					stage=runStages[i]);
		}

#ifdef PIONEER_QWEN35_PAIRED_RECURRENCE
#ifdef PIONEER_QWEN35_FUSED_RECURRENCE
		if (!fused) {
#endif
		const int pairResult=TryPair(layer,normalizedSlot,authority,service,io,projectionScratch,0);
		if (pairResult<0) {
			for (unsigned i=2;i<4;i++) {
				if (!authority.BuildProjection(layer,projectionRoles[i],normalizedSlot,
					projectionSlots[i],projectionScratch))
					return static_cast<DataflowRecurrentLayerSequenceResult>(stage=buildStages[i]);
				if (service.RunProjection(projectionScratch,io)!=0)
					return static_cast<DataflowRecurrentLayerSequenceResult>(stage=runStages[i]);
			}
		} else if (pairResult!=DataflowRecurrentLayerComplete)
			return static_cast<DataflowRecurrentLayerSequenceResult>(stage=unsigned(pairResult));
#ifdef PIONEER_QWEN35_FUSED_RECURRENCE
		}
#endif
#endif

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
