/* SPDX-License-Identifier: MIT */
// Canonical/relocated DATA checks only: no worker or GPU execution.
#define PIONEER_Q38_HYBRID_JOB_DIAGNOSTIC 1
#define PIONEER_Q38_GDN_MUTABLE_STATE
#define PIONEER_BANKS_MOCK_ONLY
#include "../PioneerQ38HybridJobBinding.h"
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <memory>
#include <vector>
int main(){
 auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();auto foreign=std::make_unique<PioneerQ38CanonicalModelPlan>();
 assert(canonical->Build() && foreign->Build());const uint64_t mask=UINT64_C(0x1fff000000000000);
 PioneerQ38HybridJobBinding binding;assert(binding.Bind(*canonical,mask) && !binding.Bind(*canonical,mask));
 const auto* relocation=binding.Plan();assert(relocation && relocation->parts.size()==canonical->Count());
 unsigned jobs4=0,jobs8=0,bands=0;
 for(unsigned layer=0;layer<64;layer++)for(unsigned role=2;role<=4;role++){
  PioneerQ38PairedProjectionJobs paired;assert(paired.Build(*canonical,layer,role,8191) && paired.Count()==119);
  const bool eight=mask & (uint64_t(1)<<layer);std::vector<unsigned> coverage(paired.Outputs(),0);
  for(unsigned ordinal=0;ordinal<paired.Count();ordinal++){
   const auto* part=paired.Part(ordinal);assert(part);const auto* stripe=canonical->ProjectionStripe(layer,role,part->stripe);assert(stripe);
   unsigned index=canonical->Count();for(unsigned i=0;i<canonical->Count();i++)if(canonical->Part(i)==stripe){index=i;break;}
   assert(index<relocation->parts.size());const auto& dest=relocation->parts[index];
   const uint64_t bandBytes=uint64_t(paired.Inputs()/32)*(eight?1088:576);
   const uint64_t within=uint64_t((part->firstRow-stripe->firstRow)/32)*bandBytes;
   const uint64_t address=uint64_t(canonical->Base)+dest.offset+within;
   assert(dest.bank==part->bank && dest.encoding==(eight?8U:0U) && within+uint64_t(part->groups/2)*bandBytes<=dest.bytes);
   if(eight){
    PioneerQ38Bfp8WorkerJob::Job job={};assert(binding.Build8(paired,layer,role,8191,ordinal,7,job));
    assert(binding.Matches8(paired,layer,role,8191,ordinal,7,job) && job.words[3]==address && job.words[4]==part->groups/2);
    PioneerQ38Bfp4WorkerJob::Job wrong={};assert(!binding.Build4(paired,layer,role,8191,ordinal,7,wrong));
    if(ordinal==0)for(unsigned word=0;word<16;word++){auto bad=job;bad.words[word]^=1;assert(!binding.Matches8(paired,layer,role,8191,ordinal,7,bad));}
    jobs8++;
   }else{
    PioneerQ38Bfp4WorkerJob::Job job={};assert(binding.Build4(paired,layer,role,8191,ordinal,7,job));
    assert(binding.Matches4(paired,layer,role,8191,ordinal,7,job) && job.words[3]==address && job.words[4]==part->groups/2);
    PioneerQ38Bfp8WorkerJob::Job wrong={};assert(!binding.Build8(paired,layer,role,8191,ordinal,7,wrong));
    if(ordinal==0)for(unsigned word=0;word<16;word++){auto bad=job;bad.words[word]^=1;assert(!binding.Matches4(paired,layer,role,8191,ordinal,7,bad));}
    jobs4++;
   }
   assert(part->firstRow>=stripe->firstRow && part->firstRow+part->groups*16<=stripe->firstRow+stripe->rows);
   for(unsigned row=part->firstRow;row<part->firstRow+part->groups*16;row++)assert(!coverage[row]++);
   bands+=part->groups/2;
  }
  assert(std::all_of(coverage.begin(),coverage.end(),[](unsigned count){return count==1;}));
 }
 assert(jobs4==51*3*119 && jobs8==13*3*119 && bands==64*(544*2+160));
 PioneerQ38PairedProjectionJobs other;assert(other.Build(*foreign,48,2,8191));PioneerQ38Bfp8WorkerJob::Job untouched={};untouched.words[0]=123;
 assert(!binding.Build8(other,48,2,8191,0,7,untouched) && untouched.words[0]==123);
 PioneerQ38PairedProjectionJobs valid;assert(valid.Build(*canonical,48,2,8191));
 assert(!binding.Build8(valid,48,2,8190,0,7,untouched) && !binding.Build8(valid,48,2,8191,119,7,untouched)
  && !binding.Build8(valid,48,2,8191,0,0,untouched) && untouched.words[0]==123);
 PioneerQ38HybridJobBinding impossible;assert(!impossible.Bind(*canonical,UINT64_C(0xfffc000000000000)) && !impossible.Plan()
  && !impossible.Bind(*canonical,mask));
 printf("PASS relocated hybrid jobs: BFP4 %u/BFP8 %u, %u bands, all layers/119 peers, exact format, rows and bank bounds; NO DEVICE\n",jobs4,jobs8,bands);
}
