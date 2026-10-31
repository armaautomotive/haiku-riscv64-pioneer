/* SPDX-License-Identifier: MIT */
// Actual original-byte ROOT/119 histories,then COW faults at real Conv closure.
// Only TEST raw CSR corruption. No supplied products or processor state.
#include "pioneer_q38_token_original_fabric.h"
struct RetainedConvFaultMachine:OriginalTokenFlowMachine {
 using OriginalTokenFlowMachine::OriginalTokenFlowMachine;
 bool forked=false,injected=false,parked=false,rootSawFault=false,parentContinued=false;
 unsigned fault=24,cases=0,doorbells=0,packetsAtFault=0;
 unsigned priorRequests=0,priorResponses=0,priorMatrixInstructions=0;
 std::array<uint32_t,8> physicalAfterInject={};
 void Predecessor(){
  namespace C=PioneerQ38MixedRecurrentCohortLayout;
  assert(!position && !layer && !completed && !allLayers && !committed && !stopJobs
   && embedded==1 && matrices==4 && convJobs==119 && !gdnJobs && Phase()==6
   && memory.at(C::Summary+16)==120);
  for(unsigned w=0;w<119;w++){
   const auto& p=*child[w];
   assert(!p.active && p.sequence && p.GdnFinished(p.sequence) && !p.GdnFaulted() && !p.pending
    && p.memory.at(M::Job+8)==7 && memory.at(C::Nonces+w*4)==p.sequence
    && p.memory.at(M::Status)==p.sequence && p.memory.at(M::Status+4)==p.sequence
    && !p.memory.at(M::Status+8));
  }
 }
 void ParkFaultWorker(){
  auto& p=*child[0];assert(injected && doorbells==1 && p.active && p.gdnActive && !p.gdnPackets && !p.pending);
  assert(p.requests==priorRequests && p.responses==priorResponses && p.matrixInstructions==priorMatrixInstructions);
  for(unsigned i=0;i<1000000 && !p.GdnFaulted();i++){cycles+=19;p.StepBlock();}
  assert(p.GdnFaulted() && p.pc==p.faultPC && !p.gdnPackets && !p.pending);
  const auto frozen=p.memory;std::array<uint32_t,10> commands;
  for(unsigned i=0;i<10;i++)commands[i]=p.command[i];
  for(unsigned i=0;i<2048;i++){cycles+=19;p.StepBlock();}
  assert(p.pc==p.faultPC && p.memory==frozen && !p.gdnPackets && !p.pending);
  assert(p.requests==priorRequests && p.responses==priorResponses && p.matrixInstructions==priorMatrixInstructions);
  assert(!p.gdnNextWrites && !p.gdnOutputWrites && !p.gdnGatedWrites && !p.clockReads);
  for(unsigned i=0;i<10;i++)assert(commands[i]==p.command[i]);
  for(unsigned i=0;i<8;i++)assert(p.Count(i)==physicalAfterInject[i]);
  p.active=false; // scheduler consequence of ACTUAL fault PC,not a completion setter
  parked=true;
 }
 void Store(uint32_t address,uint32_t value)override{
  namespace C=PioneerQ38MixedRecurrentCohortLayout;
  const bool packet=address==0xffb20040,write=command[7]==0x212;
  const unsigned remote=write?command[3]:command[0],route=write?command[5]:command[2];
  const bool target=packet && route==G::Peer(1);
  if(packet && injected)assert(!rootSawFault);
  OriginalTokenFlowMachine::Store(address,value);
  if(address==C::Summary+16 && value==120 && !forked && !position && !layer){
   Predecessor();forked=true;
   puts("OBSERVED actual original embedding+4 matrices+ALL119 Conv completions/boot+source+job+counter proofs/BEFORE first GDN descriptor;HOST TEST only");
   for(unsigned index=0;index<24;index++){
    BeforeFork();fflush(nullptr);const pid_t pid=fork();assert(pid>=0);
    if(!pid){
     injected=true;fault=index;auto& p=*child[0];
     priorRequests=p.requests;priorResponses=p.responses;priorMatrixInstructions=p.matrixInstructions;
     if(index<16)p.source[index]^=1U<<((index*7+5)%32);
     else{
      const unsigned counter=index-16;
      if(counter==2 || counter==6){p.countBase[2]++;p.countBase[6]++;}
      else p.countBase[counter]++;
     }
     for(unsigned i=0;i<8;i++)physicalAfterInject[i]=p.Count(i);
     return; // SAME completed local-store instruction continues in the child
    }
    int status=0;assert(waitpid(pid,&status,0)==pid && WIFEXITED(status) && !WEXITSTATUS(status));cases++;
   }
   assert(cases==24 && !injected && !doorbells && !parked && original.Ready());Predecessor();
  }
  if(target && write && remote==M::Doorbell && forked && child[0]->memory.at(M::Job+8)==8){
   doorbells++;assert(doorbells==1 && child[0]->gdnActive && !child[0]->gdnPackets);
   if(injected)ParkFaultWorker();
   else{
    auto& p=*child[0];
    for(unsigned i=0;i<1000000 && !p.gdnPackets && !p.GdnFaulted();i++){cycles+=19;p.StepBlock();}
    assert(p.gdnPackets==1 && !p.GdnFaulted());parentContinued=true;
   }
  }
  if(target && !write && injected && remote==M::Status && memory.at(command[3]+8)==G::Fault){
   assert(parked && doorbells==1);rootSawFault=true;packetsAtFault=packets;
  }
 }
 void CheckFault(){
  assert(injected && fault<24 && pc==0x60000 && rootSawFault && parked && doorbells==1
   && packets==packetsAtFault && !allLayers && !committed && !stopJobs && original.Ready());
  const auto commands=command;const auto counters=counts;const auto writes=peerWrites;
  // This exact ROOT has the real fault-reporting trampoline,not an inert
  // loop at60000. Execute it,then prove its actual terminal loop is inert.
  for(unsigned i=0;i<40;i++)StepToken();
  assert(memory.at(0x75448)==0x51465431 && packets==packetsAtFault && command==commands && counts==counters);
  const auto frozen=memory;const auto parkedPC=pc;
  for(unsigned i=0;i<2048;i++)StepToken();
  assert(pc==parkedPC && memory==frozen && packets==packetsAtFault && peerWrites==writes
   && command==commands && counts==counters && !child[0]->gdnPackets);
  printf("PASS retained Conv closure fault%u/%s/worker rejects BEFORE first GDN NoC/ROOT observes real error+no following packet/actual fault writer+2048 inert steps;NOT native/physical/Work/TPS\n",fault,fault<16?"source":"counter");
 }
};
int main(int argc,char** argv){
 assert(argc==5);setvbuf(stdout,nullptr,_IOLBF,0);(void)&RunPersistentGdnPair;(void)&Refusals;(void)&LateRefusals;
 puts("START SAME-open original27B authentication BEFORE retained-conv fault history;NO CPU products or hardware IO");
 WholeModelOriginalBytes original;assert(original.Open(argv[4]) && original.Ready());
 auto m=std::make_unique<RetainedConvFaultMachine>(argv[1],argv[2],argv[3],original);m->VerifyRawEndpoints();
 while(m->pc!=0x60000 && m->pc!=0x60004 && !m->parentContinued){
  m->cycles+=19;m->StepToken();if(!(m->steps&255))for(unsigned w=0;w<119;w++)m->Advance(w,64);
 }
 if(m->injected){m->CheckFault();_exit(0);}
 assert(m->forked && m->cases==24 && m->parentContinued && m->doorbells==1
  && m->pc!=0x60000 && m->pc!=0x60004 && !m->allLayers && !m->committed && !m->stopJobs && original.Ready());
 puts("PASS retained Conv closure/16 source+8 counter faults AFTER ALL119 actual Conv closures/unchanged parent actual first GDN request;no history transplant,products or cleanup invented;NOT full layer/native/physical/Work/TPS");
}
