/* SPDX-License-Identifier: MIT */
// Actual retained instruction history,not a fresh second-job processor.
#define PIONEER_Q38_TWIN_BATCH_PREFETCH_WORKER_MACHINE_ONLY
#include "pioneer_q38_twin_batch_prefetch_worker_test.cpp"
struct TwinBatchQueuedPeer:TwinBatchPrefetchPeer {
 using TwinBatchPrefetchPeer::TwinBatchPrefetchPeer;
 bool second=false,firstRead=false;
 void Store(uint32_t at,uint32_t value)override {
  if(second && at==0xffb20040 && !command[7] && !requests){
   assert(value==1 && !firstRead && !matrixInstructions && !pushes && !mailbox[0]
    && memory.at(M::Status)==2 && memory.at(M::Status+4)==1
    && memory.at(M::Job+4)==2 && memory.at(M::Doorbell)==2);firstRead=true;
  }
  TwinBatchPrefetchPeer::Store(at,value);
 }
 void ReceiveQueued(){
  assert(pc==waitPC && GdnFinished(1) && pushes==2 && !heldPayload && !pending && !matrixScope
   && memory.at(M::Job+4)==2 && memory.at(M::Job+7*4)==2026 && memory.at(M::Doorbell)==2);
  ClearTensorState();std::array<uint32_t,8> physical;
  for(unsigned i=0;i<8;i++)physical[i]=Count(i);
  for(unsigned i=0;i<8;i++)countBase[i]=physical[i];NextPackets();
  FreshInputs(*this);for(unsigned i=0;i<4096;i++)buffers[4096+i]=0x7fc00000;
  mailbox.fill(0);payload.fill(0);pushes=programmed=0;pushCommand=false;pushReadback.fill(false);queueAtCommit=false;
  // Endpoint/per-job observation only:NEVER reset guest PC/registers/LRegs/
  // Dst,raw unpack,hardware counters or actual private return history.
  stageCalls=waitCalls=restoreCommands=rawWrites=0;validatedTile=UINT32_MAX;assert(!stageReturn);
  sequence=2;second=true;Prepare();
  for(unsigned i=0;i<8;i++)assert(Count(i)==physical[i]);
  std::array<uint32_t,64> job;for(unsigned i=0;i<64;i++)job[i]=memory.at(M::Job+i*4);
  assert(Push::JobShape(job.data(),boot.data(),1,0));
 }
 void AwaitSecond(){
  while(!heldPayload && !GdnFaulted())StepBlock();
  assert(firstRead && heldPayload && pushes==1 && !GdnFaulted() && !mailbox[0]
   && memory.at(M::Status)==2 && memory.at(M::Status+4)==1);
  auto unchanged=mailbox;for(unsigned i=0;i<2048;i++)StepBlock();
  assert(heldPayload && pushes==1 && mailbox==unchanged && !GdnFaulted() && memory.at(M::Status+4)==1);
 }
 void FinishSecond(const std::array<uint32_t,16>& first){
  AckPayload();while(!GdnFinished(2) && !GdnFaulted())StepBlock();
  const std::array<uint32_t,4> slave={{4,10,4,4}};
  assert(pc==waitPC && !GdnFaulted() && !memory.at(M::Status+8) && firstRead && !heldPayload && !pending
   && pushes==2 && !programmed && matrixInstructions==ExpectedMatrixInstructions() && requests==ExpectedReadPackets()
   && responses==requests && CorrectRows() && rootMaster==rootOriginal && rootSlave==slave);
  bool changed=false;for(unsigned i=0;i<16;i++)changed|=first[i]!=buffers[4096+i];assert(changed);
  std::array<uint32_t,64> job;for(unsigned i=0;i<64;i++)job[i]=memory.at(M::Job+i*4);
  assert(job[1]==2 && job[7]==2026 && memory.at(M::Status+4)==2 && memory.at(M::Doorbell)==2
   && Push::Receipt(mailbox.data(),boot.data(),job.data(),beforePush.data()));
  for(unsigned i=0;i<64;i++){auto bad=mailbox;bad[i]^=1U<<((i*7+5)%32);assert(!Push::Receipt(bad.data(),boot.data(),job.data(),beforePush.data()));}
  const unsigned delta[]={0,0,2,0,2,5,2,2};for(unsigned i=0;i<8;i++)assert(Count(i)==beforePush[i]+delta[i]);
  CheckEnd();StopLocal();
 }
};
int main(int argc,char** argv){
 assert(argc==3);setvbuf(stdout,nullptr,_IOLBF,0);(void)&RunPersistentGdnPair;unsigned cases=0,faults=0,stack=0;
 for(unsigned type:{8U,12U,14U})for(unsigned worker:{1U,96U,119U}){
  auto p=std::make_unique<TwinBatchQueuedPeer>(argv[1],argv[2],worker,type);p->delay=3;
  p->BootPushed();p->Prepare();p->StagePushed();p->AwaitPayload();p->FinishPushed(true);p->CheckEnd();
  std::array<uint32_t,16> first;for(unsigned i=0;i<16;i++)first[i]=p->buffers[4096+i];
  p->ReceiveQueued();p->AwaitSecond();
  for(unsigned mode=0;mode<17;mode++){
   fflush(nullptr);pid_t pid=fork();assert(pid>=0);
   if(!pid){if(mode<16)p->FaultAfterPayload(mode,1);else p->TimeoutPayload(1);_exit(0);}
   int status=0;assert(waitpid(pid,&status,0)==pid && WIFEXITED(status) && !WEXITSTATUS(status));faults++;
  }
  p->FinishSecond(first);stack=std::max(stack,0x70000-p->minStack);cases++;
  printf("PASS_ACTUAL_TWIN_BATCH_PREFETCH_QUEUED worker%u type%u twoChangedInputJobs1 genuineQueuedDescriptor1 secondPayloadCommit1 realStop1;NO_NATIVE_OR_TPS\n",worker,type);
 }
 printf("PASS_ACTUAL_TWIN_BATCH_PREFETCH_QUEUED_SUMMARY retainedWorkers%u secondPayloadFaults%u maxCombinedStack%u/8192;NO_DENSE_GLOBAL_OUTPUT_ALL119_NATIVE_MODEL_WORK_OR_TPS\n",cases,faults,stack);
}
