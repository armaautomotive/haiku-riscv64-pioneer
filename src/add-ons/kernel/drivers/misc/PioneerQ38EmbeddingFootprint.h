/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_EMBEDDING_FOOTPRINT_H
#define PIONEER_Q38_EMBEDDING_FOOTPRINT_H
#include "PioneerQ38EmbeddingCode.h"
// Full ROOT-only retained footprint. All119 other cores remain held and
// untouched. QPE1 never accesses QPR1 b/c manifests;do not claim their maps.
// Same first6 spans as full projection memory,NOT that held-only lifecycle.
struct PioneerQ38EmbeddingSpan {uint32_t address,words;};
static inline PioneerQ38EmbeddingSpan PioneerQ38EmbeddingSnapshotSpan(unsigned i){
 switch(i){case 0:return {0x6000,1};case 1:return {0x64000,10240};case 2:return {0x6e000,2048};
  case 3:return {0x70000,4096};case 4:return {0x74000,57344};case 5:return {0x17ffc0,8};default:return {UINT32_MAX,0};}
}
static inline uint32_t PioneerQ38EmbeddingSnapshotAddress(unsigned word){
 for(unsigned i=0;i<6;i++){auto s=PioneerQ38EmbeddingSnapshotSpan(i);if(word<s.words)return s.address+word*4;word-=s.words;}
 return UINT32_MAX;
}
static inline bool PioneerQ38EmbeddingSnapshotContains(uint32_t a){
 if(a&3)return false;
 for(unsigned i=0;i<6;i++){auto s=PioneerQ38EmbeddingSnapshotSpan(i);if(a>=s.address && a-s.address<s.words*4)return true;}
 return false;
}
static inline uint32_t PioneerQ38EmbeddingProgramAddress(unsigned word){return word>10240?UINT32_MAX:word?0x64000+(word-1)*4:0x6000;}
static inline uint32_t PioneerQ38EmbeddingProgramWord(unsigned word){return word>10240?UINT32_MAX:!word?0x0005e06f:word<=964?PioneerQ38EmbeddingCode[word-1]:0;}
static_assert(1+10240+2048+4096+57344+8==73737,"complete ROOT snapshot HEAP ONLY");
#endif
