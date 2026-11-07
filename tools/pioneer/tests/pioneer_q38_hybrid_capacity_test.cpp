/* SPDX-License-Identifier: MIT */
// Capacity arithmetic on the existing canonical DATA plan only. This does
// not allocate GDDR, provide an address plan, or authorize a device owner.
#define PIONEER_Q38_GDN_MUTABLE_STATE
#define PIONEER_BANKS_MOCK_ONLY
#include "pioneer_resident_banks_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38CanonicalModelPlan.h"
#include "../PioneerQ38HybridModelPlan.h"
#include <memory>
struct MutatedPlan {
 using C=PioneerQ38CanonicalModelPlan;
 static constexpr uint64_t Capacity=C::Capacity;static constexpr unsigned WeightKind=C::WeightKind;
 const C& source;unsigned target,fault;mutable PioneerQ38CanonicalPart copy={};
 bool Ready()const{return source.Ready();}unsigned Count()const{return source.Count();}uint64_t Used(unsigned b)const{return source.Used(b);}
 const PioneerQ38CanonicalPart* Part(unsigned i)const{
  const auto* p=source.Part(i);if(!p)return nullptr;copy=*p;
  if(i==target){
   if(fault==0)copy.offset+=4096;if(fault==1)copy.bytes=0;if(fault==2)copy.rows++;
   if(fault==3)copy.role=0;if(fault==4)copy.bank=7;if(fault==5)copy.type=8;
   if(fault==6)copy.allocated=0;if(fault==7)copy.firstRow++;if(fault==8)copy.allocated=UINT32_MAX;
  }
  return &copy;
 }
};
int main(){
 auto plan=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(plan->Build());
 for(unsigned lastLayers:{8U,12U,13U,14U,16U,64U}){
  uint64_t extra[7]={},base=0,totalExtra=0;unsigned stripes=0,unaligned=0;
  for(unsigned layer=64-lastLayers;layer<64;layer++)for(unsigned role=0;role<3;role++){
   const uint64_t width=role==2?17408:5120;
   for(unsigned stripe=0;stripe<7;stripe++){
    const auto* p=plan->FfnStripe(layer,role,stripe);
    assert(p && p->kind==PioneerQ38CanonicalModelPlan::WeightKind && p->type==12 && p->bank<7);
    assert(uint64_t(p->bytes)==uint64_t(p->rows)*width*9/16 && !(width%32));
    unaligned+=unsigned((p->firstRow|p->rows)&31)!=0;
    const uint64_t bfp8=(uint64_t(p->rows)*width*17/16+4095)&~uint64_t(4095);
    assert(bfp8>=p->allocated);extra[p->bank]+=bfp8-p->allocated;stripes++;
   }
  }
  bool fits=true;for(unsigned b=0;b<7;b++){
   const uint64_t used=plan->Used(b),required=used+extra[b];base+=used;totalExtra+=extra[b];
   fits &= required<=PioneerQ38CanonicalModelPlan::Capacity;
   printf("BANK_CAPACITY_CANDIDATE last%u bank%u baseline%llu extra%llu required%llu limit%llu\n",lastLayers,b,
    (unsigned long long)used,(unsigned long long)extra[b],(unsigned long long)required,(unsigned long long)PioneerQ38CanonicalModelPlan::Capacity);
  }
  assert(stripes==lastLayers*3*7 && !unaligned && base==UINT64_C(20714528768));
  assert(totalExtra==uint64_t(lastLayers)*UINT64_C(133693440));
  const uint64_t totalCapacity=PioneerQ38CanonicalModelPlan::Capacity*7,required=base+totalExtra;
  assert((lastLayers<=13 && fits && required<=totalCapacity) || (lastLayers>=14 && !fits && required>totalCapacity));
  printf("BANK_CAPACITY_SUMMARY last%u stripes%u not_32_aligned%u base%llu extra%llu required%llu limit%llu aggregate_deficit%llu same_bank_lower_bound_fits%u; DATA_ONLY_NO_ALLOCATOR_OR_DEVICE_PROOF\n",
   lastLayers,stripes,unaligned,(unsigned long long)base,(unsigned long long)totalExtra,(unsigned long long)required,
   (unsigned long long)totalCapacity,(unsigned long long)(required>totalCapacity?required-totalCapacity:0),unsigned(fits));
 }
 using Hybrid=PioneerQ38HybridModelPlan;Hybrid::Proposal proposal;
 for(uint64_t mask:{UINT64_C(0),UINT64_C(0x8000000000000000),UINT64_C(0xfff8000000000000)}){
  assert(Hybrid::Build(*plan,mask,proposal) && proposal.bfp8Layers==mask && proposal.parts.size()==plan->Count());
  uint64_t end[7]={};unsigned selected=0;
  for(unsigned i=0;i<proposal.parts.size();i++){
   const auto& p=proposal.parts[i];const auto* old=plan->Part(i);assert(old && p.original==i && p.bank==old->bank && p.offset==end[p.bank]);
   const bool chosen=old->kind==PioneerQ38CanonicalModelPlan::WeightKind && old->type==12 && old->layer<64
    && old->role>=2 && old->role<=4 && (mask & (uint64_t(1)<<old->layer));
   assert(p.encoding==(chosen?8U:0U) && !(p.allocated&4095) && p.bytes<=p.allocated);
   if(chosen)selected++;else assert(p.bytes==old->bytes && p.allocated==old->allocated);
   if(!mask)assert(p.offset==old->offset);
   end[p.bank]+=p.allocated;assert(end[p.bank]<=PioneerQ38CanonicalModelPlan::Capacity);
  }
  const unsigned expected=mask==0?0:mask==UINT64_C(0x8000000000000000)?21:273;assert(selected==expected);
  for(unsigned b=0;b<7;b++)assert(end[b]==proposal.used[b]);
  printf("HYBRID_REPACK_PROPOSAL mask%016llx parts%zu converted_stripes%u; DATA_ONLY_NO_UPLOAD_OR_OWNER\n",
   (unsigned long long)mask,proposal.parts.size(),selected);
 }
 const auto saved=proposal;assert(!Hybrid::Build(*plan,UINT64_C(0xfffc000000000000),proposal));
 assert(proposal.bfp8Layers==saved.bfp8Layers && proposal.used==saved.used && proposal.parts.size()==saved.parts.size());
 auto empty=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(!Hybrid::Build(*empty,1,proposal));
 assert(proposal.bfp8Layers==saved.bfp8Layers && proposal.used==saved.used && proposal.parts.size()==saved.parts.size());
 unsigned target=plan->Count();for(unsigned i=0;i<plan->Count();i++){
  const auto* p=plan->Part(i);if(p && p->kind==PioneerQ38CanonicalModelPlan::WeightKind && p->layer==51 && p->role==2){target=i;break;}
 }
 assert(target<plan->Count());
 for(unsigned fault=0;fault<9;fault++){
  MutatedPlan changed{*plan,target,fault};assert(!Hybrid::Build(changed,UINT64_C(0xfff8000000000000),proposal));
  assert(proposal.bfp8Layers==saved.bfp8Layers && proposal.used==saved.used && proposal.parts.size()==saved.parts.size());
 }
 for(unsigned i=0;i<saved.parts.size();i++){
  const auto& a=saved.parts[i];const auto& b=proposal.parts[i];
  assert(a.original==b.original && a.bank==b.bank && a.offset==b.offset && a.bytes==b.bytes && a.allocated==b.allocated && a.encoding==b.encoding);
 }
 puts("PASS hybrid repack: original+one+last13 complete/no overlaps/preserved other bytes; last14+unready+9 malformed parts refuse atomically; NO DEVICE");
}
