/* SPDX-License-Identifier: MIT */
// Actual complete immutable parent from reset and SAME119 virgin workers.
// Stop observation BEFORE the first BF16 descriptor packet is submitted.
// No fabricated predecessor products,PC,private seal or completion history.
#define PIONEER_Q38_TOKEN_FLOW_MACHINE_ONLY
#include "pioneer_q38_token_flow_test.cpp"
#include "pioneer_q38_paired_projection_probe.h"
#include "../PioneerQ38Bf16RootPairs.h"
namespace BP=PioneerQ38Bf16ProducerLayout;
struct Bf16PrefixMachine:TokenFlowMachine {
 using TokenFlowMachine::TokenFlowMachine;
 bool AtBf16Descriptor(){
  if(Phase()!=10 || memory.at(RB::Header+43*4)!=2 || command[3]!=unsigned(BP::Descriptor)
   || command[5]!=G::Peer(1) || command[7]!=0x212 || command[8]!=256)return false;
  const unsigned i=l1[Index(pc)],op=i&127,d=(i>>7)&31,s=(i>>15)&31,t=(i>>20)&31;
  return op==0x23 && ((i>>12)&7)==2 && regs[s]+Sign(((i>>25)<<5)|d,12)==0xffb20040 && regs[t]==1;
 }
 void VerifyPrefix(){
  using P=PioneerQ38Bf16RootPairs;
  unsigned expectedJobs=0;for(unsigned slot=0;slot<5;slot++)expectedJobs+=recurrent[0]->Recipe(slot)[7];
  assert(AtBf16Descriptor() && !position && !layer && releases==1 && matrices==5 && !completed && !committed
   && matrixJobs==expectedJobs && convJobs==119 && gdnJobs==96 && gdnChecks==1 && !stopJobs);
  assert(memory.at(RB::Header+14*4)==5120 && memory.at(RB::Header+15*4)==12);
  for(unsigned w=0;w<119;w++){
   unsigned expectedNonce=1+unsigned(w<96); // actual Conv and GDN jobs
   for(unsigned slot=0;slot<5;slot++)expectedNonce+=unsigned(w<recurrent[0]->Recipe(slot)[7]);
   auto& p=*child[w];Advance(w);assert(!p.active && p.GdnFinished(p.sequence)
    && memory.at(RB::Nonces+w*4)==p.sequence && p.sequence==expectedNonce);
   PioneerPairedPartProbe wanted;assert(ProbePairedPart(0,2,w+1,wanted));
   const unsigned expected[]={wanted.address,wanted.bank,wanted.groups,0,PioneerQ38ProjectionRoute(wanted.bank),p.source[0],0,0};
   for(unsigned i=0;i<8;i++)assert(memory.at(P::Parts+w*32+i*4)==expected[i] && memory.at(P::Shadow+w*32+i*4)==expected[i]);
   assert(memory.at(P::Rows+w*4)==wanted.firstRow && memory.at(P::RowShadow+w*4)==wanted.firstRow);
  }
  for(unsigned i=0;i<5120;i++)assert(memory.at(RB::Input+i*4)==memory.at(BP::ProjectionInput+i*4));
  uint32_t descriptor[64];for(unsigned i=0;i<64;i++)descriptor[i]=memory.at(RB::Scratch+i*4);
  assert(BP::DescriptorShape(descriptor) && descriptor[7]==1 && descriptor[43]==2
   && descriptor[10]==BP::ProjectionInput && !descriptor[40] && descriptor[41]==currentToken && !descriptor[42]);
  printf("PASS ACTUAL BF16 ROOT prefix/reset->embedding->5 Q8 projections%u jobs->119 Conv+96 GDN closures->postRMS->on-card119 paired recipe transform+FP32 shadow->first typed BF16 descriptor BEFORE submission;SAME119 real recipe-derived histories,stack%u;RAW synthetic inputs,NOT executed BF16 producer/matrix/whole layer/native/physical/full-model quality/TPS proof\n",matrixJobs,0x70000-minSp);
 }
};
int main(int argc,char** argv){
 assert(argc==4);setvbuf(stdout,nullptr,_IOLBF,0);(void)&RunPersistentGdnPair;(void)&Refusals;(void)&LateRefusals;
 auto m=std::make_unique<Bf16PrefixMachine>(argv[1],argv[2],argv[3]);
 puts("START actual BF16 ROOT prefix/virgin SAME119/no predecessor state transplant");
 unsigned previous=0;
 while(!m->AtBf16Descriptor() && m->pc!=0x60000 && m->pc!=0x60004){
  previous=m->pc;m->cycles+=19;m->StepToken();
  // Match RunTokens(): every worker executes its own instructions while
  // ROOT is computing/polling. This schedules processors,not products.
  if(!(m->steps&255))for(unsigned w=0;w<119;w++)m->Advance(w,64);
  if(!(m->steps%10000000))printf("ROOT prefix observed%u instructions phase%u Q8jobs%u Conv%u GDN%u PC%x\n",m->steps,m->Phase(),m->matrixJobs,m->convJobs,m->gdnJobs,m->pc);
 }
 if(m->pc==0x60000)PioneerQ38InstructionFault::Dump(*m,previous,RB::Projected,RB::PostResidual,RB::Final,RB::Original);
 assert(m->pc!=0x60000);m->VerifyPrefix();
}
