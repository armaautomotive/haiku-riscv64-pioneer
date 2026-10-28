/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_RECURRENT_PREFIX_READBACK_PLAN_H
#define PIONEER_Q38_RECURRENT_PREFIX_READBACK_PLAN_H
#include <stddef.h>
#include "../../../../../tools/pioneer/PioneerQ38RecurrentPrefixPlan.h"
#include "../../../../../tools/pioneer/PioneerQ38RecurrentPrefixReadback.h"
struct PioneerQ38RecurrentPrefixReadbackPage {
 uint32_t bank=7,address=0,route=0,firstWord=0,words=0;
};
// Immutable LAST layer2/position0 FINAL geometry, NOT an old layer0 plan.
// No caller address/count,completed producer,STOP,hold or release authority.
class PioneerQ38RecurrentPrefixReadbackPlan {
 const PioneerQ38CanonicalModelPlan* canonical=nullptr;const PioneerQ38RecurrentPrefixPlan* prefix=nullptr;
 bool attempted=false,ready=false;
 bool Shape()const{
  namespace L=PioneerQ38MixedRecurrentBlockRootLayout;
  if(!canonical || !prefix || !prefix->Matches(*canonical))return false;
  const auto* last=prefix->Layer(2);if(!last)return false;const auto* o=last->Outer();
  if(!o || !L::OuterShape(o) || o[3]!=2 || o[4] || o[12] || o[48]!=3 || o[7]>=7
   || o[7]!=prefix->Bank() || o[8]!=PioneerQ38ProjectionRoute(o[7]) || o[9]!=prefix->Workspace())return false;
  const auto* region=canonical->Workspace(o[7]);
  return region && region->bytes==67108864 && uint64_t(canonical->Base)+region->offset==o[9] && !(o[9]&4095)
   && L::FinalOffset<=67108864-TT_Q38_RECURRENT_PREFIX_READBACK_WORDS*4
   && uint64_t(o[9])+L::FinalOffset+TT_Q38_RECURRENT_PREFIX_READBACK_WORDS*4<=uint64_t(canonical->Base)+canonical->Capacity;
 }
 bool Output(const void* value,unsigned bytes)const{
  if(!value || !bytes)return false;
  const uintptr_t a=reinterpret_cast<uintptr_t>(value);
  if(a>UINTPTR_MAX-bytes)return false;
  const void* objects[]={this,canonical,prefix};const size_t sizes[]={sizeof(*this),sizeof(*canonical),sizeof(*prefix)};
  for(unsigned i=0;i<3;i++){
   const uintptr_t b=reinterpret_cast<uintptr_t>(objects[i]);
   if(objects[i] && (b>UINTPTR_MAX-sizes[i] || (a<b+sizes[i] && b<a+bytes)))return false;
  }
  return true;
 }
public:
 enum {Pages=TT_Q38_RECURRENT_PREFIX_READBACK_PAGES,Words=TT_Q38_RECURRENT_PREFIX_READBACK_WORDS,Scratch=0x80000};
 bool Build(const PioneerQ38CanonicalModelPlan& c,const PioneerQ38RecurrentPrefixPlan& operation){
  if(attempted)return false;
  attempted=true;canonical=&c;prefix=&operation;ready=Shape();return ready;
 }
 unsigned PageCount()const{return ready?unsigned(Pages):0;}
 bool Page(unsigned index,PioneerQ38RecurrentPrefixReadbackPage& out)const{
  namespace L=PioneerQ38MixedRecurrentBlockRootLayout;
  if(!ready || index>=Pages || !Output(&out,sizeof(out)) || !Shape())return false;
  const auto* o=prefix->Layer(2)->Outer();PioneerQ38RecurrentPrefixReadbackPage p;
  p.bank=o[7];p.route=o[8];p.firstWord=index*1024;p.words=1024;
  const uint64_t address=uint64_t(o[9])+L::FinalOffset+index*4096;
  if(address>UINT32_MAX-4095 || address<canonical->Base || address+4096>uint64_t(canonical->Base)+canonical->Capacity || (address&4095))return false;
  p.address=uint32_t(address);out=p;return true;
 }
};
static_assert(PioneerQ38RecurrentPrefixReadbackPlan::Pages*1024==PioneerQ38RecurrentPrefixReadbackPlan::Words,"ONLY five LAST-prefix FINAL pages");
#endif
