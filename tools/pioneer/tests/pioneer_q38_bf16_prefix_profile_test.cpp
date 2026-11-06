/* SPDX-License-Identifier: MIT */
// Diagnostic instruction attribution of the actual cold ROOT prefix.
// Full model executes separately on hardware;this is NOT a speed result.
#define PIONEER_Q38_TOKEN_FLOW_MACHINE_ONLY
#include "pioneer_q38_token_flow_test.cpp"
#include "../PioneerQ38Bf16ProducerLayout.h"
static bool FirstBf16Submission(TokenFlowMachine& m){
 namespace B=PioneerQ38Bf16ProducerLayout;
 if(m.Phase()!=10 || m.memory.at(RB::Header+43*4)!=2 || m.command[3]!=unsigned(B::Descriptor)
  || m.command[5]!=G::Peer(1) || m.command[7]!=0x212 || m.command[8]!=256)return false;
 const unsigned i=m.l1[m.Index(m.pc)],op=i&127,d=(i>>7)&31,s=(i>>15)&31,t=(i>>20)&31;
 return op==0x23 && ((i>>12)&7)==2 && m.regs[s]+Sign(((i>>25)<<5)|d,12)==0xffb20040 && m.regs[t]==1;
}
int main(int argc,char** argv){
 assert(argc==4 && getenv("PIONEER_TEST_ROOT_PROFILE_MAP"));setvbuf(stdout,nullptr,_IOLBF,0);
 (void)&RunPersistentGdnPair;(void)&Refusals;(void)&LateRefusals;
 auto m=std::make_unique<TokenFlowMachine>(argv[1],argv[2],argv[3]);unsigned previous=0;
 puts("START actual BF16 cold-prefix instruction profile/diagnostic only/no products or processor history supplied");
 while(!FirstBf16Submission(*m) && m->pc!=0x60000 && m->pc!=0x60004){
  previous=m->pc;m->cycles+=19;m->StepToken();
  if(!(m->steps&255))for(unsigned w=0;w<119;w++)m->Advance(w,64);
 }
 if(m->pc==0x60000)PioneerQ38InstructionFault::Dump(*m,previous,RB::Projected,RB::PostResidual,RB::Final,RB::Original);
 assert(FirstBf16Submission(*m) && !m->position && !m->layer && m->releases==1 && m->matrices==5
  && m->matrixJobs==363 && m->convJobs==119 && m->gdnJobs==96 && m->gdnChecks==1
  && !m->completed && !m->committed && !m->stopJobs);
 for(unsigned w=0;w<119;w++){
  unsigned expected=1+unsigned(w<96);
  for(unsigned slot=0;slot<5;slot++)expected+=unsigned(w<m->recurrent[0]->Recipe(slot)[7]);
  auto& p=*m->child[w];m->Advance(w);
  assert(!p.active && p.sequence==expected && p.GdnFinished(expected) && m->memory.at(RB::Nonces+w*4)==expected);
 }
 m->profile.Dump(m->window.Total(m->steps));
 printf("PASS ACTUAL cold-prefix instruction attribution/ROOT instructions%llu/reservedStack%u/5Q8+Conv+GDN real SAME119;synthetic raw inputs,NOT BF16 matrix/full model/physical cycles/frequency/TPS proof\n",
  (unsigned long long)m->window.Total(m->steps),0x70000-m->minSp);
}
