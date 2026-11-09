/* SPDX-License-Identifier: MIT */
// Synthetic routing/ownership checks only: no model file or accelerator.
#define PIONEER_Q38_DENSE_Q8_MATRIX_PLAN_INSPECTION 1
#define PIONEER_Q38_HYBRID_HEAD_PREPARED_FIXTURE_ONLY
#include "pioneer_q38_hybrid_head_prepared_pages_test.cpp"
#include "../PioneerQ38HybridDenseQ8PreparedPages.h"
using DensePages=PioneerQ38HybridDenseQ8PreparedPages<FakeSource,FakePlan,FakeBfp8,HeadPages>;
using Plan=PioneerQ38HybridDenseQ8MatrixPlan;
#ifndef PIONEER_Q38_DENSE_Q8_PREPARED_FIXTURE_ONLY
int main(){
 auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(canonical->Build());
 FakeSource source;FakePlan packed;packed.source=&source;std::map<std::pair<unsigned,unsigned>,unsigned> ids;
 for(unsigned ordinal=0;ordinal<=64;ordinal++){
  const unsigned layer=ordinal?ordinal-1:64;
  for(unsigned role=0;;role++){
   PioneerQ38Tensor t;if(!PioneerQ38Expected(layer,role,t))break;
   ids[{layer,role}]=unsigned(source.tensors.size());source.tensors.push_back(t);
  }
 }
 unsigned dense=UINT32_MAX,ffn=UINT32_MAX,head=UINT32_MAX,raw=UINT32_MAX,state=UINT32_MAX;
 for(unsigned i=0;i<canonical->Count();i++){
  const auto* p=canonical->Part(i);assert(p);
  const unsigned tensor=p->kind==canonical->WeightKind?ids.at({p->layer,p->role}):851;
  packed.parts.push_back({p->kind,p->layer,p->role,tensor,p->bank,p->firstRow,p->rows,p->rowBytes,
   canonical->Base+p->offset,p->bytes,p->allocated});
  if(p->kind==canonical->WeightKind && p->layer==3 && p->role==7 && !p->firstRow)dense=i;
  if(p->kind==canonical->WeightKind && p->layer==0 && p->role==2 && !p->firstRow)ffn=i;
  if(p->kind==canonical->WeightKind && p->layer==64 && p->role==1 && !p->firstRow)head=i;
  if(p->kind==canonical->WeightKind && p->layer==64 && p->role==0 && !p->firstRow)raw=i;
  if(p->kind==canonical->WorkspaceKind && state==UINT32_MAX)state=i;
 }
 for(unsigned n:{dense,ffn,head,raw,state})assert(n!=UINT32_MAX);
 const auto sha=PioneerModelWeightCache::Hash("authenticated-fixture",21);
 const auto four=PioneerModelWeightCache::Hash("four-fingerprint",16),headHash=PioneerModelWeightCache::Hash("head-fingerprint",16);
 FakeBfp8 eight{&source};eight.digest=sha;std::string error;HeadPages parent;bool permit=true;
 assert(parent.Bind(source,packed,*canonical,eight,sha,four,headHash,"/private/tmp/four","/private/tmp/head",[&]{return permit;},error));
 DensePages early;assert(!early.Bind(source,packed,*canonical,eight,parent,sha,error));
 assert(parent.VerifyAll(error) && parent.AllVerified());DensePages pages;
 FakeBfp8 wrongEight{&source};wrongEight.digest=sha;DensePages wrong;
 assert(!wrong.Bind(source,packed,*canonical,wrongEight,parent,sha,error));
 auto wrongSha=sha;wrongSha[0]^=1;DensePages wrongIdentity;
 assert(!wrongIdentity.Bind(source,packed,*canonical,eight,parent,wrongSha,error));
 assert(pages.Bind(source,packed,*canonical,eight,parent,sha,error) && pages.Ready());
 assert(!pages.Bind(source,packed,*canonical,eight,parent,sha,error));
 std::array<uint8_t,8192> out{};
 assert(pages.Part(dense)->encoding==Plan::DenseQ8Bfp8);
 assert(pages.ReadPages(dense,0,2,out.data(),out.size(),error) && out[0]==0x8b && out[4096]==0x8b);
 assert(pages.ReadPage(ffn,0,out.data(),out.size(),error) && out[0]==0x4b);
 assert(pages.ReadPage(head,0,out.data(),out.size(),error) && out[0]==0x8d);
 assert(pages.ReadPage(raw,0,out.data(),out.size(),error) && out[0]==0x5a);
 const auto saved=out;
 assert(!pages.ReadPage(state,0,out.data(),out.size(),error) && out==saved && pages.Ready());
 assert(!pages.ReadPage(dense,0,reinterpret_cast<uint8_t*>(&pages),4096,error) && pages.Ready());
 assert(!pages.ReadPage(dense,0,reinterpret_cast<uint8_t*>(packed.parts.data()),4096,error) && pages.Ready());
 assert(!pages.ReadPage(dense,0,reinterpret_cast<uint8_t*>(const_cast<PioneerQ38HybridModelPlan::Part*>(pages.Part(dense))),4096,error) && pages.Ready());
 assert(!pages.ReadPages(dense,pages.Part(dense)->allocated/4096-1,2,out.data(),out.size(),error) && out==saved);
 FakeBfp8 denseEight{&source};denseEight.digest=sha;DensePages separate,wrongParent;
 assert(!wrongParent.Bind(source,packed,*canonical,denseEight,wrongEight,parent,sha,error));
 assert(separate.Bind(source,packed,*canonical,denseEight,eight,parent,sha,error));
 const unsigned parentBefore=eight.reads;
 assert(separate.ReadPage(dense,0,out.data(),out.size(),error) && denseEight.reads==1 && eight.reads==parentBefore);
 assert(!separate.ReadPage(dense,0,reinterpret_cast<uint8_t*>(&denseEight),4096,error) && separate.Ready());
 denseEight.live=false;assert(!separate.ReadPage(dense,0,out.data(),out.size(),error) && !separate.Ready());
 denseEight.live=true;assert(!separate.Ready());
 const unsigned previous=eight.reads;assert(pages.VerifyAll(error) && pages.AllVerified() && eight.reads==previous+336);
 assert(!pages.VerifyAll(error));
 DensePages fail;assert(fail.Bind(source,packed,*canonical,eight,parent,sha,error));
 eight.failAt=eight.reads+2;out.fill(0x71);const auto before=out;
 assert(!fail.ReadPages(dense,0,2,out.data(),out.size(),error) && out==before && !fail.Ready());
 eight.failAt=0;assert(!fail.Ready());
 DensePages changed;assert(changed.Bind(source,packed,*canonical,eight,parent,sha,error));
 packed.parts[dense].firstRow+=32;
 assert(!changed.ReadPage(dense,0,out.data(),out.size(),error) && out==before && !changed.Ready());
 packed.parts[dense].firstRow-=32;assert(!changed.Ready());
 DensePages revoked;assert(revoked.Bind(source,packed,*canonical,eight,parent,sha,error));
 permit=false;assert(!revoked.ReadPage(dense,0,out.data(),out.size(),error) && out==before && !revoked.Ready());
 permit=true;assert(!revoked.Ready());
 puts("PASS dense Q8 converted/cache page routing, coverage and failure atomicity; SYNTHETIC NO DEVICE");
}
#endif
