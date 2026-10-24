/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#include "pioneer_q38_decoder_original_source.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38CanonicalModelPlan.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ProjectionInputShape.h"
#include <cassert>
#include <cstdio>
int main(int argc,char** argv){
 assert(argc==2);setvbuf(stdout,nullptr,_IOLBF,0);auto source=OpenNextConvOriginalSource(argv[1]);
 assert(source && !source->NextConvWeightsReady() && source->Prepare(0) && source->Ready() && source->ReferenceReads()==1673
  && !source->NextConvWeightsReady() && source->SealBeforeNativeContext() && source->NextConvWeightsReady());
 auto plan=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(plan->Build());unsigned stripes=0,pages=0,rows=0,refused=0;
 std::vector<uint8_t> out(16384,0xa5);
 for(unsigned i=0;i<plan->Count();i++){
  const auto* p=plan->Part(i);if(p->kind!=plan->WeightKind || p->layer!=1 || p->role!=12)continue;
  assert(!p->type && p->rowBytes==16 && p->firstRow==rows && p->bytes%256==0);
  unsigned route=PioneerQ38ProjectionRoute(p->bank),address=plan->Base+p->offset;stripes++;pages+=p->allocated/4096;rows+=p->rows;
  for(unsigned at:{0U,p->bytes-256}){
   assert(source->Read(route,address+at,out.data(),256));
   for(unsigned bad:{0U,64U,512U,576U,2304U,4096U,16384U}){
    auto before=out;auto count=source->NextConvReads();assert(!source->Read(route,address+at,out.data(),bad) && out==before && source->NextConvReads()==count);refused++;
   }
   auto before=out;auto count=source->NextConvReads();
   assert(!source->Read(129,address+at,out.data(),256) && !source->Read(route,address+at+64,out.data(),256)
    && !source->Read(route,address+at,nullptr,256) && out==before && source->NextConvReads()==count);refused+=3;
  }
 }
 assert(stripes==7 && pages==42 && rows==10240 && source->NextConvReads()==14 && refused==140 && !source->NextProjectionReads()
  && !source->NextRmsReads() && !source->FfnReads() && !source->ProjectionReads() && !source->GdnOutputReads()
  && !source->ConvolutionReads() && !source->Prepare(0) && !source->SealBeforeNativeContext());
 std::vector<uint32_t> absent;assert(!source->CheckNextConv(absent,absent));
 puts("PASS ORIGINAL c600/ALL851schema/layer1 F32conv7stripes42pages10240channels cached BEFORE context/14exacttiles+140atomic refusals/no other learned reads;NO native/producer/CPU-product grant");
}
