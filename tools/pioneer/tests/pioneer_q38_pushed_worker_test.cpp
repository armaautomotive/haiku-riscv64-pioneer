/* SPDX-License-Identifier: MIT */
// Actual QGW3 RISC+Tensix,synthetic raw tensor/NoC endpoints. NO ROOT/native
// owner or original27B/model/physical/Work/TPS result can be manufactured here.
#define PIONEER_Q38_GLOBAL_WORKER_INITIAL_MACHINE_ONLY
#include "pioneer_q38_global_worker_initial_test.cpp"
#include "../PioneerQ38PushedWorkerLayout.h"
namespace Push=PioneerQ38PushedWorkerLayout;
struct PushedPeer:GlobalInitialPeer {
 std::vector<uint8_t> precise;
 std::array<uint32_t,64> mailbox={},payload={};
 std::array<uint32_t,8> rootMaster={},rootOriginal={},beforePush={};
 std::array<uint32_t,4> rootSlave={}; // starts,data flits,received,ACKs;not ROOT master counters.
 unsigned pushes=0,programmed=0;bool heldPayload=false,pushCommand=false,queueAtCommit=false;
 std::array<bool,10> pushReadback={};
 PushedPeer(const char* old,const char* image,unsigned worker,unsigned type=8,unsigned k=5120,unsigned groups=1):GlobalInitialPeer(old,image,worker,type,k,groups),
  precise(Read((std::string(image)+"/precise.bin").c_str())){
  assert(!steps && pc==0x6000 && !precise.empty());
  boot[0]=Push::Magic;for(unsigned i=40;i<=54;i++)boot[i]=Push::BootWord(i);
  assert(Push::BootShape(boot.data()) && !PioneerQ38PreciseWorkerLayout::BootShape(boot.data()) && !G::BootShape(boot.data()));
  for(unsigned i=0;i<64;i++)memory[M::Boot+i*4]=boot[i];
  mailbox.fill(0x7fc12345);mailbox[0]=0;
  for(unsigned i=0;i<8;i++)rootOriginal[i]=rootMaster[i]=0xfffffff0U+i;
  countBase[4]=countBase[7]=0xfffffffeU;countBase[5]=0xfffffffcU; // raw cold-boot counter values;exercise outgoing write wrap.
 }
 unsigned ExpectedMatrixInstructions()const{return MatrixPhasesPerPanel()*FfnPersistentPeer::groups*(inputs/16);}
 unsigned ExpectedReadPackets()const{return (inputs*2+16383)/16384+1+FfnPersistentPeer::groups*(inputs/(type==8?32:256));}
 const std::vector<uint8_t>* Rom(unsigned address,unsigned& base)const override{
  if(Range(address,0x30000,unsigned(precise.size()))){base=0x30000;return &precise;}
  return GlobalInitialPeer::Rom(address,base);
 }
 uint32_t Load(uint32_t address)override{
  if(Range(address,0xffb20000,40) && pushCommand){
   assert(!(address&3));const unsigned i=(address-0xffb20000)/4;pushReadback[i]=true;return command[i];
  }
  if(Range(address,Push::PushBuffer,256))return memory.at(address);
  if(address==0xffb20040 && pushes){doorbellRead=true;return 0;} // accepted is NOT a payload ACK;old reads keep their real completion handler.
  if(pushes)for(unsigned i=0;i<8;i++)if(address==0xffb20000+CounterOffset(i))assert(doorbellRead);
  if(address==0xffb20240 && heldPayload)return 1;
  return GlobalInitialPeer::Load(address);
 }
 void Store(uint32_t address,uint32_t value)override{
  if(Range(address,0xffb20000,40) && Range(pc,M::Code,unsigned(front.size()))
   && matrixInstructions==ExpectedMatrixInstructions() && requests==ExpectedReadPackets() && responses==requests){
   assert(!(address&3) && !pending && !heldPayload && drained && pushes<2 && CorrectRows() && !dotProbeReads);
   const unsigned i=(address-0xffb20000)/4,bytes=pushes?4:256;
   const uint32_t wanted[]={Push::PushBuffer,0,selfRoute,Push::Slot(logicalWorker),0,Push::Root,0,0x212,bytes,0};
   assert(i==programmed && value==wanted[i]);
   if(!i)pushReadback.fill(false);
   command[i]=value;programmed++;pushCommand=true;return;
  }
  if(Range(address,Push::PushBuffer,256)){
   assert(Range(pc,M::Code,unsigned(front.size())) && !pending);memory[address]=value;return;
  }
  if(address==0xffb20040 && command[7]==0x212){
   assert(value==1 && pushCommand && programmed==10 && !pending && !heldPayload && pushes<2 && matrixInstructions==ExpectedMatrixInstructions()
    && requests==ExpectedReadPackets() && responses==requests && CorrectRows() && !dotProbeReads);
   const unsigned bytes=pushes?4:256;
   const uint32_t wanted[]={Push::PushBuffer,0,selfRoute,Push::Slot(logicalWorker),0,Push::Root,0,0x212,bytes,0};
   for(unsigned i=0;i<10;i++)assert(command[i]==wanted[i] && pushReadback[i]);programmed=0;
   if(!pushes){
    for(unsigned i=0;i<8;i++)beforePush[i]=Count(i);
    for(unsigned i=0;i<64;i++)payload[i]=memory.at(Push::PushBuffer+i*4);
    assert(!payload[0]);heldPayload=true;
   }else{
    assert(mailbox==payload && !mailbox[0] && memory.at(Push::PushBuffer)==Push::ReceiptMagic);
    mailbox[0]=Push::ReceiptMagic;rootSlave[1]++;rootSlave[2]++;rootSlave[3]++;countBase[4]++;
    if(queueAtCommit){
     memory[M::Job+4]=2;memory[M::Job+7*4]=2026;memory[M::Doorbell]=2;
    } // ROOT may queue the next descriptor as soon as the final commit arrives.
   }
   rootSlave[0]++;countBase[2]++;countBase[6]++;countBase[5]+=(bytes+63)/64;countBase[7]++;pushes++;doorbellRead=false;
   return;
  }
  GlobalInitialPeer::Store(address,value);
 }
 void BootPushed(){
  PoisonDst();Poison();while(pc!=waitPC && pc!=faultPC)StepBlock();
  assert(pc==waitPC && !sequence && !requests && !pushes && !memory.at(M::Status+8)
   && memory.at(M::Status+12)==Push::Magic);
 }
 void StagePushed(){
  std::array<uint32_t,64> job={};const unsigned head[]={Push::Magic,1,1,logicalWorker,selfRoute,0,0,2025};
  for(unsigned i=0;i<8;i++){job[i]=head[i];job[8+i]=boot[8+i];}
  for(unsigned i=0;i<10;i++)job[16+i]=matrixJob[i];job[26]=1;
  assert(Push::JobShape(job.data(),boot.data(),0,0));
  for(unsigned i=0;i<64;i++)memory[M::Job+i*4]=job[i];memory[M::Doorbell]=1;sequence=1;
 }
 void AwaitPayload(){
  while(!heldPayload && pc!=faultPC)StepBlock();
  assert(heldPayload && pushes==1 && pc!=faultPC && !mailbox[0] && !memory.at(M::Status+4));
  const auto unchanged=mailbox;
  for(unsigned i=0;i<2048;i++)StepBlock();
  assert(pushes==1 && heldPayload && mailbox==unchanged && pc!=faultPC && !memory.at(M::Status+4));
 } // actual sender is stalled by missing ACK,even though command busy is0.
 void AckPayload(){
  assert(heldPayload && pushes==1);mailbox=payload;rootSlave[1]+=4;rootSlave[2]++;rootSlave[3]++;
  countBase[4]++;heldPayload=false;
 }
 void FaultAfterPayload(unsigned word,unsigned completed=0){
  AckPayload();source[word]^=1U<<((word*7+5)%32);
  while(pc!=faultPC && pc!=stopPC)StepBlock();
  assert(pc==faultPC && memory.at(M::Status+8)==Push::Fault && memory.at(M::Status+4)==completed
   && pushes==1 && !mailbox[0] && !heldPayload && !pending && minStack>=0x6e000);
  const auto data=memory;const auto remote=mailbox;const auto slave=rootSlave;std::array<uint32_t,10> commands;
  std::array<uint32_t,8> counts;for(unsigned i=0;i<10;i++)commands[i]=command[i];for(unsigned i=0;i<8;i++)counts[i]=Count(i);
  for(unsigned i=0;i<2048;i++)StepBlock();
  assert(pc==faultPC && memory==data && mailbox==remote && rootSlave==slave && pushes==1 && rootMaster==rootOriginal);
  for(unsigned i=0;i<10;i++)assert(command[i]==commands[i]);for(unsigned i=0;i<8;i++)assert(Count(i)==counts[i]);
 }
 void TimeoutPayload(unsigned completed=0){
  assert(heldPayload && pushes==1);
  while(pc!=faultPC && pc!=stopPC)StepBlock();
  assert(pc==faultPC && memory.at(M::Status+8)==Push::Fault && memory.at(M::Status+4)==completed
   && heldPayload && pushes==1 && !mailbox[0] && !pending && rootMaster==rootOriginal && minStack>=0x6e000);
  const auto data=memory;const auto remote=mailbox;const auto slave=rootSlave;
  std::array<uint32_t,10> commands;std::array<uint32_t,8> counts;
  for(unsigned i=0;i<10;i++)commands[i]=command[i];for(unsigned i=0;i<8;i++)counts[i]=Count(i);
  for(unsigned i=0;i<2048;i++)StepBlock();
  assert(pc==faultPC && memory==data && mailbox==remote && rootSlave==slave && heldPayload && pushes==1 && rootMaster==rootOriginal);
  for(unsigned i=0;i<10;i++)assert(command[i]==commands[i]);for(unsigned i=0;i<8;i++)assert(Count(i)==counts[i]);
 }
 void FinishPushed(bool queued=false){
  queueAtCommit=queued;
  AckPayload();while(!GdnFinished(1) && pc!=faultPC)StepBlock();
  const std::array<uint32_t,4> expectedSlave={{2,5,2,2}};
  assert(pc==waitPC && !memory.at(M::Status+8) && pushes==2 && matrixInstructions==ExpectedMatrixInstructions()
   && requests==ExpectedReadPackets() && responses==requests
   && !heldPayload && rootMaster==rootOriginal && rootSlave==expectedSlave && minStack>=0x6e000);
  std::array<uint32_t,64> job={};for(unsigned i=0;i<32;i++)job[i]=payload[8+i];
  assert(memory.at(M::Doorbell)==(queued?2U:1U) && memory.at(M::Job+4)==(queued?2U:1U)
   && memory.at(M::Job+7*4)==(queued?2026U:2025U) && memory.at(M::Status+6*4)==job[5] && memory.at(M::Status+7*4)==job[6]);
  assert(Push::Receipt(mailbox.data(),boot.data(),job.data(),beforePush.data()));
  for(unsigned word=0;word<64;word++){
   auto changed=mailbox;changed[word]^=1U<<((word*7+5)%32);
   assert(!Push::Receipt(changed.data(),boot.data(),job.data(),beforePush.data()));
  }
  const unsigned delta[]={0,0,2,0,2,5,2,2};
  for(unsigned i=0;i<8;i++)assert(mailbox[56+i]==beforePush[i] && Count(i)==beforePush[i]+delta[i]);
  assert(!Count(4) && Count(5)==1 && !Count(7));
  printf("PASS QGW3 worker%u/type%u/ACTUAL fullK%u+%utensor instructions/%u genuine reads+256B payload ACK THEN4B commit/2048 missing-ACK steps/no ROOT master-counter change/ROOT slave delta2,5,2,2/real sequence1 WAIT/queued-next-descriptor%u;SYNTHETIC endpoints,NOT model/native/physical/TPS\n",logicalWorker,type,inputs,matrixInstructions,requests,unsigned(queued));
 }
};
#ifndef PIONEER_Q38_PUSHED_WORKER_MACHINE_ONLY
int main(int argc,char** argv){
 assert(argc==3);setvbuf(stdout,nullptr,_IOLBF,0);(void)&RunPersistentGdnPair;unsigned faults=0;
 for(unsigned worker:{1U,96U,119U}){
  auto peer=std::make_unique<PushedPeer>(argv[1],argv[2],worker);peer->BootPushed();peer->StagePushed();peer->AwaitPayload();
  for(unsigned word=0;word<16;word++){
   fflush(nullptr);const pid_t pid=fork();assert(pid>=0);
   if(!pid){peer->FaultAfterPayload(word);_exit(0);}
   int status=0;assert(waitpid(pid,&status,0)==pid && WIFEXITED(status) && !WEXITSTATUS(status));faults++;
  }
  for(unsigned branch=0;branch<2;branch++){
   fflush(nullptr);const pid_t pid=fork();assert(pid>=0);
   if(!pid){if(branch)peer->FinishPushed(true);else peer->TimeoutPayload();_exit(0);}
   int status=0;assert(waitpid(pid,&status,0)==pid && WIFEXITED(status) && !WEXITSTATUS(status));
  }
  peer->FinishPushed();
 }
 printf("PASS QGW3 actual publisher/3 full tensor jobs+ACK-gated completion messages/%u source faults after real payload before commit/3 finite missing-ACK timeouts/no ready marker,following packet,retry or cleanup/inert2048 each/3 next-descriptor overwrite-at-commit branches return safely to sequence1 WAIT;NO next-job execution,ROOT consumer,original27B,native,physical,Work or TPS qualification\n",faults);
}
#endif
