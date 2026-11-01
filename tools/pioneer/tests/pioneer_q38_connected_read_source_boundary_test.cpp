/* SPDX-License-Identifier: MIT */
// COW source faults at a genuinely reached read-only packet boundary.
// Actual original embedding+119 virgin engines;no supplied math/history.
#include "pioneer_q38_token_original_fabric.h"
#if !defined(PIONEER_Q38_TEST_TID_FIFO_ENDPOINT) || !defined(PIONEER_Q38_TEST_COUNTER_GATHER_FIFO)
#error Exact isolated TID and genuine captured-command FIFO endpoints required
#endif
struct ReadSourceFaultMachine:OriginalTokenFlowMachine {
 using OriginalTokenFlowMachine::OriginalTokenFlowMachine;
 unsigned entry=0,full=0,writeGuard=0,link=0,owner=0,fault=512,cases=0,doorbells=0,packetsBefore=0;
 bool forked=false,injected=false,readReturned=false,fullEntered=false,writeEntered=false,parentContinued=false;
 void Predecessor(){
  assert(pc==entry && !regs[11] && !position && !layer && !completed && !allLayers && !committed && !stopJobs
   && embedded==1 && !matrices && !convJobs && !gdnJobs && Phase()==2);
  owner=regs[27];assert(owner>=0x6e000 && owner<=0x70000-sizeof(RB::PrivateSeal)
   && regs[10]==owner+768 && memory.at(owner+1024)==2 && (source[0]&0x10000U));
  for(const auto& p:child)assert(!p->active && !p->sequence && p->GdnFinished(0) && !p->GdnFaulted()
   && !p->pending && !p->requests && !p->responses && !p->matrixInstructions);
 }
 void StepToken(){
  if(pc==entry && !regs[11] && !forked && !position && !layer){
   Predecessor();forked=true;link=regs[1];packetsBefore=packets;
   assert(link>=0x40000 && link<0x60000 && !(link&3));
   puts("OBSERVED genuine original embedding+119 virgin workers/Phase2 BEFORE first actual read-only packet guard;HOST COW only");
   for(unsigned index=0;index<512;index++){
    BeforeFork();fflush(nullptr);const pid_t pid=fork();assert(pid>=0);
    if(!pid){injected=true;fault=index;source[index/32]^=1U<<(index%32);break;}
    int status=0;assert(waitpid(pid,&status,0)==pid && WIFEXITED(status) && !WEXITSTATUS(status));cases++;
   }
   if(!injected){assert(cases==512 && !doorbells);Predecessor();}
  }
  if(injected && pc==link)readReturned=true;
  if(injected && pc==full)fullEntered=true;
  if(injected && (pc==writeGuard || (pc==entry && regs[11])))writeEntered=true;
  OriginalTokenFlowMachine::StepToken();
 }
 void Store(uint32_t address,uint32_t value)override{
  const bool packet=address==0xffb20040U,write=command[7]==0x212;
  const unsigned remote=write?command[3]:command[0],route=write?command[5]:command[2];bool release=false;
  if(packet && write && remote==M::Doorbell)for(unsigned w=0;w<119;w++)if(route==G::Peer(w+1))release=true;
  if(injected && packet)assert(!write && "deferred live source fault must precede EVERY write,not just release");
  if(release && forked)assert(!injected);
  OriginalTokenFlowMachine::Store(address,value);
  if(release && forked){
   assert(!injected && !doorbells && child[0]->sequence==1);doorbells++;
   auto& p=*child[0];for(unsigned i=0;i<1000000 && !p.requests && !p.GdnFaulted();i++){cycles+=19;p.StepBlock();}
   assert(p.requests==1 && !p.GdnFaulted() && !p.matrixInstructions);parentContinued=true;
  }
 }
 void CheckFault(){
  assert(injected && fault<512 && forked && readReturned && !doorbells && pc==0x60000
   && !allLayers && !committed && !stopJobs && original.Ready());
  // Disabling the actual FIFO makes its live availability zero and is
  // rejected by the retained packet-quiet guard BEFORE any transfer.
  if(fault==16)assert(packets==packetsBefore && !fullEntered && !writeEntered);
  else assert(fullEntered || writeEntered);
  for(const auto& p:child)assert(!p->active && !p->sequence && !p->requests && !p->responses && !p->matrixInstructions);
  const auto commands=command;const auto counters=counts;const unsigned writes=peerWrites,packetsAtFault=packets;
  for(unsigned i=0;i<40;i++)StepToken();
  assert(memory.at(0x75448)==0x51465431 && packets==packetsAtFault && command==commands && counts==counters);
  const auto frozen=memory;const unsigned terminal=pc;
  for(unsigned i=0;i<2048;i++)StepToken();
  assert(pc==terminal && memory==frozen && packets==packetsAtFault && peerWrites==writes && command==commands && counts==counters);
  printf("PASS original read-source fault%u bit%u/readReturn1 fullEntry%u writeEntry%u packets%u/"
         "NO write,release,math,model commit/actual fault writer+2048 inert;NOT native/physical/Work/TPS\n",
   fault/32,fault%32,unsigned(fullEntered),unsigned(writeEntered),packets-packetsBefore);
 }
};
int main(int argc,char** argv){
 assert(argc==8);setvbuf(stdout,nullptr,_IOLBF,0);(void)&RunPersistentGdnPair;(void)&Refusals;(void)&LateRefusals;
 puts("START SAME-open original27B authentication BEFORE read-source fault history;NO CPU products or hardware IO");
 WholeModelOriginalBytes original;assert(original.Open(argv[4]) && original.Ready());
 auto m=std::make_unique<ReadSourceFaultMachine>(argv[1],argv[2],argv[3],original);
 m->entry=unsigned(strtoul(argv[5],nullptr,16));m->full=unsigned(strtoul(argv[6],nullptr,16));
 m->writeGuard=unsigned(strtoul(argv[7],nullptr,16));
 for(unsigned at:{m->entry,m->full,m->writeGuard})assert(at>=0x40000 && at<0x60000 && !(at&3));
 m->VerifyRawEndpoints();
 while(m->pc!=0x60000 && m->pc!=0x60004 && !m->parentContinued){
  m->cycles+=19;m->StepToken();if(!(m->steps&255))for(unsigned w=0;w<119;w++)m->Advance(w,64);
 }
 if(m->injected){m->CheckFault();_exit(0);}
 assert(m->forked && m->cases==512 && m->parentContinued && m->doorbells==1 && !m->allLayers && !m->committed && !m->stopJobs && original.Ready());
 puts("PASS original read-source boundary/ALL16 live sources ALL32bits/512 corruption children+unchanged parent first actual matrix request;"
      "NO supplied products/history;NOT full-layer/native/physical/Work/TPS");
}
