/* SPDX-License-Identifier: MIT */
#if defined(PIONEER_Q38_FFN_RESIDUAL_NATIVE) || defined(PIONEER_Q38_RESIDENT_DECODER_NATIVE) || defined(PIONEER_Q38_RESIDENT_DECODER_OWNER) || defined(PIONEER_Q38_EMBEDDING_NATIVE) || defined(PIONEER_Q38_PROJECTION_EXECUTION_OWNER) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(_KERNEL_MODE)
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
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38GdnOutputGraphShape.h"
#include <array>
namespace {
class OriginalSource final:public DecoderOriginalSource {
 struct Stripe {unsigned role,route;uint32_t address,bytes;std::vector<uint8_t> data;};
 PioneerQ38PublicModelSource original;PioneerQwen38PackedPlan plan;
 std::vector<Stripe> stripes;bool prepared=false,sealed=false,attempted=false,gdnInputs=false,gdnReferencesReady=false,gdnRecurrent=false,gdnNorm=false,gdnNative=false,gdnOutput=false,residualRms=false,ffn=false;
 uint64_t referenceReads=0,projectionReads=0,convolutionReads=0,gdnInputReads=0,gdnNormReads=0;
 uint64_t outputReads=0;
 uint64_t residualReads=0;std::array<uint32_t,5120> postNorm={};
 uint64_t ffnReads=0;
 std::array<uint32_t,48> negativeA={},dtBias={};std::vector<uint32_t> gdnReferences;
 std::vector<uint32_t> recurrentReferences; // ALL references computed BEFORE context,never fed into controller
 std::array<uint32_t,128> gdnLearnedNorm={};std::vector<uint32_t> gdnNormReferences;
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
 explicit OriginalSource(bool gdn=false,bool recurrent=false,bool gated=false,bool native=false,bool output=false,bool residual=false,bool selectedFfn=false):gdnInputs(gdn),gdnRecurrent(recurrent),gdnNorm(gated),gdnNative(native),gdnOutput(output),residualRms(residual),ffn(selectedFfn){}
 bool Open(){return original.Open() && original.Ready();}
 bool Ready()const override{return prepared && original.Ready() && plan.MatchesSource(original.Model());}
 bool Prepare(unsigned token)override{
  if(attempted || sealed || token>=248320 || !original.Ready())return false;
  attempted=true;if(!plan.Build(original.Model(),8192,true,true))return false;
  auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();if(!canonical->Build() || canonical->Count()!=plan.Parts().size())return false;
  unsigned projectionsCount=0,convCount=0,convRows=0,embeddingCount=0,normTensor=UINT32_MAX,embeddingTensor=UINT32_MAX,pages=0,gdnParts=0,gdnNormTensor=UINT32_MAX,outputStripes=0,outputRows=0,postNorms=0;
  unsigned ffnStripes[3]={},ffnRows[3]={};
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
   if(p.layer || (p.role!=0 && p.role!=5 && p.role!=6 && p.role!=10 && p.role!=11 && p.role!=12
    && (!gdnInputs || (p.role!=7 && p.role!=8)) && (!gdnNorm || p.role!=9) && (!gdnOutput || p.role!=13) && (!residualRms || p.role!=1)
    && (!ffn || p.role<2 || p.role>4)))continue;
   Stripe s={p.role,PioneerQ38ProjectionRoute(p.bank),p.proposedAddress,p.bytes,std::vector<uint8_t>(p.allocated)};
   for(unsigned page=0;page<p.allocated/4096;page++)if(!original.ReadWeightPage(plan,i,page,s.data.data()+page*4096,4096))return false;
   pages+=p.allocated/4096;
   if(!p.role){
    if(normTensor!=UINT32_MAX || c.type || p.rowBytes!=20480 || p.bytes!=20480 || p.allocated!=20480)return false;
    normTensor=p.tensor;for(unsigned j=0;j<5120;j++){const auto* b=s.data.data()+j*4;norm[j]=uint32_t(b[0])|(uint32_t(b[1])<<8)|(uint32_t(b[2])<<16)|(uint32_t(b[3])<<24);}
   }else{
    if(p.role==1){
     if(!residualRms || c.type || p.firstRow || p.rows!=1 || p.rowBytes!=20480 || p.bytes!=20480 || p.allocated!=20480
      || tensors[p.tensor].name!="blk.0.post_attention_norm.weight")return false;
     for(unsigned j=0;j<5120;j++){const auto* b=s.data.data()+j*4;postNorm[j]=uint32_t(b[0])|(uint32_t(b[1])<<8)|(uint32_t(b[2])<<16)|(uint32_t(b[3])<<24);}postNorms++;
    }else if(p.role>=2 && p.role<=4){
     unsigned role=p.role-2;const char* names[]={"blk.0.ffn_gate.weight","blk.0.ffn_up.weight","blk.0.ffn_down.weight"};
     if(!ffn || c.type!=12 || p.rowBytes!=(role==2?9792U:2880U) || tensors[p.tensor].name!=names[role])return false;
     ffnStripes[role]++;ffnRows[role]+=p.rows;
    }else if(p.role==13){
     if(!gdnOutput || c.type!=8 || p.rowBytes!=6912 || tensors[p.tensor].name!="blk.0.ssm_out.weight")return false;
     outputStripes++;outputRows+=p.rows;
    }else if(p.role==9){
     if(!gdnNorm || gdnNormTensor!=UINT32_MAX || c.type || p.firstRow || p.rows!=1 || p.rowBytes!=512 || p.bytes!=512 || p.allocated!=4096
      || tensors[p.tensor].name!="blk.0.ssm_norm.weight")return false;
     gdnNormTensor=p.tensor;
     for(unsigned j=0;j<128;j++){const auto* b=s.data.data()+j*4;gdnLearnedNorm[j]=uint32_t(b[0])|(uint32_t(b[1])<<8)|(uint32_t(b[2])<<16)|(uint32_t(b[3])<<24);}
    }else if(p.role==7 || p.role==8){
     if(!gdnInputs || c.type || p.firstRow || p.rows!=1 || p.rowBytes!=192 || p.bytes!=192 || p.allocated!=4096
      || tensors[p.tensor].name!=(p.role==7?"blk.0.ssm_a":"blk.0.ssm_dt.bias"))return false;
     auto& values=p.role==7?negativeA:dtBias;
     for(unsigned j=0;j<48;j++){const auto* b=s.data.data()+j*4;values[j]=uint32_t(b[0])|(uint32_t(b[1])<<8)|(uint32_t(b[2])<<16)|(uint32_t(b[3])<<24);}
     gdnParts++;
    }else if(p.role==12){
     if(c.type || p.rowBytes!=16 || p.allocated!=24576 || tensors[p.tensor].name!="blk.0.ssm_conv1d.weight")return false;
     convCount++;convRows+=p.rows;
    }else{if(c.type!=8 || p.rowBytes!=5760)return false;projectionsCount++;}
    stripes.push_back(std::move(s));
   }
  }
  if(embeddingCount!=1 || normTensor==UINT32_MAX || projectionsCount!=16 || convCount!=7 || convRows!=10240
   || pages!=(gdnInputs?23225U:23223U)+(gdnNorm?1U:0U)+(gdnOutput?8640U:0U)+(residualRms?5U:0U)+(ffn?36720U:0U) || gdnParts!=(gdnInputs?2U:0U)
   || outputStripes!=(gdnOutput?7U:0U) || outputRows!=(gdnOutput?5120U:0U) || postNorms!=(residualRms?1U:0U)
   || gdnNorm!=(gdnNormTensor!=UINT32_MAX))return false;
  for(unsigned role=0;role<3;role++)if(ffnStripes[role]!=(ffn?7U:0U) || ffnRows[role]!=(ffn?(role==2?5120U:17408U):0U))return false;
  Reference reference{*this};
  if(!PioneerQ38EmbeddingReference(reference,embeddingTensor,token,raw.data())
   || !PioneerQ38LearnedRmsReference(reference,normTensor,raw.data(),normalized.data()))return false;
  for(unsigned role:{10U,11U,5U,6U})if(!PioneerQ38ProjectionReference(reference,role,normalized.data(),5120,
   projections.data()+(PioneerQ38ProjectionResultOffset(role)-TT_Q38_PROJECTION_RESULT_BASE)/4,PioneerQ38ProjectionResultRows(role)))return false;
  if(!PioneerQ38ResidentDecoderReference(reference,projections.data()+(PioneerQ38ProjectionResultOffset(6)-TT_Q38_PROJECTION_RESULT_BASE)/4,
   10240,convolution.data(),40960) || referenceReads!=1672 || !original.Ready())return false;
  if(gdnInputs){
   // Independent BEFORE-context oracle,not the card math policy. Actual
   // emitted QCB1 outputs will later be read through separate NoC endpoints.
   std::array<float,2048> q={},k={},unscaled={};
   for(unsigned h=0;h<16;h++){
    float qs=0,ks=0;
    for(unsigned i=0;i<128;i++){float a=PioneerAttentionFloat(convolution[h*128+i]),b=PioneerAttentionFloat(convolution[2048+h*128+i]);qs+=a*a;ks+=b*b;}
    float inverse=1.0f/sqrtf(qs+1e-6f),qi=inverse*0.0883883461356163f,ki=1.0f/sqrtf(ks+1e-6f);
    for(unsigned i=0;i<128;i++){q[h*128+i]=PioneerAttentionFloat(convolution[h*128+i])*qi;k[h*128+i]=PioneerAttentionFloat(convolution[2048+h*128+i])*ki;}
    if(gdnRecurrent)for(unsigned i=0;i<128;i++)unscaled[h*128+i]=PioneerAttentionFloat(convolution[h*128+i])*inverse;
   }
   gdnReferences.resize(96*322);
   for(unsigned half=0;half<96;half++){
    unsigned h=half>>1,key=h%16;auto* out=gdnReferences.data()+half*322;
    for(unsigned i=0;i<128;i++){out[i]=PioneerAttentionBits(q[key*128+i]);out[128+i]=PioneerAttentionBits(k[key*128+i]);}
    for(unsigned i=0;i<64;i++)out[256+i]=convolution[4096+h*128+(half&1)*64+i];
    float shifted=PioneerAttentionFloat(projections[h])+PioneerAttentionFloat(dtBias[h]);
    double x=shifted;float softplus=float(x>0?x+log1p(exp(-x)):log1p(exp(x)));
    float exponent=PioneerAttentionFloat(negativeA[h])*softplus;
    double b=PioneerAttentionFloat(projections[1024+h]),e=exp(-fabs(b));
    out[320]=PioneerAttentionBits(expf(exponent));out[321]=PioneerAttentionBits(float(b>=0?1/(1+e):e/(1+e)));
    for(unsigned i=0;i<322;i++)if((out[i]&0x7f800000)==0x7f800000)return false;
   }
   gdnReferencesReady=true;
   if(gdnRecurrent){
    recurrentReferences.resize(96*(322+8192+64));
    for(unsigned half=0;half<96;half++){
     auto* expected=recurrentReferences.data()+half*(322+8192+64);const auto* prep=gdnReferences.data()+half*322;
     for(unsigned i=0;i<322;i++)expected[i]=i<128?PioneerAttentionBits(unscaled[(half/2%16)*128+i]):prep[i];
     float beta=PioneerAttentionFloat(expected[321]);
     for(unsigned value=0;value<64;value++){
      float delta=PioneerAttentionFloat(expected[256+value])*beta,output=0;
      for(unsigned key=0;key<128;key++){
       float updated=PioneerAttentionFloat(expected[128+key])*delta;
       expected[322+key*64+value]=PioneerAttentionBits(updated);
       output+=updated*PioneerAttentionFloat(expected[key]);
      }
      expected[322+8192+value]=PioneerAttentionBits(output*0.0883883461356163f);
     }
    }
   }
  }
  if(gdnNorm){
   if(!gdnRecurrent || recurrentReferences.size()!=96*8578)return false;
   uint8_t originalNorm[512];
   if(!reference.ReadReferencePacked(gdnNormTensor,0,512,originalNorm,512) || referenceReads!=1673
    || memcmp(originalNorm,gdnLearnedNorm.data(),512))return false;
   gdnNormReferences.resize(6144);
   for(unsigned head=0;head<48;head++){
    float raw[128],sum=0;
    for(unsigned i=0;i<128;i++){raw[i]=PioneerAttentionFloat(recurrentReferences[(head*2+i/64)*8578+8514+i%64]);sum+=raw[i]*raw[i];}
    float inverse=1.f/sqrtf(sum*.0078125f+1e-6f);
    for(unsigned i=0;i<128;i++){
     float weighted=(raw[i]*inverse)*PioneerAttentionFloat(gdnLearnedNorm[i]);
     double z=PioneerAttentionFloat(projections[(PioneerQ38ProjectionResultOffset(5)-TT_Q38_PROJECTION_RESULT_BASE)/4+head*128+i]);
     double e=exp(-fabs(z));float silu=float(z*(z>=0?1/(1+e):e/(1+e)));
     float value=weighted*silu;if(!isfinite(value))return false;
     gdnNormReferences[head*128+i]=PioneerAttentionBits(value);
    }
   }
  }
  prepared=true;return Ready();
 }
 bool SealBeforeNativeContext()override{if(!Ready() || sealed || referenceReads!=(gdnNorm?1673U:1672U) || projectionReads || convolutionReads || gdnInputReads
  || (gdnInputs && !gdnReferencesReady) || (gdnNorm && gdnNormReferences.size()!=6144))return false;sealed=true;return true;}
 const uint8_t* EmbeddingTile()const override{return Ready()?embedding.data():nullptr;}
 const uint32_t* LearnedNorm()const override{return Ready()?norm.data():nullptr;}
 const uint32_t* GdnLearnedNorm()const override{return gdnNorm && Ready()?gdnLearnedNorm.data():nullptr;}
 bool CheckGdnGatedNorm(unsigned half,const uint32_t* values)const override{
  if(!gdnNorm || !sealed || !Ready() || half>=96 || !values || gdnNormReferences.size()!=6144)return false;
  for(unsigned i=0;i<64;i++){
   double value=PioneerAttentionFloat(values[i]),expected=PioneerAttentionFloat(gdnNormReferences[half*64+i]);
   if(!isfinite(value) || fabs(value-expected)>3e-6*(1+fabs(expected)))return false;
  }
  return true;
 }
 bool Read(unsigned route,uint32_t address,uint8_t* out,unsigned bytes)override{
  if(!sealed || !Ready() || !out || (bytes!=256 && bytes!=576 && (!gdnInputs || bytes!=64) && (!gdnNative || bytes!=512)
   && (!residualRms || (bytes!=16384 && bytes!=4096)) && (!ffn || bytes!=2304)) || (address&63))return false;
  for(const auto& s:stripes)if(s.route==route && address>=s.address && address-s.address<s.bytes){
   if(s.role==1){
    unsigned at=address-s.address;if(!residualRms || s.bytes!=20480 || (at==0?bytes!=16384:at!=16384 || bytes!=4096))return false;
    memcpy(out,s.data.data()+at,bytes);residualReads++;return Ready();
   }
   if(s.role>=2 && s.role<=4){
    unsigned at=address-s.address;if(!ffn || bytes!=2304 || at%2304 || bytes>s.bytes-at)return false;
    memcpy(out,s.data.data()+at,bytes);ffnReads++;return Ready();
   }
   unsigned relative=address-s.address,tile=s.role==12?256:(s.role==7 || s.role==8)?64:s.role==9?512:576;
   if(s.role==9 && !gdnNative)return false;
   if(bytes!=tile || relative%tile || bytes>s.bytes-relative)return false;
   memcpy(out,s.data.data()+relative,bytes);if(s.role==12)convolutionReads++;
   else if(s.role==7 || s.role==8)gdnInputReads++;else if(s.role==9)gdnNormReads++;else if(s.role==13)outputReads++;else projectionReads++;
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
 uint64_t GdnInputReads()const override{return gdnInputReads;}
 uint64_t GdnNormReads()const override{return gdnNormReads;}
 bool GdnOutputWeightsReady()const override{return gdnOutput && sealed && Ready();}
 bool ResidualRmsWeightsReady()const override{return residualRms && sealed && Ready();}
 uint64_t ResidualRmsReads()const override{return residualReads;}
 bool FfnWeightsReady()const override{return ffn && residualRms && sealed && Ready();}
 uint64_t FfnReads()const override{return ffnReads;}
 bool CheckFfnResidual(const std::vector<uint32_t>& before,const std::vector<uint32_t>& after)const override{
  if(!FfnWeightsReady() || ffnReads!=65280 || before.size()!=524288 || after.size()!=524288)return false;
  for(unsigned i=0;i<5120;i++){
   float raw=PioneerAttentionFloat(before[TT_Q38_RMS_BACKUP/4+i]),down=PioneerAttentionFloat(after[274880/4+i]);
   if(!isfinite(raw) || !isfinite(down) || after[i]!=PioneerAttentionBits(raw+down))return false;
  }
  return Ready(); // actual peer math has separate independent FP32/FP64 observers
 }
 bool CheckResidualRms(const std::vector<uint32_t>& before,const std::vector<uint32_t>& after)const override{
  if(!ResidualRmsWeightsReady() || before.size()!=524288 || after.size()!=524288)return false;
  std::array<float,5120> residual;double squares=0;
  for(unsigned i=0;i<5120;i++){
   float x=PioneerAttentionFloat(before[TT_Q38_RMS_BACKUP/4+i])+PioneerAttentionFloat(before[TT_Q38_GDN_OUTPUT_RESULT/4+i]);
   if(!isfinite(x) || after[TT_Q38_RMS_BACKUP/4+i]!=PioneerAttentionBits(x))return false;
   residual[i]=x;squares+=double(x*x);
  }
  float scale=1.f/sqrtf(float(squares/5120)+1e-6f);
  for(unsigned i=0;i<5120;i++){
   float expected=(residual[i]*scale)*PioneerAttentionFloat(postNorm[i]);
   if(!isfinite(expected) || after[i]!=PioneerAttentionBits(expected))return false;
  }
  return Ready();
 }
 uint64_t GdnOutputReads()const override{return outputReads;}
 bool CheckGdnInput(unsigned half,const uint32_t* values)const override{
  if(!gdnInputs || !gdnReferencesReady || !sealed || !Ready() || half>=96 || !values)return false;
  const auto* reference=gdnReferences.data()+half*322;
  for(unsigned i=0;i<322;i++){
   double actual=PioneerAttentionFloat(values[i]),expected=PioneerAttentionFloat(reference[i]);
   if(!isfinite(actual) || fabs(actual-expected)>3e-6*(1+fabs(expected)))return false;
   if(i>=320){
    if(expected>=double(PioneerAttentionFloat(0x00800000))){if(actual<=0 || fabs(actual-expected)>3e-6*expected)return false;}
    else if(actual!=0)return false; // explicit native FP32 FTZ only
   }
  }
  return true;
 }
 bool CheckGdnRecurrent(unsigned half,const uint32_t* prepared,const uint32_t* next,const uint32_t* output)const override{
  if(!gdnRecurrent || !sealed || !Ready() || half>=96 || !prepared || !next || !output || recurrentReferences.size()!=96*(322+8192+64))return false;
  const auto* expected=recurrentReferences.data()+half*(322+8192+64);
  const uint32_t* inputs[]={prepared,next,output};const unsigned words[]={322,8192,64},offsets[]={0,322,322+8192};
  for(unsigned group=0;group<3;group++)for(unsigned i=0;i<words[group];i++){
   double value=PioneerAttentionFloat(inputs[group][i]),reference=PioneerAttentionFloat(expected[offsets[group]+i]);
   if(!isfinite(value) || fabs(value-reference)>3e-6*(1+fabs(reference)))return false;
   if(!group && i>=320){
    if(reference>=double(PioneerAttentionFloat(0x00800000))){if(value<=0 || fabs(value-reference)>3e-6*reference)return false;}
    else if(value!=0)return false;
   }
  }
  return true; // independent libm/zero-state oracle,ALL prepared BEFORE native context
 }
};
}
std::unique_ptr<DecoderOriginalSource> OpenDecoderOriginalSource(const char* path){
 if(!path || !*path)return nullptr;originalPath=path;auto source=std::make_unique<OriginalSource>();
 if(!source->Open())return nullptr;return source;
}
std::unique_ptr<DecoderOriginalSource> OpenGdnInputOriginalSource(const char* path){
 if(!path || !*path)return nullptr;originalPath=path;auto source=std::make_unique<OriginalSource>(true);
 if(!source->Open())return nullptr;return source;
}
std::unique_ptr<DecoderOriginalSource> OpenGdnRecurrentOriginalSource(const char* path){
 if(!path || !*path)return nullptr;originalPath=path;auto source=std::make_unique<OriginalSource>(true,true);
 if(!source->Open())return nullptr;return source;
}
std::unique_ptr<DecoderOriginalSource> OpenGdnGatedNormOriginalSource(const char* path){
 if(!path || !*path)return nullptr;originalPath=path;auto source=std::make_unique<OriginalSource>(true,true,true);
 if(!source->Open())return nullptr;return source;
}
std::unique_ptr<DecoderOriginalSource> OpenGdnGatedNativeOriginalSource(const char* path){
 if(!path || !*path)return nullptr;originalPath=path;auto source=std::make_unique<OriginalSource>(true,true,true,true);
 if(!source->Open())return nullptr;return source;
}
std::unique_ptr<DecoderOriginalSource> OpenGdnOutputOriginalSource(const char* path){
 if(!path || !*path)return nullptr;originalPath=path;auto source=std::make_unique<OriginalSource>(true,true,true,true,true);
 if(!source->Open())return nullptr;return source;
}
std::unique_ptr<DecoderOriginalSource> OpenResidualRmsOriginalSource(const char* path){
 if(!path || !*path)return nullptr;originalPath=path;auto source=std::make_unique<OriginalSource>(true,true,true,true,true,true);
 if(!source->Open())return nullptr;return source;
}
std::unique_ptr<DecoderOriginalSource> OpenFfnResidualOriginalSource(const char* path){
 if(!path || !*path)return nullptr;originalPath=path;auto source=std::make_unique<OriginalSource>(true,true,true,true,true,true,true);
 if(!source->Open())return nullptr;return source;
}
