/* SPDX-License-Identifier: MIT */
// Unqualified, OFFBOARD-ONLY fixed-width native BFP4/BF16 band image.
#include "PioneerQ38NativeBfp4Band.h"
#if !defined(__riscv) || __riscv_xlen != 32 || !defined(PIONEER_NATIVE_BFP4_BAND_K)
#error Select an RV32I fixed-width offboard prototype
#endif
static_assert(PioneerQ38NativeBfp4Band::Width(PIONEER_NATIVE_BFP4_BAND_K),"exact Qwen width only");
extern "C" void native_bfp_band_fault() __attribute__((noreturn));
namespace {
static inline volatile unsigned* Words(unsigned address){return reinterpret_cast<volatile unsigned*>(address);}
struct Emitter {
 __attribute__((always_inline)) inline void Op(unsigned word){
#ifdef PIONEER_NATIVE_BFP4_BAND_DIRECT
  if(__builtin_constant_p(word))asm volatile(".word %0"::"i"((word<<2)|(word>>30)):"memory");
  else *Words(0xffe40000)=word;
#else
  *Words(0xffe40000)=word;
#endif
 }
 void Drain(){
  *Words(0xffe000fc)=0;(void)*Words(0xffe000fc);asm volatile("fence":::"memory");
  Op(0xa2101816);Op(0x4500017e);
  for(unsigned attempt=0;attempt<1000000;attempt++)if(*Words(0xffe000fc)){asm volatile("fence":::"memory");return;}
  native_bfp_band_fault();
 }
 void Config(unsigned index,unsigned value){
  volatile unsigned* at=Words(0xffef0000+index*4);*at=value;
  if(*at!=value)native_bfp_band_fault();
 }
};
}
extern "C" unsigned native_bfp_band_main(){
 Emitter e;
 for(unsigned pass=0;pass<2;pass++){
  if(!PioneerQ38NativeBfp4Band::Run(e,PIONEER_NATIVE_BFP4_BAND_K))native_bfp_band_fault();
  for(unsigned row=0;row<2;row++)for(unsigned column=0;column<16;column++)
   Words(PioneerQ38NativeBfp4Band::Output+pass*128)[row*16+column]=Words(0xffbd8000+(row?40:32)*64)[column];
  asm volatile("fence":::"memory");
 }
 return (2U<<24)|PIONEER_NATIVE_BFP4_BAND_K;
}
