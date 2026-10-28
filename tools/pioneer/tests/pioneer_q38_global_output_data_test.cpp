/* SPDX-License-Identifier: MIT */
// DATA/math only; no completed all64 producer, native ownership or device IO.
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#include "../PioneerQ38GlobalOutputPlan.h"
#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <memory>
#include <vector>
namespace G=PioneerQ38GlobalWorkerLayout;
int main(){
 auto c=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(c->Build());
 auto other=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(other->Build());
 std::array<uint32_t,1920> source={};for(unsigned i=0;i<source.size();i++)source[i]=0x76540000+i+1;
 unsigned described=0,refused=0,mutations=0;PioneerQ38GdnIdentity id={1,2,3,4};
 for(unsigned position:{0U,8191U}){
  auto plan=std::make_unique<PioneerQ38GlobalOutputPlan>();assert(plan->Build(*c,position,id,source.data()) && plan->Matches(*c) && !plan->Matches(*other));
  assert(!plan->Build(*c,position,id,source.data()));
  auto jobs=std::make_unique<PioneerQ38ProjectionJobs>();assert(jobs->Build(*c,64,1,position));
  std::vector<unsigned> covered(248320,0);std::array<uint32_t,64> boot={},job={};
  for(unsigned wave=0;wave<9;wave++)for(unsigned worker=1;worker<=jobs->WaveWorkers(wave);worker++){
   unsigned previous=worker*17+wave;
   assert(plan->WorkerBoot(worker,boot.data()) && G::BootShape(boot.data()) && !G::Old::BootShape(boot.data()));
   assert(plan->Job(wave,worker,previous,job.data()) && G::JobShape(job.data(),boot.data(),previous,0)
    && !G::Old::JobShape(job.data(),boot.data(),previous,0) && !G::JobShape(job.data(),boot.data(),previous,1));
   const auto* part=jobs->Part(wave*119+worker-1);assert(part && job[22]==c->Base+part->offset && job[23]==part->groups
    && job[24]==PioneerQ38ProjectionRoute(part->bank) && job[28]==part->firstRow && job[5]==64 && job[17]==14 && job[21]==5120);
   for(unsigned i=0;i<part->groups*16;i++)assert(covered.at(part->firstRow+i)++==0);
   described++;
   if(wave==0 && worker==1){
    const auto valid=job;
    for(unsigned word=0;word<64;word++){
     job=valid;
     unsigned bad=1;
     if(word==0 || word==16 || word==31)bad=0;
     else if(word==1)bad=previous+2;else if(word==2)bad=13;else if(word==3)bad=120;else if(word==4)bad=129;
     else if(word==5)bad=63;else if(word==6)bad=8192;else if(word==7)bad=0;
     else if(word>=8 && word<16)bad=valid[word]^1;
     else if(word==17)bad=12;else if(word==18)bad=2304;else if(word==21)bad=6144;
     else if(word==22)bad=0;else if(word==23)bad=17;else if(word==24)bad=129;
     else if(word==25 || word==26 || word==30)bad=0;else if(word==27)bad=9;else if(word==28)bad=1;else if(word==29)bad=151936;
     job[word]=bad;assert(!G::JobShape(job.data(),boot.data(),previous,0));mutations++;
    }
    job=valid;
    for(unsigned i=40;i<46;i++){auto badBoot=boot;badBoot[i]^=1;assert(!G::BootShape(badBoot.data()) && !G::JobShape(job.data(),badBoot.data(),previous,0));mutations++;}
    std::array<uint32_t,64> stop={};for(unsigned i=0;i<16;i++)stop[i]=job[i];stop[2]=G::Stop;
    assert(G::JobShape(stop.data(),boot.data(),previous,0) && !G::Old::JobShape(stop.data(),boot.data(),previous,0));
    stop[27]=1;assert(!G::JobShape(stop.data(),boot.data(),previous,0));mutations++;
   }
  }
  for(unsigned n:covered)assert(n==1);
  std::array<uint32_t,64> out;out.fill(0xa55aa55a);const auto unchanged=out;
  for(unsigned bad=0;bad<8;bad++){
   bool okay=bad==0?plan->Job(9,1,0,out.data()):bad==1?plan->Job(0,0,0,out.data()):bad==2?plan->Job(8,22,0,out.data()):
    bad==3?plan->Job(0,1,0x7ffffffe,out.data()):bad==4?plan->WorkerBoot(120,out.data()):bad==5?plan->Job(0,1,0,nullptr):
    bad==6?plan->Job(0,1,0,reinterpret_cast<uint32_t*>(plan.get())):plan->Job(0,1,0,reinterpret_cast<uint32_t*>(c.get()));
   assert(!okay && out==unchanged && plan->Matches(*c));refused++;
  }
  PioneerQ38CanonicalPart* norm=nullptr;
  for(unsigned i=0;i<c->Count();i++){const auto* p=c->Part(i);if(p->kind==c->WeightKind && p->layer==64 && p->role==2)norm=const_cast<PioneerQ38CanonicalPart*>(p);}
  assert(norm && plan->NormBank()==norm->bank && plan->NormAddress()==c->Base+norm->offset);
  for(unsigned i=0;i<sizeof(*norm)/4;i++){
   PioneerQ38CanonicalPart saved=*norm;std::array<uint32_t,sizeof(*norm)/4> words;memcpy(words.data(),norm,sizeof(*norm));words[i]^=1;memcpy(norm,words.data(),sizeof(*norm));
   assert(!plan->Matches(*c) && !plan->Job(0,1,0,out.data()) && out==unchanged);*norm=saved;assert(plan->Matches(*c));mutations++;
  }
 }
 for(unsigned bad=0;bad<5;bad++){
  auto plan=std::make_unique<PioneerQ38GlobalOutputPlan>();auto changed=id;
  if(bad==0)changed.session=0;if(bad==1)changed.sequence=0;if(bad==2)changed.modelEpoch=0;if(bad==3)changed.driverGeneration=0;
  assert(!plan->Build(*c,bad==4?8192:0,changed,source.data()) && !plan->Build(*c,0,id,source.data()));refused++;
 }
 unsigned seed=0x12345678;
 for(unsigned i=0;i<20000;i++){
  seed=seed*1664525+1013904223;unsigned a=seed;if((a&0x7f800000)==0x7f800000)a=0;
  seed=seed*1664525+1013904223;unsigned b=seed;if((b&0x7f800000)==0x7f800000)b=0;
  float x,y;memcpy(&x,&a,4);memcpy(&y,&b,4);assert(G::Better(a,11,b,37)==(x>y || (x==y && 11<37)));
 }
 assert(G::Better(0,2,0x80000000,3) && !G::Better(0x80000000,3,0,2));
 printf("PASS QGW1 global DATA/positions0+8191/canonical973jobs9waves/ALL248320 rows exactly once x2/described%u/mutations%u/refusals%u/oldABI rejection/no-alias+no-partial-output/20000 independent finite greedy comparisons+signed-zero tie;NO worker runtime,all64 producer,native,physical,Work,token or TPS grant\n",described,mutations,refused);
}
