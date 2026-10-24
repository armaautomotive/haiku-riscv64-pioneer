/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#include "pioneer_q38_decoder_original_source.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38CanonicalModelPlan.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ProjectionInputShape.h"
#include <cassert>
#include <cstdio>
int main(int argc,char** argv){
 assert(argc==2);setvbuf(stdout,nullptr,_IOLBF,0);auto source=OpenNextGdnOriginalSource(argv[1]);
 assert(source && !source->NextGdnWeightsReady() && source->Prepare(0) && source->Ready() && source->ReferenceReads()==1673
  && !source->NextGdnWeightsReady() && source->SealBeforeNativeContext() && source->NextGdnWeightsReady() && source->NextConvWeightsReady());
 auto plan=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(plan->Build());unsigned mask=0,pages=0,tiles=0,refused=0;
 std::vector<uint8_t> out(16384,0xa5);
 for(unsigned i=0;i<plan->Count();i++){
  const auto* p=plan->Part(i);if(p->kind!=plan->WeightKind || p->layer!=1 || p->role<7 || p->role>9)continue;
  unsigned bit=1U<<(p->role-7),tile=p->role==9?512:64;
  assert(!(mask&bit) && !p->type && !p->firstRow && p->rows==1 && p->allocated==4096 && p->bytes==(p->role==9?512U:192U));
  mask|=bit;pages++;unsigned route=PioneerQ38ProjectionRoute(p->bank),address=plan->Base+p->offset;
  for(unsigned at=0;at<p->bytes;at+=tile){
   assert(source->Read(route,address+at,out.data(),tile));tiles++;
   for(unsigned bad:{0U,64U,256U,512U,576U,2304U,4096U,16384U})if(bad!=tile){
    auto before=out;auto count=source->NextGdnReads();assert(!source->Read(route,address+at,out.data(),bad) && out==before && source->NextGdnReads()==count);refused++;
   }
   auto before=out;auto count=source->NextGdnReads();
   assert(!source->Read(129,address+at,out.data(),tile) && !source->Read(route,address+at+1,out.data(),tile)
    && !source->Read(route,address+at,nullptr,tile) && !source->Read(route,address+p->bytes,out.data(),tile)
    && out==before && source->NextGdnReads()==count);refused+=4;
  }
 }
 assert(mask==7 && pages==3 && tiles==7 && source->NextGdnReads()==7 && refused==77 && !source->NextConvReads()
  && !source->NextProjectionReads() && !source->NextRmsReads() && !source->FfnReads() && !source->ProjectionReads()
  && !source->GdnInputReads() && !source->GdnNormReads() && !source->ConvolutionReads() && !source->Prepare(0) && !source->SealBeforeNativeContext());
 std::vector<uint32_t> absent;assert(!source->CheckNextGdn(0,absent,nullptr,nullptr,nullptr,nullptr));
 puts("PASS ORIGINAL c600/ALL851schema/layer1 F32 A+dt+norm3pages cached BEFORE context/7exacttiles+77atomic refusals/no other learned reads;NO native/producer/CPU-product grant");
}
