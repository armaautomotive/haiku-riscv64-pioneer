/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_BFP8_WORKER_CONSUMER_H
#define PIONEER_Q38_BFP8_WORKER_CONSUMER_H
#include "PioneerQ38Bfp8WorkerLoader.h"
#include "PioneerQ38Bfp8WorkerCompute.h"
#if defined(PIONEER_Q38_BFP8_INCOMING_WATCH_EXPERIMENT) && (PIONEER_Q38_BFP8_INCOMING_WATCH_EXPERIMENT != 1 \
 || !defined(PIONEER_Q38_BFP8_INPUT_REUSE_EXPERIMENT) || PIONEER_Q38_BFP8_INPUT_REUSE_EXPERIMENT != 1)
#error Incoming write watch requires the isolated BFP8 input-reuse experiment
#endif
// Isolated returning diagnostic. Guard binds only what its caller proves;
// no Work, GDDR ownership or ROOT-producer lifetime is implied here.
class PioneerQ38Bfp8WorkerConsumer {
 using W=PioneerQ38Bfp8WorkerJob;unsigned attempted=0,failed=0;
 bool Fail(){failed=1;return false;}
#ifdef PIONEER_Q38_BFP8_INCOMING_WATCH_EXPERIMENT
 // An optional on-card diagnostic, not a CPU comparison or ownership proof.
 // Both NIUs: atomic received (54/55), write data received (56/57), and
 // write requests started (60/61). These counters do not cover local stores,
 // every possible read response, prior in-flight traffic or counter wrap.
 struct Incoming {uint32_t value[12];};
 template<class IO> static __attribute__((noinline
#if defined(__riscv)
  ,optimize("Os")
#endif
  )) void SnapshotIncoming(IO& io,Incoming& result){
  io.Fence();for(unsigned n=0;n<2;n++)for(unsigned i=0;i<6;i++){
   const unsigned offset=i<4?0x2d8+i*4:0x2f0+(i-4)*4;
   result.value[n*6+i]=io.Read(0xffb20000+n*0x10000+offset);
  }
  io.Fence();
 }
 template<class IO> static __attribute__((noinline
#if defined(__riscv)
  ,optimize("Os")
#endif
  )) bool IncomingUnchanged(IO& io,const Incoming& before){
  Incoming now;SnapshotIncoming(io,now);for(unsigned i=0;i<12;i++)if(now.value[i]!=before.value[i])return false;return true;
 }
#endif
public:
 PioneerQ38Bfp8WorkerConsumer()=default;
 PioneerQ38Bfp8WorkerConsumer(const PioneerQ38Bfp8WorkerConsumer&)=delete;
 PioneerQ38Bfp8WorkerConsumer& operator=(const PioneerQ38Bfp8WorkerConsumer&)=delete;
 template<class IO,class Emitter,class Guard>bool Run(IO& io,Emitter& emitter,Guard& guard,const W::Job& input){
  if(attempted || failed)return false;
  attempted=1;if(!W::Shape(input))return Fail();
  W::Job job;for(unsigned i=0;i<W::Words;i++)job.words[i]=input.words[i];
#ifdef PIONEER_Q38_BFP8_INCOMING_WATCH_EXPERIMENT
  Incoming incoming;
#endif
  for(unsigned band=0;band<job.words[4];band++){
   if(!guard.Check(job,band))return Fail();
#ifdef PIONEER_Q38_BFP8_INCOMING_WATCH_EXPERIMENT
   if(band && !IncomingUnchanged(io,incoming))return Fail();
#endif
   PioneerQ38Bfp8WorkerLoader loader;
#ifdef PIONEER_Q38_BFP8_INPUT_REUSE_EXPERIMENT
   const bool loaded=band?loader.RunReusingInput(io,job,band):loader.Run(io,job,band);
#else
   const bool loaded=loader.Run(io,job,band);
#endif
   if(!loaded || !guard.Check(job,band))return Fail();
#ifdef PIONEER_Q38_BFP8_INCOMING_WATCH_EXPERIMENT
   if(!band)SnapshotIncoming(io,incoming);else if(!IncomingUnchanged(io,incoming))return Fail();
#endif
   if(!PioneerQ38Bfp8WorkerCompute::Compute(emitter,job.words[2]) || !emitter.Healthy() || !guard.Check(job,band))return Fail();
#ifdef PIONEER_Q38_BFP8_INCOMING_WATCH_EXPERIMENT
   if(!IncomingUnchanged(io,incoming))return Fail();
#endif
   uint32_t output[32];
   for(unsigned row=0;row<2;row++)for(unsigned col=0;col<16;col++){
    const uint32_t value=io.Read(0xffbd8000+(row?40U:32U)*64+col*4);
    if((value&0x7f800000)==0x7f800000)return Fail();
    output[row*16+col]=value;
   }
   if(!guard.Check(job,band))return Fail();
   for(unsigned i=0;i<32;i++)io.Write(W::Output+(band*32+i)*4,output[i]);
   io.Fence();
   for(unsigned i=0;i<32;i++)if(io.Read(W::Output+(band*32+i)*4)!=output[i])return Fail();
   if(!guard.Check(job,band))return Fail();
#ifdef PIONEER_Q38_BFP8_INCOMING_WATCH_EXPERIMENT
   if(!IncomingUnchanged(io,incoming))return Fail();
#endif
  }
  return true;
 }
};
#endif
