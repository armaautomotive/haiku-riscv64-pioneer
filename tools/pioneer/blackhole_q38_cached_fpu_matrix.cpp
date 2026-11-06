/* SPDX-License-Identifier: MIT */
// Returning full-K matrix service;new owner/root/producer integration required.
#if !defined(__riscv) || __riscv_xlen!=32 || !defined(PIONEER_Q38_CACHED_FPU_MATRIX_IMAGE) || PIONEER_Q38_CACHED_FPU_MATRIX_IMAGE != 1
#error Exact separate RV32 cached FPU matrix role required
#endif
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) || defined(PIONEER_Q38_FULL_OFFLOAD_CLIENT) || defined(PIONEER_Q38_BF16_PRODUCER_IMAGE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(PIONEER_Q38_COMPACT_Q4_FPU_STREAM_IMAGE) || defined(PIONEER_NATIVE_COMPACT_Q4_STREAM_DIAGNOSTIC) || defined(__FAST_MATH__)
#error Cached FPU matrix is not old104/native/Work/CPU-reference authority
#endif
#define PIONEER_DRAM_SWEEP_FREESTANDING 1
#define PIONEER_ATTENTION_FREESTANDING
#include "PioneerQ38Q4FpuCachedLoader.h"
#include "PioneerQ38Q4FpuCompactStreamCompute.h"
extern "C" void cached_fpu_fault() __attribute__((noreturn));
#ifndef PIONEER_Q38_CACHED_FPU_RETURN_ONLY
extern "C" __attribute__((noinline)) void* memset(void* destination,int value,unsigned bytes){
 const unsigned a=reinterpret_cast<unsigned>(destination);
 if(value || (a&3) || (bytes&3) || a<0x6e000 || a>0x70000 || bytes>0x70000-a)cached_fpu_fault();
 auto* p=static_cast<volatile unsigned*>(destination);for(unsigned i=0;i<bytes/4;i++)p[i]=0;return destination;
}
#endif
namespace CachedMatrix {
using L=PioneerQ38Q4FpuCachedLayout;using C=PioneerQ38Q4FpuCompactStreamCompute;
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
  cached_fpu_fault();
 }
 void Config(unsigned i,unsigned v){auto* p=Words(0xffef0000+i*4);*p=v;if(*p!=v)cached_fpu_fault();}
};
}
extern "C" unsigned q38_cached_fpu_matrix(const unsigned* descriptor){
 using namespace CachedMatrix;L::Job job;
 for(unsigned i=0;i<16;i++)job.words[i]=descriptor[i];
 if(!L::Shape(job))return 0;
 for(unsigned pair=0;pair<job.words[4]/2;pair++){
  IO io;Emitter e;PioneerQ38Q4FpuCachedLoader loader;
  if(!loader.Begin(io,job,pair))cached_fpu_fault();
  // Owned computational zero initialization AFTER actual self/FIFO checks,
  // BEFORE any packet. It never copies learned or activation payloads.
  if(!pair){for(unsigned i=0;i<272;i++)Words(L::Zero-64)[i]=0;asm volatile("fence":::"memory");}
  if(!loader.Open(io,0,0))cached_fpu_fault();
  for(unsigned s=0;s<2;s++)if(!loader.Slice(io))cached_fpu_fault();
  if(!loader.Wait(io))cached_fpu_fault();
  C::Begin(e);
  for(unsigned block=0;block<job.words[2]/256;block++){
   const unsigned buffer=block&1;const bool next=block+1<job.words[2]/256;
   if(!loader.Acquire(block,buffer) || (next && !loader.Open(io,block+1,buffer^1)))cached_fpu_fault();
   if(next && !loader.Slice(io))cached_fpu_fault();
   if(!C::LoadMetadata(e,buffer))cached_fpu_fault();
   for(unsigned group=0;group<8;group++){
    if(next && !group && !loader.Slice(io))cached_fpu_fault();
    if(!C::Group(e,buffer,group))cached_fpu_fault();
   }
   if(!loader.Release(block,buffer) || (next && !loader.Wait(io)))cached_fpu_fault();
  }
  if(!loader.Complete(io))cached_fpu_fault();
  for(unsigned half=0;half<2;half++)for(unsigned c=0;c<16;c++){
   unsigned value=Words(0xffbd8000+(half?40:32)*64)[c];
   if((value&0x7f800000)==0x7f800000 || (!(value&0x7f800000) && (value&0x7fffff)))cached_fpu_fault();
   Words(L::Output+pair*128)[half*16+c]=value;
  }
  asm volatile("fence":::"memory");
 }
 for(unsigned i=0;i<16;i++)if(descriptor[i]!=job.words[i])cached_fpu_fault();
 return job.words[2]==5120?job.words[4]*320:job.words[4]*1088;
}
#ifndef PIONEER_Q38_CACHED_FPU_RETURN_ONLY
extern "C" unsigned cached_fpu_main(){return q38_cached_fpu_matrix(const_cast<const unsigned*>(CachedMatrix::Words(0x74000)));}
#endif
