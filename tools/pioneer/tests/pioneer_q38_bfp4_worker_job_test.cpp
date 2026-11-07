/* SPDX-License-Identifier: MIT */
// Canonical data-only job/packet binding. No runtime, GPU or upload authority.
#define PIONEER_Q38_BFP4_JOB_DIAGNOSTIC 1
#define PIONEER_Q38_GDN_MUTABLE_STATE
#define PIONEER_BANKS_MOCK_ONLY
#include "../PioneerQ38Bfp4JobBinding.h"
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <memory>
#include <vector>
int main(){
 using W=PioneerQ38Bfp4WorkerJob;auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();
 auto foreign=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(canonical->Build() && foreign->Build());
 unsigned jobs=0,bands=0;
 for(unsigned layer=0;layer<64;layer++)for(unsigned role=2;role<=4;role++){
  PioneerQ38PairedProjectionJobs plan;assert(plan.Build(*canonical,layer,role,8191) && plan.Count()==119);
  std::vector<unsigned> coverage(plan.Outputs(),0);
  for(unsigned ordinal=0;ordinal<plan.Count();ordinal++){
   W::Job job={};assert(PioneerQ38Bfp4JobBinding::Build(*canonical,plan,layer,role,8191,ordinal,7,job));
   assert(W::Shape(job) && PioneerQ38Bfp4JobBinding::Matches(*canonical,plan,layer,role,8191,ordinal,7,job));
   const auto* part=plan.Part(ordinal);assert(part && job.words[4]==part->groups/2);
   assert(job.words[3]==canonical->Base+part->offset && job.words[5]==PioneerQ38ProjectionRoute(part->bank));
   const auto* stripe=canonical->ProjectionStripe(layer,role,part->stripe);assert(stripe);
   assert(part->firstRow>=stripe->firstRow && part->firstRow+job.words[4]*32<=stripe->firstRow+stripe->rows);
   for(unsigned band=0;band<job.words[4];band++){
    for(unsigned row=0;row<32;row++)assert(!coverage[part->firstRow+band*32+row]++);
    for(unsigned operand=0;operand<2;operand++)for(unsigned tile:{0U,W::L::Tiles(job.words[2])-1}){
     W::Read packet={};assert(W::Packet(job,band,operand,tile,packet));
     assert(packet.source==(operand?W::RootInput+tile*64:job.words[3]+(band*W::L::Tiles(job.words[2])+tile)*576));
     assert(packet.route==(operand?W::RootRoute:job.words[5]) && packet.bytes==(operand?64U:576U));
    }
    bands++;
   }
   if(ordinal==0 || ordinal==59 || ordinal==118){
    for(unsigned word=0;word<W::Words;word++){
     auto changed=job;changed.words[word]^=1;
     assert(!PioneerQ38Bfp4JobBinding::Matches(*canonical,plan,layer,role,8191,ordinal,7,changed));
    }
    assert(!PioneerQ38Bfp4JobBinding::Matches(*foreign,plan,layer,role,8191,ordinal,7,job));
    assert(!PioneerQ38Bfp4JobBinding::Matches(*canonical,plan,layer,role,8190,ordinal,7,job));
   }
   W::Read untouched={1,2,3,4};assert(!W::Packet(job,job.words[4],0,0,untouched) && untouched.source==1);
   assert(!W::Packet(job,0,2,0,untouched) && !W::Packet(job,0,0,W::L::Tiles(job.words[2]),untouched));jobs++;
  }
  assert(std::all_of(coverage.begin(),coverage.end(),[](unsigned count){return count==1;}));
 }
 assert(jobs==22848 && bands==64*(544*2+160));
 printf("PASS BFP4 canonical worker jobs: %u jobs, %u 32-row bands, all 64 layers/7-bank stripes, ROOT-L1 BF16 input and bounded mutation refusals; NO DEVICE OR WORK\n",jobs,bands);
}
