/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_GRAPH_FAULT_TEARDOWN_H
#define PIONEER_QWEN35_DATAFLOW_GRAPH_FAULT_TEARDOWN_H

#include "PioneerQwen35DataflowGraphOwnedRunner.h"

#ifdef PIONEER_QWEN35_DEVICE_DISPATCH
namespace PioneerQwen35 {

// Called under the owning driver's session lock. The backend must establish
// that this exact graph is retained, audit real hardware, close the service,
// and prove restoration before it can destroy the run and candidate. No
// successful result makes the faulted session or its model usable again.
class DataflowGraphFaultTeardown {
public:
	template<class IO> static DataflowServiceResult Run(DataflowGraphSession& session,
		DataflowGraphOwnedRunner& runner,DataflowGraphFaultAudit& audit,
		DataflowProjectionAuthority* live,unsigned polls,IO& io) {
		if (!live || !runner.Retained() || !session.Faulted()) return DataflowServiceInvalidState;
		if (session.SettleFaultForClose(audit,polls,io)!=DataflowGraphFaultAuditSettled)
			return DataflowServiceRecoveryPending;
		const auto closed=io.CloseFaultedGraphService();
		if (closed!=DataflowServiceClosed) return closed;
		struct Release {
			IO& io;DataflowGraphSession& session;DataflowGraphFaultAudit& audit;
			bool GraphFleetRestored() { return io.GraphFleetRestoredAfterClose(session,audit); }
			void DeleteGraphRun(DataflowGraphSessionRun* p) { io.DeleteGraphRun(p); }
			void DeleteGraphCandidate(DataflowProjectionAuthority* p) { io.DeleteGraphCandidate(p); }
		} release{io,session,audit};
		return runner.ReleaseRetainedAfterRestoration(live,release)
			?DataflowServiceClosed:DataflowServiceRecoveryPending;
	}
};
}
#endif
#endif
