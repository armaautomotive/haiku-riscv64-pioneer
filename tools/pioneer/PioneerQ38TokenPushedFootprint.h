/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_TOKEN_PUSHED_FOOTPRINT_H
#define PIONEER_Q38_TOKEN_PUSHED_FOOTPRINT_H
#include <stdint.h>
// Proposed QTP3/QGW3 GEOMETRY ONLY, in a distinct namespace. Not selected by
// the existing native owner and not an image, mapping, journal or RUN grant.
// The fixed 9128-byte sender front end exceeds QTP2's 8192-byte journal.
// Save its complete proposed 12KiB window, including arbitrary padding.
namespace PioneerQ38TokenPushedL1 {
struct Span {uint32_t address,words;};
enum {Family=0x33505451,Workers=120,FrontBytes=0x3000,
 RootSavedWords=316939,PeerSavedWords=110601,
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
  case 1:return {0x20000,FrontBytes/4};
  case 2:return {0x30000,16384};
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
static_assert(1+32768+2+8192+2048+273920+8==RootSavedWords,"complete ROOT including receipt slots and terminals");
static_assert(1+3072+16384+4096+16384+61440+9216+8==PeerSavedWords,"QGW3 front window including padding");
static_assert(SavedWords==13478458 && SavedBytes==53913832,"distinct full120 proposed QTP3 journal;HEAP ONLY");
}
#endif
