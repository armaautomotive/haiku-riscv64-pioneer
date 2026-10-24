/* SPDX-License-Identifier: MIT */
// Authentic ORIGINAL FFN bytes BEFORE any native context/instruction starts.
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#include "pioneer_q38_decoder_original_source.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38CanonicalModelPlan.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38FfnGraphShape.h"
#include <array>
#include <cassert>
#include <cstdio>
int main(int argc,char** argv){
 assert(argc==2);setvbuf(stdout,nullptr,_IOLBF,0);
 auto source=OpenFfnResidualOriginalSource(argv[1]);assert(source && !source->FfnWeightsReady() && source->Prepare(0)
  && source->Ready() && source->ReferenceReads()==1673 && !source->FfnReads() && !source->FfnWeightsReady()
  && source->SealBeforeNativeContext() && source->FfnWeightsReady() && source->ResidualRmsWeightsReady());
 auto plan=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(plan->Build());
 unsigned stripes=0,pages=0,reads=0,refused=0;std::array<uint8_t,2304> bytes={};
 for(unsigned role=0;role<3;role++)for(unsigned stripe=0;stripe<7;stripe++){
  const auto& p=*plan->FfnStripe(0,role,stripe);assert(p.type==12 && p.bytes>=2304 && !(p.bytes%2304));
  unsigned route=PioneerQ38SpanRoute(p.bank),address=plan->Base+p.offset;
  for(unsigned offset:{0U,p.bytes-2304}){
   assert(source->Read(route,address+offset,bytes.data(),2304));reads++;
   auto n=source->FfnReads();assert(!source->Read(route,address+offset+64,bytes.data(),2304)
    && !source->Read(route,address+offset,bytes.data(),2240) && !source->Read(129,address+offset,bytes.data(),2304)
    && source->FfnReads()==n);refused+=3;
  }
  stripes++;pages+=p.allocated/4096;
 }
 assert(stripes==21 && pages==36720 && reads==42 && source->FfnReads()==reads && !source->ProjectionReads()
  && !source->ConvolutionReads() && !source->GdnOutputReads() && !source->ResidualRmsReads()
  && !source->SealBeforeNativeContext() && !source->Prepare(0) && source->Ready());
 printf("PASS ORIGINAL fullSAME c600de03 SHA/all851tensor schema/ALL21FFNstripes+36720pages cached BEFORE context/42first-last2304B reads/%urefusals/no reference-product input or synthetic fallback;NO native/device/speed claim\n",refused);
}
