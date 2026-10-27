/* SPDX-License-Identifier: MIT */
// CPU-only immutable DATA timing. No device/driver/native owner or model IO.
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#include "../PioneerQ38MixedRecurrentBlockPlan.h"
#include <array>
#include <chrono>
#include <cstdio>
#include <memory>
int main(){
 auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();
 auto plan=std::make_unique<PioneerQ38MixedRecurrentBlockPlan>();
 std::array<uint32_t,1920> sources;for(unsigned i=0;i<sources.size();i++)sources[i]=0x16005+i*17;
 if(!canonical->Build())return 1;
 const auto* workspace=canonical->Workspace(0);if(!workspace)return 1;
 if(!plan->Build(*canonical,0,0,0,0,0,1001,1,{1,2,3,4},sources.data(),0,canonical->Base+workspace->offset))return 1;
 constexpr unsigned checks=8;auto began=std::chrono::steady_clock::now();
 for(unsigned i=0;i<checks;i++){
  asm volatile(""::"r"(plan.get()),"r"(canonical.get()):"memory"); // prevent invariant-call hoisting,not physical IO
  if(!plan->Matches(*canonical))return 1;
 }
 auto elapsed=std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-began).count();
 if(elapsed<0)return 1;
 printf("CPU_METADATA_ONLY immutable_plan_checks%u total_us%lld us_per_check%.3f;NO_GPU_OPEN_OR_NATIVE_OWNER_OR_MODEL_IO_NOT_TOKEN_RATE\n",
  checks,static_cast<long long>(elapsed),double(elapsed)/checks);
}
