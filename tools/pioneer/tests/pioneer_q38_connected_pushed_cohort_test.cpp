/* SPDX-License-Identifier: MIT */
// Genuine original-model matrix -> Conv handoff;raw counter faults ONLY.
#ifndef PIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION
#error Exact HOST QGW3 cohort-handoff test profile required
#endif
#include "pioneer_q38_token_original_fabric.h"
struct PushedCohortMachine:OriginalTokenFlowMachine {
 using OriginalTokenFlowMachine::OriginalTokenFlowMachine;
 bool forked=false,injected=false,badObserved=false,continued=false;
 unsigned fault=8,cases=0,handoffs=0,packetsAtBad=0,commandsAtBad=0;
 unsigned densePollReads=0,denseZeroReads=0;bool denseReceipt=false,denseFfnRetention=false;
 uint32_t Load(uint32_t address)override{
  const unsigned value=OriginalTokenFlowMachine::Load(address); // EXACTLY one actual load;no state setter.
  if(continued && !injected && !denseFfnRetention && !position && layer==3 && Phase()==8 && matrices==4 && address==G::Mailbox){
   namespace D=PioneerQ38MixedLiveBlockRootLayout;
   assert(denseReceipt && value==G::ReceiptMagic && memory.at(D::FfnCopies)==D::MatrixMagic);
   for(unsigned i=0;i<3*D::RecipeStride;i+=4)assert(memory.at(D::FfnRecipes+i)==memory.at(D::FfnCopies+i));
   for(unsigned w=0;w<119;w++)assert(memory.at(G::Mailbox+w*256)==G::ReceiptMagic);
   denseFfnRetention=true;
   puts("OBSERVED actual dense CaptureFfn/three complete recipe copies/SAME prior3layers+attention+output projection/all119 genuine retained receipt markers survive/ROOT resumes first FFN admission;NOT completed FFN/layer/native/physical/TPS proof");
  }
  if(continued && !injected && !denseReceipt && !position && layer==3 && matrices==1 && address==G::Mailbox){
   densePollReads++;
   if(!value)denseZeroReads++;
   else if(value==G::ReceiptMagic){
    const auto& p=*child[0];
    assert(p.type==8 && p.inputs==5120 && p.matrixInstructions==8960 && p.requests==1122
     && p.responses==1122 && p.pushPackets==2);
    denseReceipt=true;
    printf("OBSERVED genuine first dense Q8 receipt/SAME prior3layers+119 histories/worker0 actual8960 tensor instructions+1122 reads+payload+commit/zeroMarkerReads%u/allMarkerReads%u/afterOld65536Limit%u;NO elapsed/native/physical/TPS proof\n",
     denseZeroReads,densePollReads,unsigned(denseZeroReads>=65536));
   }
  }
  return value;
 }
 static unsigned CounterAddress(unsigned i){return 0xffb20000+FfnPersistentPeer::CounterOffset(i);}
 void Predecessor(){
  assert(!position && !layer && matrices==4 && Phase()==6 && !allLayers && !committed && !convJobs && !stopJobs);
  const auto& p=*child[0];
  assert(!p.active && p.GdnFinished(p.sequence) && p.sequence==4 && !p.GdnFaulted() && !p.pending
   && p.memory.at(M::Job+8)==1 && p.pushPackets==2 && p.requests==p.packets.size() && p.CorrectRows());
  const auto* proof=&p;const uint32_t delta[]={0,0,2,0,2,5,2,2};
  for(unsigned i=0;i<8;i++)assert(proof->Count(i)==proof->memory.at(0x83f00+(8+i)*4)+delta[i]);
 }
 void Store(uint32_t address,uint32_t value)override{
  const bool submit=address==0xffb20040,write=command[7]==0x212;
  const unsigned remote=write?command[3]:command[0],route=write?command[5]:command[2];
  const bool target=submit && route==G::Peer(1);
  if(submit && injected)assert(!badObserved);
  if(target && !write && remote==CounterAddress(0) && !forked && matrices==4 && Phase()==6){
   Predecessor();forked=true;
   for(unsigned counter=0;counter<8;counter++){
    BeforeFork();fflush(nullptr);const pid_t pid=fork();assert(pid>=0);
    if(!pid){
     injected=true;fault=counter;
     if(counter==2 || counter==6){child[0]->countBase[2]++;child[0]->countBase[6]++;}
     else child[0]->countBase[counter]++;
     OriginalTokenFlowMachine::Store(address,value);if(remote==CounterAddress(fault)){badObserved=true;packetsAtBad=packets;commandsAtBad=peerWrites;}return;
    }
    int status=0;assert(waitpid(pid,&status,0)==pid && WIFEXITED(status) && !WEXITSTATUS(status));cases++;
   }
   Predecessor();assert(cases==8 && !injected);
  }
  OriginalTokenFlowMachine::Store(address,value);
  if(target && !write && injected && remote==CounterAddress(fault)){
   assert(!badObserved);badObserved=true;packetsAtBad=packets;commandsAtBad=peerWrites;
  }
  if(target && write && remote==M::Doorbell && forked && !continued && !position && !layer
   && child[0]->memory.at(M::Job+8)==7){
   auto& p=*child[0];assert(cases==8 && !p.convPackets && p.sequence==5 && !p.GdnFaulted());
   for(unsigned i=0;i<1000000 && !p.convPackets && !p.GdnFaulted();i++){cycles+=19;p.StepBlock();}
   assert(p.convPackets==1 && !p.GdnFaulted() && !handoffs);handoffs++;continued=true;
  }
 }
 void CheckFault(){
  assert(injected && fault<8 && badObserved && pc==0x60000 && packets==packetsAtBad && peerWrites==commandsAtBad
   && !convJobs && !allLayers && !committed && !stopJobs && original.Ready());
  const auto registers=command;const auto counters=counts;const auto input=pushedSlave;
  for(unsigned i=0;i<2048;i++)StepToken();
  assert(pc==0x60000 && command==registers && counts==counters && pushedSlave==input && packets==packetsAtBad && peerWrites==commandsAtBad);
  printf("PASS QGW3 genuine matrix-to-Conv counter%u/alias2+6 moved together/ROOT rejects extra movement before Conv release/no following packet/inert2048;NOT native/physical/Work/TPS\n",fault);
 }
};
#ifndef PIONEER_Q38_PUSHED_COHORT_MACHINE_ONLY
int main(int argc,char** argv){
 assert(argc==5 || (argc==8 && !strcmp(argv[5],"--snapshot-first-layer") && !strcmp(argv[6],"--observed-trace")));
 setvbuf(stdout,nullptr,_IOLBF,0);(void)&RunPersistentGdnPair;(void)&Refusals;(void)&LateRefusals;
 WholeModelOriginalBytes source;assert(source.Open(argv[4]) && source.Ready());
 auto m=std::make_unique<PushedCohortMachine>(argv[1],argv[2],argv[3],source);m->VerifyRawEndpoints();
#ifdef PIONEER_Q38_TEST_OBSERVED_TRACE
 std::unique_ptr<PioneerQ38ObservedTrace> observed;
 if(argc==8){observed=std::make_unique<PioneerQ38ObservedTrace>(false);m->observed=observed.get();m->snapshotFirstLayerDirectory=argv[7];}
#else
 assert(argc==5);
#endif
 while(m->pc!=0x60000 && m->pc!=0x60004 && !m->continued){m->cycles+=19;m->StepToken();if(!(m->steps&255))for(unsigned w=0;w<119;w++)m->Advance(w,64);}
 if(m->injected){m->CheckFault();return 0;}
 assert(m->continued && m->cases==8 && m->handoffs==1 && !m->allLayers && !m->committed && source.Ready());
 puts("PASS connected QGW3 matrix-to-Conv/ROOT from RESET+SAME119/original four projections/actual prior payload+commit/8 physical-counter faults rejected/unchanged parent admits ALL119 then actual nonce5 Conv first read;NOT completed Conv/GDN/layer/model numerics/native/physical/Work/TPS qualification");
#ifdef PIONEER_Q38_TEST_OBSERVED_TRACE
 if(observed){
  puts("CONTINUE SAME original ROOT+119 histories AFTER actual cohort handoff gate/NO reset,transplanted processor state or fabricated token/first-layer snapshot remains INCOMPLETE DATA only");
  const bool okay=m->RunTokens();
  if(m->kvFault){assert(!okay);m->VerifyKvFault();assert(source.Ready());_exit(0);}
  assert(okay && source.Ready() && m->handoffs==1 && m->denseReceipt && m->denseFfnRetention);m->VerifyTokens();
  assert(observed->Complete() && observed->Save(argv[7]) && source.Ready());
  puts("SAVED actual complete3position traces AFTER genuine192layers+119STOP/cohort gate history preserved;NOT independent CPU match,native END,physical,Work or TPS qualification");
 }
#endif
}
#endif
