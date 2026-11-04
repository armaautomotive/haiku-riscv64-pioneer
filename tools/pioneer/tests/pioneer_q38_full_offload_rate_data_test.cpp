/* SPDX-License-Identifier: MIT */
// Synthetic count/time DATA only. No model,driver or measured throughput.
#include "../PioneerQ38FullOffloadRateData.h"
#include <cassert>
#include <cstdio>
#include <vector>
using R=PioneerQ38FullOffloadRateData;
static std::string Count(unsigned n){char text[192];snprintf(text,sizeof(text),
 "Full-card count committed%u prompts20 limit51;read-only DATA,not a terminal or cleanup receipt.",n);return text;}
static std::string Rate(){char text[768];snprintf(text,sizeof(text),
 "Full-card observed count rate sampled31 first20 last51 elapsed_min_us999990 elapsed_max_us1000010 lower_tps%.9f upper_tps%.9f observations100 first_begin_us1000 first_end_us1010 last_begin_us1001000 last_end_us1001010;AFTER_CLEAN_END,post-prompt count window,not isolated latency or broad quality.",
 31000000.0/1000010,31000000.0/999990);return text;}
static R Counts(){R r;assert(r.Observe(Count(0))==1 && r.Observe(Count(20))==1 && r.Observe(Count(51))==1);return r;}
int main(){
 unsigned refused=0;
 {auto r=Counts();assert(!r.Complete(32) && r.Observe(Rate())==1 && r.Complete(32) && r.Complete(31)
   && !r.Complete(30) && r.available && r.sampled==31 && r.firstBegin==1000 && r.lastEnd==1001010);
  assert(r.Observe(Count(51))==-1 && r.Observe(Rate())==-1 && r.Observe(R::Unavailable)==-1);}
 {R r;assert(r.Observe(Count(28))==1 && r.Observe(R::Unavailable)==1 && r.Complete(8) && !r.available);}
 {R r;assert(r.Observe("ordinary upload progress")==0 && r.Observe(Rate())==-1 && r.Observe(R::Unavailable)==-1);}
 {R r;assert(r.Observe(Count(20))==1 && r.Observe(Count(20))==-1 && r.Observe(Count(19))==-1);}
 const auto good=Rate();
 for(const auto& pair:std::vector<std::pair<std::string,std::string>>{
  {"sampled31","sampled30"},{"first20","first21"},{"last51","last50"},
  {"elapsed_min_us999990","elapsed_min_us999989"},{"elapsed_max_us1000010","elapsed_max_us1000009"},
  {"observations100","observations2"},{"observations100","observations18002"},
  {"first_begin_us1000","first_begin_us-1"},{"first_end_us1010","first_end_us999"},
  {"last_begin_us1001000","last_begin_us1010"},{"last_end_us1001010","last_end_us1000999"},
  {"lower_tps30.999690003","lower_tpsnan"},{"upper_tps31.000310003","upper_tpsinf"},
  {"lower_tps30.999690003","lower_tps32.000000000"}}){
  auto bad=good;auto at=bad.find(pair.first);assert(at!=std::string::npos);bad.replace(at,pair.first.size(),pair.second);
  auto r=Counts();assert(r.Observe(bad)==-1);refused++;
 }
 for(const auto& suffix:{" extra","\n",";"}){auto r=Counts();assert(r.Observe(good+suffix)==-1);refused++;}
 printf("PASS actual full-card rate DATA parser:available/unavailable/early-EOG accounting/raw timestamps/rate arithmetic/%u malformed-or-inconsistent refusals;synthetic ONLY,no hardware or measured speed\n",refused);
}
