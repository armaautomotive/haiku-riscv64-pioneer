/* SPDX-License-Identifier: MIT */
// Complete QGW8 page routing using synthetic immutable-source/cache stubs.
// NO actual model, initialized device memory, GPU, inference or Work.
#define PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION 1
#define PIONEER_Q38_HEAD_BLOCK_FLOAT_DIAGNOSTIC 1
#define PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION 1
#define PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT 1
#define PIONEER_Q38_PREPARED_PAGES_FIXTURE_ONLY
#include "pioneer_q38_hybrid_prepared_pages_test.cpp"
#include "../PioneerQ38HybridHeadPreparedPages.h"
#include "../PioneerQ38HeadBfp8Preparation.h"

template<class Source>struct FakeHead {
 static unsigned opened,read;static bool allowed,change;Source* source=nullptr;
 bool OpenPrepared(Source& s,const PioneerQ38PackedPart& p,const PioneerModelWeightCache::Digest&,
  const PioneerModelWeightCache::Digest&,const char*,std::string&){
  opened++;if(!allowed || !s.live || p.layer!=64 || p.role!=1)return false;source=&s;return true;
 }
 bool Prepare(Source& s,const PioneerQ38PackedPart& p,const PioneerModelWeightCache::Digest& a,
  const PioneerModelWeightCache::Digest& b,const char* directory,bool& hit,std::string& error){
  hit=false;return OpenPrepared(s,p,a,b,directory,error);
 }
 bool Ready()const{return source && source->live;}
 bool ReadPages(unsigned,unsigned count,uint8_t* output,unsigned capacity,std::string&){
  if(!source || !source->live || !count || count>16 || capacity<count*4096)return false;
  std::memset(output,0x8d,count*4096);read+=count;if(change)source->epoch++;return true;
 }
};
template<class Source>unsigned FakeHead<Source>::opened=0;
template<class Source>unsigned FakeHead<Source>::read=0;
template<class Source>bool FakeHead<Source>::allowed=true;
template<class Source>bool FakeHead<Source>::change=false;
using HeadPages=PioneerQ38HybridHeadPreparedPages<FakeSource,FakePlan,FakeBfp8,FakeFour,FakeHead>;
int main(){
 auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(canonical->Build());
 FakeSource source;FakePlan packed;packed.source=&source;std::map<std::pair<unsigned,unsigned>,unsigned> ids;
 for(unsigned ordinal=0;ordinal<=64;ordinal++){
  const unsigned layer=ordinal?ordinal-1:64;
  for(unsigned role=0;;role++){
   PioneerQ38Tensor t;if(!PioneerQ38Expected(layer,role,t))break;ids[{layer,role}]=unsigned(source.tensors.size());source.tensors.push_back(t);
  }
 }
 assert(source.tensors.size()==851);
 unsigned f48=UINT32_MAX,f49=UINT32_MAX,f50=UINT32_MAX,f60=UINT32_MAX,f61=UINT32_MAX,head=UINT32_MAX,original=UINT32_MAX,state=UINT32_MAX;
 for(unsigned i=0;i<canonical->Count();i++){
  const auto* c=canonical->Part(i);assert(c);unsigned tensor=c->kind==canonical->WeightKind?ids.at({c->layer,c->role}):851;
  packed.parts.push_back({c->kind,c->layer,c->role,tensor,c->bank,c->firstRow,c->rows,c->rowBytes,
   canonical->Base+c->offset,c->bytes,c->allocated});
  if(c->kind==canonical->WeightKind && c->role==2 && !c->firstRow){
   if(c->layer==48)f48=i;if(c->layer==49)f49=i;if(c->layer==50)f50=i;if(c->layer==60)f60=i;if(c->layer==61)f61=i;
  }
  if(c->kind==canonical->WeightKind && c->layer==64 && c->role==1 && !c->firstRow)head=i;
  if(c->kind==canonical->WeightKind && c->layer==64 && c->role==0 && !c->firstRow)original=i;
  if(c->kind==canonical->WorkspaceKind && state==UINT32_MAX)state=i;
 }
 for(auto x:{f48,f49,f50,f60,f61,head,original,state})assert(x!=UINT32_MAX);
 const auto digest=PioneerModelWeightCache::Hash("authenticated-fixture",21),fourHash=PioneerModelWeightCache::Hash("four-fingerprint",16),headHash=PioneerModelWeightCache::Hash("head-fingerprint",16);
 FakeBfp8 eight{&source};eight.digest=digest;std::string error;bool permit=true;HeadPages pages;
 auto bind=[&](HeadPages& p,FakePlan& plan,FakeBfp8& b){return p.Bind(source,plan,*canonical,b,digest,fourHash,headHash,"/private/tmp/four","/private/tmp/head",[&]{return permit;},error);};
 assert(bind(pages,packed,eight));std::array<uint8_t,4096> out={};
 assert(!pages.ReadPage(f48,0,reinterpret_cast<uint8_t*>(&pages),4096,error) && pages.Ready());
 assert(!pages.ReadPage(f48,0,reinterpret_cast<uint8_t*>(&source),4096,error) && pages.Ready());
 assert(!pages.ReadPage(f48,0,reinterpret_cast<uint8_t*>(const_cast<PioneerQ38HybridModelPlan::Part*>(pages.Part(f48))),4096,error) && pages.Ready());
 for(unsigned i:{f48,f49,f61})assert(pages.Part(i)->encoding==PioneerQ38HybridModelPlan::Bfp4 && pages.ReadPage(i,0,out.data(),out.size(),error) && out[0]==0x4b);
 for(unsigned i:{f50,f60})assert(pages.Part(i)->encoding==PioneerQ38HybridModelPlan::Bfp8 && pages.ReadPage(i,0,out.data(),out.size(),error) && out[0]==0x8b);
 assert(pages.Part(head)->encoding==PioneerQ38HybridHeadModelPlan::HeadBfp8 && pages.ReadPage(head,0,out.data(),out.size(),error) && out[0]==0x8d);
 assert(pages.ReadPage(original,0,out.data(),out.size(),error) && out[0]==0x5a);
 const auto saved=out;assert(!pages.ReadPage(state,0,out.data(),out.size(),error) && out==saved && pages.Ready());
 assert(!pages.ReadPages(head,pages.Part(head)->allocated/4096-1,2,out.data(),out.size(),error) && out==saved && pages.Ready());
 FakeFour<FakeSource>::opened=FakeFour<FakeSource>::read=0;FakeHead<FakeSource>::opened=FakeHead<FakeSource>::read=0;
 FakeBfp8 allEight{&source};allEight.digest=digest;HeadPages all;assert(bind(all,packed,allEight));unsigned progress=0;
 assert(all.VerifyAll(error,[&](unsigned done,unsigned total){assert(total==1351);progress=done;}) && all.AllVerified());
 assert(progress==1351 && FakeFour<FakeSource>::opened==1113 && FakeFour<FakeSource>::read==1113
  && allEight.reads==231 && FakeHead<FakeSource>::opened==7 && FakeHead<FakeSource>::read==7);
 auto changedPlan=packed;HeadPages changed;assert(bind(changed,changedPlan,eight));changedPlan.parts[f48].bytes++;
 assert(!changed.ReadPage(f48,0,out.data(),out.size(),error) && out==saved && !changed.Ready());
 FakeBfp8 failedEight{&source};failedEight.digest=digest;failedEight.failAt=2;HeadPages failed;assert(bind(failed,packed,failedEight));
 std::array<uint8_t,8192> atomic;atomic.fill(0x71);const auto atomicBefore=atomic;
 assert(!failed.ReadPages(f50,0,2,atomic.data(),atomic.size(),error) && atomic==atomicBefore && !failed.Ready());
 FakeHead<FakeSource>::allowed=false;HeadPages missing;assert(bind(missing,packed,eight));
 assert(!missing.ReadPage(head,0,out.data(),out.size(),error) && out==saved && !missing.Ready());FakeHead<FakeSource>::allowed=true;
 HeadPages lost;assert(bind(lost,packed,eight));FakeHead<FakeSource>::change=true;
 assert(!lost.ReadPage(head,0,out.data(),out.size(),error) && out==saved && !lost.Ready());FakeHead<FakeSource>::change=false;
 source.epoch--;assert(!lost.Ready());
 HeadPages denied;permit=false;assert(!bind(denied,packed,eight));permit=true;
 using FourPreparation=PioneerQ38HybridBfp4Preparation<FakeSource,FakePlan,FakeFour,UINT64_C(0x1ffc000000000000)>;
 FourPreparation::Statistics fourStats;FakeFour<FakeSource>::opened=0;unsigned fourProgress=0;
 assert(FourPreparation::Run(source,packed,*canonical,digest,fourHash,"/private/tmp/four",false,fourStats,error,
  [&](unsigned done,unsigned total,bool hit){assert(total==1113 && !hit);fourProgress=done;}));
 assert(fourStats.fragments==1113 && !fourStats.hits && fourProgress==1113 && FakeFour<FakeSource>::opened==1113);
 using HeadPreparation=PioneerQ38HeadBfp8Preparation<FakeSource,FakePlan,FakeHead>;
 HeadPreparation::Statistics headStats;FakeHead<FakeSource>::opened=0;unsigned headProgress=0;
 assert(HeadPreparation::Run(source,packed,*canonical,digest,headHash,"/private/tmp/head",false,headStats,error,
  [&](unsigned done,unsigned total,bool hit){assert(total==7 && !hit);headProgress=done;}));
 assert(headStats.fragments==7 && !headStats.hits && headStats.bytes==UINT64_C(1350860800) && headProgress==7 && FakeHead<FakeSource>::opened==7);
 auto malformed=packed;malformed.parts[head].proposedAddress+=4096;const auto oldStats=headStats;
 assert(!HeadPreparation::Run(source,malformed,*canonical,digest,headHash,"/private/tmp/head",false,headStats,error)
  && headStats.fragments==oldStats.fragments && FakeHead<FakeSource>::opened==7);
 auto changedDuring=packed;FakeHead<FakeSource>::opened=0;
 assert(!HeadPreparation::Run(source,changedDuring,*canonical,digest,headHash,"/private/tmp/head",false,headStats,error,
  [&](unsigned done,unsigned,bool){if(done==1)changedDuring.parts[original].bytes++;}) && FakeHead<FakeSource>::opened==1
  && headStats.fragments==oldStats.fragments);
 FakeHead<FakeSource>::opened=0;
 assert(HeadPreparation::Run(source,packed,*canonical,digest,headHash,"/private/tmp/head",true,headStats,error)
  && headStats.fragments==7 && headStats.hits==7 && FakeHead<FakeSource>::opened==7);
 puts("PASS QGW8 complete prepared page routing: all 1351 converted stripes, FFN format boundaries, head, original data, missing/changed/failed cache atomicity; synthetic source only, NO device or TPS");
}
