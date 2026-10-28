/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_RECURRENT_PREFIX_FOOTPRINT_H
#define PIONEER_Q38_RECURRENT_PREFIX_FOOTPRINT_H
#include "PioneerQ38MixedRecurrentBlockFootprint.h"
// QPX1 has DIFFERENT ROOT and Conv bytes; no old program/image/owner alias.
// Its bounded L1 ORIGINAL-SNAPSHOT geometry happens to fit the same windows.
// These are immutable DATA only, never permission to reuse an old journal.
struct PioneerQ38RecurrentPrefixSpan {uint32_t address,words;};
enum {TT_Q38_RECURRENT_PREFIX_ROOT_SAVED_WORDS=316937,TT_Q38_RECURRENT_PREFIX_PEER_SAVED_WORDS=93193,
 TT_Q38_RECURRENT_PREFIX_SAVED_WORDS=11406904,TT_Q38_RECURRENT_PREFIX_ROOT_CODE_WORDS=15972,
 TT_Q38_RECURRENT_PREFIX_PEER_CODE_WORDS=21997,TT_Q38_RECURRENT_PREFIX_CODE_WORDS=2633615};
static inline unsigned PioneerQ38RecurrentPrefixSnapshotSpans(unsigned w){return PioneerQ38RecurrentBlockSnapshotSpans(w);}
static inline PioneerQ38RecurrentPrefixSpan PioneerQ38RecurrentPrefixSnapshotSpan(unsigned w,unsigned span){
 const auto s=PioneerQ38RecurrentBlockSnapshotSpan(w,span);return {s.address,s.words};
}
static inline unsigned PioneerQ38RecurrentPrefixSnapshotWords(unsigned w){return w>=120?0:w?TT_Q38_RECURRENT_PREFIX_PEER_SAVED_WORDS:TT_Q38_RECURRENT_PREFIX_ROOT_SAVED_WORDS;}
static inline unsigned PioneerQ38RecurrentPrefixSnapshotBase(unsigned w){return w>=120?UINT32_MAX:w?TT_Q38_RECURRENT_PREFIX_ROOT_SAVED_WORDS+(w-1)*TT_Q38_RECURRENT_PREFIX_PEER_SAVED_WORDS:0;}
static inline uint32_t PioneerQ38RecurrentPrefixSnapshotAddress(unsigned w,unsigned ordinal){return PioneerQ38RecurrentBlockSnapshotAddress(w,ordinal);}
static inline bool PioneerQ38RecurrentPrefixSnapshotContains(unsigned w,uint32_t address){return PioneerQ38RecurrentBlockSnapshotContains(w,address);}
static inline unsigned PioneerQ38RecurrentPrefixCodeSpans(unsigned w){return w>=120?0:w?8:3;}
static inline PioneerQ38RecurrentPrefixSpan PioneerQ38RecurrentPrefixCodeSpan(unsigned w,unsigned span){
 if(w>=120 || span>=PioneerQ38RecurrentPrefixCodeSpans(w))return {UINT32_MAX,0};
 if(!w)return span==0?PioneerQ38RecurrentPrefixSpan{0x6000,1}:span==1?PioneerQ38RecurrentPrefixSpan{0x40000,14096}:PioneerQ38RecurrentPrefixSpan{0x64000,1875};
 const auto s=PioneerQ38RecurrentBlockCodeSpan(w,span);return {s.address,s.words};
}
static inline unsigned PioneerQ38RecurrentPrefixCodeWords(unsigned w){return w>=120?0:w?TT_Q38_RECURRENT_PREFIX_PEER_CODE_WORDS:TT_Q38_RECURRENT_PREFIX_ROOT_CODE_WORDS;}
static inline uint32_t PioneerQ38RecurrentPrefixCodeAddress(unsigned w,unsigned ordinal){
 if(w>=120 || ordinal>=PioneerQ38RecurrentPrefixCodeWords(w))return UINT32_MAX;
 for(unsigned i=0;i<PioneerQ38RecurrentPrefixCodeSpans(w);i++){const auto s=PioneerQ38RecurrentPrefixCodeSpan(w,i);
  if(ordinal<s.words)return s.address+ordinal*4;
  ordinal-=s.words;
 }
 return UINT32_MAX;
}
static inline bool PioneerQ38RecurrentPrefixCodeContains(unsigned w,uint32_t address){
 if(w>=120 || (address&3))return false;
 for(unsigned i=0;i<PioneerQ38RecurrentPrefixCodeSpans(w);i++){const auto s=PioneerQ38RecurrentPrefixCodeSpan(w,i);
  if(address>=s.address && address-s.address<s.words*4)return true;
 }
 return false;
}
static_assert(1+14096+1875==TT_Q38_RECURRENT_PREFIX_ROOT_CODE_WORDS
 && TT_Q38_RECURRENT_PREFIX_ROOT_CODE_WORDS+119*TT_Q38_RECURRENT_PREFIX_PEER_CODE_WORDS==TT_Q38_RECURRENT_PREFIX_CODE_WORDS,
 "complete distinct QPX1 ROOT+Conv and unchanged QMW1 sections");
static_assert(TT_Q38_RECURRENT_PREFIX_ROOT_SAVED_WORDS+119*TT_Q38_RECURRENT_PREFIX_PEER_SAVED_WORDS==TT_Q38_RECURRENT_PREFIX_SAVED_WORDS,
 "complete original snapshot geometry, not an old owner permission");
#endif
