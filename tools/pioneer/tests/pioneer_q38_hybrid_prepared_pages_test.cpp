/* SPDX-License-Identifier: MIT */
// Preparation/page dispatch simulation. No actual GGUF or accelerator.
#define PIONEER_Q38_BFP4_PREPARATION_DIAGNOSTIC 1
#define PIONEER_Q38_GDN_MUTABLE_STATE
#define PIONEER_BANKS_MOCK_ONLY
#include "../PioneerQ38HybridPreparedPages.h"
#include "../PioneerQ38HybridBfp4Preparation.h"
#include <cassert>
#include <cstdio>
#include <map>

struct FakeSource {
 std::vector<PioneerQ38Tensor> tensors;uint64_t epoch=1;bool live=true;unsigned reads=0;
 bool Ready()const{return live;}bool Stable()const{return live;}uint64_t Epoch()const{return epoch;}
 const std::vector<PioneerQ38Tensor>& Tensors()const{return tensors;}
 bool ReadRow(unsigned t,unsigned row,float* output,unsigned capacity){
  if(!live || t>=tensors.size() || row>=tensors[t].outputs || capacity<tensors[t].inputs)return false;
  reads++;for(unsigned i=0;i<tensors[t].inputs;i++)output[i]=float(int((row+i)%31)-15)/16.0f;return true;
 }
};
struct FakePlan {
 const FakeSource* source=nullptr;std::vector<PioneerQ38PackedPart> parts;mutable unsigned reads=0;
 bool MatchesSource(const FakeSource& s)const{return source==&s && s.live;}
 bool Tiled()const{return true;}unsigned DeltaSlots()const{return 2;}
 const std::vector<PioneerQ38PackedPart>& Parts()const{return parts;}
 bool ReadWeightPage(FakeSource& s,unsigned part,unsigned page,uint8_t* out,unsigned capacity)const{
  if(!MatchesSource(s) || part>=parts.size() || page>=parts[part].allocated/4096 || capacity<4096)return false;
  reads++;std::memset(out,0x5a,4096);return true;
 }
};
struct FakeBfp8 {
 const FakeSource* source=nullptr;bool live=true;unsigned reads=0;PioneerModelWeightCache::Digest digest={};
 bool MatchesSource(const FakeSource& s)const{return source==&s && live && s.live;}
 bool MatchesIdentity(const FakeSource& s,const PioneerModelWeightCache::Digest& hash)const{return MatchesSource(s) && digest==hash;}
 bool ReadStripePage(unsigned tensor,unsigned first,unsigned rows,unsigned page,uint8_t* out,unsigned cap,std::string&){
  if(!source || !live || !source->live || tensor>=source->tensors.size() || cap<4096)return false;
  const auto& t=source->tensors[tensor];if((first|rows)&31 || first+rows>t.outputs)return false;
  const uint64_t bytes=uint64_t(rows/32)*(t.inputs/32)*1088,offset=uint64_t(page)*4096;
  if(offset>=((bytes+4095)&~uint64_t(4095)))return false;
  const unsigned count=unsigned(std::min(uint64_t(4096),bytes>offset?bytes-offset:0));
  std::memset(out,0,4096);std::memset(out,0x8b,count);reads++;return true;
 }
};
template<class Source>struct FakeFour {
 static unsigned opened,read;Source* source=nullptr;
 bool OpenPrepared(Source& s,const PioneerQ38PackedPart&,const PioneerModelWeightCache::Digest&,
  const PioneerModelWeightCache::Digest&,const char*,std::string&){source=&s;opened++;return s.live;}
 bool Prepare(Source& s,const PioneerQ38PackedPart& p,const PioneerModelWeightCache::Digest& a,
  const PioneerModelWeightCache::Digest& b,const char* directory,bool& hit,std::string& error){
  hit=false;return OpenPrepared(s,p,a,b,directory,error);
 }
 bool Ready()const{return source && source->live;}
 bool ReadPage(unsigned,uint8_t* output,unsigned capacity,std::string&){
  if(!source || !source->live || capacity<4096)return false;
  std::memset(output,0x4b,4096);read++;return true;
 }
};
template<class Source>unsigned FakeFour<Source>::opened=0;
template<class Source>unsigned FakeFour<Source>::read=0;
using Pages=PioneerQ38HybridPreparedPages<FakeSource,FakePlan,FakeBfp8>;
using MockPages=PioneerQ38HybridPreparedPages<FakeSource,FakePlan,FakeBfp8,FakeFour>;
#ifndef PIONEER_Q38_PREPARED_PAGES_FIXTURE_ONLY
int main(int argc,char** argv){
 assert(argc==2);auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(canonical->Build());
 FakeSource source;FakePlan packed;packed.source=&source;std::map<std::pair<unsigned,unsigned>,unsigned> ids;
 for(unsigned ordinal=0;ordinal<=64;ordinal++){
  const unsigned layer=ordinal?ordinal-1:64;
  for(unsigned role=0;;role++){
   PioneerQ38Tensor tensor;if(!PioneerQ38Expected(layer,role,tensor))break;
   ids[{layer,role}]=unsigned(source.tensors.size());source.tensors.push_back(tensor);
  }
 }
 assert(source.tensors.size()==851);
 unsigned four=UINT32_MAX,eight=UINT32_MAX,unchanged=UINT32_MAX,state=UINT32_MAX;
 for(unsigned i=0;i<canonical->Count();i++){
  const auto* c=canonical->Part(i);assert(c);unsigned tensor=c->kind==canonical->WeightKind?ids.at({c->layer,c->role}):851;
  packed.parts.push_back({c->kind,c->layer,c->role,tensor,c->bank,c->firstRow,c->rows,c->rowBytes,
   canonical->Base+c->offset,c->bytes,c->allocated});
  if(c->kind==canonical->WeightKind && c->layer==0 && c->role==2 && !c->firstRow)four=i;
  if(c->kind==canonical->WeightKind && c->layer==48 && c->role==2 && !c->firstRow)eight=i;
  if(c->kind==canonical->WeightKind && c->layer==64 && c->role==0 && !c->firstRow)unchanged=i;
  if(c->kind==canonical->WorkspaceKind && state==UINT32_MAX)state=i;
 }
 assert(four!=UINT32_MAX && eight!=UINT32_MAX && unchanged!=UINT32_MAX && state!=UINT32_MAX);
 const auto digest=PioneerModelWeightCache::Hash("authenticated-fixture",21);FakeBfp8 bfp8{&source};bfp8.digest=digest;
 const auto fingerprint=PioneerModelWeightCache::Hash("converter-fingerprint",21);
 const std::string directory=std::string(argv[1])+"/cache";std::string error;bool hit=true;
 PioneerQ38Bfp4FragmentCache<FakeSource> prepare;
 assert(prepare.Prepare(source,packed.parts[four],digest,fingerprint,directory.c_str(),hit,error) && !hit);
 const unsigned rowsRead=source.reads;assert(rowsRead==packed.parts[four].rows);
 std::array<uint8_t,4096> expected={},actual={};assert(prepare.ReadPage(22,expected.data(),expected.size(),error));
 bool authorized=true;Pages pages;assert(pages.Bind(source,packed,*canonical,bfp8,digest,fingerprint,directory.c_str(),[&]{return authorized;},error));
 assert(pages.Ready() && pages.Part(four) && pages.Part(eight)->encoding==8);
 assert(pages.ReadPage(four,22,actual.data(),actual.size(),error) && actual==expected && source.reads==rowsRead);
 assert(pages.ReadPage(eight,0,actual.data(),actual.size(),error) && actual[0]==0x8b && bfp8.reads==1);
 const auto* e=pages.Part(eight);const unsigned last=e->allocated/4096-1;
 assert(pages.ReadPage(eight,last,actual.data(),actual.size(),error));
 const unsigned valid=e->bytes%4096;if(valid)for(unsigned i=valid;i<4096;i++)assert(!actual[i]);
 assert(pages.ReadPage(unchanged,0,actual.data(),actual.size(),error) && actual[0]==0x5a && packed.reads==1);
 const auto saved=actual;assert(!pages.ReadPage(state,0,actual.data(),actual.size(),error) && actual==saved && pages.Ready());
 assert(!pages.ReadPage(eight,last+1,actual.data(),actual.size(),error) && actual==saved && pages.Ready());
 authorized=false;assert(!pages.ReadPage(four,0,actual.data(),actual.size(),error) && actual==saved && !pages.Ready());
 authorized=true;assert(!pages.Ready() && !pages.ReadPage(four,0,actual.data(),actual.size(),error));
 Pages partial;assert(partial.Bind(source,packed,*canonical,bfp8,digest,fingerprint,directory.c_str(),[]{return true;},error));
 unsigned progress=0;assert(!partial.VerifyAll(error,[&](unsigned done,unsigned total){assert(total==1344);progress=done;})
  && progress==1 && !partial.AllVerified() && !partial.Ready());
 auto mutablePlan=packed;Pages changed;
 assert(changed.Bind(source,mutablePlan,*canonical,bfp8,digest,fingerprint,directory.c_str(),[]{return true;},error));
 mutablePlan.parts[four].bytes++;assert(!changed.ReadPage(four,0,actual.data(),actual.size(),error) && actual==saved && !changed.Ready());
 auto wrong=packed;wrong.parts[four].proposedAddress+=4096;Pages refuse;
 assert(!refuse.Bind(source,wrong,*canonical,bfp8,digest,fingerprint,directory.c_str(),[]{return true;},error));
 FakeBfp8 wrongSource;Pages foreign;assert(!foreign.Bind(source,packed,*canonical,wrongSource,digest,fingerprint,directory.c_str(),[]{return true;},error));
 Pages wrongIdentity;const auto otherDigest=PioneerModelWeightCache::Hash("different-source",16);
 assert(!wrongIdentity.Bind(source,packed,*canonical,bfp8,otherDigest,fingerprint,directory.c_str(),[]{return true;},error));
 FakeBfp8 allEight{&source};allEight.digest=digest;auto fullPlan=packed;MockPages complete;
 assert(complete.Bind(source,fullPlan,*canonical,allEight,digest,fingerprint,directory.c_str(),[]{return true;},error));
 unsigned verified=0;assert(complete.VerifyAll(error,[&](unsigned done,unsigned total){assert(total==1344);verified=done;}));
 assert(complete.AllVerified() && verified==1344 && FakeFour<FakeSource>::opened==1071
  && FakeFour<FakeSource>::read==1071 && allEight.reads==273);
 fullPlan.parts[four].bytes++;assert(!complete.AllVerified());
 auto duringPlan=packed;FakeBfp8 duringEight{&source};duringEight.digest=digest;MockPages during;
 assert(during.Bind(source,duringPlan,*canonical,duringEight,digest,fingerprint,directory.c_str(),[]{return true;},error));
 assert(!during.VerifyAll(error,[&](unsigned done,unsigned){if(done==1)duringPlan.parts[unchanged].bytes++;})
  && !during.AllVerified() && !during.Ready());
 using Prepare=PioneerQ38HybridBfp4Preparation<FakeSource,FakePlan,FakeFour>;
 Prepare::Statistics statistics;unsigned preparedProgress=0;FakeFour<FakeSource>::opened=0;
 assert(Prepare::Run(source,packed,*canonical,digest,fingerprint,directory.c_str(),false,statistics,error,
  [&](unsigned done,unsigned total,bool hit){assert(total==1071 && !hit);preparedProgress=done;}));
 assert(statistics.fragments==1071 && statistics.hits==0 && preparedProgress==1071 && FakeFour<FakeSource>::opened==1071);
 auto badPlan=packed;badPlan.parts[four].proposedAddress+=4096;const auto prior=statistics;
 assert(!Prepare::Run(source,badPlan,*canonical,digest,fingerprint,directory.c_str(),false,statistics,error)
  && statistics.fragments==prior.fragments && FakeFour<FakeSource>::opened==1071);
 FakeFour<FakeSource>::opened=0;
 assert(Prepare::Run(source,packed,*canonical,digest,fingerprint,directory.c_str(),true,statistics,error)
  && statistics.fragments==1071 && statistics.hits==1071 && FakeFour<FakeSource>::opened==1071);
 puts("PASS hybrid prepared page dispatch: full fixed geometry, real BFP4 cache reopen, BFP8/original simulated readers, boundaries, zero padding and revoked source; NO DEVICE");
}
#endif
