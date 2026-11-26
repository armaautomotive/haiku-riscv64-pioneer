/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_FLASH_HYBRID_PROGRAM_H
#define PIONEER_QWEN35_DATAFLOW_FLASH_HYBRID_PROGRAM_H
#if !defined(PIONEER_QWEN35_FLASH_DECODE_COMPILE_ONLY) || !defined(PIONEER_QWEN35_RECOVERY_COMPILE_ONLY) \
    || !defined(PIONEER_QWEN35_DELTA_SHARD_COMPILE_ONLY) || defined(PIONEER_QWEN35_DENSE_SHARD_COMPILE_ONLY) \
    || defined(PIONEER_QWEN35_RECOVERY_QUALIFY) || defined(PIONEER_QWEN35_DEVICE_DISPATCH_TRIAL)
#error Flash hybrid graph records require the isolated sharded recurrence witness
#endif
#include "PioneerQwen35DataflowDenseAttentionPartial.h"
namespace PioneerQwen35 {
enum : unsigned { DataflowFlashHybridMagic=0x31484635,DataflowFlashHybridVersion=1 };
enum DataflowFlashHybridPhase : unsigned { DataflowFlashHybridProducer=1,DataflowFlashHybridReducer=2 };
struct DataflowFlashHybridInvocation {
    unsigned magic,version,core,phase;
    DataflowDenseAttentionProgram program;
    bool Valid() const {
        DataflowFlashHybridPartition part{};
        return magic==DataflowFlashHybridMagic && version==DataflowFlashHybridVersion
            && (phase==DataflowFlashHybridProducer || phase==DataflowFlashHybridReducer)
            && program.mode==DataflowDenseAttentionRun && program.Valid()
            && DataflowFlashHybridAssign(core,program.position,part);
    }
};
template<DataflowFlashHybridPhase Phase> struct DataflowFlashHybridBuilder {
    static bool BindCore(const DataflowDenseAttentionProgram& program,unsigned core,DataflowFlashHybridInvocation& out) {
        DataflowFlashHybridInvocation candidate{};
        candidate.magic=DataflowFlashHybridMagic;candidate.version=DataflowFlashHybridVersion;
        candidate.core=core;candidate.phase=Phase;candidate.program=program;
        if (!candidate.Valid()) return false;
        out=candidate;return true;
    }
};
static_assert(sizeof(DataflowFlashHybridInvocation)<=DataflowProgramRegionBytes,
    "hybrid dense invocation fits the resident program slot");
}
#endif
