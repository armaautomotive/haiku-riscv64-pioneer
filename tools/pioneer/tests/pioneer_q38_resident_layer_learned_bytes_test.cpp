/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#include "pioneer_q38_resident_layer_learned_bytes.h"
#include "pioneer_q38_decoder_original_source.h"
#include <array>
#include <cassert>
#include <cstdio>
struct LayerByteSource:ProjectionExecutionLearnedBytes {
 struct Request{unsigned route,address,bytes;};std::vector<Request> requests;
 unsigned reads=0,fail=0,expire=0;bool current=true;
 bool Ready()const override{return current;}
 static uint8_t Byte(unsigned route,unsigned at){return uint8_t((route*37+at*13+(at>>9))&255);}
 bool Read(unsigned route,uint32_t at,uint8_t* out,unsigned bytes)override{
  assert(current && out && reads<requests.size());const auto& r=requests[reads++];assert(route==r.route && at==r.address && bytes==r.bytes);
  if(reads==fail)return false;for(unsigned i=0;i<bytes;i++)out[i]=Byte(route,at+i);if(reads==expire)current=false;return true;
 }
};
int main(int argc,char** argv){
 assert(argc==1 || argc==2);setvbuf(stdout,nullptr,_IOLBF,0);
 auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(canonical->Build());
 auto plan=std::make_unique<PioneerQ38ResidentLayerPlan>();std::array<uint32_t,1920> sources;std::array<uint32_t,119> seq={};sources.fill(0x16005);
 assert(plan->Build(*canonical,0,0,0,0,0,6,{1,1,1,1},sources.data(),seq.data()));const auto* m=plan->Manifest();const auto* l=plan->LayerManifest();
 LayerByteSource seed;
 for(unsigned w=0;w<120;w++)for(unsigned j=0;j<PioneerQ38ConvBatchCount(w);j++){
  const auto* d=PioneerQ38CohortConv(m,w)+j*64;seed.requests.push_back({d[13],d[12],256});
 }
 for(unsigned w=1;w<=96;w++){
  const auto* d=PioneerQ38CohortGdn(m,w);seed.requests.push_back({d[13],d[12],64});seed.requests.push_back({d[41],d[15],64});seed.requests.push_back({d[141],d[140],512});
 }
 seed.requests.push_back({l[14],l[13],16384});seed.requests.push_back({l[14],l[13]+16384,4096});assert(seed.requests.size()==930);
 unsigned writes=0;auto put=[&](unsigned route,unsigned at,uint32_t value){
  uint32_t expected=0;for(unsigned b=0;b<4;b++)expected|=uint32_t(LayerByteSource::Byte(route,at+b))<<(b*8);assert(value==expected);writes++;
 };
 auto valid=seed;assert(ResidentLayerImportLearnedBytes(*plan,valid,put) && valid.reads==930 && writes==61440);
 unsigned faults=0;
 for(unsigned at:{1U,640U,641U,928U,929U,930U})for(bool expire:{false,true}){
  auto bad=seed;if(expire)bad.expire=at;else bad.fail=at;assert(!ResidentLayerImportLearnedBytes(*plan,bad,put) && bad.reads==at);faults++;
 }
 unsigned tiles=0;
 for(unsigned role=0;role<3;role++)for(unsigned w=1;w<120;w++){
  const auto* r=l+64+PioneerQ38FfnResidualGraphRoleOffset(role);const auto* p=r+16+(w-1)*8;LayerByteSource bytes;
  for(unsigned t=0;t<p[2]*(r[3]/256);t++)bytes.requests.push_back({p[4],p[0]+t*2304,2304});
  std::vector<std::vector<uint8_t>> out;assert(ResidentLayerImportFfnMatrixBytes(*plan,role,w,bytes,out));
  for(unsigned t=0;t<out.size();t++)for(unsigned i=0;i<2304;i++)assert(out[t][i]==LayerByteSource::Byte(p[4],p[0]+t*2304+i));
  tiles+=unsigned(out.size());if(w!=1 && w!=119)continue;
  for(unsigned at:{1U,unsigned(out.size()/2),unsigned(out.size())})for(bool expire:{false,true}){
   bytes.reads=0;bytes.current=true;bytes.fail=expire?0:at;bytes.expire=expire?at:0;
   std::vector<std::vector<uint8_t>> unchanged={{1,2,3}};auto before=unchanged;
   assert(!ResidentLayerImportFfnMatrixBytes(*plan,role,w,bytes,unchanged) && unchanged==before && bytes.reads==at);faults++;
  }
 }
 for(unsigned role:{3U,UINT32_MAX}){LayerByteSource bytes;std::vector<std::vector<uint8_t>> out={{1}};auto before=out;
  assert(!ResidentLayerImportFfnMatrixBytes(*plan,role,1,bytes,out) && out==before && !bytes.reads);faults++;}
 for(unsigned w:{0U,120U,UINT32_MAX}){LayerByteSource bytes;std::vector<std::vector<uint8_t>> out={{1}};auto before=out;
  assert(!ResidentLayerImportFfnMatrixBytes(*plan,0,w,bytes,out) && out==before && !bytes.reads);faults++;}
 assert(tiles==65280);printf("PASS NEW complete-layer learned BYTE import:930Conv/GDN/norm packets/245760bytes;ALL3roles+119workers/65280original-layout2304Btiles/%urefusals,atomic matrix cache/no synthetic fallback;NO CPU product/owner/native grant\n",faults);
 if(argc==1)return 0;
 auto original=OpenResidentLayerOriginalSource(argv[1]);assert(original && original->Prepare(0) && original->SealBeforeNativeContext() && original->ResidentLayerReferenceReady());
 uint64_t words=0;assert(ResidentLayerImportLearnedBytes(*plan,*original,[&](unsigned,unsigned,uint32_t){words++;}) && words==61440);
 for(unsigned role=0;role<3;role++)for(unsigned w=1;w<120;w++){
  std::vector<std::vector<uint8_t>> out;assert(ResidentLayerImportFfnMatrixBytes(*plan,role,w,*original,out));
 }
 assert(original->ReferenceReads()==4810 && original->ConvolutionReads()==640 && original->GdnInputReads()==192
  && original->GdnNormReads()==96 && original->ResidualRmsReads()==2 && original->FfnReads()==65280
  && !original->ProjectionReads() && !original->GdnOutputReads() && original->ResidentLayerReferenceReady());
 puts("PASS ORIGINAL fullsameSHA/schema/cache/precontext4810CPUreference:ALL learnedConv/GDN/norm+65280fullFFNtiles through SAME authenticated source;BYTES ONLY,NO instruction execution/physical/inference or rate claim");
}
