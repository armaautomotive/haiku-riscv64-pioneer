/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_GRAPH_TRACE_H
#define PIONEER_QWEN35_DATAFLOW_GRAPH_TRACE_H

#include <stdint.h>
#include "PioneerQwen35DataflowGraphRunProtocol.h"
#include "PioneerQwen35DataflowArgmaxProgram.h"

namespace PioneerQwen35 {
enum : unsigned {
	DataflowGraphTraceAddress=0xab340,
	DataflowGraphTraceSlots=28,
	DataflowGraphProfileTraceFirst=15,
	DataflowGraphProjectionTraceFirst=8,
	DataflowGraphTracePending=0xffffffffU
};
struct DataflowGraphTraceRecord {
	uint32_t sequence,first,last,magnitudeOr;
};
static_assert(sizeof(DataflowGraphTraceRecord)==16,"one bounded trace sample");
static_assert(DataflowGraphTraceAddress>=DataflowGraphOperationProgressAddress+4
	&& DataflowGraphTraceAddress+DataflowGraphTraceSlots*sizeof(DataflowGraphTraceRecord)
		<=DataflowBarrierArrivalBase,"trace is in saved L1, disjoint from commands and barriers");

inline unsigned DataflowGraphTraceStep(unsigned slot) {
	const unsigned steps[DataflowGraphProfileTraceFirst]={0,2,3,4,7,9,
		DataflowGraphPromptSteps,DataflowGraphPromptSteps+1,3,3,3,3,3,3,3};
	if (slot<DataflowGraphProfileTraceFirst) return steps[slot];
	return slot<DataflowGraphTraceSlots?DataflowGraphDecodeSteps-1:DataflowGraphDecodeSteps;
}

// Captures raw device bits only; the samples are diagnostic and are never
// used as model inputs, outputs, or resource-release evidence.
inline DataflowGraphTraceRecord DataflowGraphTraceSample(unsigned sequence,
	const volatile uint32_t* page) {
	DataflowGraphTraceRecord result{sequence,0,0,0};
	if (!sequence || !page) return {};
	for (unsigned i=0;i<16;i++) {
		const uint32_t value=page[i];
		if (!i) result.first=value;
		if (i==15) result.last=value;
		result.magnitudeOr|=value&0x7fffffffU;
	}
	return result;
}
// A projection sample becomes visible only when its graph step has completed.
// Pending records cannot accidentally pass the sequence validation.
inline void DataflowGraphTracePendingSample(volatile uint32_t* record,
	const volatile uint32_t* page) {
	const auto sample=DataflowGraphTraceSample(1,page);
	DataflowStoreRelease(record[0],0);
	DataflowStoreRelease(record[1],sample.first);
	DataflowStoreRelease(record[2],sample.last);
	DataflowStoreRelease(record[3],sample.magnitudeOr);
	DataflowStoreRelease(record[0],DataflowGraphTracePending);
}
inline void DataflowGraphTracePublishPending(volatile uint32_t* record,
	unsigned sequence) {
	if (sequence && DataflowLoadAcquire(record[0])==DataflowGraphTracePending)
		DataflowStoreRelease(record[0],sequence);
}
inline bool DataflowGraphTraceValid(const DataflowGraphTraceRecord& record,
	unsigned slot,unsigned lastSequence) {
	if (slot>=DataflowGraphTraceSlots) return false;
	const unsigned step=DataflowGraphTraceStep(slot);
	const unsigned distance=DataflowGraphDecodeSteps-1-step;
	return lastSequence>distance && record.sequence==lastSequence-distance;
}
template<class IO> bool DataflowGraphTraceRead(IO& io,unsigned slot,
	const DataflowArgmaxResult& result,DataflowGraphTraceRecord& output) {
	output={};
	if (slot>=DataflowGraphTraceSlots || !DataflowArgmaxResultValid(result)) return false;
	const unsigned address=DataflowGraphTraceAddress+slot*sizeof(DataflowGraphTraceRecord);
	DataflowGraphTraceRecord first{},second{};
	if (!io.ReadCoreMemory(0,address,&first,sizeof(first))
		|| !io.ReadCoreMemory(0,address,&second,sizeof(second))
		|| first.sequence!=second.sequence || first.first!=second.first
		|| first.last!=second.last || first.magnitudeOr!=second.magnitudeOr
		|| !DataflowGraphTraceValid(first,slot,result.sequence)) return false;
	output=first;return true;
}

class DataflowGraphDiagnosticResult {
	DataflowArgmaxResult last{Vocabulary,0,0,0};
	bool valid=false;
public:
	void Reset() { last={Vocabulary,0,0,0};valid=false; }
	bool Set(const DataflowArgmaxResult& result) {
		Reset();if (!DataflowArgmaxResultValid(result)) return false;
		last=result;valid=true;return true;
	}
	bool Read(DataflowArgmaxResult& result) const {
		if (!valid) return false;
		result=last;return true;
	}
};
}
#endif
