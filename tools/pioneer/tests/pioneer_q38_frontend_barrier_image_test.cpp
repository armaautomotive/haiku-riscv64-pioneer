/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_DIRECT_SRCB_IMAGE_MODEL_ONLY
#include "pioneer_q38_direct_srcb_image_test.cpp"
// Independent delayed raw-unpacker/configuration wait-gate model. This
// deliberately does NOT turn a frontend wait into an ACK or RISC drain.
struct FrontendBarrierImage:DirectSrcBImage {
 std::vector<uint8_t> fetchCode;
 uint64_t cycle=0,rawReady=0,configReady=0;
 unsigned barriers=0,dropBarrier=0,unpackDelay=100003,configDelay=997;
 uint32_t waitWord=0,alterMask=0;
 bool configPending=false,lastRawWait=false,missingProducer=false;
 struct FrontendHazard{};
 FrontendBarrierImage(const char* path,unsigned type,unsigned k,unsigned groups,unsigned worker=69):DirectSrcBImage(path,type,k,groups,worker) {
  fetchCode=Read((std::string(path)+"/fetch.bin").c_str());assert(!fetchCode.empty() && fetchCode.size()<=0x700 && !(fetchCode.size()&3));
  for(unsigned w=0;w<fetchCode.size()/4;w++)l1[Index(0x70000+w*4)]=Word(fetchCode,w*4);
 }
 bool CodeAddress(uint32_t at)const override{return Range(at,0x70000,unsigned(fetchCode.size())) || DirectSrcBImage::CodeAddress(at);}
 void FinishGate(uint32_t next) {
  if(!waitWord)return;
  unsigned block=(waitWord>>15)&511,condition=waitWord&8191;
  unsigned opcode=next>>24,kind=opcode==0xb2?128:opcode==0x37?64:opcode>=0x70 && opcode<=0x90?256:511;
  if(!(block&kind))return; // unblocked consumer demonstrates the race
  if(rawPending && (condition&2)) {
   if(missingProducer)throw FrontendHazard{};
   cycle=std::max(cycle,rawReady);CompleteRawUnpack();lastRawWait=true;
  }
  if(configPending && (condition&0x1000)) {
   cycle=std::max(cycle,configReady);thread[5]=4;configPending=false;
  }
  waitWord=0;
 }
 void Instruction(uint32_t op)override {
  cycle++;FinishGate(op);
  if(op==0xa2ff9816) {
   assert(Active() && phaseQueue.empty() && pendingAccumulator.empty() && !recordRemaining && !pending);
   if(++barriers!=dropBarrier)waitWord=op^alterMask;
   return;
  }
  if(Active() && op==0xb2050004 && rawPending)throw FrontendHazard{};
  if(configPending && op!=0xa2101816 && op!=0x4500017e)throw FrontendHazard{};
  if(op==0x4500017e && rawPending) {
   if(missingProducer)throw FrontendHazard{};
   cycle=std::max(cycle,rawReady); // retained real ACK also waits
  }
  bool delayed=Active() && op==0xb2050004 && lastRawWait;
  uint32_t before=thread[5];DirectSrcBImage::Instruction(op);
  if((op>>24)==0x42 && rawPending)rawReady=cycle+unpackDelay;
  if(delayed){assert(before==0 && thread[5]==4);thread[5]=before;configPending=true;configReady=cycle+configDelay;lastRawWait=false;}
 }
 uint32_t Load(uint32_t at)override {
  if(Range(at,0x70000,unsigned(fetchCode.size())))return l1[Index(at)];
  if((waitWord || configPending) && (Range(at,0xffef0000,224*4) || Range(at,0xffbd8000,0x4000)))throw FrontendHazard{};
  return DirectSrcBImage::Load(at);
 }
 void Store(uint32_t at,uint32_t value)override {
  if((waitWord || configPending) && (Range(at,0xffef0000,224*4) || Range(at,0x90000,unsigned(tile.size())) || Range(at,0xffb20000,0x4000)))throw FrontendHazard{};
  DirectSrcBImage::Store(at,value);
 }
};
#ifndef PIONEER_Q38_FRONTEND_BARRIER_IMAGE_MODEL_ONLY
int main(int argc,char** argv){
 assert(argc==3 || (argc==4 && !strcmp(argv[3],"--work-pilot")));setvbuf(stdout,nullptr,_IOLBF,0);
 for(unsigned type:{8U,12U,14U})for(unsigned k:{5120U,6144U,17408U})for(unsigned groups:{1U,3U,16U})for(bool work:{false,true}) {
  if(argc==4 && (k!=5120 || groups!=1 || !work))continue;
  printf("FRONTEND_CASE type%u K%u groups%u work%u\n",type,k,groups,unsigned(work));
  DirectSrcBImage old(argv[1],type,k,groups);FrontendBarrierImage next(argv[2],type,k,groups);
  assert(old.Boot() && next.Boot());old.memory[0x74038]=next.memory[0x74038]=work;
  for(unsigned job=1;job<=2;job++) {
   if(job==2){old.NextPackets();next.NextPackets();FreshInputs(old);FreshInputs(next);}
   old.Poison();old.PoisonTail();next.Poison();next.PoisonTail();
   assert(old.Job(job) && next.Job(job));
   assert(old.requests==next.requests && old.responses==next.responses && old.matrixInstructions==next.matrixInstructions
    && old.sumWrites==next.sumWrites && old.sumReads==next.sumReads && old.vectorOps==next.vectorOps
    && old.drains==next.drains+next.barriers && !next.waitWord && !next.configPending && next.pendingAccumulator.empty());
   for(unsigned w=0;w<64;w++)assert(old.memory.at(0x83f00+w*4)==next.memory.at(0x83f00+w*4));
   for(unsigned row=0;row<groups*16;row++)assert(old.buffers[4096+row]==next.buffers[4096+row]);
  }
  old.memory[0x74038]=next.memory[0x74038]=0;assert(old.Job(3,2) && next.Job(3,2));
 }
 for(unsigned failure=0;failure<6;failure++) {
  FrontendBarrierImage bad(argv[2],8,5120,1);assert(bad.Boot());bad.memory[0x74038]=1;bad.Poison();bad.PoisonTail();
  if(failure<2)bad.dropBarrier=failure+1;
  if(failure==2)bad.alterMask=1U<<22; // B7:SETC can overtake raw UNPACK
  if(failure==3)bad.alterMask=2; // C1:does not wait for raw producer
  if(failure==4)bad.missingProducer=true;
  if(failure==5)bad.loseFinalWait=true;
  bool refused=false;try{(void)bad.Job(1);}catch(const FrontendBarrierImage::FrontendHazard&){refused=failure!=5;}
  catch(const FrontendBarrierImage::DeferredHazard&){refused=failure==5;}
  assert(refused && !bad.outputReads && !bad.memory.at(0x74144));
 }
 for(unsigned schedule=0;schedule<4;schedule++)for(unsigned type:{8U,12U,14U}) {
  FrontendBarrierImage next(argv[2],type,5120,1);assert(next.Boot());next.memory[0x74038]=1;
  next.unpackDelay=schedule==0?0:schedule==1?1:schedule==2?1000003:UINT32_MAX;
  next.configDelay=schedule==0?0:schedule==1?1000003:schedule==2?1:65537;
  next.Poison();next.PoisonTail();assert(next.Job(1) && next.CorrectRows() && !next.waitWord && !next.configPending);
 }
 {
  FrontendBarrierImage bad(argv[2],8,5120,1);assert(bad.Boot());bad.memory[0x74038]=1;bad.Poison();bad.PoisonTail();
  bad.alterMask=0x1000;bool refused=false;
  try{(void)bad.Job(1);}catch(const FrontendBarrierImage::FrontendHazard&){refused=true;}
  assert(refused && !bad.outputReads && !bad.memory.at(0x74144)); // C12 pending SETC must not be ignored
 }
 for(unsigned fault:{1U,5U,40U,208U}) {
  FrontendBarrierImage bad(argv[2],12,17408,3);assert(bad.Boot());bad.memory[0x74038]=1;bad.Poison();bad.PoisonTail();bad.faultAt=fault;
  assert(!bad.Job(1) && bad.pending && bad.requests==fault && bad.responses==fault-1 && !bad.memory.at(0x74144));
  unsigned requests=bad.requests,gates=bad.barriers;for(unsigned step=0;step<1000;step++)bad.StepBlock();assert(bad.requests==requests && bad.barriers==gates);
 }
 puts(argc==4?"PASS PARTIAL ONLY frontend Work pilots/delayed gates/faults;FULL SUITE SKIPPED":"PASS OFFLINE ACTUAL frontend barriers:ALL types/K/groups/modes/two inputs,exact old FP32+independent FP64,SAME matrix/SFPU/NoC/64proofs,delayed raw/config producer waits without ACK,all RISC-facing/final drains retained,missing/wrong gate/producer refuse before output,NoC faults inert;NO kernel/native/speed qualification");
}
#endif
