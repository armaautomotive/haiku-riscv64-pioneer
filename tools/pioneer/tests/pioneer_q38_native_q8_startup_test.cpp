/* SPDX-License-Identifier: MIT */
// Compile against the exact offboard native candidate source, without
// providing any device, physical-owner or live driver operation.
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#define PIONEER_Q38_TOKEN_PUSHED_PROGRAMS 1
#define PIONEER_Q38_TWIN_NATIVE_Q8_WIRE 1
#include "../PioneerQ38TokenStartupPlan.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <memory>

int main(){
 namespace G=PioneerQ38SelectedGlobalWorker;
 namespace W=PioneerQ38TwinNativeQ8Wire;
 auto c=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(c->Build());
 auto s=std::make_unique<PioneerQ38TokenStartupPlan>();
 std::array<uint32_t,1920> sources;
 for(unsigned i=0;i<sources.size();i++)sources[i]=0x76540001+i;
 const PioneerQ38GdnIdentity id={41,11,197,7};
 const uint32_t prompt[]={248319,17};
 assert(s->Build(*c,0,id,sources.data(),prompt,2,64,true,248044,248046));
 assert(s->MatchesPublishedBinding(*c,0,id,sources.data()));
 auto head=std::make_unique<PioneerQ38GlobalOutputPlan>();assert(head->Build(*c,63,id,sources.data()));
 unsigned jobs=0;
 std::array<uint32_t,64> boot={},expected={},job={};
 assert(s->ReadBoot(0,boot.data()) && !boot[W::BootIndex]);
 for(unsigned w=1;w<120;w++){
  assert(s->ReadBoot(w,boot.data()) && head->WorkerBoot(w,expected.data()) && G::BootShape(boot.data()));
  assert(boot==expected && boot[W::BootIndex]==W::BootTag);
  boot[W::BootIndex]=0;assert(!G::BootShape(boot.data()));boot[W::BootIndex]=W::BootTag;
  for(unsigned wave=0;wave<(w<=21?9U:8U);wave++){
   assert(head->Job(wave,w,42+wave,job.data()) && job[17]==14 && job[26]==1
    && G::JobShape(job.data(),boot.data(),42+wave,0));
   job[26]=W::Native;assert(!G::JobShape(job.data(),boot.data(),42+wave,0));jobs++;
  }
 }
 assert(jobs==973);
 puts("PASS native candidate startup: all119 marked worker boots and 973 unchanged raw Q6 head jobs; old/wrong-format data rejected; no device");
}
