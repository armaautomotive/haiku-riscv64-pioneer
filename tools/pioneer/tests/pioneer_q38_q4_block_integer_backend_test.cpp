/* SPDX-License-Identifier: MIT */
// Withhold a REAL compiled Drain acknowledgement. Never skip guest PCs,
// synthesize an output, clear pending math, or represent timeout as END.
#define PIONEER_Q38_BLOCK_INTEGER_WORKER_MACHINE_ONLY
#include "pioneer_q38_q4_block_integer_worker_test.cpp"

struct IntegerBackendPeer:BlockIntegerPeer {
 using BlockIntegerPeer::BlockIntegerPeer;
 unsigned selected=0,heldPolls=0,releaseAfter=0;
 bool held=false,injected=false,released=false;
 unsigned Condition()const{
  if(Tasks()){
   for(const auto& t:tasks)if(t.active && t.dst){
    if(t.zero && t.row==112)return 1; // explicit new scratch initialization
    if(t.format==14)return 2;       // original raw metadata load
   }
   return 4;                        // original A/B source-bank unpack
  }
  if(fpuPending)return 5;            // actual two-product FPU operation
  if(halfAndBaseStores==4 && !integerMads && sfpuPending)return 3;
  if(integerMads==4 && !sfpuPending)return 6; // first integer panel
  if(halfAndBaseStores==6 && integerMads==36 && sfpuPending)return 7;
  if(halfAndBaseStores==6 && integerMads==64 && sfpuPending)return 8;
  return 0;
 }
 void Instruction(unsigned op)override{
  assert(!held);
  if(exact && op==0x4500017e && !injected && Condition()==selected){
   assert(selected && backendWait && !gpr63);
   held=injected=true;return; // completion remains behind the backend wait
  }
  BlockIntegerPeer::Instruction(op);
 }
 unsigned Load(unsigned at)override{
  if(held && at==0xffe000fc){
   assert(exact && backendWait && !gpr63);
   heldPolls++;
   if(releaseAfter && heldPolls==releaseAfter){
    held=false;released=true;
    BlockIntegerPeer::Instruction(0x4500017e); // retire SAME pending work
   }
  }
  return BlockIntegerPeer::Load(at);
 }
};

static void Missing(const char* fixture,const char* image,unsigned selected){
 auto p=std::make_unique<IntegerBackendPeer>(fixture,image,12,5120,3);
 p->selected=selected;p->throwOnFault=true;p->delay=3;bool fault=false;
 try{p->BootPushed();p->Prepare();p->StagePushed();p->AwaitPayload();}
 catch(const InstructionPrefetchPeer::ReachedFault&){fault=true;}
 assert(fault && p->injected && p->held && !p->released && p->heldPolls==1000000
  && p->backendWait && !p->gpr63 && p->pc==p->faultPC && p->pc!=p->stopPC
  && !p->heldPayload && !p->pushes && !p->mailbox[0] && !p->exactJobs && p->prefetchEvents==10);
 const auto memory=p->memory;const auto mailbox=p->mailbox;
 const unsigned requests=p->requests,responses=p->responses,ops=p->vectorOps,matrices=p->matrixInstructions,
  stores=p->exactStores,polls=p->heldPolls,writes=p->configWrites;
 std::array<unsigned,224> config;std::copy(std::begin(p->config),std::end(p->config),config.begin());
 std::array<unsigned,68> thread;std::copy(std::begin(p->thread),std::end(p->thread),thread.begin());
 unsigned lanes[8][32];memcpy(lanes,p->math.lanes,sizeof(lanes));
 decltype(p->physical) dst;memcpy(dst,p->physical,sizeof(dst));
 const auto banks=[&](){return std::array<unsigned,14>{{
  unsigned(p->owned[0][0]),unsigned(p->owned[0][1]),unsigned(p->owned[1][0]),unsigned(p->owned[1][1]),
  p->unpack[0],p->unpack[1],p->matrix[0],p->matrix[1],p->partial[0],p->partial[1],p->rwcA,p->rwcB,p->crA,p->crB}};};
 const auto owned=banks();
 p->throwOnFault=false;for(unsigned i=0;i<2048;i++)p->StepBlock();
 assert(p->pc==p->faultPC && p->held && p->backendWait && !p->gpr63 && p->memory==memory && p->mailbox==mailbox
  && p->requests==requests && p->responses==responses && p->vectorOps==ops && p->matrixInstructions==matrices
  && p->exactStores==stores && p->heldPolls==polls && p->configWrites==writes
  && std::equal(config.begin(),config.end(),std::begin(p->config))
  && std::equal(thread.begin(),thread.end(),std::begin(p->thread)) && !memcmp(lanes,p->math.lanes,sizeof(lanes))
  && !memcmp(dst,p->physical,sizeof(dst)) && banks()==owned
  && !p->heldPayload && !p->pushes && !p->mailbox[0] && !p->exactJobs && p->prefetchEvents==10);
 printf("PASS_BLOCK_INTEGER_MISSING_BACKEND condition%u actualBoundedPolls%u inertFaultInstructions2048 RETAIN_PENDING_NO_RESULT_COMMIT_STOP_RESTORE_OR_END\n",selected,polls);
}

int main(int argc,char** argv){
 assert(argc==3);setvbuf(stdout,nullptr,_IOLBF,0);(void)&RunPersistentGdnPair;(void)&Compare;(void)&Round;
 for(unsigned selected=1;selected<=8;selected++){
  auto p=std::make_unique<IntegerBackendPeer>(argv[1],argv[2],12,5120,3);
  p->selected=selected;p->releaseAfter=257;p->delay=3;
  p->BootPushed();p->Prepare();p->StagePushed();p->AwaitPayload();p->FinishPushed();p->CheckEnd();p->StopLocal();
  assert(p->injected && p->released && !p->held && p->heldPolls==257 && p->exactJobs==1 && p->pc==p->stopPC);
  printf("PASS_BLOCK_INTEGER_DELAYED_BACKEND condition%u actualHeldPolls257 exactNewOracle_REAL_ACK_COMMIT_STOP\n",selected);
  Missing(argv[1],argv[2],selected);
 }
 puts("PASS_BLOCK_INTEGER_BACKEND delayed8_missing8_SAME_ACTUAL_ROM_AND_RV_POLL_INSTRUCTIONS MOCK_COMPLETION_ENDPOINTS_NOT_SILICON_TIMING_NATIVE_QUALITY_OR_TPS");
}
