/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_BFP4_WORKER_CONSUMER_H
#define PIONEER_Q38_BFP4_WORKER_CONSUMER_H
#include "PioneerQ38Bfp4WorkerLoader.h"
#include "PioneerQ38Bfp4WorkerCompute.h"
#ifdef PIONEER_Q38_BFP4_BULK_LOAD_EXPERIMENT
#include "PioneerQ38Bfp4BulkWorkerLoader.h"
#endif
#if defined(PIONEER_Q38_BFP4_INPUT_REUSE_CHECKSUM_EXPERIMENT) && (PIONEER_Q38_BFP4_INPUT_REUSE_CHECKSUM_EXPERIMENT != 1 \
 || !defined(PIONEER_Q38_BFP4_INPUT_REUSE_EXPERIMENT) || PIONEER_Q38_BFP4_INPUT_REUSE_EXPERIMENT != 1)
#error Input-reuse checksum requires the separate isolated input-reuse experiment
#endif
// Returning DATA/leaf proposal only. Guard is a caller-supplied proof hook;
// its implementation must bind the actual sealed job, BF16 producer, source
// ledger and lifetime. This header does not create that proof or authorize a
// native/Work build. First error permanently stops this object without cleanup.
class PioneerQ38Bfp4WorkerConsumer {
 using W=PioneerQ38Bfp4WorkerJob;unsigned attempted=0,failed=0;
 bool Fail(){failed=1;return false;}
#ifdef PIONEER_Q38_BFP4_INPUT_REUSE_CHECKSUM_EXPERIMENT
 struct InputChecksum {uint32_t a,b;};
 template<class IO> static __attribute__((noinline)) InputChecksum Checksum(IO& io,unsigned width){
  // Local on-card diagnostic only, not CPU-reference inference. This is a
  // non-cryptographic corruption detector, not an ownership/lifetime proof.
  InputChecksum result={0x6d2b79f5U,0x9e3779b9U};
  for(unsigned tile=0;tile<W::L::Tiles(width);tile++){
   const unsigned first=W::L::Input-W::L::Header+tile*W::L::InputStride;
   for(unsigned i=0;i<W::L::InputStride/4;i++){
    const uint32_t word=io.Read(first+i*4);
    result.a=((result.a<<5)|(result.a>>27))^word;
    result.b=((result.b<<7)|(result.b>>25))+(word^0x9e3779b9U)+result.a;
   }
  }
  return result;
 }
 static bool Same(InputChecksum a,InputChecksum b){return a.a==b.a && a.b==b.b;}
#endif
public:
 PioneerQ38Bfp4WorkerConsumer()=default;
 PioneerQ38Bfp4WorkerConsumer(const PioneerQ38Bfp4WorkerConsumer&)=delete;
 PioneerQ38Bfp4WorkerConsumer& operator=(const PioneerQ38Bfp4WorkerConsumer&)=delete;
 template<class IO,class Emitter,class Guard> bool Run(IO& io,Emitter& emitter,Guard& guard,const W::Job& input){
  if(attempted || failed)return false;
  attempted=1;
  if(!W::Shape(input))return Fail(); // ZERO IO
  W::Job job;for(unsigned i=0;i<W::Words;i++)job.words[i]=input.words[i];
#ifdef PIONEER_Q38_BFP4_INPUT_REUSE_CHECKSUM_EXPERIMENT
  InputChecksum retained={};
#endif
  for(unsigned band=0;band<job.words[4];band++){
   if(!guard.Check(job,band))return Fail();
#ifdef PIONEER_Q38_BFP4_INPUT_REUSE_CHECKSUM_EXPERIMENT
   if(band && (!Same(retained,Checksum(io,job.words[2])) || !guard.Check(job,band)))return Fail();
#endif
#ifdef PIONEER_Q38_BFP4_BULK_LOAD_EXPERIMENT
   PioneerQ38Bfp4BulkWorkerLoader loader;
#else
   PioneerQ38Bfp4WorkerLoader loader;
#endif
#ifdef PIONEER_Q38_BFP4_INPUT_REUSE_EXPERIMENT
   const bool loaded=band?loader.RunReusingInput(io,job,band):loader.Run(io,job,band);
#else
   const bool loaded=loader.Run(io,job,band);
#endif
   if(!loaded || !guard.Check(job,band))return Fail();
#ifdef PIONEER_Q38_BFP4_INPUT_REUSE_CHECKSUM_EXPERIMENT
   if(!band)retained=Checksum(io,job.words[2]);
   else if(!Same(retained,Checksum(io,job.words[2])))return Fail();
   if(!guard.Check(job,band))return Fail();
#endif
   if(!PioneerQ38Bfp4WorkerCompute::Compute(emitter,job.words[2]) || !emitter.Healthy() || !guard.Check(job,band))return Fail();
#ifdef PIONEER_Q38_BFP4_INPUT_REUSE_CHECKSUM_EXPERIMENT
   if(!Same(retained,Checksum(io,job.words[2])) || !guard.Check(job,band))return Fail();
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
  }
  return true;
 }
};
#endif
