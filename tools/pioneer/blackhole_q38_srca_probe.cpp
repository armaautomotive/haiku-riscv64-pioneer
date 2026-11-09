/* SPDX-License-Identifier: MIT */
// Distinct disposable BF16 SrcA diagnostic image. No installed driver is
// authorized by this image, and it does not establish exclusive ownership.
#define PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT 1
#define PIONEER_ATTENTION_FREESTANDING 1
#include "PioneerQ38SrcaProbe.h"
#if !defined(__riscv) || __riscv_xlen != 32
#error This fixed diagnostic image requires RV32
#endif
extern "C" void srca_probe_fault() __attribute__((noreturn));
namespace {
volatile unsigned* Words(unsigned a){return reinterpret_cast<volatile unsigned*>(a);}
struct L1 {
 unsigned Read(unsigned a){return *Words(a);}
 void Write(unsigned a,unsigned v){*Words(a)=v;}
 void Fence(){asm volatile("fence":::"memory");}
 unsigned Result(unsigned i){return *Words(0xffbd8000+(i<16?32:40)*64+(i&15)*4);}
};
struct Emitter {
 void Op(unsigned v){*Words(0xffe40000)=v;}
 void Config(unsigned i,unsigned v){*Words(0xffef0000+i*4)=v;if(*Words(0xffef0000+i*4)!=v)srca_probe_fault();}
 void Drain(){
  *Words(0xffe000fc)=0;(void)*Words(0xffe000fc);asm volatile("fence":::"memory");
  Op(0xa2101816);Op(0x4500017e);
  for(unsigned i=0;i<1000000;i++)if(*Words(0xffe000fc)){asm volatile("fence":::"memory");return;}
  srca_probe_fault();
 }
};
}
extern "C" unsigned srca_probe_main(){
 L1 io;Emitter emitter;
 if(!PioneerQ38SrcaProbe::Run(io,emitter))srca_probe_fault();
 return 2;
}
