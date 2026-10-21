/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_RAW_CACHE_IMAGE_MODEL_ONLY
#include "pioneer_q38_raw_unpack_cache_image_test.cpp"
#include <deque>
// Independent bounded instruction FIFO:RV32 enqueues and RETURNS,while
// the backend retires between later RISC instructions. Never treat a
// frontend C5 wait as an acknowledged CPU completion. Not silicon timing.
struct QueuedRawCache:RawCacheImage{
 using RawCacheImage::RawCacheImage;
 std::deque<uint32_t> fifo;bool servicing=false,missingReturn=false;
 unsigned capacity=8,maxQueued=0,backpressure=0,stalled=0,delay=4099;
 uint64_t cycle=0,returnAt=0;unsigned returnBank=0;bool returning=false;
 struct QueueHazard{};
 bool Service(){
  if(fifo.empty())return false;uint32_t op=fifo.front();
  if(gate&&returning){
   if(missingReturn||cycle<returnAt){stalled++;return false;}
   assert(sources[returnBank]&&ownerA);sources[returnBank]=false;ownerA=false;returning=false;
  }
  fifo.pop_front();servicing=true;
  try{RawCacheImage::Instruction(op);}catch(...){servicing=false;throw;}
  if(op==0xa2ff8020){
   // A conservative independently delayed producer-return signal. The
   // C5 gate MUST prevent UNPACR_NOP from publishing the occupied bank.
   assert(gate&&!sources[unpackBank]&&!ownerA);returnBank=unpackBank;
   sources[returnBank]=true;ownerA=true;returnAt=cycle+delay;returning=true;
  }
  servicing=false;return true;
 }
 void Instruction(uint32_t op)override{
  if(servicing){RawCacheImage::Instruction(op);return;}unsigned waited=0;
  while(fifo.size()>=capacity){cycle++;backpressure++;if(!Service()&&++waited>200000)throw QueueHazard{};}
  fifo.push_back(op);maxQueued=std::max(maxQueued,unsigned(fifo.size()));
 }
 void StepBlock()override{cycle++;Service();RawCacheImage::StepBlock();}
 bool Unretired()const{return !fifo.empty()||gate||returning||rawPending||!phaseQueue.empty();}
 uint32_t Load(uint32_t at)override{
  if(Unretired()&&(Range(at,0xffef0000,224*4)||Range(at,0xffbd8000,0x4000)))throw QueueHazard{};
  return RawCacheImage::Load(at);
 }
 void Store(uint32_t at,uint32_t value)override{
  if(Unretired()&&(Range(at,0xffef0000,224*4)||Range(at,0xffbd8000,0x4000)))throw QueueHazard{};
  if(at==0xffe000fc)for(uint32_t queued:fifo)if(queued==0x4500017e)throw QueueHazard{};
  RawCacheImage::Store(at,value);
 }
};
int main(int argc,char** argv){
 assert(argc==2);setvbuf(stdout,nullptr,_IOLBF,0);unsigned cases=0;
 for(unsigned type:{8U,12U,14U})for(unsigned depth:{1U,2U,8U,16U}){
  printf("QUEUED_RAW_CACHE type%u FIFO%u\n",type,depth);
  QueuedRawCache m(argv[1],type,5120,1);m.capacity=depth;m.delay=depth==1?0:depth==2?31:4099;
  assert(m.Boot());m.memory[0x74038]=1;
  for(unsigned job=1;job<=2;job++){
   if(job==2){m.NextPackets();FreshInputs(m);}m.Poison();m.PoisonTail();
   assert(m.Job(job)&&m.CorrectRows()&&!m.Unretired()&&m.pendingAccumulator.empty());
  }
  assert(m.maxQueued<=depth);
  // This retained-ACK sequence queues only THREE instructions before
  // RISC polling. Wide FIFOs stall without filling;depth2 MUST exercise
  // actual producer backpressure,not an impossible full depth8/16 FIFO.
  if(type==12&&depth>1)assert(m.stalled&&m.maxQueued>=2);
  if(type==12&&depth==2)assert(m.backpressure);
  m.memory[0x74038]=0;assert(m.Job(3,2));cases++;
 }
 QueuedRawCache bad(argv[1],12,5120,1);bad.capacity=2;assert(bad.Boot());bad.memory[0x74038]=1;bad.Poison();bad.PoisonTail();bad.missingReturn=true;
 bool refused=false;try{(void)bad.Job(1);}catch(const QueuedRawCache::QueueHazard&){refused=true;}
 assert(refused&&!bad.fifo.empty()&&bad.returning&&bad.gate&&!bad.outputReads&&!bad.memory.at(0x74144));
 printf("PASS %u ACTUAL RISC/asynchronous raw-cache finite-FIFO schedules,two inputs+STOP/exact FP32+FP64,C5 waits for delayed source-bank return,backpressure,NO CPU config/Dst before real ACK/stale ACK clearing;missing return stays pending/inert without output;NO native/timing qualification\n",cases);
}
