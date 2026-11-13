/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_SMALL_RECURRENT_BFP8_PLAN_INSPECTION 1
#define PIONEER_Q38_RECURRENT_Q8_PREPARED_FIXTURE_ONLY 1
#include "pioneer_q38_recurrent_q8_prepared_pages_test.cpp"
#include "../PioneerQ38HybridSmallRecurrentPreparedPages.h"
#include "../PioneerQ38SmallRecurrentUploadPlan.h"

struct FakeSmallBfp8:FakeBfp8 {
 unsigned failPage=UINT32_MAX;
 bool ReadPaddedSmallPages(unsigned tensor,unsigned page,unsigned count,uint8_t* out,unsigned capacity,std::string&){
  if(!source || !live || !source->live || tensor>=source->tensors.size() || source->tensors[tensor].outputs!=48
   || !count || count>16 || page>=85 || count>85-page || capacity<count*4096 || page==failPage)return false;
  reads+=count;std::memset(out,0x8c,count*4096);return true;
 }
};
using SmallPages=PioneerQ38HybridSmallRecurrentPreparedPages<FakeSource,FakePlan,FakeSmallBfp8,RecurrentPages>;
using SmallPlan=PioneerQ38HybridSmallRecurrentBfp8Plan;
int main(){
 auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(canonical->Build());
 FakeSource source;FakePlan packed;packed.source=&source;std::map<std::pair<unsigned,unsigned>,unsigned> ids;
 for(unsigned ordinal=0;ordinal<=64;ordinal++){
  const unsigned layer=ordinal?ordinal-1:64;
  for(unsigned role=0;;role++){PioneerQ38Tensor t;if(!PioneerQ38Expected(layer,role,t))break;
   ids[{layer,role}]=unsigned(source.tensors.size());source.tensors.push_back(t);}
 }
 unsigned small=UINT32_MAX,large=UINT32_MAX,raw=UINT32_MAX,state=UINT32_MAX;
 for(unsigned i=0;i<canonical->Count();i++){
  const auto* p=canonical->Part(i);assert(p);const unsigned tensor=p->kind==canonical->WeightKind?ids.at({p->layer,p->role}):851;
  packed.parts.push_back({p->kind,p->layer,p->role,tensor,p->bank,p->firstRow,p->rows,p->rowBytes,
   canonical->Base+p->offset,p->bytes,p->allocated});
  if(p->kind==canonical->WeightKind && p->layer==0 && !p->firstRow){if(p->role==10)small=i;else if(p->role==5)large=i;}
  if(p->kind==canonical->WeightKind && p->layer==64 && p->role==0 && !p->firstRow)raw=i;
  if(p->kind==canonical->WorkspaceKind && state==UINT32_MAX)state=i;
 }
 for(unsigned index:{small,large,raw,state})assert(index!=UINT32_MAX);
 const auto sha=PioneerModelWeightCache::Hash("authenticated-fixture",21);
 const auto four=PioneerModelWeightCache::Hash("four-fingerprint",16),headHash=PioneerModelWeightCache::Hash("head-fingerprint",16);
 FakeBfp8 parentEight{&source},recurrentEight{&source};parentEight.digest=recurrentEight.digest=sha;
 FakeSmallBfp8 smallEight;smallEight.source=&source;smallEight.digest=sha;std::string error;bool permit=true;
 HeadPages head;assert(head.Bind(source,packed,*canonical,parentEight,sha,four,headHash,"/private/tmp/four","/private/tmp/head",[&]{return permit;},error));
 assert(head.VerifyAll(error));DensePages dense;assert(dense.Bind(source,packed,*canonical,parentEight,head,sha,error) && dense.VerifyAll(error));
 RecurrentPages recurrent;assert(recurrent.Bind(source,packed,*canonical,recurrentEight,dense,sha,error));
 SmallPages early;assert(!early.Bind(source,packed,*canonical,smallEight,recurrent,sha,error));
 assert(recurrent.VerifyAll(error) && recurrent.AllVerified());SmallPages pages;
 assert(pages.Bind(source,packed,*canonical,smallEight,recurrent,sha,error) && pages.Ready());
 assert(!pages.Bind(source,packed,*canonical,smallEight,recurrent,sha,error));
 assert(pages.Part(small)->encoding==SmallPlan::SmallRecurrentBfp8 && pages.Part(small)->allocated==348160);
 assert(pages.Part(large)->encoding==RecurrentPlan::RecurrentQ8Bfp8);
 std::array<uint8_t,8192> out{};
 assert(pages.ReadPages(small,83,2,out.data(),out.size(),error) && out[0]==0x8c && out[4096]==0x8c);
 assert(pages.ReadPage(large,0,out.data(),out.size(),error) && out[0]==0x8b);
 assert(pages.ReadPage(raw,0,out.data(),out.size(),error) && out[0]==0x5a);
 const auto before=out;
 assert(!pages.ReadPages(small,84,2,out.data(),out.size(),error) && out==before && pages.Ready());
 assert(!pages.ReadPage(state,0,out.data(),out.size(),error) && out==before && pages.Ready());
 assert(!pages.ReadPage(small,0,reinterpret_cast<uint8_t*>(&smallEight),4096,error) && pages.Ready());
 const unsigned previous=smallEight.reads;assert(pages.VerifyAll(error) && pages.AllVerified() && smallEight.reads==previous+96);
 PioneerQ38SmallRecurrentUploadPlan<SmallPages> staged;assert(staged.Bind(*canonical,pages,error));
 std::vector<uint8_t> uploadBytes(256*4096,0);unsigned f=0,b=0,n=0;uint64_t address=0;
 assert(staged.Stage(uploadBytes.data(),uploadBytes.size(),f,b,address,n,error) && n && !(n&4095));
 assert(staged.Acknowledge(true));
 packed.parts[raw].tensor++;assert(!pages.AllVerified());packed.parts[raw].tensor--;assert(pages.AllVerified());
 SmallPages failed;assert(failed.Bind(source,packed,*canonical,smallEight,recurrent,sha,error));
 smallEight.failPage=1;out.fill(0x71);const auto untouched=out;
 assert(!failed.ReadPages(small,1,1,out.data(),out.size(),error) && out==untouched && !failed.Ready());
 smallEight.failPage=UINT32_MAX;assert(!failed.Ready());
 SmallPages revoked;assert(revoked.Bind(source,packed,*canonical,smallEight,recurrent,sha,error));
 permit=false;assert(!revoked.ReadPage(small,0,out.data(),out.size(),error) && out==untouched && !revoked.Ready());
 permit=true;assert(!revoked.Ready());
 puts("PASS 96 padded recurrent cache routes, parent forwarding and fail-closed coverage; SYNTHETIC NO UPLOAD/DEVICE/SPEED");
}
