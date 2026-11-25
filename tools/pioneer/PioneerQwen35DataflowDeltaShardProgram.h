/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_DELTA_SHARD_PROGRAM_H
#define PIONEER_QWEN35_DATAFLOW_DELTA_SHARD_PROGRAM_H
#if !defined(PIONEER_QWEN35_DELTA_SHARD_COMPILE_ONLY)
#error Delta shards are an offline compile-only witness
#endif
#include "PioneerQwen35DataflowDeltaProgram.h"
namespace PioneerQwen35 {
enum : unsigned { DataflowDeltaShardMagic=0x31534435,DataflowDeltaShardVersion=1 };
enum DataflowDeltaShardPhase : unsigned { DataflowDeltaShardUpdate=1,DataflowDeltaShardNormalize=2 };
struct DataflowDeltaShardAssignment { unsigned head,firstPanel,panelCount; };
static_assert(DataflowCoreCount==120 && DeltaHeads==32 && DataflowDeltaPanelsPerHead==8,
    "the shard mapping is specific to the fixed 120-worker Qwen35 geometry");

inline bool DataflowDeltaShardAssign(unsigned core,DataflowDeltaShardPhase phase,
    DataflowDeltaShardAssignment& result)
{
    result={DeltaHeads,0,0};
    if (core>=DataflowCoreCount) return false;
    if (phase==DataflowDeltaShardNormalize) {
        if (core<DeltaHeads) result={core,0,DataflowDeltaPanelsPerHead};
        return true;
    }
    if (phase!=DataflowDeltaShardUpdate) return false;
    if (core<96) result={core/4,(core%4)*2,2};
    else {
        const unsigned index=core-96,part=index%3;
        result={24+index/3,part*3,part==2?2U:3U};
    }
    return result.head<DeltaHeads && result.panelCount
        && result.firstPanel+result.panelCount<=DataflowDeltaPanelsPerHead;
}
struct DataflowDeltaShardInvocation {
    unsigned magic,version,core,phase,head,firstPanel,panelCount;
    DataflowDeltaProgram program;
    bool Valid() const {
        DataflowDeltaShardAssignment expected{};
        if (magic!=DataflowDeltaShardMagic || version!=DataflowDeltaShardVersion || !program.ShapeValid()
            || !DataflowDeltaShardAssign(core,static_cast<DataflowDeltaShardPhase>(phase),expected)
            || head!=expected.head || firstPanel!=expected.firstPanel || panelCount!=expected.panelCount) return false;
        for (unsigned panel=firstPanel;panel<firstPanel+panelCount;panel++)
            if (!program.PanelRangesValid(head*DataflowDeltaPanelsPerHead+panel)) return false;
        return true;
    }
};
inline bool DataflowDeltaShardBind(const DataflowDeltaProgram& program,unsigned core,
    DataflowDeltaShardPhase phase,DataflowDeltaShardInvocation& output)
{
    DataflowDeltaShardAssignment assignment{};
    if (!program.Valid() || !DataflowDeltaShardAssign(core,phase,assignment)) return false;
    DataflowDeltaShardInvocation candidate{};
    candidate.magic=DataflowDeltaShardMagic;candidate.version=DataflowDeltaShardVersion;
    candidate.core=core;candidate.phase=phase;candidate.head=assignment.head;
    candidate.firstPanel=assignment.firstPanel;candidate.panelCount=assignment.panelCount;
    candidate.program=program;
    if (!candidate.Valid()) return false;
    output=candidate;return true;
}
static_assert(sizeof(DataflowDeltaShardInvocation)<=DataflowProgramRegionBytes,
    "sharded delta invocation fits the resident program slot");
}
#endif
