/* SPDX-License-Identifier: MIT */
// Actual NEW full worker from virgin boot,matrix+real ACK,retained paired
// GDN commands and STOP. Synthetic bytes/NoC only;no native/hardware grant.
#define PIONEER_Q38_WIDE_GDN_RETURNING_WORKER_TEST 1
#define PIONEER_Q38_TWIN_BATCH_PREFETCH_WORKER_MACHINE_ONLY
#include "pioneer_q38_twin_batch_prefetch_worker_test.cpp"
int main(int argc,char** argv)
{
 assert(argc==3);setvbuf(stdout,nullptr,_IOLBF,0);
 unsigned halfJobs=0,maxStack=0;
 for(unsigned head:{0U,31U,47U}){
  auto a=std::make_unique<TwinBatchPrefetchPeer>(argv[1],argv[2],head*2+1);
  auto b=std::make_unique<TwinBatchPrefetchPeer>(argv[1],argv[2],head*2+2);a->peer=b.get();b->peer=a.get();
  for(auto* p:{a.get(),b.get()}){
   p->delay=3;p->BootPushed();p->Prepare();p->StagePushed();p->AwaitPayload();p->FinishPushed();p->CheckEnd();
  }
  for(unsigned token=0;token<16;token++){
   for(auto* p:{a.get(),b.get()}){
    if(token)p->prior=p->next;
    p->StageInput(1002+token,token+1,token&1?0x3f81fec2U:0xc09dde93U);
   }
   assert(RunPersistentGdnPair(*a,*b,token+2));a->CheckPreciseBeta();b->CheckPreciseBeta();halfJobs+=2;
  }
  for(auto* p:{a.get(),b.get()}){p->StopLocal();maxStack=std::max(maxStack,0x70000-p->minStack);}
  printf("PASS_ACTUAL_WIDE_GDN_WORKER head%u virginBoot1 genuineTwinMatrixACK1 recurrentSteps16 actualPriorCarries15 realSTOP1 sharedCfgLRegsPhysicalDst1;SYNTHETIC_ENDPOINTS_NOT_NATIVE_ALL64_PHYSICAL_WORK_OR_TPS\n",head);
 }
 // Every NoC-stage error still reaches the actual inert worker fault loop.
 unsigned faults=0;
 for(unsigned kind=0;kind<3;kind++)for(unsigned packet:{1U,7U,9U,11U}){
  auto p=std::make_unique<TwinBatchPrefetchPeer>(argv[1],argv[2],1);
  p->delay=3;p->BootPushed();p->Prepare();p->StagePushed();p->AwaitPayload();p->FinishPushed();p->CheckEnd();p->StageInput(1002,1,0xc09dde93);
  if(!kind)p->gdnFailPacket=packet;else if(kind==1)p->gdnCounterPacket=packet;else p->gdnSourcePacket=packet;
  while(p->pc!=p->faultPC && p->pc!=p->stopPC)p->StepBlock();
  assert(p->pc==p->faultPC && p->gdnPackets==packet && !p->gdnGatedWrites);
  auto memory=p->memory;unsigned packets=p->gdnPackets,ops=p->gdnOps;
  for(unsigned i=0;i<2048;i++)p->StepBlock();
  assert(p->pc==p->faultPC && memory==p->memory && packets==p->gdnPackets && ops==p->gdnOps);faults++;
 }
 printf("PASS_ACTUAL_WIDE_GDN_WORKER_SUMMARY pairs3 virginWorkers6 retainedHalfJobs%u actualFullK5120MatrixThenWideFP64Order1 faultCases%u maxCombinedStack%u/8192;NO detached tensor simulation,reset,CPU product,owner,full-model,physical,Work or TPS grant\n",halfJobs,faults,maxStack);
}
