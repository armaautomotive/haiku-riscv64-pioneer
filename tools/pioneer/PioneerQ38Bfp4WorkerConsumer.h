/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_BFP4_WORKER_CONSUMER_H
#define PIONEER_Q38_BFP4_WORKER_CONSUMER_H
#include "PioneerQ38Bfp4WorkerLoader.h"
#include "PioneerQ38Bfp4WorkerCompute.h"
// Returning DATA/leaf proposal only. Guard is a caller-supplied proof hook;
// its implementation must bind the actual sealed job, BF16 producer, source
// ledger and lifetime. This header does not create that proof or authorize a
// native/Work build. First error permanently stops this object without cleanup.
class PioneerQ38Bfp4WorkerConsumer {
 using W=PioneerQ38Bfp4WorkerJob;unsigned attempted=0,failed=0;
 bool Fail(){failed=1;return false;}
public:
 PioneerQ38Bfp4WorkerConsumer()=default;
 PioneerQ38Bfp4WorkerConsumer(const PioneerQ38Bfp4WorkerConsumer&)=delete;
 PioneerQ38Bfp4WorkerConsumer& operator=(const PioneerQ38Bfp4WorkerConsumer&)=delete;
 template<class IO,class Emitter,class Guard> bool Run(IO& io,Emitter& emitter,Guard& guard,const W::Job& input){
  if(attempted || failed)return false;
  attempted=1;
  if(!W::Shape(input))return Fail(); // ZERO IO
  W::Job job;for(unsigned i=0;i<W::Words;i++)job.words[i]=input.words[i];
  for(unsigned band=0;band<job.words[4];band++){
   if(!guard.Check(job,band))return Fail();
   PioneerQ38Bfp4WorkerLoader loader;if(!loader.Run(io,job,band) || !guard.Check(job,band))return Fail();
   if(!PioneerQ38Bfp4WorkerCompute::Compute(emitter,job.words[2]) || !emitter.Healthy() || !guard.Check(job,band))return Fail();
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
