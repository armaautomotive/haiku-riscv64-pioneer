/* SPDX-License-Identifier: MIT */
#define PIONEER_QWEN35_DEVICE_DISPATCH 1
#include "../PioneerQwen35DataflowStreamRecorder.h"
using namespace PioneerQwen35;
// Compile the real planner with an unresolved abstract storage backend. This
// is a kernel-ABI/stack witness, not a linked driver or a hardware operation.
struct PackedProbe { bool Workspace(unsigned,uint32_t&,uint32_t&) const; };
struct SinkProbe {
	bool Begin(const DataflowStreamPlan&);
	bool StageRecord(const DataflowStreamBinding&,const Range&,unsigned,const DataflowStreamRecord&);
	bool Seal(const DataflowStreamPlan&);
	void Abort();
};
extern "C" bool pioneer_q35_stream_recording_kernel_probe(const PackedProbe& packed,
	const DataflowProjectionAuthority& live,DataflowProjectionAuthority& candidate,
	const DataflowStreamPlan& plan,const unsigned (&routes)[2][Banks],unsigned position,
	unsigned token,unsigned slot,bool select,SinkProbe& sink,DataflowStreamRecord& record,
	DataflowProjectionProgram& scratch,unsigned& stage)
{
	return DataflowStreamRecording::Build(packed,live,candidate,plan,routes,position,token,
		slot,select,sink,record,scratch,stage);
}
