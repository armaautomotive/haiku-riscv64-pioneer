/* SPDX-License-Identifier: MIT */
// Actual prepared matrix -> recurrent steps -> STOP in ONE retained worker
// instruction/memory history. Synthetic raw/source/NoC endpoints only.
#define PIONEER_Q38_PREPARED_WORKER_MACHINE_ONLY
#include "pioneer_q38_prepared_exact_q4_worker_test.cpp"
int main(int argc,char** argv){
 assert(argc==3);setvbuf(stdout,nullptr,_IOLBF,0);(void)&Compare;(void)&Round;unsigned pairs=0,stack=0;
 for(unsigned head:{0U,34U,47U}){
#ifdef PIONEER_Q38_INSTRUCTION_PREFETCH_WORKER_TEST
  auto a=std::make_unique<PreparedTestPeer>(argv[1],argv[2],12,5120,1,13,head*2+1,0x47,0xabcdef13U^head,0x12345000+head);
  auto b=std::make_unique<PreparedTestPeer>(argv[1],argv[2],12,5120,1,13,head*2+2,0x47,0x76543210U^head,0x45678000+head);
#else
  auto a=std::make_unique<PreparedTestPeer>(argv[1],argv[2],12,5120,1,13,head*2+1);
  auto b=std::make_unique<PreparedTestPeer>(argv[1],argv[2],12,5120,1,13,head*2+2);
#endif
  a->peer=b.get();b->peer=a.get();
  for(auto* p:{a.get(),b.get()}){
   p->delay=3;p->BootPushed();p->Prepare();p->StagePushed();p->AwaitPayload();p->FinishPushed();p->CheckEnd();
   p->StageInput(1002,1,0xc09dde93);
  }
  assert(RunPersistentGdnPair(*a,*b,2));a->CheckPreciseBeta();b->CheckPreciseBeta();
  for(auto* p:{a.get(),b.get()}){p->prior=p->next;p->StageInput(1003,2,0x3f81fec2);}
  assert(RunPersistentGdnPair(*a,*b,3));a->CheckPreciseBeta();b->CheckPreciseBeta();
  for(auto* p:{a.get(),b.get()}){p->StopLocal();stack=std::max(stack,0x70000-p->minStack);}
  printf("PASS_PREPARED_Q4_GDN_HISTORY head%u preparedQ4Matrices2 fragmentPrefix13 recurrentSteps2 retainedNextState1 actualSTOP2\n",head);pairs++;
 }
 printf("PASS_PREPARED_Q4_GDN_HISTORY pairs%u virginWorkers%u GdnHalfJobs%u stack%u/8192;NO_NATIVE_ROOT_MODEL_CARD_OR_TPS\n",pairs,pairs*2,pairs*4,stack);
}
