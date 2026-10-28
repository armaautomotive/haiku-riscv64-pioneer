/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_TEST_OBSERVED_TRACE 1
#include "PioneerQ38ObservedTrace.h"
#include <cassert>
#include <limits>
static uint32_t Bits(float value){uint32_t b;std::memcpy(&b,&value,4);return b;}
int main(int argc,char** argv){
 assert(argc==2);setvbuf(stdout,nullptr,_IOLBF,0);PioneerQ38ObservedTrace trace;
 std::array<uint32_t,5120> actual={};std::vector<uint32_t> logits(248320);logits[17]=Bits(1.f);
 for(unsigned i=0;i<actual.size();i++)actual[i]=Bits(float(i%13)*.125f);
 assert(trace.Embedding(0,248319,actual.data(),5120));
 for(unsigned position=0;position<3;position++){
  for(unsigned i=0;i<actual.size();i++)actual[i]=Bits(float(position)+float(i%13)*.125f);
  for(unsigned layer=0;layer<64;layer++)assert(trace.Layer(position,layer,actual.data(),5120));
  for(unsigned first=0;first<248320;first+=256)assert(trace.Vocabulary(position,first,logits.data()+first,std::min(256U,248320-first)));
  if(position<2){
   // Actual parent computes NEXT embedding before publishing this token.
   // The third input's greedy linkage is checked when token1 commits.
   std::array<uint32_t,5120> next;
   for(unsigned i=0;i<next.size();i++)next[i]=Bits(float(position+1)+float(i%13)*.125f);
   assert(trace.Embedding(position+1,17,next.data(),5120));
  }
  std::array<uint32_t,64> record={};
  const unsigned head[]={PioneerQ38ObservedTrace::TokenMagic,1,position,position?17U:248319U,17,Bits(1.f),position+1,(position+1)&1,unsigned(position+1<2),unsigned(position+1<3),64,973};
  for(unsigned i=0;i<12;i++)record[i]=head[i];record[44]=position+1;record[45]=48;record[46]=119;
  assert(trace.Commit(position,record.data(),64));
 }
 assert(trace.Complete() && trace.Save(argv[1]) && !trace.Complete() && !trace.Save(argv[1]));
 for(unsigned bad=0;bad<7;bad++){
  PioneerQ38ObservedTrace rejected;actual.fill(0);
  if(bad==0)assert(!rejected.Layer(0,0,actual.data(),5120));
  else{
   assert(rejected.Embedding(0,248319,actual.data(),5120));
   if(bad==1)assert(!rejected.Embedding(0,248319,actual.data(),5120));
   if(bad==2)assert(!rejected.Layer(0,1,actual.data(),5120));
   if(bad==3){actual[0]=Bits(std::numeric_limits<float>::infinity());assert(!rejected.Layer(0,0,actual.data(),5120));}
   if(bad==4)assert(!rejected.Vocabulary(0,0,logits.data(),256));
   if(bad==5 || bad==6){
    for(unsigned layer=0;layer<64;layer++)assert(rejected.Layer(0,layer,actual.data(),5120));
    assert(rejected.Vocabulary(0,0,logits.data(),256));
    if(bad==5)assert(!rejected.Vocabulary(0,0,logits.data(),256));
    else{std::array<uint32_t,64> incomplete={};assert(!rejected.Commit(0,incomplete.data(),64));}
   }
  }
  assert(!rejected.Complete() && !rejected.Save(argv[1]));
 }
 puts("PASS observed DATA recorder/3positions/65hidden+allvocab+committed records/exclusive complete files/7 invalid-order+duplicate+nonfinite+missing-coverage refusals;SYNTHETIC observations,NOT execution or a numerical/native/Work/TPS grant");
}
