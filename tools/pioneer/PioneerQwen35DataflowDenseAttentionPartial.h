/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_DENSE_ATTENTION_PARTIAL_H
#define PIONEER_QWEN35_DATAFLOW_DENSE_ATTENTION_PARTIAL_H
#if !defined(PIONEER_QWEN35_FLASH_DECODE_COMPILE_ONLY) \
    || !defined(PIONEER_QWEN35_RECOVERY_COMPILE_ONLY) \
    || defined(PIONEER_QWEN35_RECOVERY_QUALIFY)
#error Flash-decode partials are an isolated non-loadable witness
#endif
#include "PioneerQwen35DataflowDenseAttentionProgram.h"
#include "PioneerQwen35DataflowDenseAttentionMath.h"

namespace PioneerQwen35 {

// Temporal shards for GQA: 30 workers per KV head; each assigned worker can
// process the four corresponding query heads while holding one K/V tile.
// Counts may be zero for short histories. This defines work, not a transport,
// synchronization protocol, matrix kernel, hardware authority or speedup.
enum : unsigned { DataflowFlashWorkersPerKv=DataflowCoreCount/DenseKeyHeads,
    DataflowFlashQueriesPerKv=DenseHeads/DenseKeyHeads };
static_assert(DataflowCoreCount==120 && DenseHeads==16 && DenseKeyHeads==4
    && DataflowFlashWorkersPerKv==30 && DataflowFlashQueriesPerKv==4,
    "flash-decode partition is specific to the fixed Qwen35 fleet");
struct DataflowFlashPartition { unsigned kvHead,firstPosition,positionCount; };
inline bool DataflowFlashAssign(unsigned core,unsigned position,DataflowFlashPartition& out)
{
    out={};
    if (core>=DataflowCoreCount || position>=MaxPositions) return false;
    const unsigned total=position+1,rank=core%DataflowFlashWorkersPerKv;
    const unsigned base=total/DataflowFlashWorkersPerKv,extra=total%DataflowFlashWorkersPerKv;
    const unsigned first=rank*base+(rank<extra?rank:extra);
    const unsigned count=base+unsigned(rank<extra);
    out={core/DataflowFlashWorkersPerKv,first,count};
    return out.kvHead<DenseKeyHeads && first<=total && count<=total-first;
}

// Hybrid position/output partition. Every worker has a nonempty position
// interval and an aligned output-block interval even for position zero.
// Long histories progressively trade redundant KV reads for temporal
// parallelism; at 30 or more positions each KV history is read only once
// across its 30 workers. The output interval can cross query-head boundaries,
// so a worker must process each intersected head separately.
enum : unsigned { DataflowFlashOutputBlock=16,
    DataflowFlashBlocksPerHead=DenseHeadWidth/DataflowFlashOutputBlock,
    DataflowFlashBlocksPerKv=DataflowFlashQueriesPerKv*DataflowFlashBlocksPerHead };
struct DataflowFlashHybridPartition {
    unsigned kvHead,positionShard,positionShards,firstPosition,positionCount;
    unsigned outputShard,outputShards,firstBlock,blockCount;
};
inline bool DataflowFlashHybridAssign(unsigned core,unsigned position,DataflowFlashHybridPartition& out)
{
    out={};
    if (core>=DataflowCoreCount || position>=MaxPositions) return false;
    const unsigned total=position+1;
    unsigned positions=DataflowFlashWorkersPerKv;
    while (positions>1 && (positions>total || DataflowFlashWorkersPerKv%positions)) --positions;
    const unsigned outputs=DataflowFlashWorkersPerKv/positions;
    const unsigned rank=core%DataflowFlashWorkersPerKv;
    const unsigned positionShard=rank/outputs,outputShard=rank%outputs;
    const unsigned positionBase=total/positions,positionExtra=total%positions;
    const unsigned blockBase=DataflowFlashBlocksPerKv/outputs,blockExtra=DataflowFlashBlocksPerKv%outputs;
    out={core/DataflowFlashWorkersPerKv,positionShard,positions,
        positionShard*positionBase+(positionShard<positionExtra?positionShard:positionExtra),
        positionBase+unsigned(positionShard<positionExtra),outputShard,outputs,
        outputShard*blockBase+(outputShard<blockExtra?outputShard:blockExtra),
        blockBase+unsigned(outputShard<blockExtra)};
    return out.positionCount && out.blockCount && out.kvHead<DenseKeyHeads
        && out.firstPosition<=total && out.positionCount<=total-out.firstPosition
        && out.firstBlock<DataflowFlashBlocksPerKv
        && out.blockCount<=DataflowFlashBlocksPerKv-out.firstBlock;
}
// An output shard can straddle heads; return its aligned lane intersection
// with one of its KV head's four query heads. Empty intersections are valid.
inline bool DataflowFlashHybridRange(const DataflowFlashHybridPartition& part,unsigned query,
    unsigned& firstLane,unsigned& laneCount)
{
    firstLane=laneCount=0;
    if (query>=DataflowFlashQueriesPerKv || part.kvHead>=DenseKeyHeads
        || part.firstBlock>=DataflowFlashBlocksPerKv || !part.blockCount
        || part.blockCount>DataflowFlashBlocksPerKv-part.firstBlock) return false;
    const unsigned start=query*DataflowFlashBlocksPerHead,end=start+DataflowFlashBlocksPerHead;
    const unsigned first=part.firstBlock>start?part.firstBlock:start;
    const unsigned limit=part.firstBlock+part.blockCount<end?part.firstBlock+part.blockCount:end;
    if (first<limit) {
        firstLane=(first-start)*DataflowFlashOutputBlock;
        laneCount=(limit-first)*DataflowFlashOutputBlock;
    }
    return true;
}

// Merge two online-softmax partials after the producer's data is complete and
// visible. Partial sums are unnormalized. The caller owns separate 256-float
// buffers, must reject uncertain transfers and must retain any failed graph.
// A failure may leave destination lanes modified; never publish that result.
inline bool DataflowFlashMerge(DataflowDenseAttentionRow& destination,
    const DataflowDenseAttentionRow& source)
{
    const uintptr_t dst=reinterpret_cast<uintptr_t>(destination.sums);
    const uintptr_t src=reinterpret_cast<uintptr_t>(source.sums);
    constexpr uintptr_t bytes=DenseHeadWidth*sizeof(float);
    if (!dst || !src || dst>UINTPTR_MAX-bytes || src>UINTPTR_MAX-bytes
        || !(dst+bytes<=src || src+bytes<=dst)
        || !DataflowDenseFinite(destination.denominator) || destination.denominator<0
        || !DataflowDenseFinite(source.denominator) || source.denominator<0
        || !DataflowDenseFinite(destination.maximum) || !DataflowDenseFinite(source.maximum)) return false;
    if (!source.denominator) return true;
    for (unsigned lane=0;lane<DenseHeadWidth;lane++)
        if (!DataflowDenseFinite(source.sums[lane])
            || (destination.denominator && !DataflowDenseFinite(destination.sums[lane]))) return false;
    if (!destination.denominator) {
        for (unsigned lane=0;lane<DenseHeadWidth;lane++) destination.sums[lane]=source.sums[lane];
        destination.maximum=source.maximum;destination.denominator=source.denominator;return true;
    }
    const float maximum=destination.maximum>source.maximum?destination.maximum:source.maximum;
    const float leftDelta=destination.maximum-maximum,rightDelta=source.maximum-maximum;
    // A negative overflow is a negligible contribution, not a positive exponent.
    const float left=DataflowDenseFinite(leftDelta)?PioneerQ38PrepareNegativeExp(leftDelta):0.0f;
    const float right=DataflowDenseFinite(rightDelta)?PioneerQ38PrepareNegativeExp(rightDelta):0.0f;
    const float denominator=left*destination.denominator+right*source.denominator;
    if (!(denominator>0) || !DataflowDenseFinite(denominator)) return false;
    for (unsigned lane=0;lane<DenseHeadWidth;lane++) {
        const float value=left*destination.sums[lane]+right*source.sums[lane];
        if (!DataflowDenseFinite(value)) return false;
        destination.sums[lane]=value;
    }
    destination.maximum=maximum;destination.denominator=denominator;
    return true;
}
}
#endif
