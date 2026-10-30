/* SPDX-License-Identifier: MIT */
// Actual persistent ROOT+119 histories+original learned bytes. No native IO.
#ifndef PIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION
#error Exact HOST QGW3 receiver test profile required
#endif
#include "pioneer_q38_token_original_fabric.h"
struct ConnectedPushedMachine:OriginalTokenFlowMachine {
 using OriginalTokenFlowMachine::OriginalTokenFlowMachine;
 bool receiptForked=false,slaveForked=false,injected=false,continued=false;
 unsigned fault=~0U,cases=0,gathers=0,atPackets=0,atWrites=0;
 uint32_t Load(uint32_t address)override{
  if(!receiptForked && address==G::Mailbox && memory.at(address)==G::ReceiptMagic){
   assert(!position && !layer && !allLayers && !committed && matrices==1 && !gathers && embedded==1);
   receiptForked=true;
   for(unsigned word=0;word<=64;word++){
    BeforeFork();fflush(nullptr);const pid_t pid=fork();assert(pid>=0);
    if(!pid){
     injected=true;fault=word;atPackets=packets;atWrites=peerWrites;
     memory[G::Mailbox+(word==64?0:word*4)]^=word==64?unsigned(G::ReceiptMagic):1U<<((word*7+5)%32);
     return OriginalTokenFlowMachine::Load(address);
    }
    int status=0;assert(waitpid(pid,&status,0)==pid && WIFEXITED(status) && !WEXITSTATUS(status));cases++;
   }
  }
  if(!slaveForked && address==0xffb20000+SlaveOffset(0) && gathers==3){
   assert(receiptForked && cases==65 && pushedMessages==6 && !allLayers && !committed && matrices==1);
   slaveForked=true;
   for(unsigned i=0;i<4;i++){
    BeforeFork();fflush(nullptr);const pid_t pid=fork();assert(pid>=0);
    if(!pid){injected=true;fault=65+i;atPackets=packets;atWrites=peerWrites;pushedSlave[i]+=0x100000;return OriginalTokenFlowMachine::Load(address);}
    int status=0;assert(waitpid(pid,&status,0)==pid && WIFEXITED(status) && !WEXITSTATUS(status));cases++;
   }
  }
  return OriginalTokenFlowMachine::Load(address);
 }
 void Store(uint32_t address,uint32_t value)override{
  const bool submit=address==0xffb20040;
  const bool proof=submit && !command[7] && command[0]==0x83f00 && command[8]>256;
  if(submit && injected)assert(false && "no ROOT packet after first bad receipt/counter observation");
  const bool next=submit && command[7]==0x212 && command[5]==G::Peer(1) && command[3]==M::Doorbell
   && matrices==2 && receiptForked && slaveForked;
  OriginalTokenFlowMachine::Store(address,value);
  if(proof)gathers++;
  if(next){
   auto& p=*child[0];assert(cases==69 && p.sequence==2 && !p.requests && !p.GdnFaulted());
   for(unsigned i=0;i<1000000 && !p.requests && !p.GdnFaulted();i++){cycles+=19;p.StepBlock();}
   assert(p.requests==1 && !p.GdnFaulted() && !p.matrixInstructions);continued=true;
  }
 }
 void CheckFault(){
  assert(injected && fault<69 && pc==0x60000 && packets==atPackets && peerWrites==atWrites
   && !allLayers && !committed && !stopJobs && original.Ready() && minSp>=0x6e000 && minStack>=0x6e000);
  const auto commands=command;const auto physical=counts;const auto incoming=pushedSlave;
  for(unsigned i=0;i<2048;i++)StepToken();
  assert(pc==0x60000 && packets==atPackets && peerWrites==atWrites && command==commands && counts==physical && pushedSlave==incoming);
  printf("PASS connected QGW3 fault%u/%s/actual reached receipt+original matrix history/ROOT parks before following packet or release/inert2048;NOT physical/native/Work/TPS\n",fault,fault<64?"receipt word":fault==64?"finite missing commit":"unexpected incoming counter");
 }
};
int main(int argc,char** argv){
 assert(argc==5);setvbuf(stdout,nullptr,_IOLBF,0);(void)&RunPersistentGdnPair;(void)&Refusals;(void)&LateRefusals;
 WholeModelOriginalBytes original;assert(original.Open(argv[4]) && original.Ready());
 auto m=std::make_unique<ConnectedPushedMachine>(argv[1],argv[2],argv[3],original);m->VerifyRawEndpoints();
 while(m->pc!=0x60000 && m->pc!=0x60004 && !m->continued){
  m->cycles+=19;m->StepToken();if(!(m->steps&255))for(unsigned w=0;w<119;w++)m->Advance(w,64);
 }
 if(m->injected){m->CheckFault();return 0;}
 assert(m->continued && m->cases==69 && m->gathers==3 && m->pushedMessages==6 && !m->allLayers && !m->committed
  && m->l0Fences && m->l0Hits && m->l0Misses && original.Ready());
 printf("PASS connected QGW3/ROOT from RESET+SAME119/first3 original fullK5120 tensor jobs/6 actual pushed packets/64 receipt mutations+finite missing-commit timeout+4 slave-counter faults/next admission+nonce2 first real read/L0 fence model hits%u misses%u stale-remote-events%u;NOT whole-layer/model numerical/native/physical/Work/TPS qualification\n",m->l0Hits,m->l0Misses,m->l0RemoteStale);
}
