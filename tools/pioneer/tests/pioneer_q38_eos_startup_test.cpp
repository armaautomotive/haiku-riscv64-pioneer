/* SPDX-License-Identifier: MIT */
// Genuine ROOT bootstrap instructions with SYNTHETIC raw byte endpoints.
// No model layer/token/terminal/native/physical qualification follows.
#define PIONEER_Q38_TOKEN_FLOW_MACHINE_ONLY
#include "pioneer_q38_token_flow_test.cpp"
int main(int argc,char** argv){
 assert(argc==4);setvbuf(stdout,nullptr,_IOLBF,0);(void)&RunPersistentGdnPair;(void)&Refusals;(void)&LateRefusals;
 unsigned entry=0;std::ifstream symbols(std::string(argv[1])+"/program.symbols");std::string address,kind,name;
 while(symbols>>address>>kind>>name)if(name=="q38_return_recurrent_layer")entry=unsigned(std::stoul(address,nullptr,16));assert(entry);
 for(bool eos:{false,true}){
  auto m=std::make_unique<TokenFlowMachine>(argv[1],argv[2],argv[3],eos);
  const unsigned session=m->outer[9]+TL::SessionOffset;
  assert(!m->steps && !m->packets && !m->parent && m->Get(m->outer[8],session+5*4)==3 && m->Get(m->outer[8],session+6*4)==2
   && m->Get(m->outer[8],session+32*4)==unsigned(eos)
   && m->Get(m->outer[8],session+33*4)==(eos?TokenFlowMachine::EosFirst:248318)
   && m->Get(m->outer[8],session+34*4)==(eos?TokenFlowMachine::EosSecond:248317));
  while(m->pc!=entry && m->pc!=0x60000){assert(m->steps<200000000);m->StepToken();}
  assert(m->pc==entry && m->releases==1 && m->embedded==1 && m->embeddingReads==20 && m->embeddingWrites==2 && m->embeddingBack==2
   && !m->allLayers && !m->matrixJobs && !m->peerWrites && !m->committed && !m->flips && !m->tokenPublications && !m->stopJobs
   && m->ActualPrivate(20)==3 && m->ActualPrivate(22)==2 && m->ActualPrivate(23)==unsigned(eos)
   && m->ActualPrivate(24)==(eos?TokenFlowMachine::EosFirst:248318) && m->ActualPrivate(25)==(eos?TokenFlowMachine::EosSecond:248317));
  for(const auto& p:m->child)assert(!p->active && !p->sequence && p->GdnFinished(0));
  printf("PASS ACTUAL ROOT from RESET/raw EOS%u session+two prompt IDs/actual private EOS+limit snapshot/5120 first embedding/first returning entry/SAME119 virgin/no model layer,token,EOS decision or STOP invented;SYNTHETIC raw endpoints only,NOT original/native/physical/Work/TPS qualification\n",unsigned(eos));
 }
}
