/* SPDX-License-Identifier: MIT */
// A real descriptor queued AT the previous commit must execute,not merely
// reach the previous WAIT. TEST-only raw endpoints;no native grant or TPS.
#define PIONEER_Q38_PUSHED_WORKER_MACHINE_ONLY
#include "pioneer_q38_pushed_worker_test.cpp"
struct QueuedPushedPeer:PushedPeer {
 using PushedPeer::PushedPeer;
 bool second=false,firstRead=false;
 void Store(uint32_t a,uint32_t value)override{
  if(second && a==0xffb20040 && command[7]==0 && !requests){
   assert(value==1 && !firstRead && !matrixInstructions && !pushes && !mailbox[0]
    && memory.at(M::Status)==2 && memory.at(M::Status+4)==1
    && memory.at(M::Job+4)==2 && memory.at(M::Doorbell)==2);
   firstRead=true;
  }
  PushedPeer::Store(a,value);
 }
 void ReceiveQueued(){
  assert(pc==waitPC && GdnFinished(1) && pushes==2 && !heldPayload && !pending
   && memory.at(M::Job+4)==2 && memory.at(M::Job+7*4)==2026 && memory.at(M::Doorbell)==2);
  ClearTensorState();std::array<uint32_t,8> physical;
  for(unsigned i=0;i<8;i++)physical[i]=Count(i);
  for(unsigned i=0;i<8;i++)countBase[i]=physical[i];
  NextPackets(); // endpoint bookkeeping only;no processor/private state changes.
  FreshInputs(*this);PoisonTail();
  for(unsigned i=0;i<4096;i++)buffers[4096+i]=0x7fc00000;
  mailbox.fill(0);payload.fill(0);pushes=programmed=0;pushCommand=false;pushReadback.fill(false);queueAtCommit=false;
  sequence=2;second=true; // transport observer sequence,NOT the actual RISCV's previous/nonce.
  for(unsigned i=0;i<8;i++)assert(Count(i)==physical[i]);
  std::array<uint32_t,64> job;for(unsigned i=0;i<64;i++)job[i]=memory.at(M::Job+i*4);
  assert(Push::JobShape(job.data(),boot.data(),1,0));
 }
 void AwaitSecondPayload(){
  while(!heldPayload && !GdnFaulted())StepBlock();
  assert(firstRead && heldPayload && pushes==1 && !GdnFaulted() && !mailbox[0]
   && memory.at(M::Status)==2 && memory.at(M::Status+4)==1);
  const auto unchanged=mailbox;
  for(unsigned i=0;i<2048;i++)StepBlock();
  assert(heldPayload && pushes==1 && mailbox==unchanged && !GdnFaulted() && memory.at(M::Status+4)==1);
 }
 void FinishSecond(const std::array<uint32_t,16>& first){
  AckPayload();while(!GdnFinished(2) && !GdnFaulted())StepBlock();
  const std::array<uint32_t,4> expectedSlave={{4,10,4,4}};
  const unsigned expectedPhases=type==12?2:4,expectedReads=type==8?162:22;
  assert(pc==waitPC && !GdnFaulted() && !memory.at(M::Status+8) && firstRead && !pending && !heldPayload
   && inputs==5120 && FfnPersistentPeer::groups==1 && MatrixPhasesPerPanel()==expectedPhases
   && pushes==2 && !programmed && matrixInstructions==expectedPhases*320 && requests==expectedReads && responses==expectedReads
   && !dotProbeReads && CorrectRows() && rootMaster==rootOriginal && rootSlave==expectedSlave && minStack>=0x6e000);
  bool changed=false;for(unsigned i=0;i<16;i++)changed|=first[i]!=buffers[4096+i];assert(changed);
  std::array<uint32_t,64> job={};for(unsigned i=0;i<64;i++)job[i]=memory.at(M::Job+i*4);
  assert(job[1]==2 && job[7]==2026 && memory.at(M::Status+4)==2 && memory.at(M::Doorbell)==2
   && Push::Receipt(mailbox.data(),boot.data(),job.data(),beforePush.data()));
  for(unsigned i=0;i<64;i++){
   auto corrupt=mailbox;corrupt[i]^=1U<<((i*7+5)%32);
   assert(!Push::Receipt(corrupt.data(),boot.data(),job.data(),beforePush.data()));
  }
  const unsigned delta[]={0,0,2,0,2,5,2,2};for(unsigned i=0;i<8;i++)assert(Count(i)==beforePush[i]+delta[i]);
  printf("PASS QGW3 worker%u/type%u/descriptor2 queued DURING genuine commit1/actual next-job first read+all%u tensor instructions+%u reads/changed raw inputs+changed exact output/second ACK-gated payload+commit/receipt2+64 mutations/cumulative slave4,10,4,4/SAME processor+tensors;NOT ROOT admission/fullmodel/native/physical/Work/TPS qualification\n",logicalWorker,type,matrixInstructions,requests);
 }
};
int main(int argc,char** argv){
 assert(argc==3);setvbuf(stdout,nullptr,_IOLBF,0);(void)&RunPersistentGdnPair;unsigned faults=0;
 for(unsigned type:{8U,12U,14U})for(unsigned worker:{1U,96U,119U}){
  auto p=std::make_unique<QueuedPushedPeer>(argv[1],argv[2],worker,type);
  p->BootPushed();p->StagePushed();p->AwaitPayload();p->FinishPushed(true);
  std::array<uint32_t,16> first;for(unsigned i=0;i<16;i++)first[i]=p->buffers[4096+i];
  p->ReceiveQueued();p->AwaitSecondPayload();
  for(unsigned mode=0;mode<17;mode++){
   fflush(nullptr);const pid_t pid=fork();assert(pid>=0);
   if(!pid){if(mode<16)p->FaultAfterPayload(mode,1);else p->TimeoutPayload(1);_exit(0);}
   int status=0;assert(waitpid(pid,&status,0)==pid && WIFEXITED(status) && !WEXITSTATUS(status));faults++;
  }
  p->FinishSecond(first);
 }
 printf("PASS actual G3 queued-next EXECUTION/9 retained two-job histories/all3 formats/%u second-payload source+missingACK refusals/inert2048/no following commit,retry or cleanup;NO ROOT/native/Work or model-speed grant\n",faults);
}
