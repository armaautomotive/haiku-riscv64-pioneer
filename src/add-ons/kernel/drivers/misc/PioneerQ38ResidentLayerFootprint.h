/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_RESIDENT_LAYER_FOOTPRINT_H
#define PIONEER_Q38_RESIDENT_LAYER_FOOTPRINT_H
#include "PioneerQ38ResidentLayerShape.h"
// NEW snapshot/code GEOMETRY ONLY. This is NOT an expanded old68 owner,
// mapping,write,restore or launch capability. ALL arbitrary original words
// must be captured before ANY mutation and retained through independently
// proved terminal/STOP/hold/restore/END. UNKNOWN retains every resource.
struct PioneerQ38ResidentLayerSpan {uint32_t address,words;};
enum {TT_Q38_RESIDENT_LAYER_PEER_SAVED_WORDS=90121,
 TT_Q38_RESIDENT_LAYER_ROOT_EXTRA_WORDS=(TT_Q38_RESIDENT_LAYER_ROM_END-0xc8000)/4,
 TT_Q38_RESIDENT_LAYER_ROOT_SAVED_WORDS=TT_Q38_RESIDENT_LAYER_PEER_SAVED_WORDS+TT_Q38_RESIDENT_LAYER_ROOT_EXTRA_WORDS,
 TT_Q38_RESIDENT_LAYER_SAVED_WORDS=TT_Q38_RESIDENT_LAYER_ROOT_SAVED_WORDS+119*TT_Q38_RESIDENT_LAYER_PEER_SAVED_WORDS,
 TT_Q38_RESIDENT_LAYER_ROOT_CODE_WORDS=28643,TT_Q38_RESIDENT_LAYER_PEER_CODE_WORDS=18838,
 TT_Q38_RESIDENT_LAYER_CODE_WORDS=TT_Q38_RESIDENT_LAYER_ROOT_CODE_WORDS+119*TT_Q38_RESIDENT_LAYER_PEER_CODE_WORDS};
static inline unsigned PioneerQ38ResidentLayerSpanCount(unsigned w){return w>=120?0:w?7:8;}
static inline PioneerQ38ResidentLayerSpan PioneerQ38ResidentLayerSnapshotSpan(unsigned w,unsigned span){
 if(w>=120 || span>=PioneerQ38ResidentLayerSpanCount(w))return {UINT32_MAX,0};
 switch(span){
  case 0:return {0x6000,1};
  case 1:return {0x64000,10240}; // ALL40KiB old+new code/gaps before stack.
  case 2:return {0x6e000,2048}; // ALL120 complete8KiB stacks.
  case 3:return {0x70000,4096};
  case 4:return {0x74000,57344}; // Complete runtime,norm+FFN buffers too.
  case 5:return {0xb8000,16384}; // Complete64KiB cohort/GDN ROM+gaps.
  case 6:if(!w)return {0xc8000,TT_Q38_RESIDENT_LAYER_ROOT_EXTRA_WORDS};
   return {0x17ffc0,8};
  default:return {0x17ffc0,8};
 }
}
static inline unsigned PioneerQ38ResidentLayerSnapshotWords(unsigned w){
 return w>=120?0:w?TT_Q38_RESIDENT_LAYER_PEER_SAVED_WORDS:TT_Q38_RESIDENT_LAYER_ROOT_SAVED_WORDS;
}
static inline unsigned PioneerQ38ResidentLayerSnapshotBase(unsigned w){
 return w>=120?UINT32_MAX:w?TT_Q38_RESIDENT_LAYER_ROOT_SAVED_WORDS+(w-1)*TT_Q38_RESIDENT_LAYER_PEER_SAVED_WORDS:0;
}
static inline uint32_t PioneerQ38ResidentLayerSnapshotAddress(unsigned w,unsigned ordinal){
 if(w>=120 || ordinal>=PioneerQ38ResidentLayerSnapshotWords(w))return UINT32_MAX;
 for(unsigned i=0;i<PioneerQ38ResidentLayerSpanCount(w);i++){
  auto s=PioneerQ38ResidentLayerSnapshotSpan(w,i);if(ordinal<s.words)return s.address+ordinal*4;ordinal-=s.words;
 }
 return UINT32_MAX;
}
static inline bool PioneerQ38ResidentLayerSnapshotContains(unsigned w,uint32_t a){
 if(w>=120 || (a&3))return false;
 for(unsigned i=0;i<PioneerQ38ResidentLayerSpanCount(w);i++){
  auto s=PioneerQ38ResidentLayerSnapshotSpan(w,i);if(a>=s.address && a-s.address<s.words*4)return true;
 }
 return false;
}
static inline unsigned PioneerQ38ResidentLayerCodeSpanCount(unsigned w){return w>=120?0:w?6:7;}
static inline PioneerQ38ResidentLayerSpan PioneerQ38ResidentLayerCodeSpan(unsigned w,unsigned span){
 if(w>=120 || span>=PioneerQ38ResidentLayerCodeSpanCount(w))return {UINT32_MAX,0};
 if(!span)return {0x6000,1};
 if(!w){
  switch(span){
   case 1:return {0x64000,1618};
   case 2:return {0x68000,3534};
   case 3:return {0x72000,32};
   case 4:return {0xb8000,14483};
   case 5:return {TT_Q38_RESIDENT_LAYER_NORM_ROM,1351};
   default:return {TT_Q38_RESIDENT_LAYER_FFN_ROM,7624};
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
static inline unsigned PioneerQ38ResidentLayerCodeWords(unsigned w){
 return w>=120?0:w?TT_Q38_RESIDENT_LAYER_PEER_CODE_WORDS:TT_Q38_RESIDENT_LAYER_ROOT_CODE_WORDS;
}
static inline bool PioneerQ38ResidentLayerProgramImmutable(unsigned w,uint32_t a){
 if(w>=120 || (a&3))return false;
 for(unsigned i=0;i<PioneerQ38ResidentLayerCodeSpanCount(w);i++){
  auto s=PioneerQ38ResidentLayerCodeSpan(w,i);if(a>=s.address && a-s.address<s.words*4)return true;
 }
 return false;
}
static_assert(TT_Q38_RESIDENT_LAYER_PEER_SAVED_WORDS==1+10240+2048+4096+57344+16384+8,"all arbitrary originals,not rebuilt zeros");
static_assert(TT_Q38_RESIDENT_LAYER_SAVED_WORDS==11000888 && TT_Q38_RESIDENT_LAYER_SAVED_WORDS*4==44003552,"NEW ALL120 snapshot HEAP ONLY;NEVER old68 grant");
static_assert(TT_Q38_RESIDENT_LAYER_CODE_WORDS==2270365,"ALL7 exact ROOT/ALL6 exact peer extents,not padded code permission");
static_assert(0xb8000+16384*4==0xc8000 && TT_Q38_RESIDENT_LAYER_ROM_END<0x17ffc0,"all complete nonoverlapping root originals through new ROMs");
#endif
