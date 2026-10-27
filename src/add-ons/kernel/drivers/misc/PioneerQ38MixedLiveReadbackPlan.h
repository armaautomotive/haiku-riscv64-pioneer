/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_MIXED_LIVE_READBACK_PLAN_H
#define PIONEER_Q38_MIXED_LIVE_READBACK_PLAN_H
#include "../../../../../tools/pioneer/PioneerQ38MixedLiveBlockPlan.h"
#include "../../../../../tools/pioneer/PioneerQ38MixedLiveReadback.h"
// Fixed standalone layer3/position0 FINAL observation DATA. No arbitrary
// coordinates/routes/count,expected products,producer or native authority.
struct PioneerQ38MixedLiveReadbackPage {
 uint32_t bank=7,address=0,route=0,firstWord=0,words=0;
};
class PioneerQ38MixedLiveReadbackPlan {
 const PioneerQ38CanonicalModelPlan* canonical=nullptr;
 const PioneerQ38MixedLiveBlockPlan* recipe=nullptr;
 bool attempted=false,ready=false;
 bool Shape()const{
  namespace L=PioneerQ38MixedLiveBlockRootLayout;
  if(!canonical || !recipe || !recipe->Matches(*canonical))return false;
  const auto* o=recipe->Outer();
  if(!o || !L::RunShape(o) || o[3]!=3 || o[4] || o[7]>=7 || o[8]!=PioneerQ38ProjectionRoute(o[7]) || o[54]!=67108864)return false;
  const auto* workspace=canonical->Workspace(o[7]);
  return workspace && uint64_t(canonical->Base)+workspace->offset==o[9]
   && !(o[9]&4095) && L::FinalOffset<=67108864-TT_Q38_MIXED_LIVE_READBACK_WORDS*4
   && uint64_t(o[9])+L::FinalOffset+TT_Q38_MIXED_LIVE_READBACK_WORDS*4<=uint64_t(canonical->Base)+canonical->Capacity;
 }
public:
 enum {Pages=TT_Q38_MIXED_LIVE_READBACK_PAGES,Words=TT_Q38_MIXED_LIVE_READBACK_WORDS,Scratch=0x80000};
 bool Build(const PioneerQ38CanonicalModelPlan& c,const PioneerQ38MixedLiveBlockPlan& operation){
  if(attempted)return false;
  attempted=true;canonical=&c;recipe=&operation;ready=Shape();return ready;
 }
 unsigned PageCount()const{return ready?unsigned(Pages):0;}
 bool Page(unsigned index,PioneerQ38MixedLiveReadbackPage& out)const{
  namespace L=PioneerQ38MixedLiveBlockRootLayout;
  if(!ready || index>=Pages || !Shape())return false;
  const auto* o=recipe->Outer();PioneerQ38MixedLiveReadbackPage p;
  p.bank=o[7];p.route=o[8];p.firstWord=index*1024;p.words=1024;
  const uint64_t address=uint64_t(o[9])+L::FinalOffset+index*4096;
  if(address>UINT32_MAX-4095 || address<canonical->Base || address+4096>uint64_t(canonical->Base)+canonical->Capacity || (address&4095))return false;
  p.address=uint32_t(address);out=p;return true; // refusal NEVER changes caller output
 }
};
static_assert(PioneerQ38MixedLiveReadbackPlan::Pages*1024==PioneerQ38MixedLiveReadbackPlan::Words,"ONLY five FINAL pages");
#endif
