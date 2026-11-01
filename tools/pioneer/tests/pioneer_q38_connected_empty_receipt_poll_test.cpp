/* SPDX-License-Identifier: MIT */
// COW faults at a genuinely reached EMPTY receipt guard. Worker arithmetic
// was already issued and CONTINUES;no new ROOT command may follow corruption.
#include "pioneer_q38_token_original_fabric.h"
#if !defined(PIONEER_Q38_TEST_TID_FIFO_ENDPOINT) || !defined(PIONEER_Q38_TEST_COUNTER_GATHER_FIFO)
#error Exact original-byte TID and captured-command FIFO endpoints required
#endif
struct EmptyPollFaultMachine:OriginalTokenFlowMachine {
 using OriginalTokenFlowMachine::OriginalTokenFlowMachine;
 unsigned entry=0,receipt=0,packetGuard=0,owner=0,link=0,fault=16,cases=0,rootCommands=0;
 bool forked=false,injected=false,emptyReturned=false,receiptEntered=false,packetEntered=false,parentContinued=false;
 void Predecessor(){
  assert(pc==entry && !position && !layer && !completed && !allLayers && !committed && !stopJobs
   && embedded==1 && matrices==1 && !convJobs && !gdnJobs && Phase()==2);
  owner=regs[27];assert(owner>=0x6e000 && owner<=0x70000-sizeof(RB::PrivateSeal)
   && regs[10]==owner+768 && memory.at(owner+1024)==2);
  printf("OBSERVED empty-helper entry pc%x marker%x seq%u active%u requests%u matrices%u phase%u\n",pc,
   memory.at(PioneerQ38PushedWorkerLayout::Mailbox),child[0]->sequence,unsigned(child[0]->active),child[0]->requests,matrices,Phase());
  assert(!memory.at(PioneerQ38PushedWorkerLayout::Mailbox) && child[0]->sequence==1 && child[0]->active && !child[0]->requests);
  // No fabricated ready/receipt/worker history:the first worker is doing
  // its REAL original-byte first job when the empty branch is reached.
  // It is still in its prologue (zero requests),not an idle or fabricated
  // completion. The later refusal/control checks require actual matrix
  // instructions,responses and the genuinely pushed receipt.
 }
 void StepToken(){
  if(pc==entry && !forked && !position && !layer){
   Predecessor();forked=true;link=regs[1];
   assert(link>=0x40000 && link<0x60000 && !(link&3));
   puts("OBSERVED actual original embedding/first matrix ALREADY executing/first genuine EMPTY receipt branch;HOST COW only");
   for(unsigned index=0;index<16;index++){
    BeforeFork();fflush(nullptr);const pid_t pid=fork();assert(pid>=0);
    if(!pid){injected=true;fault=index;source[index]^=1U<<((index*7+16)%32);break;}
    int status=0;assert(waitpid(pid,&status,0)==pid && WIFEXITED(status) && !WEXITSTATUS(status));cases++;
   }
   if(!injected){assert(cases==16);Predecessor();}
  }
  if(forked && pc==link)emptyReturned=true;
  if(forked && pc==receipt)receiptEntered=true;
  if(forked && pc==packetGuard)packetEntered=true;
  OriginalTokenFlowMachine::StepToken();
 }
 void Store(uint32_t address,uint32_t value)override{
  const bool packet=address==0xffb20040U;
  if(forked && packet){
   assert(!injected && "live-source corruption must stop BEFORE any later ROOT command");
   assert(emptyReturned && receiptEntered && packetEntered && !rootCommands);
   assert(command[7]==0 && command[0]==0x83f00 && command[2]==G::Peer(1) && command[8]>=320);
   assert(memory.at(PioneerQ38PushedWorkerLayout::Mailbox)==PioneerQ38PushedWorkerLayout::ReceiptMagic
    && child[0]->matrixInstructions && child[0]->responses);
  }
  OriginalTokenFlowMachine::Store(address,value);
  if(forked && packet){rootCommands++;parentContinued=true;}
 }
 void CheckFault(){
  assert(injected && fault<16 && forked && emptyReturned && receiptEntered && packetEntered
   && !rootCommands && pc==0x60000 && !allLayers && !committed && !stopJobs && original.Ready());
  // These workers were already computing BEFORE the fault. Do not claim
  // that no math occurred;prove no subsequent ROOT packet/release/commit.
  assert(child[0]->matrixInstructions && child[0]->responses);
  const auto commands=command;const auto counters=counts;const unsigned atFault=packets;
  for(unsigned i=0;i<40;i++)StepToken();
  assert(memory.at(0x75448)==0x51465431 && packets==atFault && command==commands && counts==counters);
  const auto frozen=memory;const unsigned terminal=pc;
  for(unsigned i=0;i<2048;i++)StepToken();
  assert(pc==terminal && memory==frozen && packets==atFault && command==commands && counts==counters);
  printf("PASS original empty-receipt source%u bit%u/empty guard RETURNS/actual worker computes+pushes real receipt/"
         "real receipt source guard REFUSES before any later ROOT packet,release or model commit/actual fault writer+2048 inert;"
         "NOT native/physical/Work/TPS\n",fault,(fault*7+16)%32);
 }
};
int main(int argc,char** argv){
 assert(argc==8);setvbuf(stdout,nullptr,_IOLBF,0);(void)&RunPersistentGdnPair;(void)&Refusals;(void)&LateRefusals;
 puts("START SAME-open original27B authentication BEFORE empty-receipt faults;NO supplied products,ready or processor history");
 WholeModelOriginalBytes original;assert(original.Open(argv[4]) && original.Ready());
 auto m=std::make_unique<EmptyPollFaultMachine>(argv[1],argv[2],argv[3],original);
 m->entry=unsigned(strtoul(argv[5],nullptr,16));m->receipt=unsigned(strtoul(argv[6],nullptr,16));
 m->packetGuard=unsigned(strtoul(argv[7],nullptr,16));
 for(unsigned at:{m->entry,m->receipt,m->packetGuard})assert(at>=0x40000 && at<0x60000 && !(at&3));
 m->VerifyRawEndpoints();
 while(m->pc!=0x60000 && m->pc!=0x60004 && !m->parentContinued){
  m->cycles+=19;m->StepToken();if(!(m->steps&255))for(unsigned w=0;w<119;w++)m->Advance(w,64);
 }
 if(m->injected){m->CheckFault();_exit(0);}
 assert(m->forked && m->cases==16 && m->parentContinued && m->rootCommands==1 && m->emptyReturned
  && m->receiptEntered && m->packetEntered && !m->allLayers && !m->committed && !m->stopJobs && original.Ready());
 puts("PASS genuine original EMPTY-receipt boundary/16 live-source COW faults/actual worker products+receipt/"
      "unchanged parent consumes actual receipt then issues first proof READ;NO timeout extension,ready or completion setter;"
      "NOT complete matrix/layer/native/physical/Work/TPS");
}
