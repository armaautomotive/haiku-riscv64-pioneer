/* SPDX-License-Identifier: MIT */
#if defined(PIONEER_Q38_RESIDENT_DECODER_NATIVE) || defined(PIONEER_Q38_RESIDENT_DECODER_OWNER) || defined(PIONEER_Q38_EMBEDDING_NATIVE) || defined(PIONEER_Q38_PROJECTION_EXECUTION_OWNER) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(_KERNEL_MODE)
#error Original-model independent observer MUST be a separate non-native DEBUG compilation unit
#endif
#define PIONEER_Q38_RESIDENT_DECODER_VALIDATION_CLIENT 1
#define PIONEER_Q38_CPU_VALIDATION 1
#define PIONEER_Q38_LEARNED_RMS_VALIDATION_REFERENCE 1
#define PIONEER_Q38_GDN_MUTABLE_STATE 1 // canonical DATA geometry only;NO native adapter/execution/Work role
#include "pioneer_q38_decoder_original_source.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
static const char* originalPath=nullptr;
static FILE* DecoderOriginalFopen(const char* path,const char* mode){
 assert(originalPath && !strcmp(path,"/boot/home/models/Qwen3.8-27B-Q4_K_M.gguf") && !strcmp(mode,"rb"));return fopen(originalPath,mode);
}
static int DecoderOriginalExecv(const char* path,char* const* argv){
 assert(!strcmp(path,"/bin/sha256sum") && !strcmp(argv[0],"sha256sum") && !argv[1]);
 char name[]="shasum",flag[]="-a",bits[]="256";char* args[]={name,flag,bits,nullptr};return execv("/usr/bin/shasum",args);
}
#define fopen DecoderOriginalFopen
#define execv DecoderOriginalExecv
#include "../PioneerQ38PublicModelSource.h"
#undef fopen
#undef execv
#include "../PioneerQ38EmbeddingValidationReference.h"
#include "../PioneerQ38LearnedRmsValidationReference.h"
#include "../PioneerQ38ResidentDecoderValidationReference.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38CanonicalModelPlan.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38LearnedRmsShape.h"
#include <array>
namespace {
class OriginalSource final:public DecoderOriginalSource {
 struct Stripe {unsigned role,route;uint32_t address,bytes;std::vector<uint8_t> data;};
 PioneerQ38PublicModelSource original;PioneerQwen38PackedPlan plan;
 std::vector<Stripe> stripes;bool prepared=false,sealed=false,attempted=false;
 uint64_t referenceReads=0,projectionReads=0,convolutionReads=0;
 std::array<uint8_t,46080> embedding={};std::array<uint32_t,5120> norm={},raw={},normalized={};
 std::vector<uint32_t> projections=std::vector<uint32_t>(18432,0),convolution=std::vector<uint32_t>(40960,0);
 struct Reference {
  OriginalSource& parent;
  bool Ready()const{return !parent.sealed && parent.original.Ready();}
  const auto& Model()const{return parent.original.Model();}
  bool ReadReferencePacked(unsigned tensor,uint64_t offset,unsigned bytes,void* out,unsigned capacity){
   if(!Ready())return false;parent.referenceReads++;
   return parent.original.ReadReferencePacked(tensor,offset,bytes,out,capacity) && Ready();
  }
 };
public:
 bool Open(){return original.Open() && original.Ready();}
 bool Ready()const override{return prepared && original.Ready() && plan.MatchesSource(original.Model());}
 bool Prepare(unsigned token)override{
  if(attempted || sealed || token>=248320 || !original.Ready())return false;
  attempted=true;if(!plan.Build(original.Model(),8192,true,true))return false;
  auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();if(!canonical->Build() || canonical->Count()!=plan.Parts().size())return false;
  unsigned projectionsCount=0,convCount=0,convRows=0,embeddingCount=0,normTensor=UINT32_MAX,embeddingTensor=UINT32_MAX,pages=0;
  const auto& parts=plan.Parts();const auto& tensors=original.Model().Tensors();
  for(unsigned i=0;i<parts.size();i++){
   const auto& p=parts[i];const auto& c=*canonical->Part(i);
   if(c.kind!=p.kind || c.layer!=p.layer || c.role!=p.role || c.bank!=p.bank || c.firstRow!=p.firstRow || c.rows!=p.rows
    || c.rowBytes!=p.rowBytes || c.offset+canonical->Base!=p.proposedAddress || c.bytes!=p.bytes || c.allocated!=p.allocated)return false;
   if(p.kind!=PIONEER_Q38_WEIGHT || p.tensor>=tensors.size())continue;
   if(p.layer==64 && !p.role && token>=p.firstRow && token-p.firstRow<p.rows){
    unsigned offset=(token-p.firstRow)/16*46080,first=offset/4096,within=offset%4096;
    std::vector<uint8_t> data(((within+46080+4095)/4096)*4096);
    for(unsigned page=0;page<data.size()/4096;page++)if(!original.ReadWeightPage(plan,i,first+page,data.data()+page*4096,4096))return false;
    memcpy(embedding.data(),data.data()+within,46080);embeddingCount++;embeddingTensor=p.tensor;
   }
   if(p.layer || (p.role!=0 && p.role!=5 && p.role!=6 && p.role!=10 && p.role!=11 && p.role!=12))continue;
   Stripe s={p.role,PioneerQ38ProjectionRoute(p.bank),p.proposedAddress,p.bytes,std::vector<uint8_t>(p.allocated)};
   for(unsigned page=0;page<p.allocated/4096;page++)if(!original.ReadWeightPage(plan,i,page,s.data.data()+page*4096,4096))return false;
   pages+=p.allocated/4096;
   if(!p.role){
    if(normTensor!=UINT32_MAX || c.type || p.rowBytes!=20480 || p.bytes!=20480 || p.allocated!=20480)return false;
    normTensor=p.tensor;for(unsigned j=0;j<5120;j++){const auto* b=s.data.data()+j*4;norm[j]=uint32_t(b[0])|(uint32_t(b[1])<<8)|(uint32_t(b[2])<<16)|(uint32_t(b[3])<<24);}
   }else{
    if(p.role==12){
     if(c.type || p.rowBytes!=16 || p.allocated!=24576 || tensors[p.tensor].name!="blk.0.ssm_conv1d.weight")return false;
     convCount++;convRows+=p.rows;
    }else{if(c.type!=8 || p.rowBytes!=5760)return false;projectionsCount++;}
    stripes.push_back(std::move(s));
   }
  }
  if(embeddingCount!=1 || normTensor==UINT32_MAX || projectionsCount!=16 || convCount!=7 || convRows!=10240 || pages!=23223)return false;
  Reference reference{*this};
  if(!PioneerQ38EmbeddingReference(reference,embeddingTensor,token,raw.data())
   || !PioneerQ38LearnedRmsReference(reference,normTensor,raw.data(),normalized.data()))return false;
  for(unsigned role:{10U,11U,5U,6U})if(!PioneerQ38ProjectionReference(reference,role,normalized.data(),5120,
   projections.data()+(PioneerQ38ProjectionResultOffset(role)-TT_Q38_PROJECTION_RESULT_BASE)/4,PioneerQ38ProjectionResultRows(role)))return false;
  if(!PioneerQ38ResidentDecoderReference(reference,projections.data()+(PioneerQ38ProjectionResultOffset(6)-TT_Q38_PROJECTION_RESULT_BASE)/4,
   10240,convolution.data(),40960) || referenceReads!=1672 || !original.Ready())return false;
  prepared=true;return Ready();
 }
 bool SealBeforeNativeContext()override{if(!Ready() || sealed || referenceReads!=1672 || projectionReads || convolutionReads)return false;sealed=true;return true;}
 const uint8_t* EmbeddingTile()const override{return Ready()?embedding.data():nullptr;}
 const uint32_t* LearnedNorm()const override{return Ready()?norm.data():nullptr;}
 bool Read(unsigned route,uint32_t address,uint8_t* out,unsigned bytes)override{
  if(!sealed || !Ready() || !out || (bytes!=256 && bytes!=576) || (address&63))return false;
  for(const auto& s:stripes)if(s.route==route && address>=s.address && address-s.address<s.bytes){
   unsigned relative=address-s.address,tile=s.role==12?256:576;
   if(bytes!=tile || relative%tile || bytes>s.bytes-relative)return false;
   memcpy(out,s.data.data()+relative,bytes);if(s.role==12)convolutionReads++;else projectionReads++;
   return Ready();
  }
  return false; // No synthetic/partial/padding/foreign-address/numeric fallback
 }
 bool CheckEmbeddingRms(const std::vector<uint32_t>& workspace)const override{
  return sealed && Ready() && workspace.size()==524288 && !memcmp(workspace.data(),normalized.data(),20480)
   && !memcmp(workspace.data()+TT_Q38_RMS_BACKUP/4,raw.data(),20480);
 }
 bool CheckProjection(unsigned role,const uint32_t* values)const override{
  unsigned n=PioneerQ38ProjectionResultRows(role);return sealed && Ready() && values && n
   && !memcmp(values,projections.data()+(PioneerQ38ProjectionResultOffset(role)-TT_Q38_PROJECTION_RESULT_BASE)/4,n*4);
 }
 bool CheckConvolution(const uint32_t* values)const override{
  if(!sealed || !Ready() || !values)return false;
  for(unsigned i=0;i<10240;i++){
   float actual=PioneerAttentionFloat(values[i]),expected=PioneerAttentionFloat(convolution[i]);
   if(!isfinite(actual) || fabs(double(actual)-expected)>3e-6*(1+fabs(double(expected))))return false;
  }
  return !memcmp(values+10240,convolution.data()+10240,122880);
 }
 uint64_t ReferenceReads()const override{return referenceReads;}
 uint64_t ProjectionReads()const override{return projectionReads;}
 uint64_t ConvolutionReads()const override{return convolutionReads;}
};
}
std::unique_ptr<DecoderOriginalSource> OpenDecoderOriginalSource(const char* path){
 if(!path || !*path)return nullptr;originalPath=path;auto source=std::make_unique<OriginalSource>();
 if(!source->Open())return nullptr;return source;
}
