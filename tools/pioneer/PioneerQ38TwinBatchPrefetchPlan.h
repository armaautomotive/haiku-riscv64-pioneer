/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_TWIN_BATCH_PREFETCH_PLAN_H
#define PIONEER_Q38_TWIN_BATCH_PREFETCH_PLAN_H
#include "PioneerQ38BatchedPrefetchPlan.h"
// Pure geometry,NOT a response,drain,owner or hardware-use grant. Preserve
// the existing Q8x4/Q4x2/Q6x2 packet grammar and original learned bytes.
// Alternate TWO complete batches inside the EXISTING saved peer L1 span.
// A caller MUST retire every previous-batch tensor reference before issuing
// into its buffer again. Neither buffer aliases metadata91000,scales94000,
// input-sum cache95000..96100 or shared vectora0000. Other service operations
// reuse this memory:there is NO cross-job cache or lifetime ownership here.
struct PioneerQ38TwinBatchPrefetchPlan {
 uint32_t currentSource=0,currentRaw=0,bulkSource=0,bulkRaw=0;
 uint32_t bulkBytes=0,nextSource=0,nextRaw=0,tileBytes=0;
 bool first=false,batchFirst=false,next=false,restore=false;
 enum {Buffer0=0x98000,Buffer1=0x9a000,BufferStride=8192,BufferLimit=0x9ba80};
#if defined(__riscv) && defined(PIONEER_Q38_WORK_TWIN_BATCH_PREFETCH)
 __attribute__((noinline,section(".text.pioneer_q38_batched_raw_input")))
#endif
 static bool RawInput(unsigned type,uint32_t base){
  const unsigned tile=PioneerQ38BatchedPrefetchPlan::Tile(type);
  const unsigned batch=PioneerQ38BatchedPrefetchPlan::Batch(type);
  if(!tile)return false;
  if(base==0x90000)return true; // unchanged checked/validation path only
  for(unsigned buffer=Buffer0;buffer<=Buffer1;buffer+=BufferStride)
   for(unsigned index=0;index<batch;index++)if(base==buffer+index*tile)return true;
  return false;
 }
 static __attribute__((always_inline)) bool Describe(unsigned type,unsigned inputs,unsigned groups,
  unsigned group,unsigned block,uint32_t firstSource,PioneerQ38TwinBatchPrefetchPlan& output){
  PioneerQ38BatchedPrefetchPlan old;
  if(!PioneerQ38BatchedPrefetchPlan::Describe(type,inputs,groups,group,block,firstSource,old))return false;
  const unsigned batch=PioneerQ38BatchedPrefetchPlan::Batch(type);
  const unsigned blocks=inputs>>(type==8?5:8),index=block&(batch-1);
  const unsigned batchOrdinal=block>>(type==8?2:1);
  PioneerQ38TwinBatchPrefetchPlan p;
  p.first=old.first;p.batchFirst=old.batchFirst;p.tileBytes=old.tileBytes;
  p.currentSource=old.currentSource;p.bulkSource=old.bulkSource;p.bulkBytes=old.bulkBytes;
  p.bulkRaw=(batchOrdinal&1)?Buffer1:Buffer0;
  p.currentRaw=p.bulkRaw+index*p.tileBytes;
  p.next=p.batchFirst && block+batch<blocks;
  p.restore=p.batchFirst && block+batch==blocks;
  if(p.next){p.nextSource=p.bulkSource+p.bulkBytes;p.nextRaw=p.bulkRaw^BufferStride;}
  if(p.bulkBytes>BufferStride || p.bulkRaw+p.bulkBytes>BufferLimit
   || p.currentRaw+p.tileBytes>p.bulkRaw+p.bulkBytes)return false;
  output=p;return true;
 }
};
#endif
