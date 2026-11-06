/* SPDX-License-Identifier: MIT */
#include "../PioneerQ38StartupTiming.h"
#define PIONEER_FULL_OFFLOAD_STARTUP_TIMING 1
#include "../PioneerQ38FullOffloadRateData.h"
#include <cassert>
#include <cstring>
#include <string>
using T=PioneerQ38StartupTiming;
static T Started(){T t;t.LoadBegin(10);t.LoadReady(100);t.Begin(200,31,62);
 t.Upload(210,220,4096);t.Upload(250,270,8192);t.Ready(300);return t;}
static std::string Summary(const T& t,int64_t end=900){char s[768];assert(t.Format(end,s,sizeof(s)));return s;}
int main(){
 const std::string expected="Full-card startup timing vocab_us90 setup_us100 upload_calls2 upload_bytes12288 upload_window_us60 upload_ioctl_us30 first_commit_valid1 first_commit_min_us150 first_commit_max_us220 answer_ready_us700;AFTER_CLEAN_END,host_us,first_commit_not_streamed_text,upload_window_includes_between_call_preparation.";
 {T::Record r;assert(T::Parse(expected,r) && r.available && r.vocab==90 && r.setup==100 && r.calls==2
   && r.bytes==12288 && r.firstMin==150 && r.firstMax==220 && r.answerReady==700);
  assert(T::Parse(T::Unavailable,r) && !r.available);
  for(const auto& bad:{expected+"extra",expected+"\n",std::string("Full-card startup timing invalid")})assert(!T::Parse(bad,r));
  for(const auto& pair:{std::pair<const char*,const char*>("vocab_us90","vocab_us-1"),
   {"setup_us100","setup_us59"},{"upload_calls2","upload_calls0"},{"upload_bytes12288","upload_bytes12289"},
   {"upload_ioctl_us30","upload_ioctl_us61"},{"first_commit_valid1","first_commit_valid2"},
   {"first_commit_min_us150","first_commit_min_us50"},{"first_commit_max_us220","first_commit_max_us149"},
   {"answer_ready_us700","answer_ready_us219"},{"vocab_us90","vocab_us090"}}){
   auto bad=expected;bad.replace(bad.find(pair.first),std::strlen(pair.first),pair.second);assert(!T::Parse(bad,r));
  }}
 {PioneerQ38FullOffloadRateData r;
  assert(r.Observe(T::Unavailable)==-1);
  assert(r.Observe("Full-card count committed0 prompts31 limit62;read-only DATA,not a terminal or cleanup receipt.")==1);
  assert(r.Observe("Full-card count committed31 prompts31 limit62;read-only DATA,not a terminal or cleanup receipt.")==1);
  assert(r.Observe("Full-card count committed62 prompts31 limit62;read-only DATA,not a terminal or cleanup receipt.")==1);
  assert(r.Observe(PioneerQ38FullOffloadRateData::Unavailable)==1 && !r.Complete(32));
  assert(r.Observe(expected)==-1); // fixture's12KiB is not the real full-model upload
  assert(r.Observe(T::Unavailable)==1 && r.Complete(32) && r.Observe(T::Unavailable)==-1);
 }
 {auto t=Started();t.Observe(0,31,62,310,320);t.Observe(30,31,62,350,360);t.Observe(31,31,62,400,420);
  t.Observe(31,31,62,430,440);t.Observe(62,31,62,800,810);assert(Summary(t)==expected);
  char small[3];assert(!t.Format(900,small,sizeof(small)));}
 {auto t=Started();t.Observe(32,31,62,310,320);assert(Summary(t).find("first_commit_valid0 first_commit_min_us0 first_commit_max_us0")!=std::string::npos);}
 {auto t=Started();t.Observe(31,31,62,310,320);assert(Summary(t).find("first_commit_min_us0 first_commit_max_us120")!=std::string::npos);}
 unsigned refused=0;
 for(unsigned failure=0;failure<17;failure++){
  auto t=Started();
  switch(failure){
   case 0:t.Upload(310,320,4096);break;case 1:t.Ready(400);break;
   case 2:t.Observe(0,31,62,299,320);break;case 3:t.Observe(0,31,62,310,309);break;
   case 4:t.Observe(63,31,62,310,320);break;case 5:t.Observe(31,30,62,310,320);break;
   case 6:t.Observe(31,31,61,310,320);break;
   case 7:t.Observe(30,31,62,310,320);t.Observe(29,31,62,330,340);break;
   case 8:t.Observe(30,31,62,310,320);t.Observe(31,31,62,319,340);break;
   case 9:t.LoadReady(400);break;case 10:t.LoadBegin(400);break;
   case 11:t.Begin(90,31,62);break;case 12:t.Begin(400,0,31);break;
   case 13:t.Begin(400,31,63);break;case 14:t.Begin(400,96,129);break;
   case 15:t.Observe(31,31,62,800,810);assert(Summary(t,809)==T::Unavailable);refused++;continue;
   case 16:assert(Summary(t)==T::Unavailable);refused++;continue;
  }
  assert(Summary(t)==T::Unavailable);refused++;
 }
 for(unsigned failure=0;failure<7;failure++){
  T t;t.LoadBegin(10);t.LoadReady(100);t.Begin(200,31,62);
  if(failure==0)t.Upload(199,220,4096);if(failure==1)t.Upload(210,209,4096);
  if(failure==2)t.Upload(210,220,0);if(failure==3)t.Upload(210,220,4097);
  if(failure==4)t.Upload(210,220,1048576+4096);
  if(failure==5){t.Upload(210,220,4096);t.Upload(219,230,4096);}
  if(failure==6)t.Ready(300);
  assert(Summary(t)==T::Unavailable);refused++;
 }
 // A later clean request resets upload/count windows,not the vocabulary span.
 {auto t=Started();t.Observe(62,31,62,310,320);t.Begin(1000,1,1);t.Upload(1010,1020,4096);
  t.Ready(1030);t.Observe(1,1,1,1040,1050);assert(Summary(t,1100).find("vocab_us90 setup_us30 upload_calls1")!=std::string::npos);}
 std::printf("PASS startup timing DATA:4interval/reset positives,24invalid/incomplete cases,skipped first commit unavailable,one clock,no GPU/model/cleanup authority;SYNTHETIC_NOT_HARDWARE_OR_TPS\n");
 assert(refused==24);
}
