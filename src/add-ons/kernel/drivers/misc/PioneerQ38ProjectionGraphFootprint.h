/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_PROJECTION_GRAPH_FOOTPRINT_H
#define PIONEER_Q38_PROJECTION_GRAPH_FOOTPRINT_H
#include "PioneerQ38ProjectionGraphImages.h"
#include "PioneerQ38ProjectionGraphShape.h"
// PRIVATE fixed L1 save/restore DATA plan,NOT mapping/write/launch authority.
// Runtime buffers are snapshotted conservatively through ac000 for ALL
// participants,including native operands/scales/receipts/output staging.
// ALL120 stacks and completion trailers are separate from code. ROOT adds
// the complete primary/shadow views. No MMIO/NoC/ARC/host DMA/GDDR is L1.
// Owner MUST snapshot EVERYTHING before FIRST write;restore ONLY after
// physical STOP+ALL terminals/hold and known-clean exclusive owner proof.
// UNKNOWN retains every snapshot/mapping,NO reset/read/restore/retry.
struct PioneerQ38ProjectionGraphSpan {uint32_t address,words;};
static inline unsigned PioneerQ38ProjectionGraphSpanCount(unsigned w){return w>=120?0:w?6:8;}
static inline PioneerQ38ProjectionGraphSpan PioneerQ38ProjectionGraphSnapshotSpan(unsigned w,unsigned span){
 if(w>=120 || span>=PioneerQ38ProjectionGraphSpanCount(w))return {UINT32_MAX,0};
 switch(span){
  case 0:return {0x6000,1};
  case 1:return {0x64000,PioneerQ38ProjectionGraphPrefixBytes(w)/4};
  case 2:return {0x6e000,2048}; // ALL120 complete8KiB stacks,not code padding
  case 3:return {0x70000,4096};
  case 4:return {0x74000,57344}; // fixed229376B work view through ac000
  case 5:return {0x17ffc0,8};
  case 6:return {TT_Q38_PROJECTION_GRAPH_MANIFEST,TT_Q38_PROJECTION_GRAPH_WORDS};
  default:return {TT_Q38_PROJECTION_GRAPH_SHADOW,TT_Q38_PROJECTION_GRAPH_WORDS};
 }
}
static inline unsigned PioneerQ38ProjectionGraphSnapshotWords(unsigned w){return w>=120?0:w?67593:90505;}
static inline unsigned PioneerQ38ProjectionGraphSnapshotBase(unsigned w){return w>=120?UINT32_MAX:w?90505+(w-1)*67593:0;}
static inline uint32_t PioneerQ38ProjectionGraphSnapshotAddress(unsigned w,unsigned ordinal){
 if(w>=120 || ordinal>=PioneerQ38ProjectionGraphSnapshotWords(w))return UINT32_MAX;
 for(unsigned i=0;i<PioneerQ38ProjectionGraphSpanCount(w);i++){
  auto s=PioneerQ38ProjectionGraphSnapshotSpan(w,i);if(ordinal<s.words)return s.address+ordinal*4;ordinal-=s.words;
 }
 return UINT32_MAX;
}
static inline bool PioneerQ38ProjectionGraphSnapshotContains(unsigned w,uint32_t a){
 if(w>=120 || (a&3))return false;
 for(unsigned i=0;i<PioneerQ38ProjectionGraphSpanCount(w);i++){
  auto s=PioneerQ38ProjectionGraphSnapshotSpan(w,i);if(a>=s.address && a-s.address<s.words*4)return true;
 }
 return false;
}
static_assert(90505+119*67593==8134072,"ALL120 complete fixed L1 snapshot words,HEAP ONLY");
static_assert(0x74000+57344*4==0xac000 && 0xab000+768<=0xac000,"complete bounded runtime views");
static_assert(TT_Q38_PROJECTION_GRAPH_MANIFEST+TT_Q38_PROJECTION_GRAPH_WORDS*4<=0xc0000
 && TT_Q38_PROJECTION_GRAPH_SHADOW+TT_Q38_PROJECTION_GRAPH_WORDS*4<0x17ffc0,"new manifest/shadow exclude trailers");
#endif
