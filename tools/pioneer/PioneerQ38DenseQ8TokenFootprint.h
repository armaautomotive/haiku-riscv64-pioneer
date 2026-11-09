/* SPDX-License-Identifier: MIT */
// Geometry proposal only: not a native journal selector or permission to use
// the worker. Its separate owner must prove aliases, exclusive lifetime,
// complete save/restore, image identity and failure handling before launch.
#if !defined(PIONEER_Q38_DENSE_Q8_MATRIX_FOOTPRINT_INSPECTION) || PIONEER_Q38_DENSE_Q8_MATRIX_FOOTPRINT_INSPECTION != 1 \
 || !defined(PIONEER_Q38_HYBRID_JOURNAL_DIAGNOSTIC) || PIONEER_Q38_HYBRID_JOURNAL_DIAGNOSTIC != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_HYBRID_FULL_NATIVE) || defined(PIONEER_Q38_HYBRID_FULL_CLIENT) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) || defined(PIONEER_Q38_FULL_OFFLOAD_CLIENT) \
 || defined(PIONEER_Q38_BF16_FULL_NATIVE) || defined(PIONEER_Q38_BF16_FULL_CLIENT) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error Dense Q8 worker footprint is isolated offline inspection only
#endif
#ifndef PIONEER_Q38_DENSE_Q8_TOKEN_FOOTPRINT_H
#define PIONEER_Q38_DENSE_Q8_TOKEN_FOOTPRINT_H
#include "PioneerQ38HybridTokenFootprint.h"
namespace PioneerQ38DenseQ8TokenL1 {
namespace Old=PioneerQ38HybridTokenL1;using Span=Old::Span;
enum : unsigned {Family=0x42505451,Precise=Old::Precise,Pushed=Old::Pushed,Bf16=Old::Bf16,
 Workers=Old::Workers,RootSavedWords=Old::RootSavedWords,
 PeerSavedWords=Old::PeerSavedWords+1024,SavedWords=RootSavedWords+119*PeerSavedWords,SavedBytes=SavedWords*4};
static inline unsigned SnapshotSpans(unsigned worker){return Old::SnapshotSpans(worker);}
static inline Span SnapshotSpan(unsigned worker,unsigned span){
 auto s=Old::SnapshotSpan(worker,span);if(worker && worker<Workers && span==1)s.words+=1024;return s;
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
static_assert(PeerSavedWords==287745 && SavedWords==34574978 && SavedBytes==138299912,"separate QGWB journal size");
}
#endif
