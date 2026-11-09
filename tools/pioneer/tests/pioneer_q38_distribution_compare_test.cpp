/* SPDX-License-Identifier: MIT */
#include "pioneer_q38_distribution_compare.h"
#include <cassert>
#include <cstdio>
#include <limits>
static uint32_t Bits(float f){uint32_t word;std::memcpy(&word,&f,4);return word;}
int main(){
 using namespace Q38DistributionCompare;
 std::vector<uint32_t> a,b;
 for(float value:{3.f,2.f,1.f,0.f,-1.f,-2.f}){a.push_back(Bits(value));b.push_back(Bits(value+10));}
 Result result={};assert(Compute(a,b,result));
 assert(result.originalTop==0 && result.candidateTop==0 && result.topFiveOverlap==5 && result.kl<1e-12);
 assert(std::fabs(result.originalTopProbability-result.candidateTopProbability)<1e-12);
 std::vector<uint32_t> uniform(5,Bits(0.f)),tilted=uniform;tilted[0]=Bits(float(std::log(2.0)));
 assert(Compute(uniform,tilted,result));
 ObservedToken observed={};assert(Observe(tilted,0,observed));
 assert(observed.token==0 && observed.rank==1 && observed.top==0 && observed.probability>0.3);
 assert(Observe(tilted,3,observed) && observed.rank==4 && observed.top==0);
 ObservedToken first={},second={};assert(ObservePair(tilted,0,3,first,second));
 assert(first.rank==1 && second.rank==4 && first.top==second.top);
 assert(!ObservePair(tilted,0,5,first,second) && first.rank==1 && second.rank==4);
 assert(!ObservePair(tilted,0,3,first,first) && first.rank==1);
 auto savedObserved=observed;assert(!Observe(tilted,5,observed) && observed.rank==savedObserved.rank);
 tilted[2]=Bits(std::numeric_limits<float>::infinity());
 assert(!Observe(tilted,3,observed) && observed.rank==savedObserved.rank);
 tilted[2]=Bits(0.f);
 const double w=std::exp(double(float(std::log(2.0))));
 assert(std::fabs(result.kl-(std::log((w+4)/5)-std::log(w)/5))<1e-12);
 b[5]=Bits(30.f);assert(Compute(a,b,result));
 assert(result.originalTop==0 && result.candidateTop==5 && result.topFiveOverlap==4 && result.kl>1);
 for(auto& word:a)word=Bits(-10000.f);for(auto& word:b)word=Bits(10000.f);
 assert(Compute(a,b,result) && result.originalTop==0 && result.candidateTop==0 && result.topFiveOverlap==5 && result.kl<1e-12);
 Result saved=result;b[2]=Bits(std::numeric_limits<float>::infinity());assert(!Compute(a,b,result));
 assert(result.kl==saved.kl && result.originalTop==saved.originalTop);
 b.pop_back();assert(!Compute(a,b,result));a.resize(4);b.resize(4);assert(!Compute(a,b,result));
 puts("PASS diagnostic probability comparison: shift invariance, KL, top-five, ties, extreme logits, atomic refusals; CPU_ONLY");
}
