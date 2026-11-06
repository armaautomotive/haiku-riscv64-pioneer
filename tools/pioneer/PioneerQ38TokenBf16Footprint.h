/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_TOKEN_BF16_FOOTPRINT_H
#define PIONEER_Q38_TOKEN_BF16_FOOTPRINT_H
#include <stdint.h>
// QTP4/QGW4 GEOMETRY ONLY. A fresh exclusive native owner must snapshot
// arbitrary originals,including padding,before installing ANY code/data.
// Old G3 or component ownership does not cover the new ROOT/FPU extents.
namespace PioneerQ38TokenBf16L1 {
struct Span {uint32_t address,words;};
enum {Family=0x34505451,Precise=1,Pushed=0,Bf16=1,Workers=120,FrontBytes=12288,
 RootSavedWords=333323,PeerSavedWords=126985,
 SavedWords=RootSavedWords+119*PeerSavedWords,SavedBytes=SavedWords*4};
static inline unsigned SnapshotSpans(unsigned worker){return worker>=Workers?0:worker?9:8;}
static inline Span SnapshotSpan(unsigned worker,unsigned span){
 if(worker>=Workers || span>=SnapshotSpans(worker))return {UINT32_MAX,0};
 if(!span)return {0x6000,1};
 if(!worker)switch(span){
  case 1:return {0x20000,16384}; // NEW64KiB ROOT BF16 coordinator,not old ROOT ownership
  case 2:return {0x40000,32768};
  case 3:return {0x60000,2};
  case 4:return {0x64000,8192};
  case 5:return {0x6e000,2048};
  case 6:return {0x74000,273920}; // includes direct input e0000 and NEW pair metadata168000..16c200
  default:return {0x17ffc0,8};
 }
 switch(span){
  case 1:return {0x20000,FrontBytes/4};
  case 2:return {0x30000,16384}; // actual returning BF16 producer,not an integer-scale grant
  case 3:return {0x40000,4096};
  case 4:return {0x50000,16384}; // NEW returning cached Q4 FPU capsule
  case 5:return {0x60000,16384};
  case 6:return {0x70000,61440};
  case 7:return {0xb8000,9216};
  default:return {0x17ffc0,8};
 }
}
static inline unsigned SnapshotWords(unsigned worker){return worker>=Workers?0:worker?PeerSavedWords:RootSavedWords;}
static inline unsigned SnapshotBase(unsigned worker){return worker>=Workers?UINT32_MAX:worker?RootSavedWords+(worker-1)*PeerSavedWords:0;}
static inline uint32_t SnapshotAddress(unsigned worker,unsigned ordinal){
 if(worker>=Workers || ordinal>=SnapshotWords(worker))return UINT32_MAX;
 for(unsigned i=0;i<SnapshotSpans(worker);i++){
  const auto s=SnapshotSpan(worker,i);if(ordinal<s.words)return s.address+ordinal*4;ordinal-=s.words;
 }
 return UINT32_MAX;
}
static inline bool SnapshotContains(unsigned worker,uint32_t address){
 if(worker>=Workers || (address&3))return false;
 for(unsigned i=0;i<SnapshotSpans(worker);i++){
  const auto s=SnapshotSpan(worker,i);if(address>=s.address && address-s.address<s.words*4)return true;
 }
 return false;
}
static_assert(1+16384+32768+2+8192+2048+273920+8==RootSavedWords,"QTP4 ROOT journal includes NEW BF16 coordinator");
static_assert(1+3072+16384+4096+16384+16384+61440+9216+8==PeerSavedWords,"QGW4 complete producer and cached FPU originals");
static_assert(SavedWords==15444538 && SavedBytes==61778152,"distinct ALL120 BF16 journal;heap only,not old G3 ownership");
}
#endif
