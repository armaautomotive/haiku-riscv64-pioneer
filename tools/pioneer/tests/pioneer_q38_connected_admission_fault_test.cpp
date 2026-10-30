/* SPDX-License-Identifier: MIT */
// Genuine executed ROOT+119-worker predecessor,then POSIX fork fault cases.
// TEST byte/CSR fabric only. Never native ownership or physical IO authority.
#include "pioneer_q38_token_original_fabric.h"
struct ConnectedAdmissionFaultMachine:OriginalTokenFlowMachine {
 const bool workerAdmission;
 bool forked=false,injected=false,workerParked=false,rootSawFault=false,parentContinued=false;
 unsigned faultWord=16,cases=0,targetDoorbells=0,packetsAtFault=0,requestsBefore=0,matrixBefore=0;
 std::array<uint32_t,8> countsBefore={};
 ConnectedAdmissionFaultMachine(const char* image,const char* old,const char* worker,WholeModelOriginalBytes& source,bool candidate)
  :OriginalTokenFlowMachine(image,old,worker,source),workerAdmission(candidate){}
 static unsigned SourceWord(unsigned word){return 0xffb20000U+BlackholeDramSourceOffset(word);}
 void CheckPredecessor(){
  assert(!position && !layer && !completed && !allLayers && !committed && !stopJobs && embedded==1 && matrices==1
   && memory.at(RB::Header+43*4)==11);
  for(unsigned w=0;w<3;w++){
   const auto& p=*child[w];
   assert(!p.active && p.sequence==1 && p.GdnFinished(1) && !p.GdnFaulted() && !p.pending
    && memory.at(RB::Nonces+w*4)==1
    && p.type==8 && p.inputs==5120 && p.matrixInstructions==1280 && p.requests==162 && p.responses==162
    && p.memory.at(M::Status)==1 && p.memory.at(M::Status+4)==1 && !p.memory.at(M::Status+8));
  }
 }
 void ParkActualFaultWorker(){
  auto& p=*child[0];assert(workerAdmission && injected && targetDoorbells==1 && p.sequence==2 && p.active
   && !p.requests && !p.responses && !p.matrixInstructions && !p.pending);
  for(unsigned i=0;i<1000000 && !p.GdnFaulted();i++){cycles+=19;p.StepBlock();}
  assert(p.GdnFaulted() && p.pc==p.faultPC && p.memory.at(M::Status+8)==G::Fault
   && !p.requests && !p.responses && !p.matrixInstructions && !p.pending);
  const auto frozen=p.memory;std::array<uint32_t,10> workerCommands;std::array<uint32_t,8> physical;
  for(unsigned i=0;i<10;i++)workerCommands[i]=p.command[i];
  for(unsigned i=0;i<8;i++)physical[i]=p.Count(i);
  for(unsigned i=0;i<2048;i++){cycles+=19;p.StepBlock();}
  assert(p.pc==p.faultPC && p.memory==frozen && !p.requests && !p.responses && !p.matrixInstructions && !p.pending);
  for(unsigned i=0;i<10;i++)assert(p.command[i]==workerCommands[i]);
  for(unsigned i=0;i<8;i++)assert(p.Count(i)==physical[i] && physical[i]==countsBefore[i]);
  p.active=false; // HOST scheduler consequence of ACTUAL fault PC,not processor/receipt state.
  workerParked=true;
 }
 void Store(uint32_t address,uint32_t value)override{
  const bool packet=address==0xffb20040,write=command[7]==0x212;
  const unsigned remote=write?command[3]:command[0],route=write?command[5]:command[2];
  const bool target=packet && route==G::Peer(1);
  if(packet && injected)assert(!rootSawFault); // first observed bad reply MUST forbid another ROOT packet.
  if(target && !write && remote==0xffb121b0 && !forked && matrices==1 && memory.at(RB::Header+43*4)==11){
   CheckPredecessor();forked=true;
   printf("PASS connected genuine predecessor/ROOTfromRESET/SAME119/first3 fullK5120 workers3840 actual tensor instructions+closed sequence1/before NEXT admission;NOT whole layer/token/native/physical/TPS\n");
   for(unsigned word=0;word<16;word++){
    BeforeFork();fflush(nullptr);const pid_t pid=fork();assert(pid>=0);
    if(!pid){
     injected=true;faultWord=word;auto& p=*child[0];requestsBefore=p.requests;matrixBefore=p.matrixInstructions;
     for(unsigned i=0;i<8;i++)countsBefore[i]=p.Count(i);
     p.source[word]^=1U<<((word*7+5)%32); // ONLY corrupt the real TEST CSR observation.
     OriginalTokenFlowMachine::Store(address,value);return; // SAME executing instruction continues normally.
    }
    int status=0;assert(waitpid(pid,&status,0)==pid && WIFEXITED(status) && !WEXITSTATUS(status));cases++;
   }
   assert(cases==16 && !injected && !targetDoorbells && !workerParked && original.Ready());
   CheckPredecessor(); // parent private PC/registers/nonce/history were never replaced.
  }
  OriginalTokenFlowMachine::Store(address,value);
  if(target && write && remote==M::Doorbell && forked){
   assert(child[0]->sequence==2);targetDoorbells++;
   if(injected)ParkActualFaultWorker();
   else{
    auto& p=*child[0];assert(!p.requests && !p.GdnFaulted());
    for(unsigned i=0;i<1000000 && !p.requests && !p.GdnFaulted();i++){cycles+=19;p.StepBlock();}
    assert(p.requests==1 && !p.GdnFaulted() && !p.matrixInstructions);parentContinued=true;
   }
  }
  if(target && !write && injected){
   const unsigned local=command[3];
   const bool physical=remote==SourceWord(faultWord) && memory.at(local)!=child[0]->boot[16+faultWord];
   const bool status=remote==M::Status && memory.at(local+8)==G::Fault;
   const bool pc=remote==0xffb13140 && memory.at(local)==child[0]->faultPC;
   if(physical || status || pc){assert(workerAdmission?!physical && workerParked:physical && !workerParked);rootSawFault=true;packetsAtFault=packets;}
  }
 }
 void CheckFaultCase(){
  assert(injected && faultWord<16 && pc==0x60000 && rootSawFault && packets==packetsAtFault
   && !allLayers && !committed && !stopJobs && original.Ready());
  const auto& p=*child[0];
  if(workerAdmission)assert(targetDoorbells==1 && workerParked && p.GdnFaulted() && p.sequence==2 && !p.requests && !p.matrixInstructions);
  else{
   assert(!targetDoorbells && !workerParked && !p.GdnFaulted() && p.sequence==1 && p.GdnFinished(1)
    && p.requests==requestsBefore && p.matrixInstructions==matrixBefore);
   for(unsigned i=0;i<8;i++)assert(p.Count(i)==countsBefore[i]);
  }
  const auto frozenCommands=command;const auto frozenCounters=counts;const auto writes=peerWrites;
  for(unsigned i=0;i<2048;i++)StepToken();
  assert(pc==0x60000 && packets==packetsAtFault && peerWrites==writes && command==frozenCommands && counts==frozenCounters);
  printf("PASS connected source%u/%s/ACTUAL prior sequence1+tensor result/source-only corruption/ROOT fault+no following packet/retry/cleanup/inert2048;TEST endpoints,NOT native/physical/Work/TPS\n",faultWord,workerAdmission?"worker rejects BEFORE first next-job NoC":"ROOT rejects BEFORE next-job doorbell");
 }
};
int main(int argc,char** argv){
 assert(argc==5 || (argc==6 && !strcmp(argv[5],"--expect-worker-sealed-admission")));
 setvbuf(stdout,nullptr,_IOLBF,0);(void)&RunPersistentGdnPair;(void)&Refusals;(void)&LateRefusals;
 WholeModelOriginalBytes original;assert(original.Open(argv[4]) && original.Ready());
 auto m=std::make_unique<ConnectedAdmissionFaultMachine>(argv[1],argv[2],argv[3],original,argc==6);m->VerifyRawEndpoints();
 while(m->pc!=0x60000 && m->pc!=0x60004 && !m->parentContinued){
  m->cycles+=19;m->StepToken();if(!(m->steps&255))for(unsigned w=0;w<119;w++)m->Advance(w,64);
 }
 if(m->injected){m->CheckFaultCase();_exit(0);}
 assert(m->forked && m->cases==16 && m->parentContinued && m->targetDoorbells==1 && m->pc!=0x60000 && m->pc!=0x60004
  && !m->allLayers && !m->committed && !m->stopJobs && original.Ready());
 puts("PASS connected admission/16 source faults AFTER genuine executed first projection/unchanged parent issues actual next-job first request/NO history transplant,products,completion or STOP supplied;NOT whole-layer/token/native/physical/Work/TPS qualification");
}
