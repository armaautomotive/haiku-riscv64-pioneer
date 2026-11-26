/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_DENSE_ATTENTION_SHARD_PROGRAM_H
#define PIONEER_QWEN35_DATAFLOW_DENSE_ATTENTION_SHARD_PROGRAM_H
#if !defined(PIONEER_QWEN35_DENSE_SHARD_COMPILE_ONLY) || !defined(PIONEER_QWEN35_RECOVERY_COMPILE_ONLY) || defined(PIONEER_QWEN35_RECOVERY_QUALIFY)
#error Dense attention shards are an isolated non-loadable witness
#endif
#include "PioneerQwen35DataflowDenseAttentionProgram.h"
namespace PioneerQwen35 {
enum : unsigned { DataflowDenseAttentionShardMagic=0x31534135,DataflowDenseAttentionShardVersion=1 };
static_assert(DataflowCoreCount==120 && DenseHeads==16 && DenseHeadWidth==256,
    "dense shard mapping requires the pinned Qwen35 geometry");
// One attention head is split into seven or eight aligned output slices.
// Each worker recomputes the full score and denominator for that head, but
// updates and writes only its own slice. This avoids inter-worker reduction
// and preserves the scalar accumulation order. It increases Q/KV reads and
// has no measured performance benefit; no hardware execution is authorized.
struct DataflowDenseAttentionShard {
    unsigned head,firstLane,laneCount;
};

inline bool DataflowDenseAttentionShardAssign(unsigned core,DataflowDenseAttentionShard& result)
{
    result={};
    if (core>=DataflowCoreCount) return false;
    // 8 heads * 8 workers + 8 heads * 7 workers = 120.
    const unsigned head=core<64?core/8:8+(core-64)/7;
    const unsigned index=core<64?core%8:(core-64)%7;
    const unsigned workers=head<8?8:7;
    const unsigned groups=DenseHeadWidth/16;
    const unsigned base=groups/workers,extra=groups%workers;
    const unsigned first=index*base+(index<extra?index:extra);
    const unsigned count=base+unsigned(index<extra);
    result={head,first*16,count*16};
    return head<DenseHeads && count && result.firstLane+result.laneCount<=DenseHeadWidth;
}

struct DataflowDenseAttentionShardInvocation {
    unsigned magic,version,core,head,firstLane,laneCount;
    DataflowDenseAttentionProgram program;
    bool Valid() const {
        DataflowDenseAttentionShard expected{};
        return magic==DataflowDenseAttentionShardMagic && version==DataflowDenseAttentionShardVersion
            && program.mode==DataflowDenseAttentionRun && program.Valid()
            && DataflowDenseAttentionShardAssign(core,expected) && head==expected.head
            && firstLane==expected.firstLane && laneCount==expected.laneCount;
    }
};
struct DataflowDenseAttentionShardBuilder {
    static bool BindCore(const DataflowDenseAttentionProgram& program,unsigned core,
        DataflowDenseAttentionShardInvocation& output) {
        DataflowDenseAttentionShard shard{};
        if (!program.Valid() || program.mode!=DataflowDenseAttentionRun
            || !DataflowDenseAttentionShardAssign(core,shard)) return false;
        DataflowDenseAttentionShardInvocation candidate{};
        candidate.magic=DataflowDenseAttentionShardMagic;candidate.version=DataflowDenseAttentionShardVersion;
        candidate.core=core;candidate.head=shard.head;candidate.firstLane=shard.firstLane;
        candidate.laneCount=shard.laneCount;candidate.program=program;
        if (!candidate.Valid()) return false;
        output=candidate;return true;
    }
};
static_assert(sizeof(DataflowDenseAttentionShardInvocation)<=DataflowProgramRegionBytes,
    "dense attention shard invocation fits the resident program slot");
}
#endif
