/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_TILE_PREFETCH_PLAN_H
#define PIONEER_Q38_TILE_PREFETCH_PLAN_H
#ifndef PIONEER_Q38_WORK_TILE_PREFETCH
#error Separate OFFLINE one-tile prefetch DATA proposal ONLY
#endif
#include "PioneerQwen38Panel.h"
// Exact original-layout DATA offsets ONLY. Not a NoC/read/completion,
// allocated-route/source/ownership capability. A future caller MUST prove
// the SAME private complete immutable extent and actual response before
// consuming either buffer;failure remains CLOSE ONLY/owner retention.
struct PioneerQ38TilePrefetchPlan {
 uint32_t currentSource=0,currentRaw=0,nextSource=0,nextRaw=0,bytes=0;
 bool next=false;
 static bool Describe(unsigned type,unsigned k,unsigned groups,unsigned group,unsigned block,uint32_t first,PioneerQ38TilePrefetchPlan& output){
  if((k!=5120&&k!=6144&&k!=17408)||!groups||groups>16||group>=groups||first<0x04000000||(first&63))return false;
  unsigned bytes=PioneerQ38PanelTileBytes(type);if(!bytes)return false;
  unsigned blocks=k>>(type==8?5:8);if(block>=blocks)return false;
  // Fixed shapes bound this product by5,013,504,also on RV32. No
  // freestanding64-bit multiply/divide/runtime helper is introduced.
  uint32_t total=groups*blocks*bytes;
  if(first>=0xc4000000||total>0xc4000000-first)return false;
  PioneerQ38TilePrefetchPlan p;
  p.currentSource=first+(group*blocks+block)*bytes;p.currentRaw=(block&1)?0x98000:0x90000;p.bytes=bytes;
  p.next=block+1<blocks;
  if(p.next){p.nextSource=p.currentSource+bytes;p.nextRaw=(block&1)?0x90000:0x98000;}
  output=p;return true; // only after whole bounded proposal is validated
 }
};
static inline bool PioneerQ38PrefetchRawInput(unsigned type,uint32_t base){
 return (type==8||type==12||type==14)&&(base==0x90000||base==0x98000);
}
#endif
