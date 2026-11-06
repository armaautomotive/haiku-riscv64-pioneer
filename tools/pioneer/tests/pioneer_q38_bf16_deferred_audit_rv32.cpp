/* SPDX-License-Identifier: MIT */
// OFFBOARD UNIT IMAGE ONLY. Mock closure observations, not worker receipts.
#if !defined(__riscv) || __riscv_xlen != 32 \
 || !defined(PIONEER_Q38_TEST_DEFERRED_AUDIT_IMAGE) || PIONEER_Q38_TEST_DEFERRED_AUDIT_IMAGE != 1
#error Exact RV32 offboard deferred-audit UNIT TEST required
#endif
#if defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) || defined(PIONEER_Q38_FULL_OFFLOAD_CLIENT) \
 || defined(PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE) || defined(PIONEER_Q38_GLOBAL_WORKER_IMAGE) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) || defined(__FAST_MATH__)
#error Unit bookkeeping test cannot be a native/worker/Work/reference image
#endif
#include "../PioneerQ38Bf16DeferredAudit.h"
extern "C" unsigned scalar_main(){
 const auto* frame=reinterpret_cast<const volatile unsigned*>(0x74000);
 const unsigned width=frame[0],used=frame[1],operation=frame[2],index=frame[3];
 if(operation>5 || (operation>=1 && operation<=3 && index>=119)
  || (operation==4 && index>=sizeof(PioneerQ38Bf16DeferredAudit)*8))return 0;
 PioneerQ38Bf16DeferredAudit audit;
 const bool begun=audit.Begin(width,used);unsigned marked=0,refused=0;
 if(begun)for(unsigned w=0;w<119;w++){
  if(operation==1 && w==index)continue;
  const bool matrix=(w<used) ^ (operation==3 && w==index);
  if(matrix?audit.MatrixClosed(w):audit.IdleClosed(w))marked++;
  else{refused++;break;}
 }
 if(begun && operation==2 && !(index<used?audit.MatrixClosed(index):audit.IdleClosed(index)))refused++;
 if(begun && operation==4){
  // Deliberate object-representation fault injection. All52 bytes are
  // thirteen32-bit words; may_alias makes the test's representation access
  // explicit, and keeps this integer harness's qualified word-only ISA.
  typedef unsigned FaultWord __attribute__((__may_alias__));
  auto* words=reinterpret_cast<volatile FaultWord*>(&audit);
  words[index/32]^=1U<<(index%32);
 }
 if(begun && operation==5 && !audit.Begin(width,used))refused++;
 auto* output=reinterpret_cast<volatile unsigned*>(0x90000);
 output[0]=begun;output[1]=marked;output[2]=audit.Closed();output[3]=refused;
 asm volatile("fence":::"memory");return 4;
}
