/* SPDX-License-Identifier: MIT */
// OFFBOARD build/fixture only. No driver, image owner or Work mode uses it.
#define PIONEER_Q38_BFP4_WORKER_DIAGNOSTIC 1
#define PIONEER_DRAM_SWEEP_FREESTANDING 1
#include "PioneerQ38Bfp4WorkerConsumer.h"
#if !defined(__riscv) || __riscv_xlen!=32
#error This isolated leaf requires RV32
#endif
extern "C" void native_dram_band_fault() __attribute__((noreturn));
#ifdef PIONEER_Q38_HYBRID_WORKER_IMAGE
extern "C" void q38_mixed_seal();
#endif
namespace {
static volatile unsigned* Words(unsigned a){return reinterpret_cast<volatile unsigned*>(a);}
struct IO {
 unsigned Read(unsigned a){return *Words(a);}
 void Write(unsigned a,unsigned v){*Words(a)=v;}
 void Fence(){asm volatile("fence":::"memory");}
};
struct Emitter {
 unsigned okay=1;
 void Op(unsigned word){if(okay)*Words(0xffe40000)=word;}
 void Config(unsigned index,unsigned value){if(!okay)return;volatile unsigned* at=Words(0xffef0000+index*4);*at=value;if(*at!=value)okay=0;}
 void Drain(){
  if(!okay)return;
  *Words(0xffe000fc)=0;(void)*Words(0xffe000fc);asm volatile("fence":::"memory");
  Op(0xa2101816);Op(0x4500017e);
  for(unsigned i=0;i<1000000;i++)if(*Words(0xffe000fc)){asm volatile("fence":::"memory");return;}
  okay=0;
 }
 bool Healthy()const{return okay;}
};
struct Guard {
 bool Check(const PioneerQ38Bfp4WorkerJob::Job& job,unsigned band){
#ifdef PIONEER_Q38_HYBRID_WORKER_IMAGE
  q38_mixed_seal();
#endif
  if(band>=job.words[4])return false;
  for(unsigned i=0;i<PioneerQ38Bfp4WorkerJob::Words;i++)if(Words(0x74000)[i]!=job.words[i])return false;
  return true; // FRAME consistency only, never source/owner/producer authority.
 }
};
}
extern "C" unsigned native_dram_band_main(){
 PioneerQ38Bfp4WorkerJob::Job job;for(unsigned i=0;i<PioneerQ38Bfp4WorkerJob::Words;i++)job.words[i]=Words(0x74000)[i];
 IO io;Emitter emitter;Guard guard;PioneerQ38Bfp4WorkerConsumer consumer;
 if(!consumer.Run(io,emitter,guard,job))native_dram_band_fault();
 return job.words[4]*32;
}
