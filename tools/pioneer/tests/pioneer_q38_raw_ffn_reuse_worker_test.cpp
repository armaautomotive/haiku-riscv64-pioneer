/* SPDX-License-Identifier: MIT */
// Actual QGW3 NoC, worker and matrix instructions on synthetic tensors.
// Compare the same up projection with and without shared-input fetches.
#define PIONEER_Q38_PUSHED_WORKER_MACHINE_ONLY
#include "pioneer_q38_pushed_worker_test.cpp"

struct ReusePeer:PushedPeer {
 bool retained=false;
 std::array<uint32_t,2560> operand={};
 std::array<uint32_t,32> scale={};
 std::vector<uint32_t> gateResult;
 ReusePeer(const char* old,const char* image,unsigned w,unsigned groups)
  :PushedPeer(old,image,w,12,5120,groups){}
 unsigned ExpectedReadPackets()const override{
  return PushedPeer::ExpectedReadPackets()-(retained?2:0);
 }
 void Gate(){
  BootPushed();StagePushed();AwaitPayload();FinishPushed();
  for(unsigned i=0;i<operand.size();i++)operand[i]=memory.at(0xa0000+i*4);
  for(unsigned i=0;i<scale.size();i++)scale[i]=memory.at(0x94000+i*4);
  gateResult.assign(buffers+4096,buffers+4096+FfnPersistentPeer::groups*16);
  CheckRetained();
 }
 void CheckRetained()const{
  for(unsigned i=0;i<operand.size();i++)assert(memory.at(0xa0000+i*4)==operand[i] && root.at(0xa0000+i*4)==operand[i]);
  for(unsigned i=0;i<scale.size();i++)assert(memory.at(0x94000+i*4)==scale[i] && root.at(0xab000+i*4)==scale[i]);
 }
 void StartUp(bool useRetained){
  assert(GdnFinished(1) && !pending && !heldPayload && pushes==2 && !programmed);
  assert(inputReads==2 && packets.size()==PushedPeer::ExpectedReadPackets());
  ClearTensorState(); // fixture observation baseline, NOT an input or guest-processor reset
  std::array<uint32_t,8> physical;
  for(unsigned i=0;i<8;i++)physical[i]=Count(i);
  for(unsigned i=0;i<8;i++)countBase[i]=physical[i];
  NextPackets();
  for(unsigned i=0;i<8;i++)assert(Count(i)==physical[i]);
  PoisonTail(); // outside the retained 10,240-byte operand, never a reload of it
  mailbox.fill(0);payload.fill(0);pushes=programmed=0;pushCommand=false;pushReadback.fill(false);
  // Gate/up use the same activations and different weight tiles; synthetic
  // Q4 nibble changes are also consumed by the independent FP32/FP64 check.
  constexpr unsigned weightGap=0x04000000;
  for(auto& t:tiles)for(unsigned i=16*16;i<t.size();i++)t[i]^=0x11;
  for(auto& p:packets)if(p.tile>=0)p.address+=weightGap;
  for(unsigned i=0;i<4096;i++)buffers[4096+i]=0x7fc00000;
  retained=useRetained;
  if(retained)packets.erase(packets.begin(),packets.begin()+inputReads);
  assert(packets.size()==ExpectedReadPackets());
  CheckRetained();
  memory[M::Job+4]=2;memory[M::Job+7*4]=2026;
  memory[M::Job+22*4]+=weightGap;memory[M::Job+25*4]=retained?0:1;
  memory[M::Doorbell]=2;sequence=2;
 }
 std::vector<uint32_t> FinishUp(){
  while(!heldPayload && !GdnFaulted())StepBlock();
  assert(heldPayload && !mailbox[0] && memory.at(M::Status+4)==1);
  AckPayload();while(!GdnFinished(2) && !GdnFaulted())StepBlock();
  const std::array<uint32_t,4> expectedSlave={{4,10,4,4}};
  assert(pc==waitPC && requests==ExpectedReadPackets() && responses==requests
   && !heldPayload && !pending && pushes==2 && matrixInstructions==ExpectedMatrixInstructions()
   && !dotProbeReads && rootMaster==rootOriginal && rootSlave==expectedSlave && CorrectRows());
  std::array<uint32_t,64> job;for(unsigned i=0;i<64;i++)job[i]=memory.at(M::Job+i*4);
  assert(Push::Receipt(mailbox.data(),boot.data(),job.data(),beforePush.data()));
  CheckRetained();
  std::vector<uint32_t> result(buffers+4096,buffers+4096+FfnPersistentPeer::groups*16);
  assert(result!=gateResult);return result;
 }
};

int main(int argc,char** argv){
 assert(argc==3);(void)&RunPersistentGdnPair;
 for(unsigned worker:{1U,60U,119U}){
  const unsigned groups=worker<=17?10:9;
  auto refresh=std::make_unique<ReusePeer>(argv[1],argv[2],worker,groups);
  auto reuse=std::make_unique<ReusePeer>(argv[1],argv[2],worker,groups);
  refresh->Gate();reuse->Gate();assert(reuse->gateResult==refresh->gateResult);
  refresh->StartUp(false);reuse->StartUp(true);
  assert(refresh->FinishUp()==reuse->FinishUp());
  assert(refresh->requests==reuse->requests+2);
  assert(reuse->beforePush[1]+162==refresh->beforePush[1]);
  printf("PASS worker%u actual gate->up retained: %u identical up rows, %u vs %u reads, 162 fewer NoC flits\n",
   worker,groups*16,reuse->requests,refresh->requests);
 }
 for(unsigned address:{0xa0000U,0x94000U}){
  auto damaged=std::make_unique<ReusePeer>(argv[1],argv[2],1,1);
  damaged->Gate();damaged->StartUp(true);
  damaged->memory.at(address)=0x7fc00000; // invalid packed operand or nonfinite scale
  while(!damaged->GdnFaulted() && !damaged->heldPayload)damaged->StepBlock();
  assert(damaged->GdnFaulted() && !damaged->requests && !damaged->matrixInstructions
   && !damaged->pushes && !damaged->heldPayload && !damaged->mailbox[0] && damaged->memory.at(M::Status+4)==1);
 }
 puts("PASS invalid retained operand or scale faults before new reads, tensor work or receipt");
 puts("PASS linked QGW3 retains every operand/scale across real receipts and identical matrices; synthetic weights and NoC, no hardware or speed");
}
