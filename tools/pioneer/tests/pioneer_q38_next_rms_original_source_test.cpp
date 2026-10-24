/* SPDX-License-Identifier: MIT */
// Original layer1 F32 bytes only;no native context,products or GPU access.
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#include "pioneer_q38_decoder_original_source.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38CanonicalModelPlan.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38LearnedRmsShape.h"
#include <cassert>
#include <cstdio>
int main(int argc,char** argv){
 assert(argc==2);setvbuf(stdout,nullptr,_IOLBF,0);
 auto source=OpenNextRmsOriginalSource(argv[1]);assert(source && !source->NextRmsWeightsReady() && source->Prepare(0)
  && source->Ready() && source->ReferenceReads()==1673 && !source->NextRmsReads() && !source->NextRmsWeightsReady()
  && source->SealBeforeNativeContext() && source->NextRmsWeightsReady() && source->FfnWeightsReady());
 auto plan=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(plan->Build());
 unsigned norms=0,refused=0;std::vector<uint8_t> bytes(20480,0xa5);
 for(unsigned i=0;i<plan->Count();i++){const auto& p=*plan->Part(i);if(p.kind!=plan->WeightKind || p.layer!=1 || p.role)continue;
  assert(!p.type && p.bytes==20480 && p.allocated==20480);norms++;
  unsigned route=PioneerQ38ProjectionRoute(p.bank),address=plan->Base+p.offset;
  for(unsigned at:{0U,16384U}){
   unsigned expected=at?4096:16384;assert(source->Read(route,address+at,bytes.data()+at,expected));
   for(unsigned size:{0U,64U,256U,512U,2304U,4096U,16384U,20480U})if(size!=expected){
    auto count=source->NextRmsReads();assert(!source->Read(route,address+at,bytes.data(),size) && source->NextRmsReads()==count);refused++;
   }
   assert(!source->Read(129,address+at,bytes.data(),expected) && !source->Read(route,address+at+64,bytes.data(),expected)
    && !source->Read(route,address+at,nullptr,expected));refused+=3;
  }
 }
 assert(norms==1 && refused==20 && source->NextRmsReads()==2 && !source->FfnReads() && !source->ResidualRmsReads()
  && !source->GdnOutputReads() && !source->Prepare(0) && !source->SealBeforeNativeContext() && source->Ready());
 puts("PASS ORIGINAL c600de03 fullSHA/ALL851tensor schema/layer1 attn_norm5120F32+ALL5pages cached BEFORE context/2exactchunks+20refusals/no other learned reads/no native or reference-product grant");
}
