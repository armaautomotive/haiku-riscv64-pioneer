/* SPDX-License-Identifier: MIT */
// Compile-only shape witness. The undefined IO methods are not a transport.
#define PIONEER_QWEN35_FLASH_DECODE_COMPILE_ONLY 1
#define PIONEER_QWEN35_RECOVERY_COMPILE_ONLY 1
#include "../PioneerQwen35DataflowDenseAttentionHybrid.h"
using namespace PioneerQwen35;
struct ProbeIO {
    bool BindShardProgram(const DataflowDenseAttentionProgram&);
    bool ReadNorm(bool,float*);
    bool ReadRope(float*);
    bool ReadProjectedRows(unsigned,float*);
    bool ReadKvTile(unsigned,bool,unsigned,unsigned,float*);
    bool PublishPartial(unsigned,unsigned,unsigned,unsigned,const DataflowDenseAttentionRow&);
    bool ReadPartial(unsigned,unsigned,unsigned,unsigned,DataflowDenseAttentionRow&);
    bool WriteContextSlice(unsigned,unsigned,const float*);
};
extern "C" bool q35_flash_hybrid_produce(const DataflowDenseAttentionProgram& p,
    unsigned core,DataflowFlashHybridScratch s,ProbeIO& io) {
    return DataflowFlashHybridProduce(p,core,s,io);
}
extern "C" bool q35_flash_hybrid_reduce(const DataflowDenseAttentionProgram& p,
    unsigned core,DataflowFlashHybridScratch s,ProbeIO& io) {
    return DataflowFlashHybridReduce(p,core,s,io);
}
