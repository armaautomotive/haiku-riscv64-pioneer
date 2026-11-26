/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_DENSE_ATTENTION_SHARDS_H
#define PIONEER_QWEN35_DATAFLOW_DENSE_ATTENTION_SHARDS_H

#if !defined(PIONEER_QWEN35_DENSE_SHARD_COMPILE_ONLY) \
    || !defined(PIONEER_QWEN35_RECOVERY_COMPILE_ONLY) \
    || defined(PIONEER_QWEN35_RECOVERY_QUALIFY)
#error Dense attention shards are an isolated non-loadable witness
#endif
#include "PioneerQwen35DataflowBlackholeDenseAttentionWorker.h"
#include "PioneerQwen35DataflowDenseAttentionShardProgram.h"

namespace PioneerQwen35 {

struct DataflowDenseAttentionShardScratch {
    float* keyTile;float* valueTile;float* query;float* gate;float* norm;float* rope;float* output;
    bool Valid() const { return keyTile && valueTile && query && gate && norm && rope && output; }
};

template<class IO>
bool PioneerQwen35DataflowDenseAttentionShardWorker(const DataflowDenseAttentionProgram& program,
    unsigned core,DataflowDenseAttentionShardScratch scratch,IO& io)
{
    DataflowDenseAttentionShard shard{};
    if (!scratch.Valid() || !program.Valid() || program.mode!=DataflowDenseAttentionRun
        || !DataflowDenseAttentionShardAssign(core,shard) || !io.BindShardProgram(program)) return false;
    const unsigned queryBase=2*shard.head*DenseHeadWidth;
    if (!io.ReadProjectedRows(queryBase,scratch.query)
        || !io.ReadProjectedRows(queryBase+DenseHeadWidth,scratch.gate)
        || !io.ReadNorm(true,scratch.norm) || !io.ReadRope(scratch.rope)
        || !DataflowDenseNormalize(scratch.query,scratch.norm,scratch.query)
        || !DataflowDenseRotate(scratch.query,scratch.rope)) return false;
    for (unsigned lane=shard.firstLane;lane<shard.firstLane+shard.laneCount;lane++) scratch.output[lane]=0;
    DataflowDenseAttentionRow row{scratch.output};
    const unsigned kvHead=shard.head/(DenseHeads/DenseKeyHeads);
    for (unsigned first=0;first<=program.position;) {
        unsigned positions=program.position-first+1;
        if (positions>DataflowDenseAttentionTilePositions) positions=DataflowDenseAttentionTilePositions;
        if (!io.ReadKvTile(kvHead,false,first,positions,scratch.keyTile)
            || !io.ReadKvTile(kvHead,true,first,positions,scratch.valueTile)) return false;
        const auto* keys=reinterpret_cast<const uint32_t*>(scratch.keyTile);
        const auto* values=reinterpret_cast<const uint32_t*>(scratch.valueTile);
        for (unsigned index=0;index<positions;index++)
            if (!row.AddSlice(scratch.query,keys+index*DenseHeadWidth/2,
                    values+index*DenseHeadWidth/2,shard.firstLane,shard.laneCount)) return false;
        first+=positions;
    }
    return row.FinishSlice(scratch.gate,shard.firstLane,shard.laneCount)
        && io.WriteContextSlice(shard.head*DenseHeadWidth+shard.firstLane,shard.laneCount,
            scratch.output+shard.firstLane);
}
}
#endif
