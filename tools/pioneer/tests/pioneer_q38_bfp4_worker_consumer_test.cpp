/* SPDX-License-Identifier: MIT */
// Simulated returning leaf and guarded output only; no device or model claim.
#define PIONEER_BFP4_WORKER_LOADER_TEST_ONLY
#include "pioneer_q38_bfp4_worker_loader_test.cpp"
#include "../PioneerQ38Bfp4WorkerConsumer.h"
#include "../PioneerNativeBfp4DramBand.h"
struct ConsumerIO:IO {
 unsigned fpuReads=0,outputWrites=0,outputReads=0;bool badFloat=false,badReadback=false;
 ConsumerIO(const W::Job& job):IO(job,0){}
 static uint32_t Value(unsigned band,unsigned row){return 0x3f000000U+band*0x10000U+row*0x80U;}
 unsigned Read(unsigned a){
  for(unsigned half=0;half<2;half++){
   const unsigned first=0xffbd8000+(half?40U:32U)*64;
   if(a>=first && a<first+64 && !(a&3)){events++;fpuReads++;return badFloat?0x7f800000:Value(band,half*16+(a-first)/4);}
  }
  if(a>=W::Output && a<W::Output+8*32*4 && !(a&3)){
   events++;outputReads++;unsigned value=l1[a]|unsigned(l1[a+1])<<8|unsigned(l1[a+2])<<16|unsigned(l1[a+3])<<24;
   return badReadback?value^1:value;
  }
  return IO::Read(a);
 }
 void Write(unsigned a,unsigned value){
  if(a>=W::Output && a<W::Output+8*32*4 && !(a&3)){
   events++;outputWrites++;for(unsigned i=0;i<4;i++)l1[a+i]=uint8_t(value>>(8*i));return;
  }
  IO::Write(a,value);
 }
};
struct Emitter {
 unsigned events=0,ops=0,configs=0,drains=0;bool healthy=true;
 void Op(unsigned){events++;ops++;}void Config(unsigned,unsigned){events++;configs++;}void Drain(){events++;drains++;}
 bool Healthy()const{return healthy;}
};
struct Guard {
 ConsumerIO& io;W::Job expected;unsigned calls=0,failAt=0;
 bool Check(const W::Job& job,unsigned band){
  calls++;io.band=band;
  if(calls==failAt)return false;
  for(unsigned i=0;i<W::Words;i++)if(job.words[i]!=expected.words[i])return false;
  return band<job.words[4];
 }
};
struct Recording {
 struct Event {unsigned kind,index,value;};std::vector<Event> events;
 void Op(unsigned value){events.push_back({0,0,value});}
 void Config(unsigned index,unsigned value){events.push_back({1,index,value});}
 void Drain(){events.push_back({2,0,0});}
};
int main(){
 for(unsigned width:{5120U,17408U}){
  Recording prior,current;assert(PioneerNativeBfp4DramBand::Compute(prior,width) && PioneerQ38Bfp4WorkerCompute::Compute(current,width));
  assert(prior.events.size()==current.events.size());
  for(unsigned i=0;i<prior.events.size();i++)assert(prior.events[i].kind==current.events[i].kind
   && prior.events[i].index==current.events[i].index && prior.events[i].value==current.events[i].value);
  auto job=Make(63,width);ConsumerIO io(job);Emitter emitter;Guard guard{io,job};PioneerQ38Bfp4WorkerConsumer consumer;
  assert(consumer.Run(io,emitter,guard,job) && guard.calls==10 && io.fpuReads==64 && io.outputWrites==64 && io.outputReads==64);
  assert(emitter.ops && emitter.configs && emitter.drains && io.issues==
#ifdef PIONEER_Q38_BFP4_BATCH_INPUT_EXPERIMENT
   2*(L::Tiles(width)+(width*2+16383)/16384)
#else
   4*L::Tiles(width)
#endif
   && !io.scalar);
  for(unsigned band=0;band<2;band++)for(unsigned row=0;row<32;row++){
   unsigned a=W::Output+(band*32+row)*4;uint32_t got=0;for(unsigned i=0;i<4;i++)got|=unsigned(io.l1[a+i])<<(8*i);
   assert(got==ConsumerIO::Value(band,row));
  }
  const auto before=io.events,emit=emitter.events;assert(!consumer.Run(io,emitter,guard,job) && io.events==before && emitter.events==emit);
 }
 for(unsigned fault=0;fault<7;fault++){
  auto job=Make(1,5120);ConsumerIO io(job);Emitter emitter;Guard guard{io,job};PioneerQ38Bfp4WorkerConsumer consumer;
  if(fault==0)guard.failAt=1;if(fault==1)guard.failAt=2;if(fault==2)guard.failAt=4;
  if(fault==3)io.fault=5;if(fault==4)emitter.healthy=false;if(fault==5)io.badFloat=true;if(fault==6)io.badReadback=true;
  assert(!consumer.Run(io,emitter,guard,job));const auto before=io.events,emit=emitter.events,calls=guard.calls;
  assert(!consumer.Run(io,emitter,guard,job) && io.events==before && emitter.events==emit && guard.calls==calls);
  if(fault==0)assert(!io.events && !emitter.events);if(fault>=1 && fault<=5)assert(!io.outputWrites);
 }
 puts("PASS BFP4 returning-worker consumer: full K5120/K17408, ROOT-L1 input, 64 bounded output words, guard/readback/fault no-retry; SIMULATION ONLY NO DEVICE OR WORK");
}
