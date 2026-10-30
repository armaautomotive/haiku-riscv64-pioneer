/* SPDX-License-Identifier: MIT */
// HOST-only: reach the first dense receipt through the original model history.
// Fault only its raw mailbox word; no processor state/product/ready injection.
#define PIONEER_Q38_PUSHED_COHORT_MACHINE_ONLY
#include "pioneer_q38_connected_pushed_cohort_test.cpp"
struct PushedDenseFaultMachine:PushedCohortMachine {
 using PushedCohortMachine::PushedCohortMachine;
 bool denseForked=false,denseInjected=false,denseContinued=false;
 unsigned denseCases=0,denseFault=2,atPackets=0,atWrites=0,atZeroReads=0,atPollReads=0;
 void DensePredecessor(){
  assert(continued && cases==8 && handoffs==1 && !injected && !position && layer==3
   && completed==3 && allLayers==3 && matrices==1 && !denseReceipt && !committed && !stopJobs);
  const auto& p=*child[0];
  assert(p.type==8 && p.inputs==5120 && p.matrixInstructions==8960 && p.requests==1122
   && p.responses==1122 && p.pushPackets==2 && memory.at(G::Mailbox)==G::ReceiptMagic && original.Ready());
  assert(l0Tags[(G::Mailbox>>4)&3]!=(G::Mailbox&~15U)); // Actual preceding ROOT fence;raw fault cannot be hidden by cached data.
 }
 uint32_t Load(uint32_t address)override{
  if(!denseForked && continued && !injected && !position && layer==3 && matrices==1
   && address==G::Mailbox && memory.at(address)==G::ReceiptMagic){
   DensePredecessor();denseForked=true;
   for(unsigned fault=0;fault<2;fault++){
    BeforeFork();fflush(nullptr);const pid_t pid=fork();assert(pid>=0);
    if(!pid){
     denseInjected=true;denseFault=fault;atPackets=packets;atWrites=peerWrites;atZeroReads=denseZeroReads;atPollReads=densePollReads;
     // Actual producer has already emitted payload+commit. Remove/corrupt only
     // its raw receipt marker BEFORE the one actual ROOT load can observe it.
     memory[G::Mailbox]=fault?unsigned(G::ReceiptMagic)^1U:0U;
     return PushedCohortMachine::Load(address);
    }
    int status=0;assert(waitpid(pid,&status,0)==pid && WIFEXITED(status) && !WEXITSTATUS(status));denseCases++;
   }
   DensePredecessor();assert(denseCases==2 && !denseInjected);
  }
  return PushedCohortMachine::Load(address);
 }
 void Store(uint32_t address,uint32_t value)override{
  const bool submit=address==0xffb20040;
  if(submit && denseInjected)assert(false && "no following ROOT packet after missing/bad dense receipt");
  const bool next=submit && denseForked && !denseInjected && denseReceipt && !position && layer==3
   && matrices==1 && !command[7] && command[0]==0x83f00 && command[2]==G::Peer(1) && command[8]==704;
  PushedCohortMachine::Store(address,value);
  if(next){assert(denseCases==2 && !denseContinued);denseContinued=true;}
 }
 void CheckDenseFault(){
  const unsigned budget=G::ReceiptPollBudget(7,5120);
  assert(denseInjected && denseFault<2 && denseForked && !denseContinued && !denseReceipt
   && !position && layer==3 && completed==3 && allLayers==3 && matrices==1 && !committed && !stopJobs
   && pc==0x60000 && packets==atPackets && peerWrites==atWrites && original.Ready() && minSp>=0x6e000 && minStack>=0x6e000);
  if(!denseFault)assert(budget==458752 && atZeroReads<budget && denseZeroReads==budget && densePollReads==budget);
  else assert(denseZeroReads==atZeroReads && densePollReads==atPollReads+1);
  const auto registers=command;const auto counters=counts;const auto incoming=pushedSlave;
  for(unsigned i=0;i<2048;i++)StepToken();
  assert(pc==0x60000 && packets==atPackets && peerWrites==atWrites && command==registers
   && counts==counters && pushedSlave==incoming);
  printf("PASS connected dense Q8/K5120/seven groups/%s/actual zero-marker polls%u/budget%u/SAME original prior3layers+119 histories/ROOT parks before next packet/inert2048;NOT native/physical/Work/TPS\n",
   denseFault?"wrong nonzero marker":"finite missing receipt",denseZeroReads,budget);
 }
};
int main(int argc,char** argv){
 assert(argc==5);setvbuf(stdout,nullptr,_IOLBF,0);(void)&RunPersistentGdnPair;(void)&Refusals;(void)&LateRefusals;
 WholeModelOriginalBytes source;assert(source.Open(argv[4]) && source.Ready());
 auto m=std::make_unique<PushedDenseFaultMachine>(argv[1],argv[2],argv[3],source);m->VerifyRawEndpoints();
 while(m->pc!=0x60000 && m->pc!=0x60004 && !m->denseContinued){
  m->cycles+=19;m->StepToken();if(!(m->steps&255))for(unsigned w=0;w<119;w++)m->Advance(w,64);
 }
 if(m->injected){m->CheckFault();return 0;}
 if(m->denseInjected){m->CheckDenseFault();return 0;}
 assert(m->denseContinued && m->denseCases==2 && m->denseReceipt && m->allLayers==3
  && !m->committed && !m->stopJobs && source.Ready());
 puts("PASS connected first dense receipt/ROOT from RESET+SAME119/original prior3layers/eight actual matrix-to-Conv counter faults+large missing receipt finite wait+wrong marker/no state transplant/unchanged parent resumes real arithmetic-proof read;NOT completed dense layer/model numerics/native/physical/Work/TPS qualification");
}
