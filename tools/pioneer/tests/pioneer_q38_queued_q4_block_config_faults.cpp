/* SPDX-License-Identifier: MIT */
// Actual compiled queued-E frontend; MOCK config/fabric endpoints only.
// This tests final-word failure handling,NOT missing backend completion.
#define PIONEER_Q38_PREPARED_WORKER_MACHINE_ONLY
#include "pioneer_q38_prepared_exact_q4_worker_test.cpp"
struct QueuedConfigFaultPeer:QueuedQ4BlockPeer {
 using QueuedQ4BlockPeer::QueuedQ4BlockPeer;
 unsigned readFault=0,stateFault=0;bool injected=false;
 std::vector<unsigned> verifyReads;
 unsigned Load(unsigned at)override{
  static const unsigned fields[]={50,64,72,76,84,124};
  if(exact && queueBlocks==1 && !queued && verifyReads.size()<6
    && Range(at,0xffef0000,224*4) && Field((at-0xffef0000)/4)){
   assert(MathIdle() && queueWaits==64);
   const unsigned index=(at-0xffef0000)/4;
   if(index!=fields[verifyReads.size()])fprintf(stderr,"CONFIG_FINAL_READ_ORDER actual%u expected%u position%zu pc%08x\n",index,fields[verifyReads.size()],verifyReads.size(),pc);
   assert(index==fields[verifyReads.size()]);verifyReads.push_back(index);
   if(stateFault==verifyReads.size()){assert(!injected);config[index]^=1;injected=true;}
   const unsigned value=QueuedQ4BlockPeer::Load(at);
   if(readFault==verifyReads.size()){assert(!injected);injected=true;return value^1;}
   return value;
  }
  return QueuedQ4BlockPeer::Load(at);
 }
};
int main(int argc,char** argv){
 assert(argc==3);setvbuf(stdout,nullptr,_IOLBF,0);(void)&RunPersistentGdnPair;(void)&Compare;(void)&Round;
 unsigned failures=0;
 for(unsigned mode=0;mode<2;mode++)for(unsigned event=1;event<=6;event++){
  auto p=std::make_unique<QueuedConfigFaultPeer>(argv[1],argv[2],12,5120,1);
  if(mode)p->stateFault=event;else p->readFault=event;
  p->throwOnFault=true;p->delay=3;bool caught=false;
  try{p->BootPushed();p->Prepare();p->StagePushed();p->AwaitPayload();}
  catch(const InstructionPrefetchPeer::ReachedFault&){caught=true;}
  assert(caught && p->injected && p->pc==p->faultPC && p->verifyReads.size()==event
   && p->queueBlocks==1 && p->queueWaits==64 && !p->queued && p->MathIdle()
   && !p->heldPayload && !p->pushes && !p->mailbox[0] && !p->exactJobs && p->prefetchEvents==10);
  const auto memory=p->memory;const auto reads=p->verifyReads;
  const unsigned requests=p->requests,responses=p->responses,writes=p->configWrites,ops=p->queueWrites;
  std::array<unsigned,224> config;std::copy(std::begin(p->config),std::end(p->config),config.begin());
  p->throwOnFault=false;for(unsigned i=0;i<2048;i++)p->StepBlock();
  assert(p->pc==p->faultPC && p->memory==memory && p->verifyReads==reads
   && p->requests==requests && p->responses==responses && p->configWrites==writes && p->queueWrites==ops
   && std::equal(config.begin(),config.end(),std::begin(p->config)) && !p->heldPayload && !p->pushes && !p->mailbox[0]);
  printf("PASS_QUEUED_COMPILED_FINAL_CONFIG_FAULT mode%u read%u inertSteps2048 noFollowingReadWriteNoCOutputCommitRestore\n",mode,event);failures++;
 }
 auto p=std::make_unique<QueuedConfigFaultPeer>(argv[1],argv[2],12,5120,1);
 p->delay=3;p->BootPushed();p->Prepare();p->StagePushed();p->AwaitPayload();p->FinishPushed();p->CheckEnd();p->StopLocal();
 assert(p->verifyReads.size()==6 && !p->injected && p->prefetchEvents==18);
 printf("PASS_QUEUED_COMPILED_CONFIG failures%u normalCompleteAndSTOP1 realFinalACK64PhaseWaits finalWordReads6 NO_BACKPRESSURE_MISSING_COMPLETION_HISTORY_NATIVE_HARDWARE_OR_SPEED_GRANT\n",failures);
}
