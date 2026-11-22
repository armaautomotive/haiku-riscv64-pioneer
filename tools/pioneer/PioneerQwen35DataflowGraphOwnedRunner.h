/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_GRAPH_OWNED_RUNNER_H
#define PIONEER_QWEN35_DATAFLOW_GRAPH_OWNED_RUNNER_H

#include "PioneerQwen35DataflowGraphSession.h"

#ifdef PIONEER_QWEN35_DEVICE_DISPATCH
namespace PioneerQwen35 {

// Manages per-run heap objects under the session lock. IO allocates from the
// owning context; successful promotion transfers the candidate to the live
// authority slot. All uncertain work stays retained until IO proves the entire
// fleet restored. The owner itself must outlive any retained run.
class DataflowGraphOwnedRunner {
	DataflowGraphSessionRun* retainedRun=nullptr;
	DataflowProjectionAuthority* retainedCandidate=nullptr;
	DataflowProjectionAuthority* retainedRetired=nullptr;
public:
	DataflowGraphOwnedRunner()=default;
	DataflowGraphOwnedRunner(const DataflowGraphOwnedRunner&)=delete;
	DataflowGraphOwnedRunner& operator=(const DataflowGraphOwnedRunner&)=delete;
	bool Retained() const { return retainedRun!=nullptr; }
	template<class IO> DataflowGraphSessionResult Run(DataflowGraphSession& session,
		DataflowProjectionAuthority*& live,unsigned position,unsigned token,bool decode,unsigned polls,
		DataflowProjectionProgram& scratch,IO& io,DataflowArgmaxResult& output,unsigned& stage) {
		output={Vocabulary,0,0,0};stage=DataflowDecoderSequenceInvalidRequest;
		if (Retained() || !live) return DataflowGraphSessionInvalid;
		auto* run=io.NewGraphRun();
		if (!run) return DataflowGraphSessionPreflight;
		auto* candidate=io.NewGraphCandidate();
		if (!candidate) { io.DeleteGraphRun(run);return DataflowGraphSessionPreflight; }
		if (candidate==live) { io.DeleteGraphRun(run);return DataflowGraphSessionInvalid; }
		DataflowProjectionAuthority* retired=nullptr;
		const auto result=session.Run(*run,*candidate,position,token,decode,polls,scratch,io,output,retired,stage);
		if (result==DataflowGraphSessionComplete) {
			// Session has audited all workers and committed candidate to live.
			io.DeleteGraphRun(run);io.DeleteGraphCandidate(retired);return result;
		}
		if (result==DataflowGraphSessionRetained) {
			retainedRun=run;retainedCandidate=candidate;retainedRetired=retired;
			output={Vocabulary,0,0,0};return result;
		}
		io.DeleteGraphRun(run);io.DeleteGraphCandidate(candidate);
		output={Vocabulary,0,0,0};return result;
	}
	// IO must prove restored fleet and mappings under the same exclusive lock;
	// a ready mailbox or a caller assertion alone is not sufficient.
	template<class IO> bool ReleaseRetainedAfterRestoration(DataflowProjectionAuthority* live,IO& io) {
		if (!retainedRun || !io.GraphFleetRestored()) return false;
		io.DeleteGraphRun(retainedRun);
		if (retainedRetired && retainedRetired!=live && retainedRetired!=retainedCandidate)
			io.DeleteGraphCandidate(retainedRetired);
		if (retainedCandidate && retainedCandidate!=live) io.DeleteGraphCandidate(retainedCandidate);
		retainedRun=nullptr;retainedCandidate=nullptr;retainedRetired=nullptr;return true;
	}
};
}
#endif
#endif
