/* SPDX-License-Identifier: MIT */
// Canonical/relocated DATA checks only: no worker or GPU execution.
#define PIONEER_Q38_HYBRID_JOB_DIAGNOSTIC 1
#define PIONEER_Q38_GDN_MUTABLE_STATE
#define PIONEER_BANKS_MOCK_ONLY
#include "../PioneerQ38HybridJobBinding.h"
#include "../PioneerQ38HybridWorkerLayout.h"
#include "../PioneerQ38Bf16RootPairs.h"
#include "../PioneerQ38HybridRootPairs.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ProjectionJobs.h"
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <memory>
#include <vector>
namespace HW=PioneerQ38HybridWorkerLayout;
struct RootProbe {
 const PioneerQ38ProjectionJobs& original;
 unsigned parts[119][8]={},rows[119]={},table[7][4]={};
 unsigned Part(unsigned w,unsigned i)const{
  const auto* p=original.Part(w);assert(p);
  return i==0?PioneerQ38CanonicalModelPlan::Base+unsigned(p->offset):i==1?p->bank:i==2?p->groups:i==4?Route(p->bank):i==5?Source(w):0;
 }
 unsigned Row(unsigned w)const{return original.Part(w)->firstRow;}
 unsigned Route(unsigned b)const{return PioneerQ38ProjectionRoute(b);}
 unsigned Source(unsigned w)const{return w+1;}
 unsigned Result(unsigned w,unsigned i)const{return parts[w][i];}
 unsigned ResultRow(unsigned w)const{return rows[w];}
 unsigned Table(unsigned bank,unsigned i)const{return table[bank][i];}
 void Output(unsigned w,unsigned i,unsigned value){parts[w][i]=value;}
 void OutputRow(unsigned w,unsigned value){rows[w]=value;}
};
template<class W>static void CheckWorker(const typename W::Job& leaf,unsigned layer,unsigned role,unsigned firstRow){
 uint32_t boot[64]={};boot[0]=HW::Magic;boot[1]=1;boot[2]=64;boot[3]=leaf.words[6];boot[4]=leaf.words[7];
 boot[5]=0x14000;boot[6]=8192;boot[7]=64;
 for(unsigned i=0;i<4;i++)boot[8+i*2]=i+1;
 boot[16]=boot[5];for(unsigned i=0;i<3;i++)for(unsigned lane=0;lane<6;lane++)boot[17+i]|=(i*6+lane)<<(lane*5);
 for(unsigned i=0;i<2;i++)for(unsigned lane=0;lane<6;lane++)boot[23+i]|=(i*6+lane)<<(lane*5);
 for(unsigned i=40;i<=50;i++)boot[i]=HW::BootWord(i);
 assert(HW::BootShape(boot));
 uint32_t j[64]={};j[0]=HW::Magic;j[1]=leaf.words[11];j[2]=1;j[3]=boot[3];j[4]=boot[4];j[5]=layer;j[6]=8191;j[7]=17;
 for(unsigned i=0;i<8;i++)j[8+i]=boot[8+i];
 for(unsigned i=0;i<16;i++)j[16+i]=leaf.words[i];
 j[32]=firstRow;j[33]=leaf.words[2]==5120?17408:5120;j[34]=role;j[35]=W::Magic==PioneerQ38Bfp8WorkerJob::Magic?8:4;
 assert(HW::JobShape(j,boot,j[1]-1,0));
 assert(!HW::JobShape(j,boot,j[1]-1,1));
 j[35]=j[35]==4?8:4;assert(!HW::JobShape(j,boot,j[1]-1,0));j[35]=j[35]==4?8:4;
 j[36]=1;assert(!HW::JobShape(j,boot,j[1]-1,0));j[36]=0;
 boot[50]^=1;assert(!HW::JobShape(j,boot,j[1]-1,0));boot[50]^=1;
 j[16]=PioneerQ38Q4FpuCachedLayout::Magic;assert(!HW::JobShape(j,boot,j[1]-1,0));
}
#ifndef PIONEER_Q38_HYBRID_BINDING_ONLY
int main(){
 auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();auto foreign=std::make_unique<PioneerQ38CanonicalModelPlan>();
 assert(canonical->Build() && foreign->Build());const uint64_t mask=UINT64_C(0x1fff000000000000);
 PioneerQ38HybridJobBinding binding;assert(binding.Bind(*canonical,mask) && !binding.Bind(*canonical,mask));
 uint32_t manifest[PioneerQ38HybridJobBinding::RootManifestWords]={};
 assert(binding.RootManifest(manifest,PioneerQ38HybridJobBinding::RootManifestWords));
 uint32_t untouchedManifest=123;assert(!binding.RootManifest(&untouchedManifest,1) && untouchedManifest==123);
 const auto* relocation=binding.Plan();assert(relocation && relocation->parts.size()==canonical->Count());
 unsigned jobs4=0,jobs8=0,bands=0;
 for(unsigned layer=0;layer<64;layer++)for(unsigned role=2;role<=4;role++){
  PioneerQ38PairedProjectionJobs paired;assert(paired.Build(*canonical,layer,role,8191) && paired.Count()==119);
  auto old=std::make_unique<PioneerQ38ProjectionJobs>();assert(old->Build(*canonical,layer,role,8191) && old->Count()==119);
  auto root=std::make_unique<RootProbe>(RootProbe{*old});
  for(unsigned stripeNumber=0;stripeNumber<7;stripeNumber++){
   const auto* stripe=canonical->ProjectionStripe(layer,role,stripeNumber);assert(stripe);
   unsigned index=canonical->Count();for(unsigned i=0;i<canonical->Count();i++)if(canonical->Part(i)==stripe){index=i;break;}
   assert(index<relocation->parts.size());const auto& dest=relocation->parts[index];
   root->table[stripe->bank][0]=canonical->Base+stripe->offset;
   root->table[stripe->bank][1]=canonical->Base+dest.offset;
   assert(manifest[(layer*3+role-2)*7+stripe->bank]==root->table[stripe->bank][1]);
   root->table[stripe->bank][2]=stripe->firstRow;root->table[stripe->bank][3]=stripe->rows;
  }
  assert(PioneerQ38Bf16RootPairs::Build(*root,paired.Inputs(),paired.Outputs()));
  for(unsigned bank=0;bank<7;bank++)for(unsigned field=0;field<4;field++){
   const unsigned saved=root->table[bank][field];root->table[bank][field]^=1;
   unsigned before[119];for(unsigned w=0;w<119;w++)before[w]=root->parts[w][0];
   assert(!PioneerQ38HybridRootPairs::Relocate(*root,layer,paired.Inputs(),paired.Outputs()));
   for(unsigned w=0;w<119;w++)assert(before[w]==root->parts[w][0]);
   root->table[bank][field]=saved;
  }
  for(unsigned w:{0U,59U,118U})for(unsigned which=0;which<4;which++){
   const unsigned saved=which==3?root->rows[w]:root->parts[w][which==0?0:which==1?1:2];
   if(which==3)root->rows[w]^=32;else root->parts[w][which==0?0:which==1?1:2]^=which==0?64:which==1?8:1;
   unsigned before[119];for(unsigned x=0;x<119;x++)before[x]=root->parts[x][0];
   assert(!PioneerQ38HybridRootPairs::Relocate(*root,layer,paired.Inputs(),paired.Outputs()));
   for(unsigned x=0;x<119;x++)assert(before[x]==root->parts[x][0]);
   if(which==3)root->rows[w]=saved;else root->parts[w][which==0?0:which==1?1:2]=saved;
  }
  if(layer==48 && role==2){
   // Geometry is deliberately not a weight-authentication capability: a
   // different aligned in-range destination is still a valid shape. A real
   // owner must bind the table to initialized converted pages separately.
   auto alternate=std::make_unique<RootProbe>(*root);alternate->table[0][1]+=4096;
   assert(PioneerQ38HybridRootPairs::Relocate(*alternate,layer,paired.Inputs(),paired.Outputs()));
   bool differs=false;for(unsigned w=0;w<119;w++)if(alternate->parts[w][1]==0){
    PioneerQ38Bfp8WorkerJob::Job expected={};assert(binding.Build8(paired,layer,role,8191,w,7,expected));
    differs|=alternate->parts[w][0]!=expected.words[3];
   }assert(differs);
  }
  assert(PioneerQ38HybridRootPairs::Relocate(*root,layer,paired.Inputs(),paired.Outputs()));
  const bool eight=mask & (uint64_t(1)<<layer);std::vector<unsigned> coverage(paired.Outputs(),0);
  for(unsigned ordinal=0;ordinal<paired.Count();ordinal++){
   const auto* part=paired.Part(ordinal);assert(part);const auto* stripe=canonical->ProjectionStripe(layer,role,part->stripe);assert(stripe);
   unsigned index=canonical->Count();for(unsigned i=0;i<canonical->Count();i++)if(canonical->Part(i)==stripe){index=i;break;}
   assert(index<relocation->parts.size());const auto& dest=relocation->parts[index];
   const uint64_t bandBytes=uint64_t(paired.Inputs()/32)*(eight?1088:576);
   const uint64_t within=uint64_t((part->firstRow-stripe->firstRow)/32)*bandBytes;
   const uint64_t address=uint64_t(canonical->Base)+dest.offset+within;
   assert(dest.bank==part->bank && dest.encoding==(eight?8U:0U) && within+uint64_t(part->groups/2)*bandBytes<=dest.bytes);
   assert(root->parts[ordinal][0]==address && root->parts[ordinal][1]==part->bank
    && root->parts[ordinal][2]==part->groups && root->rows[ordinal]==part->firstRow);
   if(eight){
    PioneerQ38Bfp8WorkerJob::Job job={};assert(binding.Build8(paired,layer,role,8191,ordinal,7,job));
    assert(binding.Matches8(paired,layer,role,8191,ordinal,7,job) && job.words[3]==address && job.words[4]==part->groups/2);
    CheckWorker<PioneerQ38Bfp8WorkerJob>(job,layer,role,part->firstRow);
    PioneerQ38Bfp4WorkerJob::Job wrong={};assert(!binding.Build4(paired,layer,role,8191,ordinal,7,wrong));
    if(ordinal==0)for(unsigned word=0;word<16;word++){auto bad=job;bad.words[word]^=1;assert(!binding.Matches8(paired,layer,role,8191,ordinal,7,bad));}
    jobs8++;
   }else{
    PioneerQ38Bfp4WorkerJob::Job job={};assert(binding.Build4(paired,layer,role,8191,ordinal,7,job));
    assert(binding.Matches4(paired,layer,role,8191,ordinal,7,job) && job.words[3]==address && job.words[4]==part->groups/2);
    CheckWorker<PioneerQ38Bfp4WorkerJob>(job,layer,role,part->firstRow);
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
 PioneerQ38HybridJobBinding wrongMask;assert(wrongMask.Bind(*canonical,0));
 manifest[0]=123;assert(!wrongMask.RootManifest(manifest,PioneerQ38HybridJobBinding::RootManifestWords) && manifest[0]==123);
 printf("PASS relocated hybrid jobs and experimental QGW5 worker grammar: BFP4 %u/BFP8 %u, %u bands, all layers/119 peers, format, rows and bank bounds; NO DEVICE\n",jobs4,jobs8,bands);
}
#endif
