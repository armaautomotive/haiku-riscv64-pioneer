/* SPDX-License-Identifier: MIT */
// Freestanding compile witness only; no boot vector or live dispatch entry.
#define PIONEER_QWEN35_DELTA_SHARD_COMPILE_ONLY 1
#include "../PioneerQwen35DataflowDeltaShards.h"
extern "C" bool q35_delta_shard_probe(const PioneerQwen35::DataflowDeltaProgram& program,
    unsigned core,PioneerQwen35::DataflowDeltaShardPhase phase,
    PioneerQwen35::DataflowDeltaShardScratch scratch)
{
    PioneerQwen35::DataflowBlackholeDeltaOps io;
    return PioneerQwen35::PioneerQwen35DataflowDeltaShardWorker(program,core,phase,scratch,io);
}
