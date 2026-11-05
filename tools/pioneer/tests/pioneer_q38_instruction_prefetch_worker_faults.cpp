/* SPDX-License-Identifier: MIT */
// Actual compiled frontend over MOCK fixed config/fabric endpoints.
#define PIONEER_Q38_PREPARED_WORKER_MACHINE_ONLY
#include "pioneer_q38_prepared_exact_q4_worker_test.cpp"
static bool ReadEvent(unsigned event){
 return event!=5 && event!=7 && event!=9 && event!=13 && event!=15 && event!=17;
}
static void Fault(InstructionPrefetchPeer& p,unsigned event){
 if(ReadEvent(event))p.corruptRead=event;else p.corruptWrite=event;
 p.throwOnFault=true;bool caught=false;
 try{if(event<=10)p.BootPushed();else p.StopLocal();}catch(const InstructionPrefetchPeer::ReachedFault&){caught=true;}
 const unsigned detected=ReadEvent(event)?(event<=2?event+2:event):event+1;
 assert(caught && p.pc==p.faultPC && p.prefetchEvents==detected && !p.heldPayload);
 if(event<=10)assert(!p.requests && !p.matrixInstructions && !p.pushes);
 const auto config203=p.config[203],config208=p.config[208],requests=p.requests;
 p.throwOnFault=false;for(unsigned i=0;i<2048;i++)p.StepBlock();
 assert(p.pc==p.faultPC && p.prefetchEvents==detected && p.requests==requests
  && p.config[203]==config203 && p.config[208]==config208);
}
int main(int argc,char** argv){
 assert(argc==3);setvbuf(stdout,nullptr,_IOLBF,0);(void)&RunPersistentGdnPair;(void)&Compare;(void)&Round;
 unsigned initial=0,terminal=0,positive=0;
 for(unsigned event=1;event<=10;event++){
  auto p=std::make_unique<InstructionPrefetchPeer>(argv[1],argv[2],12,5120,1,0,69,0x47,0xabcdef13,0x34567);
  Fault(*p,event);initial++;
 }
 auto p=std::make_unique<InstructionPrefetchPeer>(argv[1],argv[2],12,5120,1,0,69,0x47,0xabcdef13,0x34567);
 p->delay=3;p->BootPushed();p->Prepare();p->StagePushed();p->AwaitPayload();p->FinishPushed();p->CheckEnd();
 assert(p->prefetchEvents==10 && p->config[203]==0xc0c67 && p->config[208]==((0xabcdef13U&~0x1e1U)|0x81U));
 for(unsigned event=11;event<=18;event++){
  fflush(nullptr);const auto child=fork();assert(child>=0);
  if(!child){Fault(*p,event);_exit(0);}
  int status=0;assert(waitpid(child,&status,0)==child && WIFEXITED(status) && !WEXITSTATUS(status));terminal++;
 }
 p->StopLocal();assert(p->prefetchEvents==18 && p->config[203]==0x34567 && p->config[208]==0xabcdef13);positive++;
 for(unsigned control:{0U,1U,0xffffffffU,0x12345678U}){
  auto boot=std::make_unique<InstructionPrefetchPeer>(argv[1],argv[2],12,5120,1,0,69,0x47,control,0xffffffffU);
  boot->BootPushed();assert(boot->prefetchEvents==10 && !boot->requests && !boot->matrixInstructions);positive++;
 }
 printf("PASS_COMPILED_PREFETCH_FRONTEND initFaults%u restoreFaults%u positives%u every18_IO_boundary two_stable_initial_reads consumed_write_readbacks fault_parks2048_NO_following_config_or_NoC_IO normal_STOP_exact_original203_208;MOCK_ONLY_NOT_CACHE_TIMING_OR_HARDWARE\n",initial,terminal,positive);
}
