/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#include "pioneer_q38_resident_output_learned_bytes.h"
#include "pioneer_q38_decoder_original_source.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentOutputPlan.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <map>
#include <memory>
struct ByteSource:ProjectionExecutionLearnedBytes {
 struct Request{unsigned route,address,bytes;};std::vector<Request> requests;
 unsigned reads=0,fail=0,expire=0;bool current=true;
 bool Ready()const override{return current;}
 static uint8_t Byte(unsigned route,unsigned at){return uint8_t((route*37+at*13+(at>>9))&255);}
 bool Read(unsigned route,uint32_t at,uint8_t* out,unsigned bytes)override{
  assert(current && out && reads<requests.size());const auto& r=requests[reads++];
  assert(route==r.route && at==r.address && bytes==r.bytes);
  if(reads==fail)return false;
  for(unsigned i=0;i<bytes;i++)out[i]=Byte(route,at+i);
  if(reads==expire)current=false;
  return true;
 }
};
int main(int argc,char** argv){
 assert(argc==1 || argc==2);setvbuf(stdout,nullptr,_IOLBF,0);
 auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(canonical->Build());
 auto plan=std::make_unique<PioneerQ38ResidentOutputPlan>();
 std::array<uint32_t,1920> sources;std::array<uint32_t,119> sequences={};sources.fill(0x16005);
 assert(plan->Build(*canonical,0,0,0,0,0,6,{1,1,1,1},sources.data(),sequences.data()));
 const auto* m=plan->Manifest();const auto* q=plan->PostManifest();ByteSource seed;
 for(unsigned w=0;w<120;w++)for(unsigned j=0;j<PioneerQ38ConvBatchCount(w);j++){
  const auto* d=PioneerQ38CohortConv(m,w)+j*64;seed.requests.push_back({d[13],d[12],256});
 }
 for(unsigned w=1;w<=96;w++){
  const auto* d=PioneerQ38CohortGdn(m,w);seed.requests.push_back({d[13],d[12],64});
  seed.requests.push_back({d[41],d[15],64});seed.requests.push_back({d[141],d[140],512});
 }
 assert(seed.requests.size()==928);unsigned writes=0;
 auto put=[&](unsigned route,unsigned at,uint32_t value){
  uint32_t expected=0;for(unsigned b=0;b<4;b++)expected|=uint32_t(ByteSource::Byte(route,at+b))<<(b*8);
  assert(value==expected);writes++;
 };
 auto valid=seed;assert(ResidentOutputImportLearnedBytes(m,valid,put) && valid.reads==928 && writes==56320);
 unsigned faults=0;
 for(unsigned at:{1U,640U,641U,642U,643U,928U})for(bool expire:{false,true}){
  auto bad=seed;if(expire)bad.expire=at;else bad.fail=at;
  assert(!ResidentOutputImportLearnedBytes(m,bad,put) && bad.reads==at);faults++;
 }
 auto absent=seed;absent.current=false;assert(!ResidentOutputImportLearnedBytes(m,absent,put) && !absent.reads);
 assert(!ResidentOutputImportLearnedBytes(nullptr,seed,put) && !seed.reads);
 unsigned tiles=0;
 for(unsigned w=0;w<119;w++){
  const auto* p=q+64+w*8;ByteSource source;
  for(unsigned t=0;t<p[2]*192;t++)source.requests.push_back({p[4],p[0]+t*576,576});
  std::vector<std::vector<uint8_t>> bytes;assert(ResidentOutputImportMatrixBytes(p,source,bytes));
  for(unsigned t=0;t<bytes.size();t++)for(unsigned i=0;i<576;i++)assert(bytes[t][i]==ByteSource::Byte(p[4],p[0]+t*576+i));
  tiles+=unsigned(bytes.size());
  if(w)continue;
  for(unsigned at:{1U,unsigned(source.requests.size()/2),unsigned(source.requests.size())})for(bool expire:{false,true}){
   source.reads=0;source.current=true;source.fail=expire?0:at;source.expire=expire?at:0;
   std::vector<std::vector<uint8_t>> untouched={{1,2,3}};const auto before=untouched;
   assert(!ResidentOutputImportMatrixBytes(p,source,untouched) && untouched==before && source.reads==at);faults++;
  }
 }
 assert(tiles==61440 && faults==18);
 puts("PASS TEST learned-byte import:640Conv+192A/bias+96norm packets/225280bytes;61440original-layout Q8tiles/ALL119matrix jobs;18missing-or-changed-source refusals;NO numeric/owner/native readiness or synthetic fallback");
 if(argc==1)return 0;
 puts("ORIGINAL_GGUF_AUTH_AND_REFERENCES_BEFORE_ANY_NATIVE_CONTEXT_OR_DEVICE");
 auto original=OpenFfnResidualOriginalSource(argv[1]);
 assert(original && original->Prepare(0) && original->SealBeforeNativeContext() && original->FfnWeightsReady());
 uint64_t imported=0;auto originalPut=[&](unsigned,unsigned,uint32_t){imported++;};
 assert(ResidentOutputImportLearnedBytes(m,*original,originalPut) && imported==56320);
 for(unsigned w=0;w<119;w++){
  std::vector<std::vector<uint8_t>> bytes;assert(ResidentOutputImportMatrixBytes(q+64+w*8,*original,bytes));
 }
 assert(original->ReferenceReads()==1673 && !original->ProjectionReads() && original->ConvolutionReads()==640
  && original->GdnInputReads()==192 && original->GdnNormReads()==96 && original->GdnOutputReads()==61440 && !original->FfnReads() && original->Ready());
 puts("PASS FULL-SHA ORIGINAL token0/layer0 complete first-layer source/sealed-before-context:ALL640Conv+192A/bias+96norm packets+61440output tiles through SAME authentic source;BYTE ENDPOINTS ONLY,NOT execution/physical/rate qualification");
}
