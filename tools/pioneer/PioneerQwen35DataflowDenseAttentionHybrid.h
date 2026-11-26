/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_DENSE_ATTENTION_HYBRID_H
#define PIONEER_QWEN35_DATAFLOW_DENSE_ATTENTION_HYBRID_H

#include "PioneerQwen35DataflowDenseAttentionPartial.h"
#include "PioneerQwen35DataflowBlackholeDenseAttentionWorker.h"

namespace PioneerQwen35 {
// Keep both K/V tiles, four queries and four partial sums in the existing
// activation-cache allocation. The transport must separately reserve and
// authenticate its partial buffers and prove publication visibility.
enum : unsigned { DataflowFlashHybridTilePositions=16,
    DataflowFlashHybridScratchBytes=2*DataflowFlashHybridTilePositions*DataflowDenseAttentionHeadBytes
        +2*DataflowFlashQueriesPerKv*DenseHeadWidth*sizeof(float)
        +3*DenseHeadWidth*sizeof(float)+Rotary*sizeof(float) };
static_assert(unsigned(DataflowFlashHybridScratchBytes)<=unsigned(DataflowActivationCacheBytes),
    "hybrid partial scratch fits the reserved activation cache");
struct DataflowFlashHybridScratch {
    float *keyTile,*valueTile,*queries,*sums,*norm,*rope,*gate,*incoming;
    // The trusted caller must reserve disjoint regions of the sizes above.
    bool Valid() const {
        return keyTile && valueTile && queries && sums && norm && rope && gate && incoming;
    }
};

// IO owns the validated program binding, bounded K/V reads and one-shot
// partial publication. Failure may follow a partial write; the enclosing
// graph must retain the fleet rather than retry or consume any partials.
template<class IO>
bool DataflowFlashHybridProduce(const DataflowDenseAttentionProgram& program,unsigned core,
    DataflowFlashHybridScratch scratch,IO& io)
{
    DataflowFlashHybridPartition part{};
    if (!scratch.Valid() || !program.Valid() || program.mode!=DataflowDenseAttentionRun
        || !DataflowFlashHybridAssign(core,program.position,part) || !io.BindShardProgram(program)
        || !io.ReadNorm(true,scratch.norm) || !io.ReadRope(scratch.rope)) return false;
    DataflowDenseAttentionRow rows[DataflowFlashQueriesPerKv]{};
    unsigned first[DataflowFlashQueriesPerKv]{},count[DataflowFlashQueriesPerKv]{};
    for (unsigned q=0;q<DataflowFlashQueriesPerKv;q++) {
        rows[q].sums=scratch.sums+q*DenseHeadWidth;
        for (unsigned lane=0;lane<DenseHeadWidth;lane++) rows[q].sums[lane]=0;
        if (!DataflowFlashHybridRange(part,q,first[q],count[q])) return false;
        if (count[q]) {
            float* query=scratch.queries+q*DenseHeadWidth;
            const unsigned head=part.kvHead*DataflowFlashQueriesPerKv+q;
            if (!io.ReadProjectedRows(2*head*DenseHeadWidth,query)
                || !DataflowDenseNormalize(query,scratch.norm,query)
                || !DataflowDenseRotate(query,scratch.rope)) return false;
        }
    }
    for (unsigned offset=0;offset<part.positionCount;) {
        unsigned countPositions=part.positionCount-offset;
        if (countPositions>DataflowFlashHybridTilePositions) countPositions=DataflowFlashHybridTilePositions;
        if (!io.ReadKvTile(part.kvHead,false,part.firstPosition+offset,countPositions,scratch.keyTile)
            || !io.ReadKvTile(part.kvHead,true,part.firstPosition+offset,countPositions,scratch.valueTile)) return false;
        const auto* keys=reinterpret_cast<const uint32_t*>(scratch.keyTile);
        const auto* values=reinterpret_cast<const uint32_t*>(scratch.valueTile);
        for (unsigned i=0;i<countPositions;i++) for (unsigned q=0;q<DataflowFlashQueriesPerKv;q++)
            if (count[q] && !rows[q].AddSlice(scratch.queries+q*DenseHeadWidth,
                keys+i*DenseHeadWidth/2,values+i*DenseHeadWidth/2,first[q],count[q])) return false;
        offset+=countPositions;
    }
    for (unsigned q=0;q<DataflowFlashQueriesPerKv;q++)
        if (count[q] && !io.PublishPartial(core,q,first[q],count[q],rows[q])) return false;
    return true;
}

// Call only after all producers complete and the caller proves visibility.
// ReadPartial must authenticate the source, query, lane range, graph ticket,
// position and a stable complete payload. It must fail closed on uncertainty.
// Only position-shard zero writes each disjoint final output slice.
template<class IO>
bool DataflowFlashHybridReduce(const DataflowDenseAttentionProgram& program,unsigned core,
    DataflowFlashHybridScratch scratch,IO& io)
{
    DataflowFlashHybridPartition part{};
    if (!scratch.Valid() || !program.Valid() || program.mode!=DataflowDenseAttentionRun
        || !DataflowFlashHybridAssign(core,program.position,part)) return false;
    if (part.positionShard) return true;
    if (!io.BindShardProgram(program)) return false;
    for (unsigned q=0;q<DataflowFlashQueriesPerKv;q++) {
        unsigned first=0,count=0;
        if (!DataflowFlashHybridRange(part,q,first,count)) return false;
        if (!count) continue;
        for (unsigned lane=0;lane<DenseHeadWidth;lane++) scratch.sums[lane]=0;
        DataflowDenseAttentionRow result{scratch.sums};
        for (unsigned shard=0;shard<part.positionShards;shard++) {
            const unsigned source=part.kvHead*DataflowFlashWorkersPerKv
                +shard*part.outputShards+part.outputShard;
            DataflowDenseAttentionRow incoming{scratch.incoming};
            if (!io.ReadPartial(source,q,first,count,incoming) || !(incoming.denominator>0)) return false;
            for (unsigned lane=0;lane<DenseHeadWidth;lane++)
                if ((lane<first || lane>=first+count) && incoming.sums[lane]!=0.0f) return false;
            if (!DataflowFlashMerge(result,incoming)) return false;
        }
        const unsigned head=part.kvHead*DataflowFlashQueriesPerKv+q;
        if (!io.ReadProjectedRows((2*head+1)*DenseHeadWidth,scratch.gate)
            || !result.FinishSlice(scratch.gate,first,count)
            || !io.WriteContextSlice(head*DenseHeadWidth+first,count,scratch.sums+first)) return false;
    }
    return true;
}
}
#endif
