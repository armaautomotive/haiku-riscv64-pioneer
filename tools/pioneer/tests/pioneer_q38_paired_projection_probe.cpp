/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_GDN_MUTABLE_STATE
#define PIONEER_BANKS_MOCK_ONLY
#include "pioneer_resident_banks_test.cpp"
#include "../PioneerQ38PairedProjectionJobs.h"
#include "../PioneerQ38Bf16RootPairs.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ProjectionJobs.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ProjectionInputShape.h"
#include "pioneer_q38_paired_projection_probe.h"
#include <memory>
struct RootPairProbe {
 const PioneerQ38ProjectionJobs& original;
 unsigned parts[119][8]={},rows[119]={};
 unsigned Part(unsigned w,unsigned i)const{
  const auto* p=original.Part(w);assert(p);
  return i==0?PioneerQ38CanonicalModelPlan::Base+unsigned(p->offset):i==1?p->bank:i==2?p->groups:i==4?Route(p->bank):i==5?Source(w):0;
 }
 unsigned Row(unsigned w)const{return original.Part(w)->firstRow;}
 unsigned Route(unsigned bank)const{return PioneerQ38ProjectionRoute(bank);}
 unsigned Source(unsigned w)const{return w+1;}
 void Output(unsigned w,unsigned i,unsigned value){assert(w<119 && i<8);parts[w][i]=value;}
 void OutputRow(unsigned w,unsigned value){assert(w<119);rows[w]=value;}
};
bool ProbePairedPart(unsigned layer,unsigned role,unsigned worker,PioneerPairedPartProbe& output){
 if(!worker || worker>119)return false;
 auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();if(!canonical->Build())return false;
 PioneerQ38PairedProjectionJobs jobs;if(!jobs.Build(*canonical,layer,role,0))return false;
 const auto* p=jobs.Part(worker-1);if(!p || !jobs.Matches(*canonical,layer,role,0))return false;
 output={p->bank,p->firstRow,p->groups,canonical->Base+unsigned(p->offset),jobs.Inputs(),jobs.Outputs()};return true;
}
unsigned ProbePairedCoverage(){
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&BankProof;
 auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(canonical->Build());
 auto foreign=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(foreign->Build());unsigned count=0,refused=0;
 for(unsigned layer=0;layer<64;layer++)for(unsigned role=2;role<=4;role++){
  PioneerQ38PairedProjectionJobs jobs;assert(jobs.Build(*canonical,layer,role,8191) && jobs.Matches(*canonical,layer,role,8191)
   && jobs.Count()==119 && jobs.Waves()==1 && jobs.WaveWorkers(0)==119 && !jobs.Part(119)
   && !jobs.Matches(*foreign,layer,role,8191) && !jobs.Matches(*canonical,layer,role,8190));
  auto original=std::make_unique<PioneerQ38ProjectionJobs>();assert(original->Build(*canonical,layer,role,8191) && original->Count()==119);
  RootPairProbe derived{*original};assert(PioneerQ38Bf16RootPairs::Build(derived,jobs.Inputs(),jobs.Outputs()));
  std::vector<unsigned> covered(jobs.Outputs(),0);
  for(unsigned w=0;w<119;w++){
   const auto* p=jobs.Part(w);assert(p);const auto* stripe=canonical->ProjectionStripe(layer,role,p->stripe);
   const unsigned expected[]={canonical->Base+unsigned(p->offset),p->bank,p->groups,0,PioneerQ38ProjectionRoute(p->bank),w+1,0,0};
   for(unsigned i=0;i<8;i++)assert(derived.parts[w][i]==expected[i]);assert(derived.rows[w]==p->firstRow);
   assert(stripe && p->bank==stripe->bank && !(p->firstRow&31) && !(p->groups&1)
    && p->firstRow>=stripe->firstRow && p->firstRow+p->groups*16<=stripe->firstRow+stripe->rows
    && p->offset==stripe->offset+uint64_t(p->firstRow-stripe->firstRow)*stripe->rowBytes);
   for(unsigned row=p->firstRow;row<p->firstRow+p->groups*16;row++)assert(!covered[row]++);
   if(w==0 || w==59 || w==118)for(unsigned mode=0;mode<4;mode++){
    auto* changed=const_cast<PioneerQ38PairedProjectionPart*>(p);const auto saved=*p;
    if(!mode)changed->bank=7;if(mode==1)changed->groups|=1;if(mode==2)changed->firstRow+=32;if(mode==3)changed->offset+=64;
    assert(!jobs.Matches(*canonical,layer,role,8191));*changed=saved;assert(jobs.Matches(*canonical,layer,role,8191));refused++;
   }
   count++;
  }
  assert(std::all_of(covered.begin(),covered.end(),[](unsigned n){return n==1;}));
 }
 assert(count==22848 && refused==2304);
 puts("PASS PAIRED Q4 canonical metadata192 matrices/jobs22848/refusals2304/all64layers/exact original7-bank stripe coverage/on-card ORIGINAL119-part recipe derivation matches independent canonical paired plan/NO weight repack or runtime/producer/native grant");
 return count;
}
