/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_FRONTEND_BARRIER_IMAGE_MODEL_ONLY
#include "pioneer_q38_frontend_barrier_image_test.cpp"
#include <deque>
// Actual RISC runs ahead:Instruction pushes a bounded FIFO and RETURNS.
// Backend service happens independently between later RISC instructions;
// full FIFO backpressures the producer. Not a silicon latency model.
struct QueuedFrontend:FrontendBarrierImage {
 using FrontendBarrierImage::FrontendBarrierImage;
 std::deque<uint32_t> fifo;
 bool servicing=false;
 bool missingHotProducer=false;
 unsigned capacity=8,maxQueued=0,backpressure=0,stalled=0;
 struct QueueHazard{};
 bool Service() {
  if(fifo.empty())return false;
  uint32_t op=fifo.front();
  if(waitWord) {
   unsigned kind=(op>>24)==0xb2?128:(op>>24)==0x37?64:(op>>24)>=0x70 && (op>>24)<=0x90?256:511;
   if(((waitWord>>15)&511)&kind) {
    if((rawPending && (waitWord&2) && (missingProducer || cycle<rawReady))
     || (configPending && (waitWord&0x1000) && cycle<configReady)){stalled++;return false;}
   }
  }
  fifo.pop_front();servicing=true;
  try{FrontendBarrierImage::Instruction(op);}catch(...){servicing=false;throw;}
  if(missingHotProducer && rawPending && !rawFloat)missingProducer=true;
  servicing=false;return true;
 }
 void Instruction(uint32_t op)override {
  if(servicing){FrontendBarrierImage::Instruction(op);return;}
  unsigned waited=0;
  while(fifo.size()>=capacity) {
   cycle++;backpressure++;
   if(!Service() && ++waited>200000)throw QueueHazard{};
  }
  fifo.push_back(op);maxQueued=std::max(maxQueued,unsigned(fifo.size()));
 }
 void StepBlock()override {cycle++;Service();FrontendBarrierImage::StepBlock();}
 bool Unretired()const{return !fifo.empty() || waitWord || configPending || rawPending || !phaseQueue.empty();}
 uint32_t Load(uint32_t at)override {
  if(Unretired() && (Range(at,0xffef0000,224*4) || Range(at,0xffbd8000,0x4000)))throw QueueHazard{};
  return FrontendBarrierImage::Load(at);
 }
 void Store(uint32_t at,uint32_t value)override {
  if(Unretired() && (Range(at,0xffef0000,224*4) || Range(at,0xffbd8000,0x4000)))throw QueueHazard{};
  if(at==0xffe000fc)for(uint32_t queued:fifo)if(queued==0x4500017e)throw QueueHazard{};
  FrontendBarrierImage::Store(at,value);
 }
};
int main(int argc,char** argv){
 assert(argc==2);setvbuf(stdout,nullptr,_IOLBF,0);unsigned cases=0;
 for(unsigned type:{8U,12U,14U})for(unsigned depth:{1U,2U,8U,16U}) {
  printf("QUEUED_FRONTEND type%u FIFO%u\n",type,depth);
  QueuedFrontend m(argv[1],type,5120,1);m.capacity=depth;m.unpackDelay=depth==1?0:depth==2?31:4099;m.configDelay=depth==1?0:depth==2?257:17;
  assert(m.Boot());m.memory[0x74038]=1;
  for(unsigned job=1;job<=2;job++) {
   if(job==2){m.NextPackets();FreshInputs(m);}
   m.Poison();m.PoisonTail();assert(m.Job(job) && m.CorrectRows() && !m.Unretired() && m.pendingAccumulator.empty());
  }
  assert(m.maxQueued<=depth && (depth<8 || (m.backpressure && m.stalled)));
  m.memory[0x74038]=0;assert(m.Job(3,2));cases++;
 }
 {
  puts("QUEUED_FRONTEND_MISSING_HOT_PRODUCER");
  QueuedFrontend bad(argv[1],8,5120,1);bad.capacity=2;assert(bad.Boot());bad.memory[0x74038]=1;bad.Poison();bad.PoisonTail();bad.missingHotProducer=true;
  bool refused=false;try{(void)bad.Job(1);}catch(const QueuedFrontend::QueueHazard&){refused=true;}
  assert(refused && !bad.fifo.empty() && !bad.outputReads && !bad.memory.at(0x74144));
 }
 printf("PASS %u ACTUAL RISC/asynchronous finite-FIFO schedules:two changed-input jobs/STOP/exact FP32+FP64,backpressure and delayed raw/config producers;NO RISC config/Dst access before real ACK,no stale ACK clear,missing producer blocks without output;NO native/timing qualification\n",cases);
}
