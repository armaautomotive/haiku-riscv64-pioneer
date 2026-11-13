/* SPDX-License-Identifier: MIT */
// Separate host-only physical geometry observer, not device authority.
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) || defined(PIONEER_Q38_FULL_OFFLOAD_CLIENT) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error Synthetic recurrent address observer is host-only
#endif
#define PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION 1
#define PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION 1
#define PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT 1
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#define PIONEER_Q38_RECURRENT_Q8_MATRIX_PLAN_INSPECTION 1
#define PIONEER_Q38_DENSE_Q8_MATRIX_PLAN_INSPECTION 1
#define PIONEER_Q38_SMALL_RECURRENT_BFP8_PLAN_INSPECTION 1
#include "../PioneerQ38HybridSmallRecurrentBfp8Plan.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38CanonicalModelPlan.h"
#include <memory>
bool Q38SyntheticRecurrentAddress(unsigned& bank,unsigned& address){
 auto c=std::make_unique<PioneerQ38CanonicalModelPlan>();PioneerQ38HybridSmallRecurrentBfp8Plan::Proposal p;
 if(!c->Build() || !PioneerQ38HybridSmallRecurrentBfp8Plan::Build(*c,p))return false;
 unsigned found=0,b=0,a=0;
 for(unsigned i=0;i<c->Count();i++){
  const auto* part=c->Part(i);if(part->kind!=c->WeightKind || part->layer!=0 || part->role!=10)continue;
  const auto& q=p.parts[i];if(++found!=1 || q.bytes!=348160 || q.bank>=7 || q.offset>UINT32_MAX-c->Base)return false;
  b=q.bank;a=c->Base+q.offset;
 }
 if(found!=1)return false;bank=b;address=a;return true;
}
