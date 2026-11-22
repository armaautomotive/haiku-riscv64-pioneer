/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_DEVICE_GRAPH_H
#define PIONEER_QWEN35_DATAFLOW_DEVICE_GRAPH_H

#include "PioneerQwen35DataflowDeviceStream.h"
#include "PioneerQwen35DataflowDeviceBarrier.h"

namespace PioneerQwen35 {

enum DataflowDeviceGraphState { DataflowGraphInvalid=0, DataflowGraphReady=1,
	DataflowGraphWaiting=2, DataflowGraphLocalComplete=3, DataflowGraphRetain=4 };
enum DataflowDeviceGraphFault { DataflowGraphNoFault=0, DataflowGraphFetchFault=1,
	DataflowGraphAuthorityFault=2, DataflowGraphExecutionUncertain=3,
	DataflowGraphBarrierFault=4, DataflowGraphDeadlineFault=5,
	DataflowGraphWorkerFault=6, DataflowGraphQueueFault=7 };

// Per-worker control loop. A local completion is not a fleet-completion proof:
// the host must observe and audit every worker before retiring the graph.
// IO::Authorize must establish authority over the exact fetched invocation,
// not merely validate its checksum or type. It must also prove the stream
// remains immutable. No implementation of that authority is supplied here.
// IO::Execute may report an ordinary worker failure via status/detail only
// when its transaction state is known; false means uncertainty and immediate
// retention. All faults, including an ordinary worker failure, retain the
// entire graph. A successful release can precede a different peer's failure.
template<class IO,class BarrierIO> class DataflowDeviceGraph {
	IO& io;
	DataflowDeviceBarrier<BarrierIO> barrier;
	DataflowDeviceGraphState state=DataflowGraphInvalid;
	DataflowDeviceGraphFault fault=DataflowGraphNoFault;
	unsigned core=0,epoch=0,count=0,index=0,remaining=0,pollBudget=0;
	unsigned failureCore=DataflowBarrierWorkers,failureStatus=0;
	DataflowDeviceGraphState Retain(DataflowDeviceGraphFault why) {
		fault=why;state=DataflowGraphRetain;return state;
	}
public:
	DataflowDeviceGraph(IO& execution,BarrierIO& transport):io(execution),barrier(transport) {}
	DataflowDeviceGraph(const DataflowDeviceGraph&)=delete;
	DataflowDeviceGraph& operator=(const DataflowDeviceGraph&)=delete;
	bool Start(const DataflowStreamBinding& binding,unsigned worker,unsigned maxPolls) {
		if (state!=DataflowGraphInvalid || !binding.Valid() || worker!=binding.core || !maxPolls)
			return false;
		core=worker;epoch=binding.epoch;count=binding.count;pollBudget=maxPolls;
		state=DataflowGraphReady;return true;
	}
	DataflowDeviceGraphState Tick() {
		if (state==DataflowGraphReady) {
			if (!io.Fetch(index)) return Retain(DataflowGraphFetchFault);
			if (!io.Authorize(index)) return Retain(DataflowGraphAuthorityFault);
			unsigned status=0,detail=0;
			if (!io.Execute(index+1,status,detail)) return Retain(DataflowGraphExecutionUncertain);
			if (!status && detail) return Retain(DataflowGraphExecutionUncertain);
			if (!barrier.Start(core,epoch,index+1,status,detail)) return Retain(DataflowGraphBarrierFault);
			remaining=pollBudget;state=DataflowGraphWaiting;
		}
		if (state!=DataflowGraphWaiting) return state;
		const auto result=barrier.Poll();
		if (result==DataflowBarrierPending) {
			if (--remaining) return state;
			barrier.Expire();return Retain(DataflowGraphDeadlineFault);
		}
		if (result==DataflowBarrierUnsafe) return Retain(DataflowGraphBarrierFault);
		if (result==DataflowBarrierFailed) {
			failureCore=barrier.FailureCore();failureStatus=barrier.Failure();
			return Retain(DataflowGraphWorkerFault);
		}
		if (result!=DataflowBarrierPassed) return Retain(DataflowGraphBarrierFault);
		index++;
		if (index==count) { state=DataflowGraphLocalComplete;return state; }
		if (!io.ResetQueues() || !barrier.NextStep()) return Retain(DataflowGraphQueueFault);
		state=DataflowGraphReady;return state;
	}
	DataflowDeviceGraphState State() const { return state; }
	DataflowDeviceGraphFault Fault() const { return fault; }
	unsigned CompletedSteps() const { return index; }
	unsigned FailureCore() const { return failureCore; }
	unsigned FailureStatus() const { return failureStatus; }
};
}
#endif
