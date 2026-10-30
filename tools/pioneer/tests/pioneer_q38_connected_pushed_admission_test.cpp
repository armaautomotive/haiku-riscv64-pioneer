/* SPDX-License-Identifier: MIT */
// Actual original-model predecessor and actual retained-receipt admission.
// Raw TEST bytes/registers ONLY;no PC/private-ready/product/native setter.
#ifndef PIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION
#error Exact HOST QGW3 admission profile required
#endif
#include "pioneer_q38_token_original_fabric.h"
struct PushedAdmissionMachine:OriginalTokenFlowMachine {
 using OriginalTokenFlowMachine::OriginalTokenFlowMachine;
 enum {LocalCases=92,PhysicalCases=88,TotalCases=LocalCases+PhysicalCases};
 bool forked=false,injected=false,workerParked=false,parentContinued=false,waiting=false;
 unsigned fault=TotalCases,cases=0,doorbells=0,packetsAtInjection=0,writesAtInjection=0,packetsAtWait=0;
 std::array<uint32_t,8> physicalBefore={};
 bool Physical()const{return fault>=LocalCases;}
 bool ContinueRoot()const{return fault==LocalCases || fault==LocalCases+16;}
 void Predecessor(){
  assert(!position && !layer && !allLayers && !committed && !stopJobs && embedded==1 && matrices==1
   && memory.at(RB::Header+43*4)==11);
  for(unsigned w=0;w<3;w++){
   const auto& p=*child[w];assert(!p.active && p.sequence==1 && p.GdnFinished(1) && !p.GdnFaulted()
    && !p.pending && p.type==8 && p.inputs==5120 && p.matrixInstructions==1280
    && p.requests==162 && p.responses==162 && p.pushPackets==2 && memory.at(RB::Nonces+w*4)==1);
  }
 }
 void Inject(unsigned which){
  injected=true;fault=which;packetsAtInjection=packets;writesAtInjection=peerWrites;auto& p=*child[0];
  if(which<64)memory[G::Mailbox+which*4]^=1U<<((which*7+5)%32);
  else if(which==64)memory[G::Mailbox]=0;
  else if(which<73){unsigned i=which-65;memory[RB::Counters+i*4]++;if(i==2)memory[RB::Counters+6*4]++;if(i==6)memory[RB::Counters+2*4]++;}
  else if(which==73)memory[RB::Nonces]++;
  else if(which<85){const unsigned words[]={0,1,3,7,16,17,24,26,31,32,63};memory[RB::PeerJobs+words[which-74]*4]^=1;}
  else if(which<LocalCases){const unsigned words[]={0,3,4,16,40,54,63};memory[RB::PeerBoots+words[which-85]*4]^=1;}
  else{
   unsigned at=which-LocalCases;
   if(at<16)p.source[at]^=1U<<((at*7+5)%32);
   else if(at<24){unsigned i=at-16;p.countBase[i]++;if(i==2)p.countBase[6]++;if(i==6)p.countBase[2]++;}
   else p.memory[M::Boot+(at-24)*4]^=1;
  }
  for(unsigned i=0;i<8;i++)physicalBefore[i]=p.Count(i);
 }
 uint32_t Load(uint32_t address)override{
  if(!forked && address==G::Mailbox && memory.at(address)==G::ReceiptMagic
   && matrices==1 && memory.at(RB::Header+43*4)==11){
   Predecessor();forked=true;
   for(unsigned which=0;which<TotalCases;which++){
    BeforeFork();fflush(nullptr);const pid_t pid=fork();assert(pid>=0);
    if(!pid){Inject(which);return OriginalTokenFlowMachine::Load(address);}
    int status=0;assert(waitpid(pid,&status,0)==pid && WIFEXITED(status) && !WEXITSTATUS(status));cases++;
   }
   Predecessor();assert(cases==TotalCases && !injected && !doorbells && original.Ready());
  }
  if(workerParked && address==G::Mailbox && !waiting){
   assert(ContinueRoot() && !memory.at(address));waiting=true;packetsAtWait=packets;
  }
  return OriginalTokenFlowMachine::Load(address);
 }
 void ParkWorker(){
  auto& p=*child[0];assert(injected && Physical() && doorbells==1 && p.active && p.sequence==2
   && !p.requests && !p.responses && !p.matrixInstructions && !p.pending);
  for(unsigned i=0;i<1000000 && !p.GdnFaulted();i++){cycles+=19;p.StepBlock();}
  assert(p.GdnFaulted() && p.memory.at(M::Status+8)==G::Fault && p.memory.at(M::Status+4)==1
   && !p.requests && !p.responses && !p.matrixInstructions && !p.pending && p.minStack>=0x6e000);
  const auto saved=p.memory;std::array<uint32_t,10> commands;
  for(unsigned i=0;i<10;i++)commands[i]=p.command[i];
  for(unsigned i=0;i<2048;i++){cycles+=19;p.StepBlock();}
  assert(p.GdnFaulted() && p.memory==saved && !p.requests && !p.responses && !p.matrixInstructions && !p.pending);
  for(unsigned i=0;i<10;i++)assert(commands[i]==p.command[i]);
  for(unsigned i=0;i<8;i++)assert(physicalBefore[i]==p.Count(i));
  workerParked=true;p.active=false; // endpoint scheduler consequence of ACTUAL fault,not a processor/status/receipt setter.
  if(!ContinueRoot()){
   printf("PASS actual warm-admission physical%u/queued nonce2/worker rejects BEFORE first read or tensor/completed1 retained/inert2048;NOT ROOT timeout/model/native/physical/TPS qualification\n",fault-LocalCases);
   _exit(0);
  }
 }
 void Store(uint32_t address,uint32_t value)override{
  const bool submit=address==0xffb20040;
  if(submit && injected && !Physical())assert(false && "bad retained data must not issue another ROOT packet");
  if(submit && waiting)assert(false && "no ROOT packet during failed worker's missing-receipt wait");
  const bool next=submit && command[7]==0x212 && command[5]==G::Peer(1) && command[3]==M::Doorbell && matrices==2 && forked;
  OriginalTokenFlowMachine::Store(address,value);
  if(next){
   doorbells++;
   if(injected)ParkWorker();
   else{
    auto& p=*child[0];assert(cases==TotalCases && doorbells==1 && p.sequence==2 && !p.requests && !p.GdnFaulted());
    for(unsigned i=0;i<1000000 && !p.requests && !p.GdnFaulted();i++){cycles+=19;p.StepBlock();}
    assert(p.requests==1 && !p.GdnFaulted() && !p.matrixInstructions);parentContinued=true;
   }
  }
 }
 void CheckFault(){
  assert(injected && pc==0x60000 && !allLayers && !committed && !stopJobs && original.Ready());
  if(Physical())assert(ContinueRoot() && workerParked && waiting && doorbells==1 && packets==packetsAtWait
   && child[0]->GdnFaulted() && !child[0]->requests && !memory.at(G::Mailbox));
  else assert(!workerParked && !doorbells && packets==packetsAtInjection && peerWrites==writesAtInjection);
  const auto commands=command;const auto counters=counts;const auto slave=pushedSlave;const unsigned issued=packets,writes=peerWrites;
  for(unsigned i=0;i<2048;i++)StepToken();
  assert(pc==0x60000 && command==commands && counts==counters && pushedSlave==slave && packets==issued && peerWrites==writes);
  printf("PASS actual warm-admission case%u/%s/ROOT parks without later packet,retry,cleanup/inert2048;NOT fullmodel/native/physical/Work/TPS qualification\n",
   fault,Physical()?"genuine worker refusal propagates to finite missing-receipt timeout":"retained receipt/nonce/frontier/job/boot rejected BEFORE next release");
 }
};
int main(int argc,char** argv){
 assert(argc==5);setvbuf(stdout,nullptr,_IOLBF,0);(void)&RunPersistentGdnPair;(void)&Refusals;(void)&LateRefusals;
 WholeModelOriginalBytes original;assert(original.Open(argv[4]) && original.Ready());
 auto m=std::make_unique<PushedAdmissionMachine>(argv[1],argv[2],argv[3],original);m->VerifyRawEndpoints();
 while(m->pc!=0x60000 && m->pc!=0x60004 && !m->parentContinued){
  m->cycles+=19;m->StepToken();if(!(m->steps&255))for(unsigned w=0;w<119;w++)m->Advance(w,64);
 }
 if(m->injected){m->CheckFault();return 0;}
 assert(m->forked && m->cases==PushedAdmissionMachine::TotalCases && m->parentContinued && m->doorbells==1
  && !m->allLayers && !m->committed && !m->stopJobs && original.Ready());
 puts("PASS genuine original ROOT+SAME119/warm-admission92 local-data faults+88 idle physical faults/two actual worker failures propagated to finite ROOT timeout/unchanged parent nonce2 first read;NO model/native/physical/Work/TPS qualification");
}
