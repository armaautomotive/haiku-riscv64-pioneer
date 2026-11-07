/* SPDX-License-Identifier: MIT */
#include "PioneerQ38HybridKernelCompileOnly.h"
#ifndef PIONEER_Q38_HYBRID_TOKEN_FOOTPRINT_H
#define PIONEER_Q38_HYBRID_TOKEN_FOOTPRINT_H
#if !defined(PIONEER_Q38_HYBRID_JOURNAL_DIAGNOSTIC) || PIONEER_Q38_HYBRID_JOURNAL_DIAGNOSTIC != 1 \
 || (defined(_KERNEL_MODE) && !defined(PIONEER_Q38_HYBRID_KERNEL_COMPILE_ONLY)) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) || defined(PIONEER_Q38_FULL_OFFLOAD_CLIENT) \
 || defined(PIONEER_Q38_BF16_FULL_NATIVE) || defined(PIONEER_Q38_BF16_FULL_CLIENT) \
 || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) || defined(PIONEER_DRIVER_BUILD_TAG) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error Hybrid token journal geometry is experimental DATA only, not native or Work authority
#endif
#include "PioneerQ38TokenBf16Footprint.h"

// Conservative QGW5 snapshot proposal. It includes the complete QGW4
// footprint plus BFP4/BFP8 staging, a bounded output guard and the entire
// relocated GDN code reservation. It is not an ownership grant: other users,
// phase overlap, mapping aliases, input lifetime, image identity and native
// failure/restore paths must still be independently qualified.
namespace PioneerQ38HybridTokenL1 {
struct Span {uint32_t address,words;};
enum {Family=0x35505451,Precise=1,Pushed=0,Bf16=1,Workers=120,
 RootSavedWords=PioneerQ38TokenBf16L1::RootSavedWords,PeerSavedWords=286721,
 SavedWords=RootSavedWords+119*PeerSavedWords,SavedBytes=SavedWords*4};
static inline unsigned SnapshotSpans(unsigned worker){return worker>=Workers?0:8;}
static inline Span SnapshotSpan(unsigned worker,unsigned span){
 if(worker>=Workers || span>=SnapshotSpans(worker))return {UINT32_MAX,0};
 if(!worker){const auto s=PioneerQ38TokenBf16L1::SnapshotSpan(0,span);return {s.address,s.words};}
 switch(span){
  case 0:return {0x6000,1};
  case 1:return {0x20000,3072};
  case 2:return {0x30000,16384};
  case 3:return {0x40000,4096};
  case 4:return {0x50000,16384};
  case 5:return {0x60000,16384};
  case 6:return {0x70000,214016}; // through 0x140fff: old spans, staging, output guard
  default:return {0x170000,16384}; // full relocated code and terminal/status, through 0x17ffff
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
static_assert(1+3072+16384+4096+16384+16384+214016+16384==PeerSavedWords,"complete proposed QGW5 peer footprint");
static_assert(SavedWords==34453122 && SavedBytes==137812488,"distinct ALL120 experimental journal, HEAP ONLY");
}
#endif
