/* SPDX-License-Identifier: MIT */
// CPU-only DATA timing;NO driver/device/native owner/model or reference IO.
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#define PIONEER_Q38_RECURRENT_PREFIX_PLAN_MIRROR_PROTOTYPE 1
#include "../PioneerQ38RecurrentPrefixPlanMirror.h"
#include <array>
#include <chrono>
#include <cstdio>
#include <memory>
int main(){
 auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();
 auto plan=std::make_unique<PioneerQ38RecurrentPrefixPlan>();auto mirror=std::make_unique<PioneerQ38RecurrentPrefixPlanMirror>();
 std::array<uint32_t,1920> sources;for(unsigned i=0;i<sources.size();i++)sources[i]=0x16005+i*131;
 if(!canonical->Build() || !plan->Build(*canonical,0,{1,2,3,4},sources.data()) || !mirror->Capture(*canonical,*plan))return 1;
 constexpr unsigned fullChecks=3,mirrorChecks=128;
 auto begin=std::chrono::steady_clock::now();
 for(unsigned i=0;i<fullChecks;i++){
  asm volatile(""::"r"(plan.get()),"r"(canonical.get()):"memory");
  if(!plan->Matches(*canonical))return 1;
 }
 auto middle=std::chrono::steady_clock::now();
 for(unsigned i=0;i<mirrorChecks;i++){
  asm volatile(""::"r"(mirror.get()),"r"(plan.get()),"r"(canonical.get()):"memory");
  if(!mirror->Matches(*canonical,*plan))return 1;
 }
 auto end=std::chrono::steady_clock::now();
 auto full=std::chrono::duration_cast<std::chrono::microseconds>(middle-begin).count();
 auto exact=std::chrono::duration_cast<std::chrono::microseconds>(end-middle).count();if(full<0 || exact<0)return 1;
 printf("CPU_METADATA_ONLY full_prefix_checks%u total_us%lld us_per_check%.3f exact_mirror_checks%u total_us%lld us_per_check%.3f canonical_bytes%zu prefix_bytes%zu;NO_GPU_OPEN_OR_NATIVE_OWNER_OR_MODEL_IO_NOT_INFERENCE_OR_TOKEN_RATE\n",
  fullChecks,(long long)full,double(full)/fullChecks,mirrorChecks,(long long)exact,double(exact)/mirrorChecks,sizeof(*canonical),sizeof(*plan));
}
