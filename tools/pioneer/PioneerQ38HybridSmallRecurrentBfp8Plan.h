/* SPDX-License-Identifier: MIT */
// Offline layout proposal only: no cache bytes, upload, device or Work mode.
#if !defined(PIONEER_Q38_SMALL_RECURRENT_BFP8_PLAN_INSPECTION) || PIONEER_Q38_SMALL_RECURRENT_BFP8_PLAN_INSPECTION != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error Small recurrent BFP8 plan is offline geometry only
#endif
#ifndef PIONEER_Q38_HYBRID_SMALL_RECURRENT_BFP8_PLAN_H
#define PIONEER_Q38_HYBRID_SMALL_RECURRENT_BFP8_PLAN_H
#include "PioneerQ38HybridRecurrentQ8MatrixPlan.h"

// Preserve every other address. The old 48-row allocations cannot contain
// 64 BFP8 rows, so append each padded tensor beyond the prior high-water
// mark of its existing bank. Old small-tensor reservations become holes.
struct PioneerQ38HybridSmallRecurrentBfp8Plan {
 using Parent=PioneerQ38HybridRecurrentQ8MatrixPlan;
 using Proposal=Parent::Proposal;
 enum : unsigned {SmallRecurrentBfp8=12, PaddedRows=64, RowBytes=5440, Bytes=PaddedRows*RowBytes};
 template<class Canonical>static bool Build(const Canonical& source,Proposal& output){
  Proposal parent;if(!Parent::Build(source,parent) || parent.parts.size()!=source.Count())return false;
  Proposal staged=parent;unsigned count=0;bool seen[64][2]={};
  for(unsigned i=0;i<source.Count();i++){
   const auto* p=source.Part(i);if(!p)return false;
   if(p->kind!=Canonical::WeightKind || p->layer>=64 || p->layer%4==3 || (p->role!=10 && p->role!=11))continue;
   if(p->bank>=7)return false;
   const unsigned role=p->role-10;const auto& old=parent.parts[i];auto& part=staged.parts[i];auto& at=staged.used[p->bank];
   if(seen[p->layer][role] || old.bank!=p->bank || old.original!=i
    || p->type!=8 || p->firstRow || p->rows!=48 || p->rowBytes!=5760 || p->bytes!=48*5760
    || old.encoding!=PioneerQ38HybridModelPlan::Original || old.bytes!=p->bytes
    || (at&4095) || at>Canonical::Capacity || Bytes>Canonical::Capacity-at || at>UINT32_MAX-Bytes)return false;
   part.offset=uint32_t(at);part.bytes=Bytes;part.allocated=Bytes;part.encoding=SmallRecurrentBfp8;
   at+=Bytes;seen[p->layer][role]=true;count++;
  }
  for(unsigned l=0;l<64;l++)if(l%4!=3 && (!seen[l][0] || !seen[l][1]))return false;
  if(count!=96)return false;
  output=std::move(staged);return true;
 }
};
#endif
