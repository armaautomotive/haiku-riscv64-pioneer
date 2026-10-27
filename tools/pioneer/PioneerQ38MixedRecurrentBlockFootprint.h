/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_MIXED_RECURRENT_BLOCK_FOOTPRINT_H
#define PIONEER_Q38_MIXED_RECURRENT_BLOCK_FOOTPRINT_H
#include <stdint.h>
// Exact QRB1 + unchanged QMW1 L1 DATA,not a launch/journal/native grant.
// The returning ROOT Conv capsule at64000 needs its OWN saved window;
// the older dense journal does not cover it. Capture arbitrary originals,
// including padding,not reconstructed zeros. NoC/Tensix/GDDR/PCI/host/clock
// lifetimes and independently proved STOP/HOLD/restore/END remain separate.
struct PioneerQ38MixedRecurrentBlockSpan {uint32_t address,words;};
enum {TT_Q38_RECURRENT_BLOCK_ROOT_SAVED_WORDS=316937,TT_Q38_RECURRENT_BLOCK_PEER_SAVED_WORDS=93193,
 TT_Q38_RECURRENT_BLOCK_SAVED_WORDS=11406904,TT_Q38_RECURRENT_BLOCK_ROOT_CODE_WORDS=15025,
 TT_Q38_RECURRENT_BLOCK_PEER_CODE_WORDS=21997,TT_Q38_RECURRENT_BLOCK_CODE_WORDS=2632668};
static inline unsigned PioneerQ38RecurrentBlockSnapshotSpans(unsigned w){return w>=120?0:w?7:6;}
static inline PioneerQ38MixedRecurrentBlockSpan PioneerQ38RecurrentBlockSnapshotSpan(unsigned w,unsigned span){
 if(w>=120 || span>=PioneerQ38RecurrentBlockSnapshotSpans(w))return {UINT32_MAX,0};
 if(!span)return {0x6000,1};
 if(!w){
  switch(span){
   case 1:return {0x40000,32768}; // complete bounded ROOT text window
   case 2:return {0x64000,8192}; // complete separate Conv capsule window
   case 3:return {0x6e000,2048}; // complete8KiB stack
   case 4:return {0x74000,273920}; // ALL runtime/shadows/tables through17f800
   default:return {0x17ffc0,8};
  }
 }
 switch(span){
  case 1:return {0x20000,2048};
  case 2:return {0x40000,4096};
  case 3:return {0x60000,16384};
  case 4:return {0x70000,61440};
  case 5:return {0xb8000,9216};
  default:return {0x17ffc0,8};
 }
}
static inline unsigned PioneerQ38RecurrentBlockSnapshotWords(unsigned w){return w>=120?0:w?TT_Q38_RECURRENT_BLOCK_PEER_SAVED_WORDS:TT_Q38_RECURRENT_BLOCK_ROOT_SAVED_WORDS;}
static inline unsigned PioneerQ38RecurrentBlockSnapshotBase(unsigned w){
 return w>=120?UINT32_MAX:w?TT_Q38_RECURRENT_BLOCK_ROOT_SAVED_WORDS+(w-1)*TT_Q38_RECURRENT_BLOCK_PEER_SAVED_WORDS:0;
}
static inline uint32_t PioneerQ38RecurrentBlockSnapshotAddress(unsigned w,unsigned ordinal){
 if(w>=120 || ordinal>=PioneerQ38RecurrentBlockSnapshotWords(w))return UINT32_MAX;
 for(unsigned i=0;i<PioneerQ38RecurrentBlockSnapshotSpans(w);i++){
  const auto s=PioneerQ38RecurrentBlockSnapshotSpan(w,i);if(ordinal<s.words)return s.address+ordinal*4;ordinal-=s.words;
 }
 return UINT32_MAX;
}
static inline bool PioneerQ38RecurrentBlockSnapshotContains(unsigned w,uint32_t a){
 if(w>=120 || (a&3))return false;
 for(unsigned i=0;i<PioneerQ38RecurrentBlockSnapshotSpans(w);i++){
  const auto s=PioneerQ38RecurrentBlockSnapshotSpan(w,i);if(a>=s.address && a-s.address<s.words*4)return true;
 }
 return false;
}
static inline unsigned PioneerQ38RecurrentBlockCodeSpans(unsigned w){return w>=120?0:w?8:3;}
static inline PioneerQ38MixedRecurrentBlockSpan PioneerQ38RecurrentBlockCodeSpan(unsigned w,unsigned span){
 if(w>=120 || span>=PioneerQ38RecurrentBlockCodeSpans(w))return {UINT32_MAX,0};
 if(!span)return {0x6000,1};
 if(!w)return span==1?PioneerQ38MixedRecurrentBlockSpan{0x40000,13149}:PioneerQ38MixedRecurrentBlockSpan{0x64000,1875};
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
static inline unsigned PioneerQ38RecurrentBlockCodeWords(unsigned w){return w>=120?0:w?TT_Q38_RECURRENT_BLOCK_PEER_CODE_WORDS:TT_Q38_RECURRENT_BLOCK_ROOT_CODE_WORDS;}
static inline uint32_t PioneerQ38RecurrentBlockCodeAddress(unsigned w,unsigned ordinal){
 if(w>=120 || ordinal>=PioneerQ38RecurrentBlockCodeWords(w))return UINT32_MAX;
 for(unsigned i=0;i<PioneerQ38RecurrentBlockCodeSpans(w);i++){
  const auto s=PioneerQ38RecurrentBlockCodeSpan(w,i);if(ordinal<s.words)return s.address+ordinal*4;ordinal-=s.words;
 }
 return UINT32_MAX;
}
static inline bool PioneerQ38RecurrentBlockCodeContains(unsigned w,uint32_t a){
 if(w>=120 || (a&3))return false;
 for(unsigned i=0;i<PioneerQ38RecurrentBlockCodeSpans(w);i++){
  const auto s=PioneerQ38RecurrentBlockCodeSpan(w,i);if(a>=s.address && a-s.address<s.words*4)return true;
 }
 return false; // no zero-filled holes or mutable runtime as executable DATA
}
static_assert(1+32768+8192+2048+273920+8==TT_Q38_RECURRENT_BLOCK_ROOT_SAVED_WORDS,"complete recurrent ROOT originals");
static_assert(1+2048+4096+16384+61440+9216+8==TT_Q38_RECURRENT_BLOCK_PEER_SAVED_WORDS,"unchanged QMW1 originals");
static_assert(TT_Q38_RECURRENT_BLOCK_ROOT_SAVED_WORDS+119*TT_Q38_RECURRENT_BLOCK_PEER_SAVED_WORDS==TT_Q38_RECURRENT_BLOCK_SAVED_WORDS
 && TT_Q38_RECURRENT_BLOCK_SAVED_WORDS*4==45627616,"ALL120 originals HEAP ONLY,not old dense journal capacity");
static_assert(1+13149+1875==TT_Q38_RECURRENT_BLOCK_ROOT_CODE_WORDS
 && 1+1100+2583+4114+2472+1340+1496+8891==TT_Q38_RECURRENT_BLOCK_PEER_CODE_WORDS
 && TT_Q38_RECURRENT_BLOCK_ROOT_CODE_WORDS+119*TT_Q38_RECURRENT_BLOCK_PEER_CODE_WORDS==TT_Q38_RECURRENT_BLOCK_CODE_WORDS,"exact11 immutable sections");
#endif
