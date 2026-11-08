/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_BFP8_WORKER_LOADER_H
#define PIONEER_Q38_BFP8_WORKER_LOADER_H
#if !defined(PIONEER_Q38_BFP8_WORKER_DIAGNOSTIC) || PIONEER_Q38_BFP8_WORKER_DIAGNOSTIC != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) || defined(PIONEER_Q38_BFP4_WORKER_DIAGNOSTIC)
#error BFP8 worker loader is an isolated offboard diagnostic, not Work or device authority
#endif
#if defined(PIONEER_Q38_BFP8_INPUT_REUSE_EXPERIMENT) && (PIONEER_Q38_BFP8_INPUT_REUSE_EXPERIMENT != 1 \
 || defined(PIONEER_Q38_HYBRID_WORKER_IMAGE) || defined(PIONEER_Q38_GLOBAL_WORKER_IMAGE) \
 || defined(PIONEER_Q38_BF16_WORKER_IMAGE) || defined(PIONEER_DRIVER_BUILD_TAG) \
 || defined(PIONEER_Q38_HEAD_MATRIX_WORKER_INSPECTION) || defined(PIONEER_Q38_HEAD_MATRIX_ROOT_INSPECTION) \
 || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_CLIENT) || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) \
 || defined(PIONEER_Q38_HYBRID_FULL_CLIENT) || defined(PIONEER_Q38_HYBRID_FULL_NATIVE) \
 || defined(PIONEER_Q38_QGW8_NATIVE_INSPECTION))
#error BFP8 input reuse is an isolated leaf experiment, never a hybrid or driver image
#endif
#include "PioneerQ38Bfp8WorkerJob.h"
#include "PioneerDramCohortReadSweep.h"
// C64 batched GDDR weight reads plus ROOT-L1 BF16 input reads. The caller
// still owes immutable weight-page and producer ownership/lifetime proofs.
class PioneerQ38Bfp8WorkerLoader {
 using W=PioneerQ38Bfp8WorkerJob;using L=W::L;using Q=PioneerDramQueuedReadSweep;
 W::Job job;uint32_t baseline[9],command[10];unsigned attempted,failed,issued,flits;
 bool Fail(){failed=1;return false;}
 static unsigned Counter(unsigned i){const unsigned indices[9]={1,2,3,4,5,8,10,12,14};return 0xffb20200+indices[i]*4;}
 unsigned Expected(unsigned i)const{return i==2?flits:(i==1 || i==3 || i==4 || i==8)?issued:0;}
 template<class IO> bool Quiet(IO& io)const{
  uint32_t available=0;return W::Existing::Self(io,job.words[6]) && Q::Fifo(io,available)
   && available==0x10101010 && PioneerDramReadSweep::Idle(io);
 }
 template<class IO> bool Commands(IO& io)const{
  for(unsigned i=0;i<10;i++)if(io.Read(0xffb20000+i*4)!=command[i])return false;
  return true;
 }
 template<class IO> bool Drain(IO& io){
  bool empty=false;
  for(unsigned poll=0;poll<Q::PollLimit;poll++){
   uint32_t available=0;if(!Q::Fifo(io,available))return false;if(available==0x10101010){empty=true;break;}
  }
  if(!empty)return false;
  for(unsigned poll=0;poll<Q::PollLimit;poll++){
   unsigned busy=io.Read(0xffb20040);if(busy>1)return false;bool complete=!busy;
   for(unsigned i=0;i<9;i++){const uint32_t delta=uint32_t(io.Read(Counter(i))-baseline[i]);if(delta>Expected(i))return false;complete&=delta==Expected(i);}
   for(unsigned i=0;i<32;i++){const unsigned count=io.Read(0xffb20240+i*4)&255;if(count>(i==0?16U:0U))return false;complete&=!count;}
   if(complete){if(!Quiet(io) || !Commands(io))return false;io.Fence();return true;}
  }
  return false;
 }
public:
 PioneerQ38Bfp8WorkerLoader(){
  volatile uint32_t* words=job.words;for(unsigned i=0;i<W::Words;i++)words[i]=0;
  volatile uint32_t* first=baseline;for(unsigned i=0;i<9;i++)first[i]=0;
  volatile uint32_t* second=command;for(unsigned i=0;i<10;i++)second[i]=0;
  attempted=failed=issued=flits=0;
 }
 PioneerQ38Bfp8WorkerLoader(const PioneerQ38Bfp8WorkerLoader&)=delete;
 PioneerQ38Bfp8WorkerLoader& operator=(const PioneerQ38Bfp8WorkerLoader&)=delete;
#ifdef PIONEER_Q38_BFP8_INPUT_REUSE_EXPERIMENT
private:
 friend class PioneerQ38Bfp8WorkerConsumer;
 template<class IO> bool RunInternal(IO& io,const W::Job& selected,unsigned band,bool reuseInput){
#else
 template<class IO> bool Run(IO& io,const W::Job& selected,unsigned band){
#endif
  if(attempted || failed)return false;
  attempted=1;
#ifdef PIONEER_Q38_BFP8_INPUT_REUSE_EXPERIMENT
  if(reuseInput && !band)return Fail();
#endif
  if(!W::Shape(selected) || band>=selected.words[4])return Fail();
  for(unsigned i=0;i<W::Words;i++)job.words[i]=selected.words[i];
  if(!Quiet(io))return Fail();
  for(unsigned i=0;i<9;i++)baseline[i]=io.Read(Counter(i));
  for(unsigned operand=0;operand<
#ifdef PIONEER_Q38_BFP8_INPUT_REUSE_EXPERIMENT
    (reuseInput?1U:2U)
#else
    2U
#endif
    ;operand++){
   for(unsigned tile=0;tile<L::Tiles(job.words[2]);tile++){
    W::Read packet;if(!W::SelectedPacket(job,band,operand,tile,packet))return Fail();
    for(unsigned word=0;word<L::Header/4;word++){
     const unsigned address=packet.destination-L::Header+word*4;io.Write(address,0);if(io.Read(address))return Fail();
    }
   }
   io.Fence();W::Read first;if(!W::SelectedPacket(job,band,operand,0,first) || !Quiet(io))return Fail();
   for(unsigned i=0;i<10;i++){
    const unsigned value=i==0?first.source:i==2?first.route:i==3?first.destination:i==5?job.words[7]:i==8?first.bytes:0;
    volatile uint32_t* slot=command+i;*slot=value;io.Write(0xffb20000+i*4,value);
   }
   if(!Commands(io))return Fail();
   for(unsigned batch=0;batch<L::Tiles(job.words[2]);batch+=16){
    for(unsigned slot=0;slot<16;slot++){
     W::Read p;if(!W::SelectedPacket(job,band,operand,batch+slot,p) || !Q::Slot(io))return Fail();
     command[0]=p.source;command[3]=p.destination;
     io.Write(0xffb20000,p.source);io.Write(0xffb2000c,p.destination);
     if(io.Read(0xffb20000)!=p.source || io.Read(0xffb2000c)!=p.destination)return Fail();
     io.Fence();io.Write(0xffb20040,1);if(io.Read(0xffb20040)>1)return Fail();io.Fence();issued++;flits+=p.bytes/64;
    }
    if(!Drain(io))return Fail();
   }
  }
  if(issued!=
#ifdef PIONEER_Q38_BFP8_INPUT_REUSE_EXPERIMENT
    (reuseInput?L::Tiles(job.words[2]):2*L::Tiles(job.words[2]))
    || flits!=(reuseInput?17*L::Tiles(job.words[2]):18*L::Tiles(job.words[2]))
#else
    2*L::Tiles(job.words[2]) || flits!=18*L::Tiles(job.words[2])
#endif
    || !Quiet(io) || !Commands(io))return Fail();
  return true;
 }
#ifdef PIONEER_Q38_BFP8_INPUT_REUSE_EXPERIMENT
public:
 template<class IO> bool Run(IO& io,const W::Job& selected,unsigned band){return RunInternal(io,selected,band,false);}
private:
 // Valid only inside the same consumer after its first band loaded input.
 // Input ownership and preservation are NOT proved by this experiment.
 template<class IO> bool RunReusingInput(IO& io,const W::Job& selected,unsigned band){return RunInternal(io,selected,band,true);}
#endif
};
#endif
