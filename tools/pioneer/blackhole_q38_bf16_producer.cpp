/* SPDX-License-Identifier: MIT */
// Actual on-card packing of live FP32 state for the NEW cached FPU consumer.
#if !defined(__riscv) || __riscv_xlen!=32 || !defined(PIONEER_Q38_BF16_PRODUCER_IMAGE) || PIONEER_Q38_BF16_PRODUCER_IMAGE != 1
#error Exact separate RV32 BF16 producer role required
#endif
#if defined(PIONEER_Q38_CACHED_FPU_MATRIX_IMAGE) || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(PIONEER_NATIVE_COMPACT_Q4_STREAM_DIAGNOSTIC) || defined(PIONEER_Q38_COMPACT_Q4_FPU_STREAM_IMAGE)
#error BF16 producer cannot alias matrix/old104/native roles
#endif
#define PIONEER_ATTENTION_FREESTANDING
#include "PioneerQ38Bf16Input.h"
extern "C" void bf16_producer_fault() __attribute__((noreturn));
extern "C" unsigned bf16_producer_main(){
 const auto* frame=reinterpret_cast<const volatile unsigned*>(0x74000);
 const unsigned count=frame[2];
 if(frame[0]!=PioneerQ38Bf16Input::Magic || frame[1]!=1 || (count!=5120 && count!=17408)
  || frame[3]!=0xb0000 || frame[4]!=0xa0000 || !frame[5])return 0;
 for(unsigned i=6;i<16;i++)if(frame[i])return 0;
 for(unsigned b=0;b<count/256;b++){
  if(!PioneerQ38Bf16Input::Pack(reinterpret_cast<const unsigned*>(0xb0000+b*1024),256,
   reinterpret_cast<unsigned*>(0xa0000+b*512),128))bf16_producer_fault();
 }
 asm volatile("fence":::"memory");return count;
}
