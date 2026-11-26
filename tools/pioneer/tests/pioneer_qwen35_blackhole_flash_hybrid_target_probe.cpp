/* SPDX-License-Identifier: MIT */
// Compile-only witness: the context method is deliberately undefined and
// does not grant ownership, an all-fleet barrier, or hardware authority.
#define PIONEER_QWEN35_FLASH_DECODE_COMPILE_ONLY 1
#define PIONEER_QWEN35_DEVICE_DISPATCH 1
#define PIONEER_QWEN35_RECOVERY_COMPILE_ONLY 1
#define PIONEER_QWEN35_SHARED_ROLE_GRAPH_WITNESS 1
#include "../PioneerQwen35DataflowBlackholeFlashHybridIO.h"
using namespace PioneerQwen35;
struct ProbeContext {bool FlashContextMatches(unsigned,unsigned,unsigned,unsigned,unsigned) const;};
extern "C" bool q35_blackhole_flash_produce(const DataflowDenseAttentionProgram& program,
    unsigned core,unsigned epoch,unsigned step,ProbeContext& context) {
    DataflowBlackholeFlashHybridIO<ProbeContext> io(context,core,epoch,step,false);
    return DataflowFlashHybridProduce(program,core,DataflowBlackholeFlashHybridScratch(),io);
}
extern "C" bool q35_blackhole_flash_reduce(const DataflowDenseAttentionProgram& program,
    unsigned core,unsigned epoch,unsigned step,ProbeContext& context) {
    DataflowBlackholeFlashHybridIO<ProbeContext> io(context,core,epoch,step,true);
    return DataflowFlashHybridReduce(program,core,DataflowBlackholeFlashHybridScratch(),io);
}
