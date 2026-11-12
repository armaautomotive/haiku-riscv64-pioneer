/* SPDX-License-Identifier: MIT */
// Synthetic page routing and lifetime checks: no model or card access.
#define PIONEER_Q38_RECURRENT_Q8_MATRIX_PLAN_INSPECTION 1
#define PIONEER_Q38_DENSE_Q8_PREPARED_FIXTURE_ONLY 1
#include "pioneer_q38_dense_q8_prepared_pages_test.cpp"
#include "../PioneerQ38HybridRecurrentQ8PreparedPages.h"
using RecurrentPages=PioneerQ38HybridRecurrentQ8PreparedPages<FakeSource,FakePlan,FakeBfp8,DensePages>;
using RecurrentPlan=PioneerQ38HybridRecurrentQ8MatrixPlan;

int main(){
 auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(canonical->Build());
 FakeSource source;FakePlan packed;packed.source=&source;std::map<std::pair<unsigned,unsigned>,unsigned> ids;
 for(unsigned ordinal=0;ordinal<=64;ordinal++){
  const unsigned layer=ordinal?ordinal-1:64;
  for(unsigned role=0;;role++){PioneerQ38Tensor t;if(!PioneerQ38Expected(layer,role,t))break;
   ids[{layer,role}]=unsigned(source.tensors.size());source.tensors.push_back(t);}
 }
 unsigned in5120=UINT32_MAX,in6144=UINT32_MAX,small=UINT32_MAX,dense=UINT32_MAX,ffn=UINT32_MAX,raw=UINT32_MAX,state=UINT32_MAX;
 for(unsigned i=0;i<canonical->Count();i++){
  const auto* p=canonical->Part(i);assert(p);
  const unsigned tensor=p->kind==canonical->WeightKind?ids.at({p->layer,p->role}):851;
  packed.parts.push_back({p->kind,p->layer,p->role,tensor,p->bank,p->firstRow,p->rows,p->rowBytes,
   canonical->Base+p->offset,p->bytes,p->allocated});
  if(p->kind==canonical->WeightKind && p->layer==0 && !p->firstRow){
   if(p->role==5)in5120=i;else if(p->role==13)in6144=i;else if(p->role==10)small=i;else if(p->role==2)ffn=i;
  }
  if(p->kind==canonical->WeightKind && p->layer==3 && p->role==5 && !p->firstRow)dense=i;
  if(p->kind==canonical->WeightKind && p->layer==64 && p->role==0 && !p->firstRow)raw=i;
  if(p->kind==canonical->WorkspaceKind && state==UINT32_MAX)state=i;
 }
 for(unsigned n:{in5120,in6144,small,dense,ffn,raw,state})assert(n!=UINT32_MAX);
 const auto sha=PioneerModelWeightCache::Hash("authenticated-fixture",21);
 const auto four=PioneerModelWeightCache::Hash("four-fingerprint",16),headHash=PioneerModelWeightCache::Hash("head-fingerprint",16);
 FakeBfp8 parentEight{&source},recurrentEight{&source};parentEight.digest=recurrentEight.digest=sha;
 std::string error;HeadPages head;bool permit=true;
 assert(head.Bind(source,packed,*canonical,parentEight,sha,four,headHash,"/private/tmp/four","/private/tmp/head",[&]{return permit;},error));
 assert(head.VerifyAll(error));DensePages parent;
 assert(parent.Bind(source,packed,*canonical,parentEight,head,sha,error));
 RecurrentPages early;assert(!early.Bind(source,packed,*canonical,recurrentEight,parent,sha,error));
 assert(parent.VerifyAll(error) && parent.MatchesBinding(source,packed,*canonical,sha));
 assert(parent.MatchesVerifiedSnapshot(packed.parts));auto altered=packed.parts;altered[raw].tensor++;
 assert(!parent.MatchesVerifiedSnapshot(altered) && parent.AllVerified());
 // Binding is a cheap identity check; full live-plan comparison remains a
 // separate required AllVerified checkpoint, not a scan on every page read.
 packed.parts[raw].tensor++;assert(parent.MatchesBinding(source,packed,*canonical,sha) && !parent.AllVerified());
 packed.parts[raw].tensor--;assert(parent.AllVerified());
 auto wrongSha=sha;wrongSha[0]^=1;RecurrentPages wrong;
 assert(!wrong.Bind(source,packed,*canonical,recurrentEight,parent,wrongSha,error));
 FakeSource otherSource;FakeBfp8 wrongSource{&otherSource};wrongSource.digest=sha;RecurrentPages wrongIdentity;
 assert(!wrongIdentity.Bind(source,packed,*canonical,wrongSource,parent,sha,error));
 RecurrentPages pages;assert(pages.Bind(source,packed,*canonical,recurrentEight,parent,sha,error) && pages.Ready());
 assert(!pages.Bind(source,packed,*canonical,recurrentEight,parent,sha,error));
 assert(pages.Part(in5120)->encoding==RecurrentPlan::RecurrentQ8Bfp8
  && pages.Part(in6144)->encoding==RecurrentPlan::RecurrentQ8Bfp8
  && pages.Part(small)->encoding==PioneerQ38HybridModelPlan::Original);
 std::array<uint8_t,8192> out{};const unsigned beforeParent=parentEight.reads;
 assert(pages.ReadPages(in5120,0,2,out.data(),out.size(),error) && out[0]==0x8b && out[4096]==0x8b);
 assert(pages.ReadPage(in6144,0,out.data(),out.size(),error) && out[0]==0x8b && recurrentEight.reads==3
  && parentEight.reads==beforeParent);
 const auto* endPart=pages.Part(in6144);const unsigned last=endPart->allocated/4096-1,valid=endPart->bytes%4096;
 assert(pages.ReadPage(in6144,last,out.data(),out.size(),error));
 if(valid)for(unsigned i=valid;i<4096;i++)assert(!out[i]);
 assert(pages.ReadPage(small,0,out.data(),out.size(),error) && out[0]==0x5a);
 assert(pages.ReadPage(dense,0,out.data(),out.size(),error) && out[0]==0x8b && parentEight.reads==beforeParent+1);
 assert(pages.ReadPage(ffn,0,out.data(),out.size(),error) && out[0]==0x4b);
 assert(pages.ReadPage(raw,0,out.data(),out.size(),error) && out[0]==0x5a);
 const auto saved=out;
 assert(!pages.ReadPage(state,0,out.data(),out.size(),error) && out==saved && pages.Ready());
 assert(!pages.ReadPage(in5120,0,reinterpret_cast<uint8_t*>(&pages),4096,error) && pages.Ready());
 assert(!pages.ReadPage(in5120,0,reinterpret_cast<uint8_t*>(&recurrentEight),4096,error) && pages.Ready());
 assert(!pages.ReadPage(in5120,0,reinterpret_cast<uint8_t*>(packed.parts.data()),4096,error) && pages.Ready());
 assert(!pages.ReadPage(in5120,0,reinterpret_cast<uint8_t*>(const_cast<PioneerQ38HybridModelPlan::Part*>(pages.Part(in5120))),4096,error) && pages.Ready());
 assert(!pages.ReadPages(in5120,pages.Part(in5120)->allocated/4096-1,2,out.data(),out.size(),error) && out==saved);
 const unsigned beforeVerify=recurrentEight.reads;
 assert(pages.VerifyAll(error) && pages.AllVerified() && recurrentEight.reads==beforeVerify+1008);
 packed.parts[raw].tensor++;assert(!pages.AllVerified());packed.parts[raw].tensor--;assert(pages.AllVerified());
 assert(!pages.VerifyAll(error));
 RecurrentPages fail;assert(fail.Bind(source,packed,*canonical,recurrentEight,parent,sha,error));
 recurrentEight.failAt=recurrentEight.reads+2;out.fill(0x71);const auto before=out;
 assert(!fail.ReadPages(in5120,0,2,out.data(),out.size(),error) && out==before && !fail.Ready());
 recurrentEight.failAt=0;assert(!fail.Ready());
 RecurrentPages revoked;assert(revoked.Bind(source,packed,*canonical,recurrentEight,parent,sha,error));
 permit=false;assert(!revoked.ReadPage(in6144,0,out.data(),out.size(),error) && out==before && !revoked.Ready());
 permit=true;assert(!revoked.Ready());
 puts("PASS recurrent BFP8 prepared pages: two widths, parent routing, 1008-stripe coverage, revocation and atomic failures; SYNTHETIC NO DEVICE");
}
