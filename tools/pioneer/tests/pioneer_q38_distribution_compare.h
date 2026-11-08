/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_DISTRIBUTION_COMPARE_H
#define PIONEER_Q38_DISTRIBUTION_COMPARE_H
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) || defined(__FAST_MATH__)
#error Distribution comparison is CPU-only diagnostic code
#endif
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <numeric>
#include <vector>

// Shift-invariant next-token distribution metrics; no vocabulary/model/device IO.
// The KL direction is original || candidate. Top-k ties use the smaller ID.
namespace Q38DistributionCompare {
struct Result {double kl,originalTopProbability,candidateTopProbability,candidateOriginalTopProbability;
 unsigned originalTop,candidateTop,topFiveOverlap;};
struct ObservedToken {unsigned token,rank,top;double logit,topLogit,probability;};
// Rank a real observed ID against an independently computed CPU distribution.
// The result is diagnostic data only, not proof of the source of a mismatch.
inline bool Observe(const std::vector<uint32_t>& words,unsigned token,ObservedToken& output){
 if(words.empty() || words.size()>UINT32_MAX || token>=words.size())return false;
 std::vector<float> values(words.size());unsigned top=0;
 for(size_t i=0;i<words.size();i++){
  std::memcpy(&values[i],&words[i],4);if(!std::isfinite(values[i]))return false;
  if(values[i]>values[top])top=unsigned(i);
 }
 unsigned rank=1;double sum=0;
 for(size_t i=0;i<values.size();i++){
  if(values[i]>values[token] || (values[i]==values[token] && i<token))rank++;
  sum+=std::exp(double(values[i])-values[top]);
 }
 if(!std::isfinite(sum) || !(sum>0))return false;
 ObservedToken result={token,rank,top,values[token],values[top],std::exp(double(values[token])-values[top])/sum};
 output=result;return true;
}
inline bool Compute(const std::vector<uint32_t>& original,const std::vector<uint32_t>& candidate,Result& output){
 if(original.size()<5 || original.size()!=candidate.size() || original.size()>UINT32_MAX)return false;
 std::vector<float> a(original.size()),b(original.size());
 for(size_t i=0;i<a.size();i++){
  std::memcpy(&a[i],&original[i],4);std::memcpy(&b[i],&candidate[i],4);
  if(!std::isfinite(a[i]) || !std::isfinite(b[i]))return false;
 }
 std::vector<unsigned> x(a.size()),y(a.size());std::iota(x.begin(),x.end(),0);std::iota(y.begin(),y.end(),0);
 auto order=[](const std::vector<float>& values,unsigned p,unsigned q){return values[p]!=values[q]?values[p]>values[q]:p<q;};
 std::partial_sort(x.begin(),x.begin()+5,x.end(),[&](unsigned p,unsigned q){return order(a,p,q);});
 std::partial_sort(y.begin(),y.begin()+5,y.end(),[&](unsigned p,unsigned q){return order(b,p,q);});
 const double maxA=a[x[0]],maxB=b[y[0]];double sumA=0,sumB=0;
 for(size_t i=0;i<a.size();i++){sumA+=std::exp(double(a[i])-maxA);sumB+=std::exp(double(b[i])-maxB);}
 if(!std::isfinite(sumA) || !std::isfinite(sumB) || !(sumA>0) || !(sumB>0))return false;
 const double logA=std::log(sumA),logB=std::log(sumB);double kl=0;
 for(size_t i=0;i<a.size();i++){
  const double logp=double(a[i])-maxA-logA,logq=double(b[i])-maxB-logB;
  const double p=std::exp(logp);if(p)kl+=p*(logp-logq);
 }
 if(!std::isfinite(kl) || kl < -1e-10)return false;
 unsigned overlap=0;for(unsigned i=0;i<5;i++)for(unsigned j=0;j<5;j++)if(x[i]==y[j])overlap++;
 Result result={std::max(0.0,kl),1/sumA,1/sumB,std::exp(double(b[x[0]])-maxB)/sumB,x[0],y[0],overlap};
 output=result;return true;
}
}
#endif
