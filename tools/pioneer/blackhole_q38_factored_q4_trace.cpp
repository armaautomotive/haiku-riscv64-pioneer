/* SPDX-License-Identifier: MIT */
// Diagnostic scalar observations of genuine on-card intermediates. NOT Work.
#if !defined(__riscv) || __riscv_xlen!=32 || !defined(PIONEER_Q38_FACTORED_Q4_TRACE_BUILD) || PIONEER_Q38_FACTORED_Q4_TRACE_BUILD!=1 || defined(PIONEER_Q38_FACTORED_Q4_COMPARE_BUILD) || defined(PIONEER_Q38_FACTORED_Q4_MATRIX_BUILD)
#error Exact separate factored Q4 TRACE BUILD required
#endif
#define PIONEER_DRAM_SWEEP_FREESTANDING 1
#define PIONEER_ATTENTION_FREESTANDING
#define PIONEER_Q38_Q4_FACTORED_BFP8_OFFBOARD 1
#define PIONEER_Q38_Q4_FACTORED_TRACE_DIAGNOSTIC 1
#include "PioneerQ38Q4FpuCompactStreamLoader.h"
#include "PioneerQ38Q4FpuCompactStreamCompute.h"
#include "PioneerQ38Q4FactoredTrace.h"
extern "C" __attribute__((noreturn)) void factored_trace_fault();
extern "C" __attribute__((noinline)) void* memset(void* destination,int value,unsigned bytes){
 const unsigned a=reinterpret_cast<unsigned>(destination);
 if(value || (a&3) || (bytes&3) || a<0x6e000 || a>0x70000 || bytes>0x70000-a)factored_trace_fault();
 auto* p=static_cast<volatile unsigned*>(destination);for(unsigned i=0;i<bytes/4;i++)p[i]=0;return destination;
}
namespace FactoredTrace {
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
  factored_trace_fault();
 }
 void Config(unsigned i,unsigned v){auto* p=Words(0xffef0000+i*4);*p=v;if(*p!=v)factored_trace_fault();}
};
// Retain the old->new context transition from111. It is not a new old-path
// numerical qualification:the final output area is replaced by the trace.
static void Old(unsigned route){
 using C=PioneerQ38Q4FpuCompactStreamCompute;
 IO io;Emitter e;PioneerQ38Q4FpuCompactStreamLoader loader;
 if(!loader.Begin(io,5120,route) || !loader.Open(io,0,0))factored_trace_fault();
 for(unsigned s=0;s<2;s++)if(!loader.Slice(io))factored_trace_fault();
 if(!loader.Wait(io))factored_trace_fault();
 C::Begin(e);
 for(unsigned block=0;block<20;block++){
  const unsigned buffer=block&1;const bool next=block<19;
  if(!loader.Acquire(block,buffer) || (next && !loader.Open(io,block+1,buffer^1)))factored_trace_fault();
  if(next && !loader.Slice(io))factored_trace_fault();
  if(!C::LoadMetadata(e,buffer))factored_trace_fault();
  for(unsigned group=0;group<8;group++){
   if(next && !group && !loader.Slice(io))factored_trace_fault();
   if(!C::Group(e,buffer,group))factored_trace_fault();
  }
  if(!loader.Release(block,buffer) || (next && !loader.Wait(io)))factored_trace_fault();
 }
 if(!loader.Complete(io))factored_trace_fault();
 for(unsigned half=0;half<2;half++)for(unsigned col=0;col<16;col++)Words(L::Output)[half*16+col]=Words(0xffbd8000+(32+half*8)*64)[col];
 io.Fence();
}
static void Trace(unsigned route){
 using C=PioneerQ38Q4FactoredCompute;
 IO io;Emitter e;PioneerQ38Q4FpuCompactStreamLoader loader;PioneerQ38Q4FactoredTrace trace;
 if(!loader.Begin(io,5120,route))factored_trace_fault();
 C::Constants(io);io.Fence();
 if(!loader.Open(io,0,0))factored_trace_fault();
 for(unsigned s=0;s<2;s++)if(!loader.Slice(io))factored_trace_fault();
 if(!loader.Wait(io))factored_trace_fault();
 C::Begin(e);
 for(unsigned block=0;block<20;block++){
  const unsigned buffer=block&1;const bool next=block<19;
  if(!loader.Acquire(block,buffer) || (next && !loader.Open(io,block+1,buffer^1)))factored_trace_fault();
  if(next && !loader.Slice(io))factored_trace_fault();
  if(!C::LoadMetadata(e,buffer))factored_trace_fault();
  for(unsigned group=0;group<8;group++){
   if(next && !group && !loader.Slice(io))factored_trace_fault();
   if(!trace.Group(e,io,block,buffer,group))factored_trace_fault();
  }
  if(!trace.EndBlock(block) || !loader.Release(block,buffer) || (next && !loader.Wait(io)))factored_trace_fault();
 }
 if(!loader.Complete(io) || !trace.Publish(io))factored_trace_fault();
 // Keep the existing16-word auxiliary region defined;NO clock/TPS claim.
 for(unsigned i=0;i<16;i++)Words(0x98100)[i]=0;
 io.Fence();
}
}
extern "C" unsigned factored_trace_main(){
 using namespace FactoredTrace;unsigned frame[8];for(unsigned i=0;i<8;i++)frame[i]=Words(L::Frame)[i];
 if(frame[0]!=0x31525151 || frame[1]!=1 || frame[2]!=5120 || !L::Route(frame[3]) || frame[4]!=1 || frame[5] || frame[6] || frame[7])factored_trace_fault();
 Old(frame[3]);Trace(frame[3]);
 for(unsigned i=0;i<8;i++)if(Words(L::Frame)[i]!=frame[i])factored_trace_fault();
 return 0x04001400;
}
