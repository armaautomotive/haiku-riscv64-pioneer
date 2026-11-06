/* SPDX-License-Identifier: MIT */
// Fixed isolated old/new full-K comparison;NO Work/model/native owner grant.
#if !defined(__riscv) || __riscv_xlen!=32 || !defined(PIONEER_Q38_FACTORED_Q4_COMPARE_BUILD) || PIONEER_Q38_FACTORED_Q4_COMPARE_BUILD!=1
#error Exact separate factored-Q4 comparison BUILD required
#endif
#if defined(PIONEER_Q38_FACTORED_Q4_MATRIX_BUILD) || defined(PIONEER_Q38_COMPACT_Q4_FPU_STREAM_IMAGE) || defined(PIONEER_Q38_CACHED_FPU_MATRIX_IMAGE)
#error Comparison cannot alias old or returning component roles
#endif
#define PIONEER_DRAM_SWEEP_FREESTANDING 1
#define PIONEER_ATTENTION_FREESTANDING
#define PIONEER_Q38_Q4_FACTORED_BFP8_OFFBOARD 1
#include "PioneerQ38Q4FpuCompactStreamLoader.h"
#include "PioneerQ38Q4FpuCompactStreamCompute.h"
#include "PioneerQ38Q4FactoredCompute.h"
#include "PioneerQ38Q4KernelTiming.h"
extern "C" __attribute__((noreturn)) void factored_compare_fault();
extern "C" unsigned q38_q4_kernel_clock(uint32_t*);
extern "C" __attribute__((noinline)) void* memset(void* destination,int value,unsigned bytes){
 const unsigned a=reinterpret_cast<unsigned>(destination);
 if(value || (a&3) || (bytes&3) || a<0x6e000 || a>0x70000 || bytes>0x70000-a)factored_compare_fault();
 auto* p=static_cast<volatile unsigned*>(destination);for(unsigned i=0;i<bytes/4;i++)p[i]=0;return destination;
}
namespace FactoredCompare {
using L=PioneerQ38Q4FpuCompactStreamLayout;
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
  factored_compare_fault();
 }
 void Config(unsigned i,unsigned v){auto* p=Words(0xffef0000+i*4);*p=v;if(*p!=v)factored_compare_fault();}
};
template<class Compute,unsigned Accumulator> static void Run(unsigned width,unsigned route,unsigned pass){
 uint32_t start[2],end[2];const bool startValid=q38_q4_kernel_clock(start);
 IO io;Emitter e;PioneerQ38Q4FpuCompactStreamLoader loader;
 if(!loader.Begin(io,width,route))factored_compare_fault();
 if constexpr(Accumulator==80){PioneerQ38Q4FactoredCompute::Constants(io);io.Fence();}
 if(!loader.Open(io,0,0))factored_compare_fault();
 for(unsigned s=0;s<2;s++)if(!loader.Slice(io))factored_compare_fault();
 if(!loader.Wait(io))factored_compare_fault();
 Compute::Begin(e);
 for(unsigned block=0;block<width/256;block++){
  const unsigned buffer=block&1;const bool next=block+1<width/256;
  if(!loader.Acquire(block,buffer) || (next && !loader.Open(io,block+1,buffer^1)))factored_compare_fault();
  if(next && !loader.Slice(io))factored_compare_fault();
  if(!Compute::LoadMetadata(e,buffer))factored_compare_fault();
  for(unsigned group=0;group<8;group++){
   if(next && !group && !loader.Slice(io))factored_compare_fault();
   if(!Compute::Group(e,buffer,group))factored_compare_fault();
  }
  if(!loader.Release(block,buffer) || (next && !loader.Wait(io)))factored_compare_fault();
 }
 if(!loader.Complete(io))factored_compare_fault();
 const bool endValid=q38_q4_kernel_clock(end);uint32_t record[8];
 PioneerQ38Q4KernelTiming::Record(width,pass,start,startValid,end,endValid,record);
 for(unsigned i=0;i<8;i++)Words(PioneerQ38Q4KernelTiming::Address+pass*32)[i]=record[i];
 for(unsigned half=0;half<2;half++)for(unsigned c=0;c<16;c++){
  const unsigned value=Words(0xffbd8000+(Accumulator+half*8)*64)[c];
  if((value&0x7f800000)==0x7f800000 || (!(value&0x7f800000) && (value&0x7fffff)))factored_compare_fault();
  Words(L::Output+pass*128)[half*16+c]=value;
 }
 io.Fence();
}
}
extern "C" unsigned factored_compare_main(){
 using namespace FactoredCompare;unsigned frame[8];for(unsigned i=0;i<8;i++)frame[i]=Words(L::Frame)[i];
 if(frame[0]!=0x31435151 || frame[1]!=1 || !L::Width(frame[2]) || !L::Route(frame[3]) || frame[4]!=1 || frame[5] || frame[6] || frame[7])factored_compare_fault();
 // SAME original weights/input,one context/loader/layout,old then new.
 // One pair only,not repeatability,nominal clock-Hz or model TPS authority.
 Run<PioneerQ38Q4FpuCompactStreamCompute,32>(frame[2],frame[3],0);
 Run<PioneerQ38Q4FactoredCompute,80>(frame[2],frame[3],1);
 for(unsigned i=0;i<8;i++)if(Words(L::Frame)[i]!=frame[i])factored_compare_fault();
 return 0x04000000|frame[2];
}
