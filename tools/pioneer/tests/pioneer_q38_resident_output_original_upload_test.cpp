/* SPDX-License-Identifier: MIT */
// Actual authenticated source's physical-client raw upload seam;CPU only.
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#include "../PioneerQ38OriginalSource.h"
#include "../PioneerQwen38PackedPlan.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ProjectionMatrixShape.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>
int main(int argc,char** argv){
 assert(argc==2);setvbuf(stdout,nullptr,_IOLBF,0);
 puts("ORIGINAL_UPLOAD_SAME_FULL_SHA_SOURCE_BEFORE_CONTEXT;NO_ACCELERATOR_FD");
 auto source=OpenFfnResidualOriginalSource(argv[1]);PioneerQwen38PackedPlan plan;
 std::array<uint8_t,4096> page;page.fill(0xa5);const auto untouched=page;
 assert(source && !source->SealedModel() && !source->ReadWeightPage(plan,0,0,page.data(),4096) && page==untouched);
 assert(source->Prepare(0) && !source->SealedModel() && source->SealBeforeNativeContext());
 const auto* model=source->SealedModel();assert(model && source->FfnWeightsReady() && plan.Build(*model,8192,true,true));
 unsigned parts=0,checkedPages=0,embedding=UINT32_MAX,padded=0;
 for(unsigned i=0;i<plan.Parts().size();i++){
  const auto& p=plan.Parts()[i];if(p.kind!=PIONEER_Q38_WEIGHT)continue;
  if(p.layer==64 && !p.role && !p.firstRow){embedding=i;continue;}
  if(p.layer || p.role>13)continue;
  assert(source->ReadWeightPage(plan,i,0,page.data(),4096));checkedPages++;
  if(!p.role)assert(!memcmp(page.data(),source->LearnedNorm(),4096));
  else{
   unsigned bytes=p.role==1?16384:p.role<=4?2304:p.role==12?256:p.role==7 || p.role==8?64:p.role==9?512:576;
   std::vector<uint8_t> cached(bytes);
   assert(source->Read(PioneerQ38ProjectionRoute(p.bank),p.proposedAddress,cached.data(),bytes)
    && !memcmp(page.data(),cached.data(),std::min(4096U,bytes)));
  }
  unsigned last=p.allocated/4096-1;
  assert(source->ReadWeightPage(plan,i,last,page.data(),4096));checkedPages++;
  unsigned valid=p.bytes-last*4096;
  assert(valid && valid<=4096);
  for(unsigned at=valid;at<4096;at++){assert(!page[at]);padded++;}
  if(!p.role)assert(!memcmp(page.data(),reinterpret_cast<const uint8_t*>(source->LearnedNorm())+last*4096,4096));
  if(p.role==1){std::array<uint8_t,4096> cached;
   assert(last==4 && source->Read(PioneerQ38ProjectionRoute(p.bank),p.proposedAddress+last*4096,cached.data(),4096)
    && cached==page);}
  parts++;
 }
 assert(parts==56 && embedding!=UINT32_MAX && checkedPages==112 && padded);
 std::array<uint8_t,49152> embeddingPages;
 for(unsigned at=0;at<12;at++)assert(source->ReadWeightPage(plan,embedding,at,embeddingPages.data()+at*4096,4096));
 assert(!memcmp(embeddingPages.data(),source->EmbeddingTile(),46080));
 page.fill(0xa5);
 assert(!source->ReadWeightPage(plan,UINT32_MAX,0,page.data(),4096) && page==untouched);
 assert(!source->ReadWeightPage(plan,embedding,UINT32_MAX,page.data(),4096) && page==untouched);
 assert(!source->ReadWeightPage(plan,embedding,0,page.data(),4095) && page==untouched);
 assert(!source->ReadWeightPage(plan,embedding,0,nullptr,4096) && source->SealedModel()==model && source->Ready());
 printf("PASS ACTUAL FULL-SHA SAME sealed source:metadata+raw physical-upload pages/ALL56first-layer starts+tails/12embedding pages/cached-original tile+norm equality/%upadding bytes zero/4invalid reads inert;NO CPU numeric products,accelerator FD or silicon claim\n",padded);
}
