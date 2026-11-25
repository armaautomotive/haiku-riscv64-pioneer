/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_GRAPH_METADATA_H
#define PIONEER_QWEN35_DATAFLOW_GRAPH_METADATA_H

#include "PioneerQwen35DataflowAuthorization.h"
#include "PioneerQwen35DataflowDecoderSequence.h"
#include "PioneerQwen35DataflowGraphRunProtocol.h"
#ifdef PIONEER_QWEN35_PAIRED_RECURRENCE
#include "PioneerQwen35DataflowProjectionPair.h"
#endif

#ifdef PIONEER_QWEN35_DEVICE_DISPATCH
namespace PioneerQwen35 {

// Predict a run's authority transitions without rebuilding its 120 per-core
// stream records. This is metadata only: the candidate is not a receipt and
// cannot be promoted to the live authority by this class.
class DataflowGraphMetadataService {
	DataflowProjectionAuthority& candidate;
	unsigned steps=0;
	bool failed=false;
	int Done(bool ok) { if (!ok) { failed=true;return 1; } steps++;return 0; }
public:
	explicit DataflowGraphMetadataService(DataflowProjectionAuthority& authority):candidate(authority) {}
	unsigned Steps() const { return steps; }
	bool Failed() const { return failed; }
	template<class IO> int EmbedToken(const DataflowEmbeddingProgram& p,IO&) {
		return Done(candidate.BeginEmbedding(p) && candidate.CompleteEmbedding(p));
	}
	template<class IO> int RunProjection(const DataflowProjectionProgram& p,IO&) {
		return Done(candidate.Begin(p) && candidate.Complete(p));
	}
#ifdef PIONEER_QWEN35_PAIRED_RECURRENCE
	template<class IO> int RunIndependentProjectionPair(const DataflowProjectionProgram& a,
		const DataflowProjectionProgram& b,IO&) {
		return Done(DataflowProjectionPair::Compatible(a,b) && candidate.Authorize(a)
			&& candidate.Authorize(b) && candidate.Begin(a) && candidate.Complete(a)
			&& candidate.Begin(b) && candidate.Complete(b));
	}
#ifdef PIONEER_QWEN35_FUSED_RECURRENCE
	template<class IO> int RunIndependentProjectionTriple(const DataflowProjectionProgram& a,
		const DataflowProjectionProgram& b,const DataflowProjectionProgram& c,IO&) {
		return Done(DataflowProjectionTriple::Compatible(a,b,c) && candidate.Authorize(a)
			&& candidate.Authorize(b) && candidate.Authorize(c)
			&& candidate.Begin(a) && candidate.Complete(a)
			&& candidate.Begin(b) && candidate.Complete(b)
			&& candidate.Begin(c) && candidate.Complete(c));
	}
#endif
#endif
	template<class IO> int RunRms(const DataflowRmsProgram& p,IO&) {
		return Done(candidate.BeginRms(p) && candidate.CompleteRms(p));
	}
	template<class IO> int RunSilu(const DataflowSiluProgram& p,IO&) {
		return Done(candidate.BeginSilu(p) && candidate.CompleteSilu(p));
	}
	template<class IO> int RunResidual(const DataflowResidualProgram& p,IO&) {
		return Done(candidate.BeginResidual(p) && candidate.CompleteResidual(p));
	}
	template<class IO> int RunConv(const DataflowConvProgram& p,IO&) {
		return Done(candidate.BeginConv(p) && candidate.CompleteConv(p));
	}
	template<class IO> int RunDelta(const DataflowDeltaProgram& p,IO&) {
		const int result=Done(candidate.BeginDelta(p) && candidate.CompleteDelta(p));
#ifdef PIONEER_QWEN35_DELTA_SHARD_COMPILE_ONLY
		if (!result) ++steps;
#endif
		return result;
	}
	template<class IO> int RunDenseAttention(const DataflowDenseAttentionProgram& p,IO&) {
		return Done(candidate.BeginDenseAttention(p) && candidate.CompleteDenseAttention(p));
	}
	template<class IO> int RunArgmax(const DataflowArgmaxProgram& p,unsigned& token,IO&) {
		token=0; // Only a metadata placeholder; never a generated result.
		return Done(candidate.BeginArgmax(p) && candidate.CompleteArgmax(p,token));
	}
};

class DataflowGraphMetadata {
public:
	static bool Prepare(const DataflowProjectionAuthority& live,DataflowProjectionAuthority& candidate,
		unsigned position,unsigned token,unsigned inputSlot,bool decode,
		DataflowProjectionProgram& scratch,unsigned& stage)
	{
		stage=DataflowDecoderSequenceInvalidRequest;
		const auto* manifest=live.Manifest();
		if (!manifest || live.Pending() || position>=manifest->context || token>=Vocabulary
			|| inputSlot>=ActivationBuffers || !live.CloneForGraphPlanning(candidate)) return false;
		DataflowGraphMetadataService service(candidate);
		struct NoIO {} noIO;
		unsigned ignored=Vocabulary;
		return DataflowDecoderSequence::Run(position,token,inputSlot,candidate,service,noIO,
			ignored,stage,scratch,decode)==DataflowDecoderSequenceComplete
			&& !service.Failed() && service.Steps()==(decode?DataflowGraphDecodeSteps:DataflowGraphPromptSteps);
	}
};
}
#endif
#endif
