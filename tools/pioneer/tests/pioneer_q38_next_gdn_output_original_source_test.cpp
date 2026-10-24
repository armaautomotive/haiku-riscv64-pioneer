/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#include "pioneer_q38_decoder_original_source.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38NextGdnOutputLearned.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ProjectionInputShape.h"
#include <cassert>
#include <cstdio>
int main(int argc,char** argv){
 assert(argc==2);setvbuf(stdout,nullptr,_IOLBF,0);auto source=OpenNextGdnOutputOriginalSource(argv[1]);
 assert(source && !source->NextGdnOutputWeightsReady() && source->Prepare(0) && source->Ready() && source->ReferenceReads()==1673
  && !source->NextGdnOutputWeightsReady() && source->SealBeforeNativeContext() && source->NextGdnOutputWeightsReady() && source->NextGdnWeightsReady());
 auto plan=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(plan->Build());unsigned stripes=0,pages=0,rows=0,tiles=0,refused=0,pageFaults=0;
 std::vector<uint8_t> ledger(PioneerResidentBanks::Pages,0),out(16384,0xa5);
 assert(!PioneerQ38NextGdnOutputLearned(*plan,ledger.data(),unsigned(ledger.size())));
 for(unsigned i=0;i<plan->Count();i++){
  const auto* p=plan->Part(i);if(p->kind!=plan->WeightKind || p->layer!=1 || p->role!=13)continue;
  assert(p->type==8 && p->firstRow==rows && p->rowBytes==6912 && p->rows && p->allocated%4096==0);
  stripes++;rows+=p->rows;pages+=p->allocated/4096;
  unsigned first=p->bank*786432+p->offset/4096;for(unsigned page=0;page<p->allocated/4096;page++)ledger[first+page]=1;
  for(unsigned at:{0U,p->bytes-576}){
   unsigned route=PioneerQ38ProjectionRoute(p->bank),address=plan->Base+p->offset+at;
   assert(source->Read(route,address,out.data(),576));tiles++;
   for(unsigned bad:{0U,64U,256U,512U,2304U,4096U,16384U}){
    auto before=out;auto count=source->NextGdnOutputReads();assert(!source->Read(route,address,out.data(),bad)
     && out==before && source->NextGdnOutputReads()==count);refused++;
   }
   auto before=out;auto count=source->NextGdnOutputReads();assert(!source->Read(129,address,out.data(),576)
    && !source->Read(route,address+1,out.data(),576) && !source->Read(route,address,nullptr,576)
    && !source->Read(route,plan->Base+p->offset+p->bytes,out.data(),576) && out==before && source->NextGdnOutputReads()==count);refused+=4;
  }
 }
 assert(stripes==7 && rows==5120 && pages==8640 && tiles==14 && refused==154
  && PioneerQ38NextGdnOutputLearned(*plan,ledger.data(),unsigned(ledger.size())));
 for(unsigned i=0;i<plan->Count();i++){
  const auto* p=plan->Part(i);if(p->kind!=plan->WeightKind || p->layer!=1 || p->role!=13)continue;
  unsigned first=p->bank*786432+p->offset/4096,count=p->allocated/4096;
  for(unsigned page:{0U,count/2,count-1})for(uint8_t bad:{uint8_t(0),uint8_t(2)}){
   ledger[first+page]=bad;assert(!PioneerQ38NextGdnOutputLearned(*plan,ledger.data(),unsigned(ledger.size())));ledger[first+page]=1;pageFaults++;
  }
 }
 assert(pageFaults==42 && !PioneerQ38NextGdnOutputLearned(*plan,nullptr,unsigned(ledger.size()))
  && !PioneerQ38NextGdnOutputLearned(*plan,ledger.data(),unsigned(ledger.size()-1)) && source->NextGdnOutputReads()==14
  && !source->NextGdnReads() && !source->NextConvReads() && !source->NextProjectionReads() && !source->NextRmsReads() && !source->FfnReads()
  && !source->GdnOutputReads() && !source->ProjectionReads() && !source->ConvolutionReads() && !source->Prepare(0) && !source->SealBeforeNativeContext());
 std::vector<uint32_t> absent;assert(!source->CheckNextGdnOutput(absent,absent));
 puts("PASS ORIGINAL c600/ALL851schema/layer1 Q8_0 ssm_out7stripes8640pages BEFORE context/14edge tiles+154atomic refusals/42missing-corrupt page gates/no other learned reads;NO_NATIVE_PRODUCER_NUMERIC_OR_SPEED_GRANT");
}
