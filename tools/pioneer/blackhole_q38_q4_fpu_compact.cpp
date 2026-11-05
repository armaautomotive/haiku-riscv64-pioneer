/* SPDX-License-Identifier: MIT */
#if !defined(__riscv) || __riscv_xlen != 32 \
 || !defined(PIONEER_Q38_COMPACT_Q4_FPU_IMAGE) || PIONEER_Q38_COMPACT_Q4_FPU_IMAGE != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) || defined(PIONEER_Q38_FULL_OFFLOAD_CLIENT) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) \
 || defined(PIONEER_DRIVER_BUILD_TAG) || defined(TT_PIONEER_DRIVER_RELOAD) \
 || defined(PIONEER_Q4_FPU_BLOCK_DIRECT) || defined(__FAST_MATH__)
#error Distinct compact Q4 OFFBOARD RV32 image only;no native or Work alias
#endif
using uint32_t=unsigned;
#define PIONEER_ATTENTION_FREESTANDING
#include "PioneerQ38Q4FpuCompactBlock.h"
extern "C" void q4_fpu_compact_fault() __attribute__((noreturn));
namespace {
static inline volatile unsigned* Words(unsigned a){return reinterpret_cast<volatile unsigned*>(a);}
struct Emitter {
 __attribute__((always_inline)) inline void Op(unsigned word){
  if(__builtin_constant_p(word))asm volatile(".word %0"::"i"((word<<2)|(word>>30)):"memory");
  else *Words(0xffe40000)=word;
 }
 void Drain(){
  *Words(0xffe000fc)=0;(void)*Words(0xffe000fc);asm volatile("fence":::"memory");
  Op(0xa2101816);Op(0x4500017e);
  for(unsigned attempt=0;attempt<1000000;attempt++)if(*Words(0xffe000fc)){asm volatile("fence":::"memory");return;}
  q4_fpu_compact_fault();
 }
 void Config(unsigned index,unsigned value){
  volatile unsigned* at=Words(0xffef0000+index*4);*at=value;if(*at!=value)q4_fpu_compact_fault();
 }
};
}
extern "C" unsigned q4_fpu_compact_main(){
 Emitter e;
 for(unsigned pass=0;pass<2;pass++){
  PioneerQ38Q4FpuCompactBlock::Run(e);
  for(unsigned row=0;row<2;row++)for(unsigned c=0;c<16;c++)
   Words(PioneerQ38Q4FpuBlock::Output+pass*128)[row*16+c]=Words(0xffbd8000+(row?40:32)*64)[c];
  asm volatile("fence":::"memory");
 }
 return 0x04000100;
}
