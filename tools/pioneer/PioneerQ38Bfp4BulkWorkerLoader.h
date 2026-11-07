/* SPDX-License-Identifier: MIT */
#if !defined(PIONEER_Q38_BFP4_BULK_LOAD_EXPERIMENT) || PIONEER_Q38_BFP4_BULK_LOAD_EXPERIMENT != 1 \
 || !defined(PIONEER_Q38_BFP4_WORKER_DIAGNOSTIC) || PIONEER_Q38_BFP4_WORKER_DIAGNOSTIC != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(PIONEER_Q38_HYBRID_WORKER_IMAGE) \
 || defined(__FAST_MATH__) || defined(PIONEER_Q38_BFP4_INPUT_REUSE_EXPERIMENT) \
 || defined(PIONEER_Q38_BFP4_INPUT_REUSE_CHECKSUM_EXPERIMENT) || defined(PIONEER_Q38_HYBRID_FULL_CLIENT) \
 || defined(PIONEER_Q38_HYBRID_FULL_NATIVE) || defined(PIONEER_Q38_FULL_OFFLOAD_CLIENT) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) || defined(PIONEER_Q38_BF16_FULL_CLIENT) || defined(PIONEER_Q38_BF16_FULL_NATIVE) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error BFP4 bulk loading is an isolated offline experiment, not a driver or Work role
#endif
#ifndef PIONEER_Q38_BFP4_BULK_WORKER_LOADER_H
#define PIONEER_Q38_BFP4_BULK_WORKER_LOADER_H
#include "PioneerQ38Bfp4WorkerJob.h"
#include "PioneerDramCohortReadSweep.h"

// Offline experiment: read 16 contiguous payloads into a temporary L1
// window and scatter them into the existing header-padded format. This saves
// NoC commands at the cost of local word copies. Scratch ownership, hardware
// behavior, on-card speed and model correctness are NOT established here.
class PioneerQ38Bfp4BulkWorkerLoader {
 using W=PioneerQ38Bfp4WorkerJob;using L=PioneerNativeBfp4DramBandLayout;using Q=PioneerDramQueuedReadSweep;
 W::Job job={};uint32_t baseline[9]={},command[10]={};unsigned attempted=0,failed=0,issued=0,flits=0;
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
   uint32_t available=0;if(!Q::Fifo(io,available))return false;
   if(available==0x10101010){empty=true;break;}
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
 enum : unsigned {Scratch=0x150000,Group=16,ScratchBytes=Group*L::WeightPayload};
 PioneerQ38Bfp4BulkWorkerLoader()=default;
 PioneerQ38Bfp4BulkWorkerLoader(const PioneerQ38Bfp4BulkWorkerLoader&)=delete;
 PioneerQ38Bfp4BulkWorkerLoader& operator=(const PioneerQ38Bfp4BulkWorkerLoader&)=delete;
 template<class IO> bool Run(IO& io,const W::Job& selected,unsigned band){
  if(attempted || failed)return false;attempted=1;
  if(!W::Shape(selected) || band>=selected.words[4] || L::Tiles(selected.words[2])%Group)return Fail();
  for(unsigned i=0;i<W::Words;i++)job.words[i]=selected.words[i];
  if(!Quiet(io))return Fail();
  for(unsigned i=0;i<9;i++)baseline[i]=io.Read(Counter(i));
  for(unsigned operand=0;operand<2;operand++){
   for(unsigned tile=0;tile<L::Tiles(job.words[2]);tile++){
    W::Read packet;if(!W::Packet(job,band,operand,tile,packet))return Fail();
    for(unsigned word=0;word<L::Header/4;word++){
     const unsigned address=packet.destination-L::Header+word*4;io.Write(address,0);if(io.Read(address))return Fail();
    }
   }
   io.Fence();W::Read first;if(!W::Packet(job,band,operand,0,first) || !Quiet(io))return Fail();
   const unsigned bytes=first.bytes*Group;if(!bytes || bytes>16384 || (bytes&63) || bytes>ScratchBytes)return Fail();
   for(unsigned i=0;i<10;i++){
    const unsigned value=i==0?first.source:i==2?first.route:i==3?Scratch:i==5?job.words[7]:i==8?bytes:0;
    command[i]=value;io.Write(0xffb20000+i*4,value);
   }
   if(!Commands(io))return Fail();
   for(unsigned batch=0;batch<L::Tiles(job.words[2]);batch+=Group){
    W::Read p;if(!W::Packet(job,band,operand,batch,p) || p.bytes!=first.bytes || p.route!=first.route || !Q::Slot(io))return Fail();
    command[0]=p.source;io.Write(0xffb20000,p.source);
    if(io.Read(0xffb20000)!=p.source)return Fail();
    io.Fence();io.Write(0xffb20040,1);if(io.Read(0xffb20040)>1)return Fail();
    io.Fence();issued++;flits+=bytes/64;if(!Drain(io))return Fail();
    for(unsigned tile=0;tile<Group;tile++){
     W::Read target;if(!W::Packet(job,band,operand,batch+tile,target)
      || target.source!=p.source+tile*p.bytes || target.bytes!=p.bytes || target.route!=p.route)return Fail();
     for(unsigned word=0;word<p.bytes/4;word++){
      const unsigned value=io.Read(Scratch+tile*p.bytes+word*4);
      io.Write(target.destination+word*4,value);
     }
    }
    io.Fence();
   }
  }
  return issued==2*L::Tiles(job.words[2])/Group && flits==10*L::Tiles(job.words[2]) && Quiet(io) && Commands(io);
 }
};
static_assert(PioneerQ38Bfp4BulkWorkerLoader::Scratch>=PioneerNativeBfp4DramBandLayout::Output+256
 && PioneerQ38Bfp4BulkWorkerLoader::Scratch+PioneerQ38Bfp4BulkWorkerLoader::ScratchBytes<=0x16c000,
 "experimental scratch stays within the offline geometry, not a hardware ownership proof");
#endif
