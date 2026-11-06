/* SPDX-License-Identifier: MIT */
// Returning full-K old/new component. No native/source/Work authority.
#if !defined(__riscv) || __riscv_xlen!=32 || !defined(PIONEER_Q38_PREPARED_BF16_COMPARE_BUILD) || PIONEER_Q38_PREPARED_BF16_COMPARE_BUILD!=1
#error Exact separate prepared BF16 comparison RV32 BUILD required
#endif
#if defined(PIONEER_Q38_FACTORED_Q4_COMPARE_BUILD) || defined(PIONEER_Q38_FACTORED_CONFIGURED_COMPUTE) || defined(PIONEER_Q38_PREPARED_BF16_Q4_BUILD) || defined(PIONEER_Q38_CACHED_FPU_MATRIX_IMAGE) || defined(PIONEER_Q38_COMPACT_Q4_FPU_STREAM_IMAGE)
#error Prepared BF16 comparison cannot alias old/computation-only image roles
#endif
#define PIONEER_DRAM_SWEEP_FREESTANDING 1
#define PIONEER_ATTENTION_FREESTANDING
#define PIONEER_Q38_Q4_FACTORED_BFP8_OFFBOARD 1
#define PIONEER_Q38_PREPARED_BF16_Q4_OFFBOARD 1
#include "PioneerQ38Q4FpuCompactStreamLoader.h"
#include "PioneerQ38Q4FpuCompactStreamCompute.h"
#include "PioneerQ38PreparedBf16Q4StreamLoader.h"
#include "PioneerQ38PreparedBf16Q4Compute.h"
#include "PioneerQ38Q4KernelTiming.h"
extern "C" __attribute__((noreturn)) void prepared_bf16_compare_fault();
extern "C" unsigned q38_q4_kernel_clock(uint32_t*);
extern "C" __attribute__((noinline)) void* memset(void* destination,int value,unsigned bytes){
 const unsigned a=reinterpret_cast<unsigned>(destination);
 if(value || (a&3) || (bytes&3) || a<0x6e000 || a>0x70000 || bytes>0x70000-a)prepared_bf16_compare_fault();
 auto* p=static_cast<volatile unsigned*>(destination);for(unsigned i=0;i<bytes/4;i++)p[i]=0;return destination;
}
namespace PreparedBf16Compare {
using Old=PioneerQ38Q4FpuCompactStreamLayout;using Data=PioneerQ38PreparedBf16Q4StreamLayout;
using Prepared=PioneerQ38PreparedBf16Q4Compute;
static volatile unsigned* Words(unsigned a){return reinterpret_cast<volatile unsigned*>(a);}
struct IO {unsigned Read(unsigned a){return *Words(a);}void Write(unsigned a,unsigned v){*Words(a)=v;}void Fence(){asm volatile("fence":::"memory");}};
struct Emitter {
 __attribute__((always_inline)) inline void Op(unsigned v){
  if(__builtin_constant_p(v))asm volatile(".word %0"::"i"((v<<2)|(v>>30)):"memory");else *Words(0xffe40000)=v;
 }
 void Drain(){
  *Words(0xffe000fc)=0;(void)*Words(0xffe000fc);asm volatile("fence":::"memory");Op(0xa2101816);Op(0x4500017e);
  for(unsigned p=0;p<1000000;p++)if(*Words(0xffe000fc)){asm volatile("fence":::"memory");return;}
  prepared_bf16_compare_fault();
 }
 void Config(unsigned i,unsigned v){*Words(0xffef0000+i*4)=v;if(*Words(0xffef0000+i*4)!=v)prepared_bf16_compare_fault();}
 unsigned MaskWord(unsigned b,unsigned g,unsigned face,unsigned word){return *Words(Data::Mask+b*Data::BufferStride+(face/2)*512+(g*2+face%2)*32+word*4);}
 void EscapeWord(unsigned face,unsigned word,unsigned v){*Words(Prepared::Escape+face*Prepared::EscapeStride+word*4)=v;}
 void SealData(){asm volatile("fence":::"memory");}
};
template<class Layout,class Loader,class Compute,unsigned Accumulator> static void Run(unsigned width,unsigned route,unsigned pass){
 uint32_t start[2],end[2];const bool startValid=q38_q4_kernel_clock(start);IO io;Emitter e;Loader loader;
 if constexpr(Accumulator==80){io.Write(0x87ff0,0x32425151);Compute::Constants(io);io.Fence();}
 if(!loader.Begin(io,width,route) || !loader.Open(io,0,0))prepared_bf16_compare_fault();
 for(unsigned s=0;s<Layout::Slices;s++)if(!loader.Slice(io))prepared_bf16_compare_fault();
 if(!loader.Wait(io))prepared_bf16_compare_fault();
 Compute::Begin(e);
 for(unsigned block=0;block<width/256;block++){
  const unsigned buffer=block&1;const bool next=block+1<width/256;
  if(!loader.Acquire(block,buffer) || (next && !loader.Open(io,block+1,buffer^1)))prepared_bf16_compare_fault();
  if(next && !loader.Slice(io))prepared_bf16_compare_fault();
  if(!Compute::LoadMetadata(e,buffer))prepared_bf16_compare_fault();
  for(unsigned group=0;group<8;group++){
   if(next && group<Layout::Slices-1 && !loader.Slice(io))prepared_bf16_compare_fault();
   if(!Compute::Group(e,buffer,group))prepared_bf16_compare_fault();
  }
  if(!loader.Release(block,buffer) || (next && !loader.Wait(io)))prepared_bf16_compare_fault();
 }
 if(!loader.Complete(io))prepared_bf16_compare_fault();
 const bool endValid=q38_q4_kernel_clock(end);uint32_t record[8];
 PioneerQ38Q4KernelTiming::Record(width,pass,start,startValid,end,endValid,record);
 for(unsigned i=0;i<8;i++)Words(PioneerQ38Q4KernelTiming::Address+pass*32)[i]=record[i];
 for(unsigned half=0;half<2;half++)for(unsigned col=0;col<16;col++){
  const unsigned value=Words(0xffbd8000+(Accumulator+half*8)*64)[col];
  if((value&0x7f800000)==0x7f800000 || (!(value&0x7f800000) && (value&0x7fffff)))prepared_bf16_compare_fault();
  Words(Old::Output+pass*128)[half*16+col]=value;
 }
 io.Fence();
}
}
extern "C" unsigned prepared_bf16_compare_main(){
 using namespace PreparedBf16Compare;unsigned frame[8];for(unsigned i=0;i<8;i++)frame[i]=Words(Old::Frame)[i];
 if(frame[0]!=0x31425151 || frame[1]!=1 || !Old::Width(frame[2]) || !Old::Route(frame[3]) || frame[4]!=1 || frame[5] || frame[6] || frame[7])prepared_bf16_compare_fault();
 Run<Old,PioneerQ38Q4FpuCompactStreamLoader,PioneerQ38Q4FpuCompactStreamCompute,32>(frame[2],frame[3],0);
 Run<Data,PioneerQ38PreparedBf16Q4StreamLoader,Prepared,80>(frame[2],frame[3],1);
 for(unsigned i=0;i<8;i++)if(Words(Old::Frame)[i]!=frame[i])prepared_bf16_compare_fault();
 return 0x04000000|frame[2];
}
