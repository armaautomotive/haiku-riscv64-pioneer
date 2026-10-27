/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_MIXED_LIVE_FOOTPRINT_H
#define PIONEER_Q38_MIXED_LIVE_FOOTPRINT_H
#include <stdint.h>
// NEW full fixed L1 geometry for QBL1 + QMW1. Not an old native journal,
// map, initialized-state, code/source/clock/host or execution permission.
// Every arbitrary original WORD (including UINT32_MAX) must be captured
// before any install and retained until independently proved STOP/HOLD,
// complete restore/second proof and genuine native END. L1 ONLY: actual
// NoC/Tensix/PCI/host/GDDR ownership and journals are separately required.
struct PioneerQ38MixedLiveSpan {uint32_t address,words;};
enum {TT_Q38_MIXED_LIVE_ROOT_SAVED_WORDS=308745,TT_Q38_MIXED_LIVE_PEER_SAVED_WORDS=93193,
 TT_Q38_MIXED_LIVE_SAVED_WORDS=11398712,TT_Q38_MIXED_LIVE_ROOT_CODE_WORDS=12657,
 TT_Q38_MIXED_LIVE_PEER_CODE_WORDS=21997,TT_Q38_MIXED_LIVE_CODE_WORDS=2630300};
static inline unsigned PioneerQ38MixedLiveSnapshotSpans(unsigned w){return w>=120?0:w?7:5;}
static inline PioneerQ38MixedLiveSpan PioneerQ38MixedLiveSnapshotSpan(unsigned w,unsigned span){
 if(w>=120 || span>=PioneerQ38MixedLiveSnapshotSpans(w))return {UINT32_MAX,0};
 if(!span)return {0x6000,1};
 if(!w){
  switch(span){
   case 1:return {0x40000,32768}; // entire bounded new ROOT code window
   case 2:return {0x6e000,2048}; // complete stack,not reconstructed zeros
   case 3:return {0x74000,273920}; // through17f800: ALL root runtime/input/shadows/cohort mailboxes
   default:return {0x17ffc0,8};
  }
 }
 switch(span){
  case 1:return {0x20000,2048}; // NEW mixed front end + untouched padding
  case 2:return {0x40000,4096}; // NEW returning dense capsule
  case 3:return {0x60000,16384}; // new FFN+output helper,holes,and full8KiB stack
  case 4:return {0x70000,61440}; // affine/service/runtime/operands/results through ac000
  case 5:return {0xb8000,9216}; // complete GDN capsule+padding through c1000
  default:return {0x17ffc0,8};
 }
}
static inline unsigned PioneerQ38MixedLiveSnapshotWords(unsigned w){return w>=120?0:w?TT_Q38_MIXED_LIVE_PEER_SAVED_WORDS:TT_Q38_MIXED_LIVE_ROOT_SAVED_WORDS;}
static inline unsigned PioneerQ38MixedLiveSnapshotBase(unsigned w){return w>=120?UINT32_MAX:w?TT_Q38_MIXED_LIVE_ROOT_SAVED_WORDS+(w-1)*TT_Q38_MIXED_LIVE_PEER_SAVED_WORDS:0;}
static inline uint32_t PioneerQ38MixedLiveSnapshotAddress(unsigned w,unsigned ordinal){
 if(w>=120 || ordinal>=PioneerQ38MixedLiveSnapshotWords(w))return UINT32_MAX;
 for(unsigned i=0;i<PioneerQ38MixedLiveSnapshotSpans(w);i++){
  const auto s=PioneerQ38MixedLiveSnapshotSpan(w,i);if(ordinal<s.words)return s.address+ordinal*4;ordinal-=s.words;
 }
 return UINT32_MAX;
}
static inline bool PioneerQ38MixedLiveSnapshotContains(unsigned w,uint32_t address){
 if(w>=120 || (address&3))return false;
 for(unsigned i=0;i<PioneerQ38MixedLiveSnapshotSpans(w);i++){
  const auto s=PioneerQ38MixedLiveSnapshotSpan(w,i);if(address>=s.address && address-s.address<s.words*4)return true;
 }
 return false;
}
static inline unsigned PioneerQ38MixedLiveCodeSpans(unsigned w){return w>=120?0:w?8:2;}
static inline PioneerQ38MixedLiveSpan PioneerQ38MixedLiveCodeSpan(unsigned w,unsigned span){
 if(w>=120 || span>=PioneerQ38MixedLiveCodeSpans(w))return {UINT32_MAX,0};
 if(!span)return {0x6000,1};
 if(!w)return {0x40000,12656};
 switch(span){
  case 1:return {0x20000,1100};
  case 2:return {0x40000,2583};
  case 3:return {0x60000,4114};
  case 4:return {0x68000,2472};
  case 5:return {0x70b00,1340};
  case 6:return {0x72000,1496};
  default:return {0xb8000,8891};
 }
}
static inline unsigned PioneerQ38MixedLiveCodeWords(unsigned w){return w>=120?0:w?TT_Q38_MIXED_LIVE_PEER_CODE_WORDS:TT_Q38_MIXED_LIVE_ROOT_CODE_WORDS;}
static inline uint32_t PioneerQ38MixedLiveCodeAddress(unsigned w,unsigned ordinal){
 if(w>=120 || ordinal>=PioneerQ38MixedLiveCodeWords(w))return UINT32_MAX;
 for(unsigned i=0;i<PioneerQ38MixedLiveCodeSpans(w);i++){
  const auto s=PioneerQ38MixedLiveCodeSpan(w,i);if(ordinal<s.words)return s.address+ordinal*4;ordinal-=s.words;
 }
 return UINT32_MAX;
}
static inline bool PioneerQ38MixedLiveCodeContains(unsigned w,uint32_t address){
 if(w>=120 || (address&3))return false;
 for(unsigned i=0;i<PioneerQ38MixedLiveCodeSpans(w);i++){
  const auto s=PioneerQ38MixedLiveCodeSpan(w,i);if(address>=s.address && address-s.address<s.words*4)return true;
 }
 return false; // NO implicit zero-padded holes or private-runtime permission
}
static_assert(1+32768+2048+273920+8==TT_Q38_MIXED_LIVE_ROOT_SAVED_WORDS,"entire new ROOT footprint");
static_assert(1+2048+4096+16384+61440+9216+8==TT_Q38_MIXED_LIVE_PEER_SAVED_WORDS,"entire new QMW1 footprint,NOT the old peer snapshot");
static_assert(TT_Q38_MIXED_LIVE_ROOT_SAVED_WORDS+119*TT_Q38_MIXED_LIVE_PEER_SAVED_WORDS==TT_Q38_MIXED_LIVE_SAVED_WORDS
 && TT_Q38_MIXED_LIVE_SAVED_WORDS*4==45594848,"ALL120 arbitrary originals HEAP ONLY");
static_assert(1+1100+2583+4114+2472+1340+1496+8891==TT_Q38_MIXED_LIVE_PEER_CODE_WORDS
 && TT_Q38_MIXED_LIVE_ROOT_CODE_WORDS+119*TT_Q38_MIXED_LIVE_PEER_CODE_WORDS==TT_Q38_MIXED_LIVE_CODE_WORDS,"exact NEW code,not padding");
#endif
