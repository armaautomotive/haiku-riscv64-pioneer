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
