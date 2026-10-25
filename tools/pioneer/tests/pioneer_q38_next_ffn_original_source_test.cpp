/* SPDX-License-Identifier: MIT */
// Original layer1 FFN byte endpoints only;NO native owner or speed claim.
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#include "pioneer_q38_decoder_original_source.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38CanonicalModelPlan.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38FfnGraphShape.h"
#include <array>
#include <cassert>
#include <cstdio>
int main(int argc,char** argv){
 assert(argc==2);setvbuf(stdout,nullptr,_IOLBF,0);
 auto plan=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(plan->Build());
 auto source=OpenNextFfnResidualOriginalSource(argv[1]);assert(source && !source->NextFfnWeightsReady());
 std::array<uint8_t,4608> data,saved;data.fill(0xa5);saved=data;
 const auto* first=plan->FfnStripe(1,0,0);unsigned route=PioneerQ38SpanRoute(first->bank),address=plan->Base+first->offset;
 assert(!source->Read(route,address,data.data(),2304) && data==saved && !source->NextFfnReads());
 assert(source->Prepare(0) && source->Ready() && source->ReferenceReads()==1673 && !source->NextFfnWeightsReady());
 assert(!source->Read(route,address,data.data(),2304) && data==saved && !source->NextFfnReads());
 assert(source->SealBeforeNativeContext() && source->NextFfnWeightsReady() && source->NextResidualRmsWeightsReady());
 unsigned stripes=0,pages=0,reads=0,refused=0;
 for(unsigned role=0;role<3;role++)for(unsigned stripe=0;stripe<7;stripe++){
  const auto* p=plan->FfnStripe(1,role,stripe);assert(p && p->type==12 && !(p->bytes%2304));
  route=PioneerQ38SpanRoute(p->bank);address=plan->Base+p->offset;
  for(unsigned offset:{0U,p->bytes-2304}){
   assert(source->Read(route,address+offset,data.data(),2304));reads++;data.fill(0xa5);
   auto count=source->NextFfnReads();
   for(unsigned bytes:{0U,64U,256U,576U,2240U,2308U,4096U,4608U}){
    assert(!source->Read(route,address+offset,data.data(),bytes) && data==saved && source->NextFfnReads()==count);refused++;
   }
   assert(!source->Read(route,address+offset+64,data.data(),2304) && !source->Read(129,address+offset,data.data(),2304)
    && !source->Read(route,address+offset,nullptr,2304) && data==saved && source->NextFfnReads()==count);refused+=3;
  }
  stripes++;pages+=p->allocated/4096;
 }
 assert(stripes==21 && pages==36720 && reads==42 && source->NextFfnReads()==reads && !source->FfnReads()
  && !source->NextResidualRmsReads() && !source->NextGdnOutputReads() && !source->NextGdnReads() && !source->NextConvReads()
  && !source->NextProjectionReads() && !source->NextRmsReads() && !source->ResidualRmsReads() && !source->GdnOutputReads()
  && !source->ProjectionReads() && !source->ConvolutionReads() && !source->Prepare(0) && !source->SealBeforeNativeContext());
 printf("PASS ORIGINAL c600/ALL851schema/ALL21 layer1 FFN stripes+36720pages cached BEFORE context/42first-last2304B reads/%u atomic refusals/no CPU products or fallback;NO native/device/speed claim\n",refused);
}
