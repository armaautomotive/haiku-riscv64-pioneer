/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_TOKEN_MATRIX_PHASE_DATA_TEST 1
#include "../PioneerQ38TokenMatrixPhase.h"
#include "../PioneerQ38TokenMatrixPhaseData.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <initializer_list>
int main(){
 namespace Q=PioneerQ38TokenMatrixPhaseData;
 static_assert(Q::Address==PioneerQ38TokenMatrixPhase::Address && Q::Words==PioneerQ38TokenMatrixPhase::Words
  && Q::Magic==PioneerQ38TokenMatrixPhase::Magic && Q::Version==PioneerQ38TokenMatrixPhase::Version,"same fixed record geometry");
 unsigned positive=0,negative=0;
 for(unsigned worker=1;worker<120;worker++)for(unsigned k:{5120U,6144U,17408U})for(unsigned groups=1;groups<=16;groups++){
  const unsigned nonce=worker*100+groups,stop=nonce+1;
  PioneerQ38TokenMatrixPhase profile;profile.Init(worker,nonce,k,groups);
  uint64_t now=UINT64_C(1)<<40;
  for(unsigned g=0;g<groups;g++)for(unsigned b=0;b<k/256;b++){
   assert(profile.Begin(g,b,now+=13) && profile.Fetched(now+=17));
   for(unsigned p=0;p<16;p++)assert(profile.Decoded(p,now+=19) && profile.Executed(p,now+=23));
  }
  uint32_t data[Q::Words];assert(profile.Describe(data,true) && Q::Classify(data,worker,stop)==Q::Intervals);positive++;
  auto refuse=[&](unsigned word,unsigned value){
   uint32_t copy[Q::Words];memcpy(copy,data,sizeof(copy));copy[word]=value;
   assert(Q::Classify(copy,worker,stop)==Q::Malformed);negative++;
  };
  for(unsigned word:{0U,1U,3U,4U})refuse(word,data[word]^1);
  refuse(2,0);refuse(2,stop);refuse(5,1);refuse(6,0);refuse(6,17);refuse(7,2);refuse(14,1);refuse(15,1);
  assert(Q::Classify(data,worker,nonce)==Q::Malformed && Q::Classify(data,worker,0)==Q::Malformed
   && Q::Classify(data,0,stop)==Q::Malformed && Q::Classify(data,120,stop)==Q::Malformed);negative+=4;
  uint32_t copy[Q::Words];memcpy(copy,data,sizeof(copy));copy[7]=0;
  assert(Q::Classify(copy,worker,stop)==Q::Malformed);negative++;
  for(unsigned i=8;i<14;i++)copy[i]=0;
  assert(Q::Classify(copy,worker,stop)==Q::InvalidClock);positive++;
  copy[7]=1;assert(Q::Classify(copy,worker,stop)==Q::Malformed);negative++;
  memcpy(copy,data,sizeof(copy));copy[8]=copy[9]=~uint32_t(0);
  assert(Q::Classify(copy,worker,stop)==Q::Malformed);negative++;
  for(auto& word:copy)word=0;
  assert(Q::Classify(copy,worker,stop)==Q::Missing);positive++;
 }
 printf("PASS same sampler DATA state machine+observation grammar/%u positives/%u malformed-or-unbound refusals;NO freshness,exact latest-job prediction,native,HOLD,model,physical,clock-frequency or TPS authority\n",positive,negative);
}
