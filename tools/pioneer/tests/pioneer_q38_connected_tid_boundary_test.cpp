/* SPDX-License-Identifier: MIT */
// COW faults at a genuine first Phase2 packet after original embedding.
// No owner,processor history,products,worker completion or release supplied.
#include "pioneer_q38_token_original_fabric.h"
#ifndef PIONEER_Q38_TEST_TID_FIFO_ENDPOINT
#error Explicit isolated serial FIFO endpoint required
#endif
struct TidBoundaryFaultMachine:OriginalTokenFlowMachine {
 using OriginalTokenFlowMachine::OriginalTokenFlowMachine;
 unsigned entry=0,full=0,link=0,owner=0,fault=46,cases=0,doorbells=0,packetsBefore=0;
 bool forked=false,injected=false,packetReturned=false,fullEntered=false,parentContinued=false;
 void Predecessor(){
  assert(pc==entry && !position && !layer && !completed && !allLayers && !committed && !stopJobs
   && embedded==1 && !matrices && !convJobs && !gdnJobs && Phase()==2);
  owner=regs[27];assert(owner>=0x6e000 && owner<=0x70000-sizeof(RB::PrivateSeal)
   && regs[10]==owner+768 && memory.at(owner+1024)==2 && (source[0]&0x10000U));
  for(const auto& p:child)assert(!p->active && !p->sequence && p->GdnFinished(0) && !p->GdnFaulted()
   && !p->pending && !p->requests && !p->responses && !p->matrixInstructions);
 }
 void Inject(unsigned index){
  assert(index<46);injected=true;fault=index;packetsBefore=packets;
  if(index<30){unsigned tid=index<15?index+1:index+2;assert(tid<32 && tid!=16);tidStatus[tid]=1U<<((index*3)%8);}
  else if(index<32)tidStatus[(index-30)*16]=1;
  else if(index<36)initiatorStatus[index-32]=1;
  else if(index<40){unsigned shift=(index-36)*8;fifoAvailable=(fifoAvailable&~(255U<<shift))|(15U<<shift);}
  else if(index<44)fifoOverflow=1U<<(index-40);
  else if(index==44){
   // Keep ALL compared local source mirrors coherent,so the explicit
   // FIFO-enabled selection check must refuse,not a mirror mismatch.
   source[0]&=~0x10000U;
   for(unsigned at:{unsigned(RB::Boot)+16*4,owner+16*4,unsigned(RB::Header)+16*4,owner+768+16*4})memory.at(at)=source[0];
  }else{fifoAvailable=0;tidStatus[0]=1;}
 }
 void StepToken(){
  if(pc==entry && !forked && !position && !layer){
   Predecessor();forked=true;link=regs[1];assert(link>=0x40000 && link<0x60000 && !(link&3));
   puts("OBSERVED genuine original embedding+119 virgin workers/FIFO-enabled Phase2 BEFORE first packet;HOST COW only");
   for(unsigned index=0;index<46;index++){
    BeforeFork();fflush(nullptr);const pid_t pid=fork();assert(pid>=0);
    if(!pid){Inject(index);break;}
    int status=0;assert(waitpid(pid,&status,0)==pid && WIFEXITED(status) && !WEXITSTATUS(status));cases++;
   }
   if(!injected){assert(cases==46 && !doorbells);Predecessor();}
  }
  if(injected && pc==link)packetReturned=true;
  if(injected && pc==full)fullEntered=true;
  OriginalTokenFlowMachine::StepToken();
 }
 void Store(uint32_t address,uint32_t value)override{
  const bool packet=address==0xffb20040U,write=command[7]==0x212;
  const unsigned remote=write?command[3]:command[0],route=write?command[5]:command[2];bool release=false;
  if(packet && write && remote==M::Doorbell)for(unsigned w=0;w<119;w++)if(route==G::Peer(w+1))release=true;
  if(injected && fault>=30 && (packet || (address>=0xffb20000U && address<0xffb20028U)))assert(false && "immediate fault must precede any new command");
  if(release && forked)assert(!injected);
  OriginalTokenFlowMachine::Store(address,value);
  if(release && forked){
   assert(!injected && !doorbells && child[0]->sequence==1);doorbells++;
   auto& p=*child[0];for(unsigned i=0;i<1000000 && !p.requests && !p.GdnFaulted();i++){cycles+=19;p.StepBlock();}
   assert(p.requests==1 && !p.GdnFaulted() && !p.matrixInstructions);parentContinued=true;
  }
 }
 void CheckFault(){
  assert(injected && fault<46 && forked && !doorbells && pc==0x60000 && !allLayers && !committed && !stopJobs && original.Ready());
  if(fault<30)assert(packetReturned && fullEntered);
  else{assert(packets==packetsBefore);if(fault==44)assert(!packetReturned && !fullEntered);}
  for(const auto& p:child)assert(!p->active && !p->sequence && !p->requests && !p->responses && !p->matrixInstructions);
  const auto commands=command;const auto counters=counts;const unsigned writes=peerWrites,packetsAtFault=packets;
  for(unsigned i=0;i<40;i++)StepToken();
  assert(memory.at(0x75448)==0x51465431 && packets==packetsAtFault && command==commands && counts==counters);
  const auto frozen=memory;const unsigned terminal=pc;
  for(unsigned i=0;i<2048;i++)StepToken();
  assert(pc==terminal && memory==frozen && packets==packetsAtFault && peerWrites==writes && command==commands && counts==counters);
  printf("PASS original TID/FIFO fault%u/%s/packetReturn%u fullEntry%u/NO release,math,model commit/actual fault writer+2048 inert;NOT native/physical/Work/TPS\n",
   fault,fault<30?"inactive TID deferred then full refusal":fault<32?"active TID immediate":fault<36?"initiator immediate":fault<40?"FIFO backlog immediate":fault<44?"FIFO overflow immediate":fault==44?"coherent FIFO-disabled config refused":"FIFO+TID immediate",unsigned(packetReturned),unsigned(fullEntered));
 }
};
int main(int argc,char** argv){
 assert(argc==7);setvbuf(stdout,nullptr,_IOLBF,0);(void)&RunPersistentGdnPair;(void)&Refusals;(void)&LateRefusals;
 puts("START SAME-open original27B authentication BEFORE TID/FIFO fault history;NO CPU products or hardware IO");
 WholeModelOriginalBytes original;assert(original.Open(argv[4]) && original.Ready());
 auto m=std::make_unique<TidBoundaryFaultMachine>(argv[1],argv[2],argv[3],original);
 m->entry=unsigned(strtoul(argv[5],nullptr,16));m->full=unsigned(strtoul(argv[6],nullptr,16));
 assert(m->entry>=0x40000 && m->entry<0x60000 && m->full>=0x40000 && m->full<0x60000 && !((m->entry|m->full)&3));
 m->VerifyRawEndpoints();
 while(m->pc!=0x60000 && m->pc!=0x60004 && !m->parentContinued){
  m->cycles+=19;m->StepToken();if(!(m->steps&255))for(unsigned w=0;w<119;w++)m->Advance(w,64);
 }
 if(m->injected){m->CheckFault();_exit(0);}
 assert(m->forked && m->cases==46 && m->parentContinued && m->doorbells==1 && !m->allLayers && !m->committed && !m->stopJobs && original.Ready());
 puts("PASS original TID boundary/46 corruption children+unchanged parent first actual matrix request;NO supplied products/history;NOT full-layer/native/physical/Work/TPS");
}
