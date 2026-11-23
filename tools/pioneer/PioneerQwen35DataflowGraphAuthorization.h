/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_GRAPH_AUTHORIZATION_H
#define PIONEER_QWEN35_DATAFLOW_GRAPH_AUTHORIZATION_H

#include "PioneerQwen35DataflowGraphRunProtocol.h"

namespace PioneerQwen35 {

// Local consistency check after a successful dynamic fetch. The caller must
// already have validated its private descriptor copy. This does not establish
// model authority: the kernel must create the exact stream from its validated
// model, keep the bank claim sealed and disjoint from every device write, and
// exclusively own the descriptor and run ticket for the full execution.
inline bool DataflowGraphFetchedCommandMatches(const DataflowGraphRunDescriptor& run,
	unsigned index,const DataflowStreamRecord& staging,const volatile uint32_t* program,unsigned programBytes,
	unsigned readBytes=DataflowStreamRecordBytes)
{
	if (!program || (reinterpret_cast<uintptr_t>(program)&3) || programBytes<DataflowProgramRegionBytes
		|| !run.ticket.count || index>=run.ticket.count || index>=run.binding.count
		|| run.core!=run.binding.core || run.core>=DataflowCoreCount
		|| !DataflowStreamRecordStructureValidBytes(staging,run.binding.epoch,index+1,run.core,readBytes)
		|| staging.header[6]>programBytes) return false;
	const unsigned words=staging.header[6]/sizeof(uint32_t);
	// Fetch publishes word zero with release semantics after the other words.
	if (DataflowLoadAcquire(program[0])!=staging.payload[0]) return false;
	for (unsigned i=1;i<words;i++) if (program[i]!=staging.payload[i]) return false;
	return true;
}
}
#endif
