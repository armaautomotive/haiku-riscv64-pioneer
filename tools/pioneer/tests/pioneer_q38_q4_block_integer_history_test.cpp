/* SPDX-License-Identifier: MIT */
// SAME compiled CPU/register/Dst/config/NoC history,Q4/GDN/changed Q4/STOP.
#define PIONEER_Q38_BLOCK_INTEGER_WORKER_MACHINE_ONLY
#include "pioneer_q38_q4_block_integer_worker_test.cpp"
static void ReturnToIntegerQ4(BlockIntegerPeer& p){
 assert(p.GdnFinished(3) && !p.pending && p.MathIdle() && p.BanksFree());
 p.ClearTensorState();std::array<unsigned,8> physical{};
 for(unsigned i=0;i<8;i++)physical[i]=p.Count(i);
 for(unsigned i=0;i<8;i++)p.countBase[i]=physical[i];p.NextPackets();
 // Retire mock packet/receipt observations only. Preserve every actual
 // guest register,PC,LReg,Dst,replay,configuration,bank and NoC counter.
 p.gdnLifetime=p.gdnActive=p.convActive=p.activation=p.outputActive=p.denseActive=false;
 FreshInputs(p);p.PrepareSource(4);
 p.mailbox.fill(0);p.payload.fill(0);p.pushes=p.programmed=0;p.pushCommand=false;p.pushReadback.fill(false);p.queueAtCommit=false;
 p.stageCalls=p.waitCalls=p.restoreCommands=p.rawWrites=0;p.validatedTile=UINT32_MAX;
 for(unsigned i=0;i<8;i++)assert(p.Count(i)==physical[i]);
 p.Prepare();std::array<unsigned,64> job{};
 const unsigned head[]={Push::Magic,4,1,p.logicalWorker,p.selfRoute,0,0,2027};
 for(unsigned i=0;i<8;i++){job[i]=head[i];job[8+i]=p.boot[8+i];}
 for(unsigned i=0;i<10;i++)job[16+i]=p.matrixJob[i];job[26]=1;
 assert(Push::JobShape(job.data(),p.boot.data(),3,0));
 for(unsigned i=0;i<64;i++)p.memory[M::Job+i*4]=job[i];p.memory[M::Doorbell]=4;p.sequence=4;
 while(!p.heldPayload && !p.GdnFaulted())p.StepBlock();
 assert(!p.GdnFaulted() && p.heldPayload && p.pushes==1 && !p.mailbox[0] && p.memory.at(M::Status+4)==3);
 const auto mailbox=p.mailbox;for(unsigned i=0;i<2048;i++)p.StepBlock();
 assert(p.heldPayload && p.pushes==1 && p.mailbox==mailbox && p.memory.at(M::Status+4)==3);
 p.AckPayload();while(!p.GdnFinished(4) && !p.GdnFaulted())p.StepBlock();
 assert(p.GdnFinished(4) && !p.GdnFaulted() && p.pushes==2 && !p.heldPayload && p.rootMaster==p.rootOriginal);
 assert(Push::Receipt(p.mailbox.data(),p.boot.data(),job.data(),p.beforePush.data()));
 const unsigned delta[]={0,0,2,0,2,5,2,2};for(unsigned i=0;i<8;i++)assert(p.Count(i)==p.beforePush[i]+delta[i]);
 p.CheckEnd();assert(p.exactJobs==2);
}
int main(int argc,char** argv){
 assert(argc==3);setvbuf(stdout,nullptr,_IOLBF,0);(void)&Compare;(void)&Round;
 unsigned pairs=0,stack=0;
 for(unsigned head=0;head<48;head++){
  auto a=std::make_unique<BlockIntegerPeer>(argv[1],argv[2],12,5120,1,true,0,head*2+1);
  auto b=std::make_unique<BlockIntegerPeer>(argv[1],argv[2],12,5120,1,true,0,head*2+2);
  a->peer=b.get();b->peer=a.get();
  for(auto* p:{a.get(),b.get()}){
   p->delay=3;p->BootPushed();p->Prepare();p->StagePushed();p->AwaitPayload();p->FinishPushed();p->CheckEnd();
   assert(p->exactJobs==1);p->StageInput(1002,1,0xc09dde93);
  }
  assert(RunPersistentGdnPair(*a,*b,2));a->CheckPreciseBeta();b->CheckPreciseBeta();
  for(auto* p:{a.get(),b.get()}){p->prior=p->next;p->StageInput(1003,2,0x3f81fec2);}
  assert(RunPersistentGdnPair(*a,*b,3));a->CheckPreciseBeta();b->CheckPreciseBeta();
  for(auto* p:{a.get(),b.get()}){
   ReturnToIntegerQ4(*p);p->StopLocal();stack=std::max(stack,0x70000-p->minStack);
   assert(p->MathIdle() && p->BanksFree() && p->prefetchEvents==18);
  }
  printf("PASS_BLOCK_INTEGER_RETAINED_GDN head%u workers2 actualQ4Jobs4 GdnHalfJobs4 changedBetaAndPrior ReturnToChangedInputQ4 REAL_STOP2_NO_CPU_STATE_RESET_OR_SUPPLIED_RESULT\n",head);pairs++;
 }
 printf("PASS_BLOCK_INTEGER_Q4_GDN_HISTORY pairs%u workers%u Q4Jobs%u GdnHalfJobs%u combinedStack%u/8192 SAME_ACTUAL_CPU_CONFIG_REPLAY_DST_HISTORY MOCK_ENDPOINTS_NOT_NATIVE_MODEL_QUALITY_OR_SPEED\n",pairs,pairs*2,pairs*4,pairs*4,stack);
}
