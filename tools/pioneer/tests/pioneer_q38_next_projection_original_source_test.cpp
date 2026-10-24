/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#include "pioneer_q38_decoder_original_source.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38CanonicalModelPlan.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ProjectionResultLayout.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ProjectionInputShape.h"
#include <cassert>
#include <cstdio>
int main(int argc,char** argv){
 assert(argc==2);setvbuf(stdout,nullptr,_IOLBF,0);auto source=OpenNextProjectionOriginalSource(argv[1]);
 assert(source && !source->NextProjectionWeightsReady() && source->Prepare(0) && source->Ready() && source->ReferenceReads()==1673
  && !source->NextProjectionWeightsReady() && source->SealBeforeNativeContext() && source->NextProjectionWeightsReady());
 auto plan=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(plan->Build());unsigned stripes=0,pages=0,refused=0;
 std::vector<uint8_t> out(16384,0xa5);
 for(unsigned role:{10U,11U,5U,6U})for(unsigned stripe=0;stripe<(role==10 || role==11?1U:7U);stripe++){
  const auto* p=plan->ProjectionStripe(1,role,stripe);assert(p && p->type==8 && p->rowBytes==5760 && p->bytes%576==0);
  unsigned route=PioneerQ38ProjectionRoute(p->bank),address=plan->Base+p->offset;stripes++;pages+=p->allocated/4096;
  for(unsigned at:{0U,p->bytes-576}){
   assert(source->Read(route,address+at,out.data(),576));
   for(unsigned bad:{0U,64U,256U,512U,2304U,4096U,16384U}){
    auto before=out;auto count=source->NextProjectionReads();assert(!source->Read(route,address+at,out.data(),bad) && out==before && source->NextProjectionReads()==count);refused++;
   }
   auto count=source->NextProjectionReads();auto before=out;
   assert(!source->Read(129,address+at,out.data(),576) && !source->Read(route,address+at+64,out.data(),576)
    && !source->Read(route,address+at,nullptr,576) && out==before && source->NextProjectionReads()==count);refused+=3;
  }
 }
 assert(stripes==16 && pages==23176 && source->NextProjectionReads()==32 && refused==320 && !source->NextRmsReads()
  && !source->FfnReads() && !source->ProjectionReads() && !source->GdnOutputReads() && !source->Prepare(0) && !source->SealBeforeNativeContext());
 std::vector<uint32_t> absent;assert(!source->CheckNextProjections(absent,absent));
 puts("PASS ORIGINAL c600de03/ALL851schema/layer1 all4roles/16stripes/23176pages cached BEFORE context/32exact original Q8tiles+320atomic refusals/no other learned reads;NO producer/native/CPU-product grant");
}
