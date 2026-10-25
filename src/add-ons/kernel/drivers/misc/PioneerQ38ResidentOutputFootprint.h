/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_RESIDENT_OUTPUT_FOOTPRINT_H
#define PIONEER_Q38_RESIDENT_OUTPUT_FOOTPRINT_H
#include "PioneerQ38ResidentOutputCohortShape.h"
// Fixed L1 SNAPSHOT GEOMETRY ONLY, not mapping/write/launch/restore authority.
// A genuine joint native owner must capture ALL originals before ANY write,
// retain code/model/source/init/clock/ALL120 ownership across execution,
// prove unheld terminal/STOP before hold, and independently restore/END.
// UNKNOWN retains every owner and snapshot; no reset/read/restore/retry.
// Conservative code padding contains arbitrary originals, NEVER rebuilt0.
struct PioneerQ38ResidentOutputSpan {uint32_t address,words;};
enum {TT_Q38_RESIDENT_OUTPUT_PEER_SAVED_WORDS=90121,
 TT_Q38_RESIDENT_OUTPUT_ROOT_EXTRA_WORDS=(0x14da00-0xc8000)/4,
 TT_Q38_RESIDENT_OUTPUT_ROOT_SAVED_WORDS=TT_Q38_RESIDENT_OUTPUT_PEER_SAVED_WORDS+TT_Q38_RESIDENT_OUTPUT_ROOT_EXTRA_WORDS,
 TT_Q38_RESIDENT_OUTPUT_SAVED_WORDS=TT_Q38_RESIDENT_OUTPUT_ROOT_SAVED_WORDS+119*TT_Q38_RESIDENT_OUTPUT_PEER_SAVED_WORDS,
 TT_Q38_RESIDENT_OUTPUT_ROOT_CODE_WORDS=17858,TT_Q38_RESIDENT_OUTPUT_PEER_CODE_WORDS=18838,
 TT_Q38_RESIDENT_OUTPUT_CODE_WORDS=TT_Q38_RESIDENT_OUTPUT_ROOT_CODE_WORDS+119*TT_Q38_RESIDENT_OUTPUT_PEER_CODE_WORDS};
static inline unsigned PioneerQ38ResidentOutputSpanCount(unsigned w){return w>=120?0:w?7:8;}
static inline PioneerQ38ResidentOutputSpan PioneerQ38ResidentOutputSnapshotSpan(unsigned w,unsigned span){
 if(w>=120 || span>=PioneerQ38ResidentOutputSpanCount(w))return {UINT32_MAX,0};
 switch(span){
  case 0:return {0x6000,1};
  case 1:return {0x64000,10240}; // FULL40KiB code/gaps through6e000.
  case 2:return {0x6e000,2048}; // ALL120 complete8KiB stacks.
  case 3:return {0x70000,4096}; // complete old operand/immutable-prefix view.
  case 4:return {0x74000,57344}; // ALL runtime through ac000, including GDN.
  case 5:return {0xb8000,16384}; // NEW full64KiB ROM/gaps for ALL120.
  case 6:if(!w)return {0xc8000,TT_Q38_RESIDENT_OUTPUT_ROOT_EXTRA_WORDS};
   return {0x17ffc0,8};
  default:return {0x17ffc0,8};
 }
}
static inline unsigned PioneerQ38ResidentOutputSnapshotWords(unsigned w){
 return w>=120?0:w?TT_Q38_RESIDENT_OUTPUT_PEER_SAVED_WORDS:TT_Q38_RESIDENT_OUTPUT_ROOT_SAVED_WORDS;
}
static inline unsigned PioneerQ38ResidentOutputSnapshotBase(unsigned w){
 return w>=120?UINT32_MAX:w?TT_Q38_RESIDENT_OUTPUT_ROOT_SAVED_WORDS+(w-1)*TT_Q38_RESIDENT_OUTPUT_PEER_SAVED_WORDS:0;
}
static inline uint32_t PioneerQ38ResidentOutputSnapshotAddress(unsigned w,unsigned ordinal){
 if(w>=120 || ordinal>=PioneerQ38ResidentOutputSnapshotWords(w))return UINT32_MAX;
 for(unsigned i=0;i<PioneerQ38ResidentOutputSpanCount(w);i++){
  auto s=PioneerQ38ResidentOutputSnapshotSpan(w,i);if(ordinal<s.words)return s.address+ordinal*4;ordinal-=s.words;
 }
 return UINT32_MAX;
}
static inline bool PioneerQ38ResidentOutputSnapshotContains(unsigned w,uint32_t a){
 if(w>=120 || (a&3))return false;
 for(unsigned i=0;i<PioneerQ38ResidentOutputSpanCount(w);i++){
  auto s=PioneerQ38ResidentOutputSnapshotSpan(w,i);if(a>=s.address && a-s.address<s.words*4)return true;
 }
 return false;
}
// ONLY these complete emitted code extents may be immutable program DATA.
// This enumeration grants NO installer or authority to write their gaps.
static inline unsigned PioneerQ38ResidentOutputCodeSpanCount(unsigned w){return w>=120?0:w?6:5;}
static inline PioneerQ38ResidentOutputSpan PioneerQ38ResidentOutputCodeSpan(unsigned w,unsigned span){
 if(w>=120 || span>=PioneerQ38ResidentOutputCodeSpanCount(w))return {UINT32_MAX,0};
 if(!span)return {0x6000,1};
 if(!w){
  switch(span){
   case 1:return {0x64000,1618};
   case 2:return {0x68000,3534};
   case 3:return {0x72000,32};
   default:return {0xb8000,12673};
  }
 }
 switch(span){
  case 1:return {0x64000,4088};
  case 2:return {0x68000,2575};
  case 3:return {0x70b00,1340};
  case 4:return {0x72000,1985};
  default:return {0xb8000,8849};
 }
}
static inline unsigned PioneerQ38ResidentOutputCodeWords(unsigned w){
 return w>=120?0:w?TT_Q38_RESIDENT_OUTPUT_PEER_CODE_WORDS:TT_Q38_RESIDENT_OUTPUT_ROOT_CODE_WORDS;
}
static inline bool PioneerQ38ResidentOutputProgramImmutable(unsigned w,uint32_t a){
 if(w>=120 || (a&3))return false;
 for(unsigned i=0;i<PioneerQ38ResidentOutputCodeSpanCount(w);i++){
  auto s=PioneerQ38ResidentOutputCodeSpan(w,i);if(a>=s.address && a-s.address<s.words*4)return true;
 }
 return false;
}
static_assert(TT_Q38_RESIDENT_OUTPUT_PEER_SAVED_WORDS==1+10240+2048+4096+57344+16384+8,"all arbitrary peer originals,not just nonzero code");
static_assert(TT_Q38_RESIDENT_OUTPUT_SAVED_WORDS==10951352 && TT_Q38_RESIDENT_OUTPUT_SAVED_WORDS*4==43805408,"NEW ALL120 snapshot,HEAP ONLY;not old grant");
static_assert(TT_Q38_OUTPUT_COHORT_SHADOW+TT_Q38_OUTPUT_COHORT_WORDS*4==0x14da00 && 0x14da00<0x17ffc0,"entire manifest/shadow/counters/stats/post context before trailer");
static_assert(0xb8000+16384*4==0xc8000 && 0x74000+57344*4==0xac000,"closed non-overlapping L1-only geometry");
#endif
