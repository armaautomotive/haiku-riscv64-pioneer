/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_DENSE_Q8_RECIPE_INSPECTION_H
#define PIONEER_Q38_DENSE_Q8_RECIPE_INSPECTION_H
#if !defined(PIONEER_Q38_DENSE_Q8_RECIPE_INSPECTION) || PIONEER_Q38_DENSE_Q8_RECIPE_INSPECTION != 1 \
 || !defined(PIONEER_Q38_DENSE_Q8_CANONICAL_INSPECTION) || PIONEER_Q38_DENSE_Q8_CANONICAL_INSPECTION != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) || defined(PIONEER_Q38_FULL_OFFLOAD_CLIENT) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error Dense Q8 recipe is isolated inspection only
#endif
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38DenseProjectionPlan.h"

// The existing coordinator checks an original scalar-Q8-shaped recipe even
// when QGWD actually schedules BFP8 tiles. Keep that logical recipe separate
// from the physical canonical layout and bind BOTH to identical reserved
// addresses. This does not authorize a scalar read of converted weights,
// authenticate source bytes, stage device memory or start inference.
class PioneerQ38DenseQ8RecipeInspection {
 using C=PioneerQ38CanonicalModelPlan;
 const C* logical=nullptr;const C* physical=nullptr;PioneerQ38DenseProjectionPlan recipe;
 bool attempted=false,ready=false;
 static bool SameLayouts(const C& l,const C& p){
  if(!l.Qgw8InspectionLayout() || !p.DenseQ8InspectionLayout() || l.Count()!=4678 || p.Count()!=l.Count())return false;
  unsigned converted=0;
  for(unsigned i=0;i<l.Count();i++){
   const auto* a=l.Part(i);const auto* b=p.Part(i);if(!a || !b || a->kind!=b->kind || a->layer!=b->layer || a->role!=b->role
    || a->type!=b->type || a->bank!=b->bank || a->offset!=b->offset || a->firstRow!=b->firstRow
    || a->rows!=b->rows || a->rowBytes!=b->rowBytes)return false;
   const bool dense=a->kind==C::WeightKind && a->layer<64 && a->layer%4==3 && a->role>=5 && a->role<=7;
   if(dense){
    const uint64_t n=uint64_t(a->rows)*5440;
    if(a->type!=8 || a->rowBytes!=5760 || uint64_t(a->rows)*5760!=a->bytes || b->bytes!=n
     || b->allocated!=((n+4095)&~UINT64_C(4095)) || b->allocated>a->allocated)return false;
    converted++;
   }else if(a->bytes!=b->bytes || a->allocated!=b->allocated)return false;
  }
  if(converted!=336)return false;
  for(unsigned b=0;b<7;b++)if(l.Used(b)!=p.Used(b))return false;
  return true;
 }
public:
 PioneerQ38DenseQ8RecipeInspection()=default;
 PioneerQ38DenseQ8RecipeInspection(const PioneerQ38DenseQ8RecipeInspection&)=delete;
 PioneerQ38DenseQ8RecipeInspection& operator=(const PioneerQ38DenseQ8RecipeInspection&)=delete;
 bool Build(const C& logicalParent,const C& actual,unsigned layer,unsigned position,unsigned token,unsigned bank,
  unsigned phase,const PioneerQ38GdnIdentity& id,const uint32_t* sources){
  if(attempted)return false;attempted=true;
  if(!SameLayouts(logicalParent,actual) || !recipe.Build(logicalParent,layer,position,token,bank,phase,id,sources))return false;
  logical=&logicalParent;physical=&actual;ready=true;
  if(!Matches(logicalParent,actual)){ready=false;return false;}
  return true;
 }
 bool Matches(const C& l,const C& p)const{return ready && logical==&l && physical==&p && SameLayouts(l,p) && recipe.Matches(l);}
 bool PhysicalPagesPresent(const uint8_t* pages,unsigned count)const{
  if(!logical || !physical || !Matches(*logical,*physical))return false;
  const auto* f=recipe.Frame(0);if(!f)return false;
  const unsigned roles[]={5,6,7,10};
  for(unsigned i=0;i<4;i++)if(!physical->ProjectionPagesPresent(f[42],roles[i],pages,count))return false;
  return true; // presence only, NOT format binding or scalar authority
 }
 const PioneerQ38DenseProjectionPlan* Recipe()const{
  return logical && physical && Matches(*logical,*physical)?&recipe:nullptr;
 }
};
#endif
