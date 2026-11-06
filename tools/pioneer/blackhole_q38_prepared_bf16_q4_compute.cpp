/* SPDX-License-Identifier: MIT */
// Code-generation entry points only. NOT a native/returning service.
#if !defined(__riscv) || __riscv_xlen != 32 || !defined(PIONEER_Q38_PREPARED_BF16_Q4_BUILD) || PIONEER_Q38_PREPARED_BF16_Q4_BUILD != 1
#error Exact separate prepared BF16 Q4 RV32 BUILD selection required
#endif
using uint32_t=unsigned;
#define PIONEER_ATTENTION_FREESTANDING
#define PIONEER_Q38_Q4_FACTORED_BFP8_OFFBOARD 1
#define PIONEER_Q38_PREPARED_BF16_Q4_OFFBOARD 1
#include "PioneerQ38PreparedBf16Q4Compute.h"
extern "C" __attribute__((noreturn)) void pioneer_prepared_bf16_q4_fault();
namespace {
using Compute=PioneerQ38PreparedBf16Q4Compute;
static volatile unsigned* Words(unsigned a){return reinterpret_cast<volatile unsigned*>(a);}
struct IO {
 void Write(unsigned a,unsigned v){*Words(a)=v;}
};
struct Emitter {
 __attribute__((always_inline)) inline void Op(unsigned v){
  if(__builtin_constant_p(v))asm volatile(".word %0"::"i"((v<<2)|(v>>30)):"memory");
  else *Words(0xffe40000)=v;
 }
 void Drain(){
  *Words(0xffe000fc)=0;(void)*Words(0xffe000fc);asm volatile("fence":::"memory");Op(0xa2101816);Op(0x4500017e);
  for(unsigned p=0;p<1000000;p++)if(*Words(0xffe000fc)){asm volatile("fence":::"memory");return;}
  pioneer_prepared_bf16_q4_fault();
 }
 void Config(unsigned i,unsigned v){*Words(0xffef0000+i*4)=v;if(*Words(0xffef0000+i*4)!=v)pioneer_prepared_bf16_q4_fault();}
 unsigned MaskWord(unsigned buffer,unsigned group,unsigned face,unsigned word){
  return *Words(0x8c000+buffer*0x10000+(face/2)*512+(group*2+face%2)*32+word*4);
 }
 void EscapeWord(unsigned face,unsigned word,unsigned value){*Words(Compute::Escape+face*Compute::EscapeStride+word*4)=value;}
 void SealData(){asm volatile("fence":::"memory");}
};
}
extern "C" void pioneer_prepared_bf16_q4_begin(){
 IO io;Emitter e;Compute::Constants(io);e.SealData();Compute::Begin(e);
}
extern "C" unsigned pioneer_prepared_bf16_q4_block(unsigned buffer){
 Emitter e;if(buffer>=2 || !Compute::LoadMetadata(e,buffer))pioneer_prepared_bf16_q4_fault();
 for(unsigned group=0;group<8;group++)if(!Compute::Group(e,buffer,group))pioneer_prepared_bf16_q4_fault();
 return 256;
}
