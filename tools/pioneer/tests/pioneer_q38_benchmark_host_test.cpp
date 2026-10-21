/* SPDX-License-Identifier: MIT */
#include <cassert>
#include <cstdio>
#include <cstring>
#include <initializer_list>
#define B_DEVICE_OP_CODES_END 10000
#define PIONEER_Q38_BENCHMARK_HOST
#include "../PioneerQ38BenchmarkHost.h"
int main() {
 unsigned tags=0;
 for(uint64_t tag: {
#ifdef PIONEER_Q38_BENCHMARK_HOST_BATCH_RAW_QUALIFICATION
 UINT64_C(0x2026102100010017),UINT64_C(0x2026102100010019),UINT64_C(0x2026102100010020)
#elif defined(PIONEER_Q38_BENCHMARK_HOST_BATCH4_QUALIFICATION)
 UINT64_C(0x2026102100010017),UINT64_C(0x2026102100010019)
#elif defined(PIONEER_Q38_BENCHMARK_HOST_PROFILE_QUALIFICATION)
 UINT64_C(0x2026102100010018)
#else
 UINT64_C(0x2026102100010012),UINT64_C(0x2026102100010016),UINT64_C(0x2026102100010017)
#endif
 }) {
  tt_pioneer_driver_reload good={};good.version=1;good.driver=55;good.guard_version=1;
  good.generation=33;good.open_count=1;good.build_tag=tag;
  assert(PioneerQ38BenchmarkHostTag(good));tags++;
  for(unsigned fault=0;fault<10;fault++) {
   auto r=good;
   switch(fault) {
    case 0:r.version=0;break;case 1:r.driver=54;break;case 2:r.guard_version=0;break;
    case 3:r.generation=0;break;case 4:r.open_count=0;break;case 5:r.open_count=2;break;
    case 6:r.build_tag^=UINT64_C(0x1000);break;case 7:r.quiesced=1;break;case 8:r.cold=1;break;case 9:r.active_owner=1;break;
   }
   assert(!PioneerQ38BenchmarkHostTag(r));tags++;
  }
 }
 for(uint64_t tag:{UINT64_C(0x2026102100010014),UINT64_C(0x2026102100010015)}) {
  tt_pioneer_driver_reload r={};r.version=1;r.driver=55;r.guard_version=1;r.generation=33;r.open_count=1;r.build_tag=tag;
  assert(!PioneerQ38BenchmarkHostTag(r));tags++;
 }
 for(uint64_t tag:{UINT64_C(0x2026102100010012),UINT64_C(0x2026102100010016),UINT64_C(0x2026102100010017),UINT64_C(0x2026102100010018),UINT64_C(0x2026102100010019),UINT64_C(0x2026102100010020)}) {
  tt_pioneer_driver_reload r={};r.version=1;r.driver=55;r.guard_version=1;r.generation=33;r.open_count=1;r.build_tag=tag;
#ifdef PIONEER_Q38_BENCHMARK_HOST_BATCH_RAW_QUALIFICATION
  assert(PioneerQ38BenchmarkHostTag(r)==(tag==UINT64_C(0x2026102100010017) || tag==UINT64_C(0x2026102100010019) || tag==UINT64_C(0x2026102100010020)));
#elif defined(PIONEER_Q38_BENCHMARK_HOST_BATCH4_QUALIFICATION)
  assert(PioneerQ38BenchmarkHostTag(r)==(tag==UINT64_C(0x2026102100010017) || tag==UINT64_C(0x2026102100010019)));
#elif defined(PIONEER_Q38_BENCHMARK_HOST_PROFILE_QUALIFICATION)
  assert(PioneerQ38BenchmarkHostTag(r)==(tag==UINT64_C(0x2026102100010018)));
#else
  assert(PioneerQ38BenchmarkHostTag(r)==(tag==UINT64_C(0x2026102100010012) || tag==UINT64_C(0x2026102100010016) || tag==UINT64_C(0x2026102100010017)));
#endif
  tags++;
 }
 using P=PioneerQ38BenchmarkHostPolicy;
 assert(P::Command("measure\n")==1 && P::Command("quit\n")==2 && !P::Command(nullptr));
 for(const char* line: {"measure","quit","measure\r\n"," measure\n","measure \n","measure\nmeasure\n","MEASURE\n","","\n"})assert(!P::Command(line));
 {P p;assert(!p.Waiting() && !p.Begin() && !p.Authenticated(true) && !p.Close());}
 {P p;assert(!p.Authenticated(false) && !p.Authenticated(true) && !p.Begin());}
 {P p;assert(p.Authenticated(true) && !p.ChildFinished(true) && !p.Begin());}
 {P p;assert(p.Authenticated(true) && p.Begin() && !p.Begin() && !p.ChildFinished(true));}
 {P p;assert(p.Authenticated(true) && p.Begin() && !p.ChildFinished(false) && !p.Begin() && !p.Close());}
 {P p;assert(p.Authenticated(true) && p.Begin() && !p.Close() && !p.ChildFinished(true));}
 {P p;assert(p.Authenticated(true) && p.Close() && !p.Begin() && !p.Authenticated(true));}
 {P p;assert(p.Authenticated(true));for(unsigned n=1;n<=1024;n++)assert(p.Begin() && p.Round()==n && p.ChildFinished(true) && p.Waiting());assert(!p.Begin() && !p.Close());}
 printf("PASS benchmark host metadata-only policy: %u exact idle-tag cases;exact commands;1024 clean rounds;sticky authentication/transition/child-failure/close refusals\n",tags);
}
