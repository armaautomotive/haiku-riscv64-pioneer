/* SPDX-License-Identifier: MIT */
// Genuine original embedding+virgin workers,then cold ROOT source audit COW.
#include "pioneer_q38_token_original_fabric.h"
struct ColdSourceFaultMachine:OriginalTokenFlowMachine {
 using OriginalTokenFlowMachine::OriginalTokenFlowMachine;
 bool forked=false,injected=false,rootSawFault=false,parentContinued=false;
 unsigned fault=16,cases=0,doorbells=0,packetsAtFault=0;
 std::array<uint32_t,8> physicalAfterInject={};
 static unsigned SourceWord(unsigned i){return 0xffb20000U+BlackholeDramSourceOffset(i);}
 void Predecessor(){
  assert(!position && !layer && !completed && !allLayers && !committed && !stopJobs
   && embedded==1 && !matrices && !convJobs && !gdnJobs);
  const auto& p=*child[0];
  assert(!p.active && !p.sequence && p.GdnFinished(0) && !p.GdnFaulted() && !p.pending
   && !p.memory.at(M::Status) && !p.memory.at(M::Status+4) && !p.memory.at(M::Status+8)
   && !p.requests && !p.responses && !p.matrixInstructions);
 }
 void Store(uint32_t address,uint32_t value)override{
  const bool packet=address==0xffb20040,write=command[7]==0x212;
  const unsigned remote=write?command[3]:command[0],route=write?command[5]:command[2];
  const bool target=packet && route==G::Peer(1);
  if(packet && injected)assert(!rootSawFault);
  if(target && !write && remote==SourceWord(0) && !forked && !child[0]->sequence){
   Predecessor();forked=true;
   puts("OBSERVED actual original embedding+119 virgin worker histories/BEFORE first cold remote source read;HOST COW only");
   for(unsigned index=0;index<16;index++){
    BeforeFork();fflush(nullptr);const pid_t pid=fork();assert(pid>=0);
    if(!pid){
     injected=true;fault=index;child[0]->source[index]^=1U<<((index*7+5)%32);
     for(unsigned i=0;i<8;i++)physicalAfterInject[i]=child[0]->Count(i);
     OriginalTokenFlowMachine::Store(address,value);
     if(remote==SourceWord(fault)){rootSawFault=true;packetsAtFault=packets;}
     return; // Continue SAME actual store instruction,not a supplied reply.
    }
    int status=0;assert(waitpid(pid,&status,0)==pid && WIFEXITED(status) && !WEXITSTATUS(status));cases++;
   }
   assert(cases==16 && !injected && !doorbells);Predecessor();
  }
  OriginalTokenFlowMachine::Store(address,value);
  // A missed observation must FAIL at the first real matrix job,not run
  // the remaining model indefinitely or report a vacuous fault pass.
  if(target && write && remote==M::Job && child[0]->memory.at(M::Job+8)==1)
   assert(forked && cases==16 && !injected);
  if(target && !write && injected && remote==SourceWord(fault)){
   assert(memory.at(command[3])!=child[0]->boot[16+fault]);rootSawFault=true;packetsAtFault=packets;
  }
  if(target && write && remote==M::Doorbell && forked){
   assert(!injected && !doorbells && child[0]->sequence==1);doorbells++;
   auto& p=*child[0];
   for(unsigned i=0;i<1000000 && !p.requests && !p.GdnFaulted();i++){cycles+=19;p.StepBlock();}
   assert(p.requests==1 && !p.GdnFaulted() && !p.matrixInstructions);parentContinued=true;
  }
 }
 void CheckFault(){
  assert(injected && fault<16 && rootSawFault && pc==0x60000 && packets==packetsAtFault && !doorbells
   && !child[0]->sequence && !child[0]->active && !child[0]->GdnFaulted() && !child[0]->requests
   && !child[0]->matrixInstructions && !allLayers && !committed && !stopJobs && original.Ready());
  for(unsigned i=0;i<8;i++)assert(child[0]->Count(i)==physicalAfterInject[i]);
  const auto commands=command;const auto counters=counts;const auto writes=peerWrites;
  for(unsigned i=0;i<40;i++)StepToken();
  assert(memory.at(0x75448)==0x51465431 && packets==packetsAtFault && command==commands && counts==counters);
  const auto frozen=memory;const auto parkedPC=pc;
  for(unsigned i=0;i<2048;i++)StepToken();
  assert(pc==parkedPC && memory==frozen && packets==packetsAtFault && peerWrites==writes && command==commands && counts==counters);
  printf("PASS original cold source%u/ROOT rejects BEFORE first matrix doorbell/worker remains virgin/no following packet/actual fault writer+2048 inert steps;NOT native/physical/Work/TPS\n",fault);
 }
};
int main(int argc,char** argv){
 assert(argc==5);setvbuf(stdout,nullptr,_IOLBF,0);(void)&RunPersistentGdnPair;(void)&Refusals;(void)&LateRefusals;
 WholeModelOriginalBytes original;assert(original.Open(argv[4]) && original.Ready());
 auto m=std::make_unique<ColdSourceFaultMachine>(argv[1],argv[2],argv[3],original);m->VerifyRawEndpoints();
 while(m->pc!=0x60000 && m->pc!=0x60004 && !m->parentContinued){
  m->cycles+=19;m->StepToken();if(!(m->steps&255))for(unsigned w=0;w<119;w++)m->Advance(w,64);
 }
 if(m->injected){m->CheckFault();_exit(0);}
 assert(m->forked && m->cases==16 && m->parentContinued && m->doorbells==1 && !m->allLayers && !m->committed
  && !m->stopJobs && original.Ready());
 puts("PASS original cold remote audit/ALL16 physical-source refusals/unchanged parent actual first matrix packet;no fabricated processor history or products;NOT native/physical/Work/TPS");
}
