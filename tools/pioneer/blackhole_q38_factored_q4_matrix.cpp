/* SPDX-License-Identifier: MIT */
// New returning full-K BUILD component,NOT an installed/native lease grant.
#if !defined(__riscv) || __riscv_xlen!=32 || !defined(PIONEER_Q38_FACTORED_Q4_MATRIX_BUILD) || PIONEER_Q38_FACTORED_Q4_MATRIX_BUILD!=1
#error Exact separate factored-Q4 RV32 BUILD role required
#endif
#if defined(PIONEER_Q38_CACHED_FPU_MATRIX_IMAGE) || defined(PIONEER_Q38_CACHED_FPU_PAIR_ISSUE_IMAGE) || defined(PIONEER_Q38_BF16_PRODUCER_IMAGE) || defined(PIONEER_Q38_Q4_FACTORED_TILE_BUILD) || defined(PIONEER_Q38_COMPACT_Q4_FPU_STREAM_IMAGE)
#error Factored returning BUILD cannot alias another component role
#endif
#if defined(PIONEER_Q38_FACTORED_Q4_RETURN_ONLY) && PIONEER_Q38_FACTORED_Q4_RETURN_ONLY!=1
#error Exact positive return-only selector required
#endif
#define PIONEER_DRAM_SWEEP_FREESTANDING 1
#define PIONEER_ATTENTION_FREESTANDING
#define PIONEER_Q38_Q4_FACTORED_BFP8_OFFBOARD 1
#include "PioneerQ38Q4FpuCachedLoader.h"
#include "PioneerQ38Q4FactoredCompute.h"
extern "C" __attribute__((noreturn)) void factored_q4_fault();
#ifndef PIONEER_Q38_FACTORED_Q4_RETURN_ONLY
extern "C" __attribute__((noinline)) void* memset(void* destination,int value,unsigned bytes){
 const unsigned a=reinterpret_cast<unsigned>(destination);
 if(value || (a&3) || (bytes&3) || a<0x6e000 || a>0x70000 || bytes>0x70000-a)factored_q4_fault();
 auto* p=static_cast<volatile unsigned*>(destination);for(unsigned i=0;i<bytes/4;i++)p[i]=0;return destination;
}
#endif
namespace FactoredQ4 {
using L=PioneerQ38Q4FpuCachedLayout;using C=PioneerQ38Q4FactoredCompute;
static volatile unsigned* Words(unsigned a){return reinterpret_cast<volatile unsigned*>(a);}
struct IO {
 unsigned Read(unsigned a){return *Words(a);}void Write(unsigned a,unsigned v){*Words(a)=v;}
 void Fence(){asm volatile("fence":::"memory");}
};
struct Emitter {
 __attribute__((always_inline)) inline void Op(unsigned v){
  if(__builtin_constant_p(v))asm volatile(".word %0"::"i"((v<<2)|(v>>30)):"memory");else *Words(0xffe40000)=v;
 }
 void Drain(){
  *Words(0xffe000fc)=0;(void)*Words(0xffe000fc);asm volatile("fence":::"memory");Op(0xa2101816);Op(0x4500017e);
  for(unsigned p=0;p<1000000;p++)if(*Words(0xffe000fc)){asm volatile("fence":::"memory");return;}
  factored_q4_fault();
 }
 void Config(unsigned i,unsigned v){auto* p=Words(0xffef0000+i*4);*p=v;if(*p!=v)factored_q4_fault();}
};
}
extern "C" unsigned q38_factored_q4_matrix(const unsigned* descriptor){
 using namespace FactoredQ4;L::Job job;
 for(unsigned i=0;i<16;i++)job.words[i]=descriptor[i];
 if(!L::Shape(job))return 0;
 for(unsigned pair=0;pair<job.words[4]/2;pair++){
  IO io;Emitter e;PioneerQ38Q4FpuCachedLoader loader;
  if(!loader.Begin(io,job,pair))factored_q4_fault();
  // Computational constants ONLY after self/FIFO checks. The NEW native
  // owner must grant these temporary tiles;an old cached lease is not enough.
  if(!pair){C::Constants(io);io.Fence();}
  if(!loader.Open(io,0,0))factored_q4_fault();
  for(unsigned s=0;s<2;s++)if(!loader.Slice(io))factored_q4_fault();
  if(!loader.Wait(io))factored_q4_fault();
  C::Begin(e);
  for(unsigned block=0;block<job.words[2]/256;block++){
   const unsigned buffer=block&1;const bool next=block+1<job.words[2]/256;
   if(!loader.Acquire(block,buffer) || (next && !loader.Open(io,block+1,buffer^1)))factored_q4_fault();
   if(next && !loader.Slice(io))factored_q4_fault();
   if(!C::LoadMetadata(e,buffer))factored_q4_fault();
   for(unsigned group=0;group<8;group++){
    if(next && !group && !loader.Slice(io))factored_q4_fault();
    if(!C::Group(e,buffer,group))factored_q4_fault();
   }
   if(!loader.Release(block,buffer) || (next && !loader.Wait(io)))factored_q4_fault();
  }
  if(!loader.Complete(io))factored_q4_fault();
  for(unsigned half=0;half<2;half++)for(unsigned c=0;c<16;c++){
   const unsigned value=Words(0xffbd8000+(C::Accumulator+half*8)*64)[c];
   if((value&0x7f800000)==0x7f800000 || (!(value&0x7f800000) && (value&0x7fffff)))factored_q4_fault();
   Words(L::Output+pair*128)[half*16+c]=value;
  }
  io.Fence();
 }
 for(unsigned i=0;i<16;i++)if(descriptor[i]!=job.words[i])factored_q4_fault();
 return job.words[2]==5120?job.words[4]*320:job.words[4]*1088;
}
#ifndef PIONEER_Q38_FACTORED_Q4_RETURN_ONLY
extern "C" unsigned factored_q4_main(){return q38_factored_q4_matrix(const_cast<const unsigned*>(FactoredQ4::Words(0x74000)));}
#endif
