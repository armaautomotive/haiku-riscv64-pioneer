/* SPDX-License-Identifier: MIT */
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE)
#error Full-card rate DATA is host observation arithmetic,not model inference or native authority
#endif
#ifndef PIONEER_Q38_FULL_OFFLOAD_RATE_DATA_H
#define PIONEER_Q38_FULL_OFFLOAD_RATE_DATA_H
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#ifdef PIONEER_FULL_OFFLOAD_STARTUP_TIMING
#if PIONEER_FULL_OFFLOAD_STARTUP_TIMING != 1
#error Exact startup timing DATA selection required
#endif
#include "PioneerQ38StartupTiming.h"
#endif
// Parse only the controlled full-card worker's already-returned integer/time
// DATA. No accelerator API,model products,token selection or cleanup grant.
struct PioneerQ38FullOffloadRateData {
 unsigned rows=0,prompts=0,limit=0,lastCount=0,firstWarm=0;
 unsigned sampled=0,first=0,last=0,observations=0;
 long long minimum=0,maximum=0,firstBegin=0,firstEnd=0,lastBegin=0,lastEnd=0;
 double lower=0,upper=0;
 bool haveWarm=false,rateSeen=false,available=false;
#ifdef PIONEER_FULL_OFFLOAD_STARTUP_TIMING
 bool startupSeen=false;PioneerQ38StartupTiming::Record startup;
#endif
 static constexpr const char* Unavailable="Full-card rate unavailable: insufficient post-prompt window or invalid timing;AFTER_CLEAN_END,no extrapolation.";
 static bool Prefix(const std::string& s,const char* prefix){return s.compare(0,std::strlen(prefix),prefix)==0;}
 static bool Near(double value,double expected){
  return std::isfinite(value) && value>0 && std::fabs(value-expected)<=5.1e-10+1e-12*expected;
 }
 // 0 other progress,1 accepted count/rate DATA,-1 malformed/inconsistent.
 int Observe(const std::string& text){
#ifdef PIONEER_FULL_OFFLOAD_STARTUP_TIMING
  if(Prefix(text,"Full-card startup timing ")){
   if(!rateSeen || startupSeen || !PioneerQ38StartupTiming::Parse(text,startup)
    || (startup.available && startup.bytes!=UINT64_C(19399024640)))return -1;
   startupSeen=true;return 1;
  }
#endif
  if(Prefix(text,"Full-card count ")){
   unsigned n=0,p=0,l=0;int end=-1;
   if(rateSeen || std::sscanf(text.c_str(),"Full-card count committed%u prompts%u limit%u;read-only DATA,not a terminal or cleanup receipt.%n",&n,&p,&l,&end)!=3
    || end<0 || size_t(end)!=text.size() || !p || p>96 || l!=p+31 || l>128 || n>l
    || (rows && (p!=prompts || l!=limit || n<=lastCount)) || rows>=129)return -1;
   prompts=p;limit=l;lastCount=n;rows++;
   if(!haveWarm && n>=p){firstWarm=n;haveWarm=true;}return 1;
  }
  if(Prefix(text,"Full-card observed count rate ")){
   if(rateSeen || !rows)return -1;
   int end=-1;
   if(std::sscanf(text.c_str(),"Full-card observed count rate sampled%u first%u last%u elapsed_min_us%lld elapsed_max_us%lld lower_tps%lf upper_tps%lf observations%u first_begin_us%lld first_end_us%lld last_begin_us%lld last_end_us%lld;AFTER_CLEAN_END,post-prompt count window,not isolated latency or broad quality.%n",
    &sampled,&first,&last,&minimum,&maximum,&lower,&upper,&observations,&firstBegin,&firstEnd,&lastBegin,&lastEnd,&end)!=12
    || end<0 || size_t(end)!=text.size() || !haveWarm || first!=firstWarm || first<prompts || last!=limit || last!=lastCount
    || last<=first || sampled!=last-first || sampled<16 || sampled>31 || observations<rows || observations>18001
    || firstBegin<0 || firstEnd<firstBegin || lastBegin<=firstEnd || lastEnd<lastBegin
    || minimum!=lastBegin-firstEnd || maximum!=lastEnd-firstBegin || minimum<=0 || maximum<minimum
    || !Near(lower,double(sampled)*1000000.0/double(maximum))
    || !Near(upper,double(sampled)*1000000.0/double(minimum)) || lower>upper)return -1;
   rateSeen=available=true;return 1;
  }
  if(Prefix(text,"Full-card rate unavailable:")){
   if(rateSeen || !rows || text!=Unavailable)return -1;
   rateSeen=true;return 1;
  }
  return 0;
 }
 bool Complete(unsigned outputTokens)const{
#ifdef PIONEER_FULL_OFFLOAD_STARTUP_TIMING
  if(!startupSeen)return false;
#endif
  if(!rateSeen || !rows || !haveWarm || outputTokens>32 || lastCount<prompts)return false;
  const unsigned steps=lastCount-prompts+1;
  return steps==outputTokens || steps==outputTokens+1; // EOG can be omitted from text
 }
};
#endif
