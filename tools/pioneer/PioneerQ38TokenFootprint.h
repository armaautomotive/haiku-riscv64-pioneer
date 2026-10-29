/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_TOKEN_FOOTPRINT_H
#define PIONEER_Q38_TOKEN_FOOTPRINT_H
#include <stdint.h>
// Full-token ROOT + QGW1 L1 GEOMETRY ONLY. Not a native map,held-core
// journal,source/code authentication,initialization,RUN/STOP or cleanup grant.
// A DISTINCT full-token owner must capture arbitrary originals,including
// untouched padding,before any install and retain them through proved END.
namespace PioneerQ38TokenL1 {
struct Span {uint32_t address,words;};
enum {Workers=120,RootSavedWords=316939,PeerSavedWords=93193,
 SavedWords=RootSavedWords+119*PeerSavedWords,SavedBytes=SavedWords*4};
static inline unsigned SnapshotSpans(unsigned worker){return worker>=Workers?0:7;}
static inline Span SnapshotSpan(unsigned worker,unsigned span){
 if(worker>=Workers || span>=SnapshotSpans(worker))return {UINT32_MAX,0};
 if(!span)return {0x6000,1};
 if(!worker){
  switch(span){
   case 1:return {0x40000,32768}; // entire bounded persistent ROOT text window
   case 2:return {0x60000,2}; // DISTINCT fault+STOP instructions;old prefix omits these
   case 3:return {0x64000,8192}; // entire returning/parent auxiliary window
   case 4:return {0x6e000,2048}; // complete8KiB stack
   case 5:return {0x74000,273920}; // runtime,tables,head,session,record,cancel,shadows
   default:return {0x17ffc0,8};
  }
 }
 switch(span){
  case 1:return {0x20000,2048}; // NEW QGW1,not the older mixed-worker ROM
  case 2:return {0x40000,4096};
  case 3:return {0x60000,16384}; // FFN,service,holes and full peer stack
  case 4:return {0x70000,61440}; // affine,main,descriptors,operands,results
  case 5:return {0xb8000,9216}; // complete GDN capsule+padding
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
static_assert(1+32768+2+8192+2048+273920+8==RootSavedWords,"full token ROOT INCLUDING separate terminal instructions");
static_assert(1+2048+4096+16384+61440+9216+8==PeerSavedWords,"full QGW1 originals,NOT an old participant owner");
static_assert(SavedWords==11406906 && SavedBytes==45627624,"ALL120 arbitrary originals HEAP ONLY;old prefix journal is eight bytes short");
}
#endif
