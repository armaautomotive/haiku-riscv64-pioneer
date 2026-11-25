/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_STREAM_RECORDER_H
#define PIONEER_QWEN35_DATAFLOW_STREAM_RECORDER_H

#include "PioneerQwen35DataflowDecoderSequence.h"
#include "PioneerQwen35DataflowStreamPlan.h"
#include "PioneerQwen35DataflowProjectionPair.h"
#include <new>

#ifdef PIONEER_QWEN35_DEVICE_DISPATCH
namespace PioneerQwen35 {

// Metadata-only speculative recorder. Each Begin/Complete advances only the
// private planning authority: it is not evidence of device execution. Sink
// must stage records and verify all page writes before sealing, retain
// uncertain writes, and prevent mutation once sealed. Its transaction
// is never permission to start hardware; the driver must bind the packed plan,
// protect the stream and prove fleet completion before updating live state.
template<class Sink> class DataflowStreamRecorder {
	DataflowProjectionAuthority& candidate;
	const DataflowStreamPlan& plan;
	Sink& sink;
	DataflowStreamRecord& record;
	unsigned steps=0;
	bool failed=false;
	template<class Builder,class Invocation,class Program>
	bool Emit(const Program& program) {
		if (failed || steps>=plan.count) return false;
		for (unsigned core=0;core<DataflowCoreCount;core++) {
			Invocation invocation{};
			if (!Builder::BindCore(program,core,invocation)
				|| !DataflowStreamEncode(invocation,plan.epoch,steps+1,record)
				|| !sink.StageRecord(plan.binding[core],plan.lease[core],steps,record)) {
				failed=true;return false;
			}
		}
		steps++;return true;
	}
	int Fail() { failed=true;return 1; }
public:
	DataflowStreamRecorder(DataflowProjectionAuthority& authority,const DataflowStreamPlan& stream,
		Sink& writer,DataflowStreamRecord& scratch):candidate(authority),plan(stream),sink(writer),record(scratch) {}
	unsigned Steps() const { return steps; }
	bool Failed() const { return failed; }
	template<class IO> int EmbedToken(const DataflowEmbeddingProgram& p,IO&) {
		return candidate.BeginEmbedding(p) && Emit<DataflowEmbeddingProgramBuilder,DataflowEmbeddingInvocation>(p)
			&& candidate.CompleteEmbedding(p)?0:Fail();
	}
	template<class IO> __attribute__((noinline)) int RunProjection(const DataflowProjectionProgram& p,IO&) {
		return candidate.Begin(p) && Emit<DataflowProgramBuilder,DataflowCoreInvocation>(p)
			&& candidate.Complete(p)?0:Fail();
	}
	// The caller must build both programs from this candidate before calling;
	// this path advances only speculative metadata until the full graph passes.
	template<class IO> __attribute__((noinline)) int RunIndependentProjectionPair(const DataflowProjectionProgram& a,
		const DataflowProjectionProgram& b,IO&) {
		if (failed || steps>=plan.count || !DataflowProjectionPair::Compatible(a,b)
			|| !candidate.Authorize(a) || !candidate.Authorize(b)
			|| !candidate.Begin(a) || !candidate.Complete(a)
			|| !candidate.Begin(b) || !candidate.Complete(b)) return Fail();
		// Kept off Haiku's small kernel stack; the model graph is recorded
		// once, and no allocation occurs in the resident token loop.
		auto* invocation=new(std::nothrow) DataflowCoreInvocation;
		if (!invocation) return Fail();
		for (unsigned core=0;core<DataflowCoreCount;core++) {
			if (!DataflowProjectionPair::BindCore(a,b,core,*invocation)
				|| !DataflowStreamEncode(*invocation,plan.epoch,steps+1,record)
				|| !sink.StageRecord(plan.binding[core],plan.lease[core],steps,record)) {
				delete invocation;return Fail();
			}
		}
		delete invocation;
		steps++;return 0;
	}
#ifdef PIONEER_QWEN35_FUSED_RECURRENCE
	template<class IO> __attribute__((noinline)) int RunIndependentProjectionTriple(const DataflowProjectionProgram& a,
		const DataflowProjectionProgram& b,const DataflowProjectionProgram& c,IO&) {
		if (failed || steps>=plan.count || !DataflowProjectionTriple::Compatible(a,b,c)
			|| !candidate.Authorize(a) || !candidate.Authorize(b) || !candidate.Authorize(c)
			|| !candidate.Begin(a) || !candidate.Complete(a)
			|| !candidate.Begin(b) || !candidate.Complete(b)
			|| !candidate.Begin(c) || !candidate.Complete(c)) return Fail();
		auto* invocation=new(std::nothrow) DataflowCoreInvocation;
		if (!invocation) return Fail();
		for (unsigned core=0;core<DataflowCoreCount;core++) {
			if (!DataflowProjectionTriple::BindCore(a,b,c,core,*invocation)
				|| !DataflowStreamEncode(*invocation,plan.epoch,steps+1,record)
				|| !sink.StageRecord(plan.binding[core],plan.lease[core],steps,record)) {
				delete invocation;return Fail();
			}
		}
		delete invocation;steps++;return 0;
	}
#endif
	template<class IO> int RunRms(const DataflowRmsProgram& p,IO&) {
		return candidate.BeginRms(p) && Emit<DataflowRmsProgramBuilder,DataflowRmsInvocation>(p)
			&& candidate.CompleteRms(p)?0:Fail();
	}
	template<class IO> int RunSilu(const DataflowSiluProgram& p,IO&) {
		return candidate.BeginSilu(p) && Emit<DataflowSiluProgramBuilder,DataflowSiluInvocation>(p)
			&& candidate.CompleteSilu(p)?0:Fail();
	}
	template<class IO> int RunResidual(const DataflowResidualProgram& p,IO&) {
		return candidate.BeginResidual(p) && Emit<DataflowResidualProgramBuilder,DataflowResidualInvocation>(p)
			&& candidate.CompleteResidual(p)?0:Fail();
	}
	template<class IO> int RunConv(const DataflowConvProgram& p,IO&) {
		return candidate.BeginConv(p) && Emit<DataflowConvProgramBuilder,DataflowConvInvocation>(p)
			&& candidate.CompleteConv(p)?0:Fail();
	}
	template<class IO> int RunDelta(const DataflowDeltaProgram& p,IO&) {
		return candidate.BeginDelta(p) && Emit<DataflowDeltaProgramBuilder,DataflowDeltaInvocation>(p)
			&& candidate.CompleteDelta(p)?0:Fail();
	}
	template<class IO> int RunDenseAttention(const DataflowDenseAttentionProgram& p,IO&) {
		return candidate.BeginDenseAttention(p) && Emit<DataflowDenseAttentionProgramBuilder,DataflowDenseAttentionInvocation>(p)
			&& candidate.CompleteDenseAttention(p)?0:Fail();
	}
	template<class IO> int RunArgmax(const DataflowArgmaxProgram& p,unsigned& token,IO&) {
		// The sequence only requires an in-range value for its metadata path.
		// This placeholder is never a model result or returned to the caller.
		token=0;
		return candidate.BeginArgmax(p) && Emit<DataflowArgmaxProgramBuilder,DataflowArgmaxInvocation>(p)
			&& candidate.CompleteArgmax(p,token)?0:Fail();
	}
};

class DataflowStreamRecording {
public:
	template<class PackedPlan,class Sink>
	static bool Build(const PackedPlan& packed,const DataflowProjectionAuthority& live,
		DataflowProjectionAuthority& candidate,const DataflowStreamPlan& plan,
		const unsigned (&routes)[2][Banks],unsigned position,unsigned token,unsigned inputSlot,
		bool selectNextToken,Sink& sink,DataflowStreamRecord& record,
		DataflowProjectionProgram& projectionScratch,unsigned& stage)
	{
		stage=DataflowDecoderSequenceInvalidRequest;
		const auto* manifest=live.Manifest();const auto* graph=live.GraphWorkspace();
		if (!manifest || !graph || live.Pending() || position>=manifest->context
			|| token>=Vocabulary || inputSlot>=ActivationBuffers || !live.PlanningRoutesMatch(routes)
			|| !DataflowStreamPlanBuilder::Validate(packed,*manifest,*graph,live.PlanningRope(),routes,plan)
			|| !live.CloneForGraphPlanning(candidate)) return false;
		if (!sink.Begin(plan)) { sink.Abort();return false; }
		DataflowStreamRecorder<Sink> recorder(candidate,plan,sink,record);
		unsigned ignoredToken=Vocabulary;
		const auto result=DataflowDecoderSequence::Run(position,token,inputSlot,candidate,
			recorder,sink,ignoredToken,stage,projectionScratch,selectNextToken);
		if (result!=DataflowDecoderSequenceComplete || recorder.Failed()
			|| recorder.Steps()!=plan.count || !sink.Seal(plan)) {
			sink.Abort();return false;
		}
		return true;
	}
};
}
#endif
#endif
