/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_BATCHED_PREFETCH_PLAN_H
#define PIONEER_Q38_BATCHED_PREFETCH_PLAN_H
#ifndef PIONEER_ATTENTION_FREESTANDING
#include <stdint.h>
#else
typedef __UINT32_TYPE__ uint32_t;
#endif
// Pure geometry ONLY. SAME original weight bytes,old saved L1 extents.
// Q8 batches4 (2304B),Q4 batches2 (4608B),Q6 batches2 (6784B). Keep the
// existing <=8KiB precaution,NOT a universal hardware safety guarantee:
// https://github.com/tenstorrent/tt-metal/issues/52270
// Only the LAST tile is copied to90000 BEFORE reusing scratch98000..99a80
// for the next batch. Actual tensor drain,response/source/owner proofs
// are the caller's obligation;this header grants NONE of them.
struct PioneerQ38BatchedPrefetchPlan {
 uint32_t currentSource=0,currentRaw=0,copySource=0,tileBytes=0;
 uint32_t bulkSource=0,bulkBytes=0,nextSource=0;
 bool first=false,batchFirst=false,copyLast=false,next=false;
 static unsigned Batch(unsigned type){return type==8?4U:type==12 || type==14?2U:0U;}
 static unsigned Tile(unsigned type){return type==8?576U:type==12?2304U:type==14?3392U:0U;}
 static unsigned ReadBlocks(unsigned type,unsigned inputs,bool batched){
  const unsigned batch=Batch(type);
  if(!batch || (inputs!=5120 && inputs!=6144 && inputs!=17408))return 0;
  const unsigned blocks=inputs>>(type==8?5:8);
  return batched?blocks>>(type==8?2:1):blocks;
 }
#if defined(__riscv) && defined(PIONEER_Q38_WORK_BATCHED_PREFETCH)
 __attribute__((noinline,section(".text.pioneer_q38_batched_raw_input")))
#endif
 static bool RawInput(unsigned type,uint32_t base){
  const unsigned tile=Tile(type),batch=Batch(type);
  if(!tile)return false;
  return base==0x90000 || base==0x98000 || base==0x98000+tile
   || (batch==4 && (base==0x98000+2*tile || base==0x98000+3*tile));
 }
 static __attribute__((always_inline)) bool Describe(unsigned type,unsigned inputs,unsigned groups,
  unsigned group,unsigned block,uint32_t firstSource,PioneerQ38BatchedPrefetchPlan& output){
  const unsigned tile=Tile(type),batch=Batch(type);
  if(!tile || (inputs!=5120 && inputs!=6144 && inputs!=17408) || !groups || groups>16
   || group>=groups || firstSource<0x04000000 || firstSource>=0xc4000000 || (firstSource&63))return false;
  const unsigned blocks=inputs>>(type==8?5:8),mask=batch-1;
  if((blocks&mask) || block>=blocks || groups*blocks*tile>0xc4000000-firstSource)return false;
  PioneerQ38BatchedPrefetchPlan p;
  p.first=!block;p.batchFirst=!(block&mask);p.copyLast=(block&mask)==mask;
  p.tileBytes=tile;p.bulkBytes=tile*batch;
  p.bulkSource=firstSource+(group*blocks+(block&~mask))*tile;
  p.currentSource=firstSource+(group*blocks+block)*tile;
  p.copySource=0x98000+(block&mask)*tile;
  p.currentRaw=p.copyLast?0x90000:p.copySource;
  p.next=p.copyLast && block+1<blocks;
  if(p.next)p.nextSource=p.bulkSource+p.bulkBytes;
  if(p.bulkBytes>8192 || 0x98000+p.bulkBytes>0x99a80 || p.copySource+tile>0x98000+p.bulkBytes)return false;
  output=p;return true;
 }
};
#endif
