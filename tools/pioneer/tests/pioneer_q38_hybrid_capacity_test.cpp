/* SPDX-License-Identifier: MIT */
// Capacity arithmetic on the existing canonical DATA plan only. This does
// not allocate GDDR, provide an address plan, or authorize a device owner.
#define PIONEER_Q38_GDN_MUTABLE_STATE
#define PIONEER_BANKS_MOCK_ONLY
#include "pioneer_resident_banks_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38CanonicalModelPlan.h"
#include <memory>
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
}
