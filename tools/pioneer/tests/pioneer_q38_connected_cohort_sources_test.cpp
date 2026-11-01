/* SPDX-License-Identifier: MIT */
// Actual original-byte ROOT+119 histories; COW only at real StageConv entry.
// Mutates raw physical CSRs,NEVER boot mirrors,products or processor history.
#include "pioneer_q38_token_original_fabric.h"
struct CohortSourceFaultMachine:OriginalTokenFlowMachine {
 using OriginalTokenFlowMachine::OriginalTokenFlowMachine;
 unsigned entry=0,fault=16,cases=0,doorbells=0,packetsAtFault=0;
 unsigned priorRequests=0,priorResponses=0,priorMath=0;
 bool forked=false,injected=false,parked=false,rootSawFault=false,parentContinued=false;
 std::array<uint32_t,8> physicalAfterInject={};
 void Predecessor(){
  assert(pc==entry && !position && !layer && !completed && !allLayers && !committed && !stopJobs
   && embedded==1 && matrices==4 && !convJobs && !gdnJobs && Phase()==6);
  const auto& p=*child[0];
  assert(!p.active && p.GdnFinished(p.sequence) && p.sequence==4 && !p.GdnFaulted() && !p.pending
   && p.memory.at(M::Job+8)==1 && p.pushPackets==2 && p.requests==p.packets.size() && p.CorrectRows());
 }
 void StepToken(){
  if(pc==entry && !forked && !position && !layer){
   Predecessor();forked=true;
   puts("OBSERVED actual original embedding+four closed matrices/BEFORE first StageConv instruction;HOST COW only");
   for(unsigned index=0;index<16;index++){
    BeforeFork();fflush(nullptr);const pid_t pid=fork();assert(pid>=0);
    if(!pid){
     injected=true;fault=index;auto& p=*child[0];
     priorRequests=p.requests;priorResponses=p.responses;priorMath=p.matrixInstructions;
     p.source[index]^=1U<<((index*7+5)%32);
     for(unsigned i=0;i<8;i++)physicalAfterInject[i]=p.Count(i);
     break; // Continue the SAME not-yet-executed StageConv instruction.
    }
    int status=0;assert(waitpid(pid,&status,0)==pid && WIFEXITED(status) && !WEXITSTATUS(status));cases++;
   }
   if(!injected){assert(cases==16 && !doorbells && !parked);Predecessor();}
  }
  OriginalTokenFlowMachine::StepToken();
 }
 void ParkFaultWorker(){
  auto& p=*child[0];assert(injected && doorbells==1 && p.active && p.convActive && !p.convPackets && !p.pending);
  assert(p.requests==priorRequests && p.responses==priorResponses && p.matrixInstructions==priorMath);
  for(unsigned i=0;i<1000000 && !p.GdnFaulted();i++){cycles+=19;p.StepBlock();}
  assert(p.GdnFaulted() && p.pc==p.faultPC && !p.convPackets && !p.convWrites && !p.pending);
  const auto frozen=p.memory;std::array<uint32_t,10> commands;
  for(unsigned i=0;i<10;i++)commands[i]=p.command[i];
  for(unsigned i=0;i<2048;i++){cycles+=19;p.StepBlock();}
  assert(p.pc==p.faultPC && p.memory==frozen && !p.convPackets && !p.convWrites && !p.pending);
  assert(p.requests==priorRequests && p.responses==priorResponses && p.matrixInstructions==priorMath);
  for(unsigned i=0;i<10;i++)assert(commands[i]==p.command[i]);
  for(unsigned i=0;i<8;i++)assert(p.Count(i)==physicalAfterInject[i]);
  p.active=false; // Scheduler consequence of ACTUAL fault PC,not a completion setter.
  parked=true;
 }
 void Store(uint32_t address,uint32_t value)override{
  const bool packet=address==0xffb20040,write=command[7]==0x212;
  const unsigned remote=write?command[3]:command[0],route=write?command[5]:command[2];
  const bool target=packet && route==G::Peer(1);
  if(packet && injected)assert(!rootSawFault);
  OriginalTokenFlowMachine::Store(address,value);
  if(target && write && remote==M::Doorbell && forked && child[0]->memory.at(M::Job+8)==7){
   doorbells++;assert(doorbells==1 && child[0]->convActive && !child[0]->convPackets);
   if(injected)ParkFaultWorker();
   else{
    auto& p=*child[0];
    for(unsigned i=0;i<1000000 && !p.convPackets && !p.GdnFaulted();i++){cycles+=19;p.StepBlock();}
    assert(p.convPackets==1 && !p.GdnFaulted());parentContinued=true;
   }
  }
  if(target && !write && injected && remote==M::Status && memory.at(command[3]+8)==G::Fault){
   assert(parked && doorbells==1);rootSawFault=true;packetsAtFault=packets;
  }
 }
 void CheckFault(){
  assert(injected && fault<16 && pc==0x60000 && rootSawFault && parked && doorbells==1
   && packets==packetsAtFault && !allLayers && !committed && !stopJobs && original.Ready());
  const auto commands=command;const auto counters=counts;const auto writes=peerWrites;
  for(unsigned i=0;i<40;i++)StepToken();
  assert(memory.at(0x75448)==0x51465431 && packets==packetsAtFault && command==commands && counts==counters);
  const auto frozen=memory;const auto parkedPC=pc;
  for(unsigned i=0;i<2048;i++)StepToken();
  assert(pc==parkedPC && memory==frozen && packets==packetsAtFault && peerWrites==writes
   && command==commands && counts==counters && !child[0]->convPackets && !child[0]->convWrites);
  printf("PASS original matrix->Conv source%u/actual worker rejects BEFORE first Conv NoC or math/ROOT observes real fault+no following packet/actual fault writer+2048 inert steps;NOT native/physical/Work/TPS\n",fault);
 }
};
int main(int argc,char** argv){
 assert(argc==6);setvbuf(stdout,nullptr,_IOLBF,0);(void)&RunPersistentGdnPair;(void)&Refusals;(void)&LateRefusals;
 WholeModelOriginalBytes original;assert(original.Open(argv[4]) && original.Ready());
 auto m=std::make_unique<CohortSourceFaultMachine>(argv[1],argv[2],argv[3],original);
 m->entry=unsigned(strtoul(argv[5],nullptr,16));assert(m->entry>=0x40000 && m->entry<0x60000 && !(m->entry&3));
 m->VerifyRawEndpoints();
 while(m->pc!=0x60000 && m->pc!=0x60004 && !m->parentContinued){
  m->cycles+=19;m->StepToken();if(!(m->steps&255))for(unsigned w=0;w<119;w++)m->Advance(w,64);
 }
 if(m->injected){m->CheckFault();_exit(0);}
 assert(m->forked && m->cases==16 && m->parentContinued && m->doorbells==1 && !m->allLayers && !m->committed
  && !m->stopJobs && original.Ready());
 puts("PASS original four-matrix->Conv/ALL16 physical-source faults before coordinator admission/unchanged parent actual first Conv packet;no transplanted history or supplied products;NOT native/physical/Work/TPS");
}
