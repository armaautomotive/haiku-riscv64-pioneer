/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_GRAPH_WAIT_H
#define PIONEER_QWEN35_DATAFLOW_GRAPH_WAIT_H

#include "PioneerQwen35DataflowGraphRunProtocol.h"
#include "PioneerQwen35DataflowOwner.h"

namespace PioneerQwen35 {
enum : uint64_t {
	DataflowGraphWaitIdleMicros=30000000ULL,
	DataflowGraphWaitTotalMicros=600000000ULL,
	DataflowGraphWaitMaxPolls=8000000ULL
};

// Progress only extends the idle deadline. It is never completion evidence
// and cannot authorize acknowledgement, recovery or release of resources.
// start is shared across the entire fleet wait, not reset for each core.
template<class IO> bool DataflowGraphWaitForCompletion(IO& io,unsigned core,
	unsigned sequence,uint64_t start,DataflowWorkerSnapshot& snapshot)
{
	if (core>=DataflowCoreCount || !sequence) return false;
	uint64_t lastProgress=io.NowMicros();
	unsigned greatest=0;
	for (uint64_t poll=0;poll<DataflowGraphWaitMaxPolls;poll++) {
		const uint64_t now=io.NowMicros();
		if (now<start || now<lastProgress || now-start>=DataflowGraphWaitTotalMicros
			|| now-lastProgress>=DataflowGraphWaitIdleMicros) return false;
		DataflowWorkerSnapshot observed{};
		if (io.ReadWorkerSnapshot(core,observed)) {
			snapshot=observed;
			if (observed.commandSequence==sequence && observed.completedSequence==sequence
				&& (observed.state==DataflowWorkerComplete || observed.state==DataflowWorkerFailed))
				return true;
			if (observed.state==DataflowWorkerFatal || observed.state==DataflowWorkerStopped
				|| observed.commandSequence>sequence) return false;
			const unsigned detail=observed.failureDetail;
			const unsigned step=(detail>>4)&0x3ffU,phase=detail&0xfU;
			if (observed.commandSequence==sequence && observed.state==DataflowWorkerRunning
				&& observed.phase==1 && (detail&0xffffc000U)==0x40000000U
				&& step<DataflowGraphDecodeSteps && phase>=DataflowGraphProgressFetch
				&& phase<=DataflowGraphProgressResetQueues) {
				const unsigned ordinal=(step<<3)|phase;
				if (ordinal>greatest) { greatest=ordinal;lastProgress=now; }
			}
		}
		io.Pause();
	}
	return false;
}
}
#endif
