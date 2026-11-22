/* SPDX-License-Identifier: MIT */
#define PIONEER_QWEN35_DEVICE_DISPATCH 1
#include "../PioneerQwen35DataflowGraphMetadata.h"
using namespace PioneerQwen35;

// ABI and stack witness only. It does not publish or commit model state.
extern "C" bool pioneer_q35_graph_metadata_kernel_probe(
	const DataflowProjectionAuthority& live,DataflowProjectionAuthority& candidate,
	unsigned position,unsigned token,unsigned slot,bool decode,
	DataflowProjectionProgram& scratch,unsigned& stage)
{
	return DataflowGraphMetadata::Prepare(live,candidate,position,token,slot,decode,scratch,stage);
}
