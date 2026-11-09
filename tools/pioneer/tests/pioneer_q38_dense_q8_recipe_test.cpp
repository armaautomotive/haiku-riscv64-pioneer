/* SPDX-License-Identifier: MIT */
// Logical scalar-shaped recipes bound to distinct BFP8 physical geometry.
// Synthetic data only; never a device or scalar-weight authorization.
#define PIONEER_Q38_DENSE_Q8_KERNEL_LAYOUT_FIXTURE_ONLY 1
#define PIONEER_Q38_DENSE_Q8_RECIPE_INSPECTION 1
#include "pioneer_q38_dense_q8_kernel_layout_test.cpp"
#include "../PioneerQ38DenseQ8RecipeInspection.h"
#include <array>
#include <vector>
int main(){
 using C=PioneerQ38CanonicalModelPlan;using Fixed=PioneerQ38DenseQ8KernelLayoutInspection;
 auto original=std::make_unique<C>();auto parent=std::make_unique<Fixed::Parent>();auto fixed=std::make_unique<Fixed>();
 auto logical=std::make_unique<C>();auto physical=std::make_unique<C>();
 assert(original->Build() && parent->Build(*original) && fixed->Build(*original,*parent));
 assert(logical->BuildDenseQ8LogicalParentInspection(*parent) && logical->Qgw8InspectionLayout());
 assert(physical->BuildDenseQ8Inspection(*fixed) && physical->DenseQ8InspectionLayout());
 std::array<uint32_t,1920> sources={};for(unsigned i=0;i<sources.size();i++)sources[i]=0x16005+i*131;
 const PioneerQ38GdnIdentity id={1,2,3,42};
 std::vector<uint8_t> ledger(PioneerResidentBanks::Pages,0);
 for(unsigned i=0;i<physical->Count();i++){
  const auto* p=physical->Part(i);assert(p);if(p->kind!=C::WeightKind)continue;
  const unsigned at=p->bank*unsigned(C::Capacity/4096)+p->offset/4096;
  for(unsigned n=0;n<p->allocated/4096;n++)ledger[at+n]=1;
 }
 for(unsigned layer=3;layer<64;layer+=4){
  auto plan=std::make_unique<PioneerQ38DenseQ8RecipeInspection>();
  assert(plan->Build(*logical,*physical,layer,0,248319,0,1000,id,sources.data()) && plan->Matches(*logical,*physical));
  const auto* recipe=plan->Recipe();assert(recipe && recipe->Matches(*logical));
  for(unsigned slot=0;slot<4;slot++)assert(PioneerQ38DenseProjectionRecipe(recipe->Frame(slot),recipe->Header(slot),
   recipe->Banks(),recipe->Parts(slot),recipe->Rows(slot),false));
  assert(plan->PhysicalPagesPresent(ledger.data(),ledger.size()));
  assert(!recipe->LearnedPagesPresent(ledger.data(),ledger.size())); // physical holes block the old scalar path
  const auto* stripe=physical->ProjectionStripe(layer,5,0);assert(stripe);
  const unsigned page=stripe->bank*unsigned(C::Capacity/4096)+stripe->offset/4096;
  ledger[page]=0;assert(!plan->PhysicalPagesPresent(ledger.data(),ledger.size()));ledger[page]=1;
  auto* changed=const_cast<PioneerQ38CanonicalPart*>(stripe);const auto saved=*changed;changed->offset+=4096;
  assert(!plan->Matches(*logical,*physical) && !plan->Recipe() && !plan->PhysicalPagesPresent(ledger.data(),ledger.size()));*changed=saved;
  assert(plan->Matches(*logical,*physical));
 }
 auto wrong=std::make_unique<PioneerQ38DenseQ8RecipeInspection>();
 assert(!wrong->Build(*original,*physical,3,0,248319,0,1000,id,sources.data()) && !wrong->Recipe());
 puts("PASS 16 QGWD logical recipes bound to physical converted extents, hole/refusal checks; NO DEVICE OR SCALAR AUTHORITY");
}
