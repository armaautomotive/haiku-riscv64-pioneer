/* SPDX-License-Identifier: MIT */
// Timed BUILD only;not a native driver,model or Work selector.
#if !defined(__riscv) || __riscv_xlen!=32 \
 || defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) || defined(PIONEER_Q38_FULL_OFFLOAD_CLIENT) \
 || defined(__FAST_MATH__)
#error Exact separate Q4 kernel timing BUILD only
#endif
#ifdef PIONEER_Q38_FACTORED_Q4_COMPARE_BUILD
#if PIONEER_Q38_FACTORED_Q4_COMPARE_BUILD!=1 || defined(PIONEER_Q38_COMPACT_Q4_TIMING_IMAGE) || defined(PIONEER_Q38_COMPACT_Q4_FPU_STREAM_IMAGE) || defined(PIONEER_Q4_FPU_COMPACT_STREAM_K)
#error Exact separate comparison clock role only
#endif
#else
#if !defined(PIONEER_Q38_COMPACT_Q4_TIMING_IMAGE) || PIONEER_Q38_COMPACT_Q4_TIMING_IMAGE!=1 || !defined(PIONEER_Q38_COMPACT_Q4_FPU_STREAM_IMAGE) || PIONEER_Q38_COMPACT_Q4_FPU_STREAM_IMAGE!=1 || !defined(PIONEER_Q4_FPU_COMPACT_STREAM_K) || (PIONEER_Q4_FPU_COMPACT_STREAM_K!=5120 && PIONEER_Q4_FPU_COMPACT_STREAM_K!=17408)
#error Exact separate Q4 kernel timing BUILD only
#endif
#endif
typedef __UINT32_TYPE__ uint32_t;
extern "C" __attribute__((noinline,noclone)) unsigned q38_q4_kernel_clock(uint32_t* out){
 for(unsigned attempt=0;attempt<4;attempt++){
  uint32_t high0,low,high1;
  asm volatile("csrrs %0,0xb80,x0\ncsrrs %1,0xb00,x0\ncsrrs %2,0xb80,x0"
   :"=&r"(high0),"=&r"(low),"=&r"(high1)::"memory");
  if(high0==high1){out[0]=low;out[1]=high0;return 1;}
 }
 out[0]=out[1]=0;return 0; // Telemetry unavailable;NOT a math/cleanup failure.
}
