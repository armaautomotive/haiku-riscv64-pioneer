/* SPDX-License-Identifier: MIT */
// Complete QTP2/QGW2 journal GEOMETRY ONLY;no native,source or launch grant.
#if !defined(PIONEER_Q38_TOKEN_PRECISE_PROGRAMS) || PIONEER_Q38_TOKEN_PRECISE_PROGRAMS != 1
#error Exact QTP2 footprint DATA selection required
#endif
#ifndef PIONEER_Q38_TOKEN_PRECISE_FOOTPRINT_H
#define PIONEER_Q38_TOKEN_PRECISE_FOOTPRINT_H
#include <stdint.h>
namespace PioneerQ38TokenL1 {
struct Span {uint32_t address,words;};
enum {Precise=1,Workers=120,RootSavedWords=316939,PeerSavedWords=109577,
 SavedWords=RootSavedWords+119*PeerSavedWords,SavedBytes=SavedWords*4};
static inline unsigned SnapshotSpans(unsigned worker){return worker>=Workers?0:worker?8:7;}
static inline Span SnapshotSpan(unsigned worker,unsigned span){
 if(worker>=Workers || span>=SnapshotSpans(worker))return {UINT32_MAX,0};
 if(!span)return {0x6000,1};
 if(!worker)switch(span){
  case 1:return {0x40000,32768};
  case 2:return {0x60000,2};
  case 3:return {0x64000,8192};
  case 4:return {0x6e000,2048};
  case 5:return {0x74000,273920};
  default:return {0x17ffc0,8};
 }
 switch(span){
  case 1:return {0x20000,2048};
  case 2:return {0x30000,16384}; // ENTIRE bounded64KiB QPA1 capsule+padding
  case 3:return {0x40000,4096};
  case 4:return {0x60000,16384};
  case 5:return {0x70000,61440};
  case 6:return {0xb8000,9216};
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
static_assert(1+32768+2+8192+2048+273920+8==RootSavedWords,"unchanged complete ROOT window");
static_assert(1+2048+16384+4096+16384+61440+9216+8==PeerSavedWords,"complete QGW2 originals including precise ROM padding");
static_assert(SavedWords==13356602 && SavedBytes==53426408,"distinct full120 QTP2 journal HEAP ONLY");
}
#endif
