/* SPDX-License-Identifier: MIT */
#if defined(PIONEER_Q38_RESIDENT_OPERATOR_IMAGE) || defined(PIONEER_Q38_RESIDENT_GDN_OPERATOR_IMAGE) || defined(PIONEER_Q38_RESIDENT_COHORT_ROOT_IMAGE) || defined(PIONEER_Q38_RESIDENT_GDN_OUTPUT_OPERATOR_IMAGE)
#error Original-model observer remains separate from resident operator IMAGE
#endif
#ifdef PIONEER_Q38_RESIDENT_SILU_PRECISION_NATIVE
#error Original-model observer remains separate from native66
#endif
#ifdef PIONEER_Q38_NEXT_FFN_RESIDUAL_NATIVE
#error Original-model observer remains separate from native65
#endif
#ifdef PIONEER_Q38_CONV_SILU_PRECISION_NATIVE
#error Original-model CPU observer never compiles into native64
#endif
#if defined(PIONEER_Q38_NEXT_RESIDUAL_RMS_NATIVE)
#error Original residual CPU observer is separate from native63
#endif
#if defined(PIONEER_Q38_NEXT_GDN_OUTPUT_NATIVE)
#error Original output CPU observer is separate from native62
#endif
#if defined(PIONEER_Q38_NEXT_GDN_NATIVE) || defined(PIONEER_Q38_NEXT_CONV_NATIVE) || defined(PIONEER_Q38_NEXT_PROJECTION_NATIVE) || defined(PIONEER_Q38_NEXT_RMS_NATIVE) || defined(PIONEER_Q38_FFN_RESIDUAL_NATIVE) || defined(PIONEER_Q38_RESIDENT_DECODER_NATIVE) || defined(PIONEER_Q38_RESIDENT_DECODER_OWNER) || defined(PIONEER_Q38_EMBEDDING_NATIVE) || defined(PIONEER_Q38_PROJECTION_EXECUTION_OWNER) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(_KERNEL_MODE)
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
#include "../PioneerQ38NextProjectionValidationReference.h"
#define PIONEER_Q38_NEXT_GDN_OUTPUT_VALIDATION_REFERENCE 1
#include "../PioneerQ38NextGdnOutputValidationReference.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38CanonicalModelPlan.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ConvShape.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38LearnedRmsShape.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38GdnOutputGraphShape.h"
#include <array>
namespace {
class OriginalSource final:public DecoderOriginalSource {
 struct Stripe {unsigned role,layer,route;uint32_t address,bytes;std::vector<uint8_t> data;};
 PioneerQ38PublicModelSource original;PioneerQwen38PackedPlan plan;
 std::vector<Stripe> stripes;bool prepared=false,sealed=false,attempted=false,gdnInputs=false,gdnReferencesReady=false,gdnRecurrent=false,gdnNorm=false,gdnNative=false,gdnOutput=false,residualRms=false,ffn=false,nextRms=false;
 uint64_t referenceReads=0,projectionReads=0,convolutionReads=0,gdnInputReads=0,gdnNormReads=0;
 uint64_t outputReads=0;
 uint64_t residualReads=0;std::array<uint32_t,5120> postNorm={};
 uint64_t ffnReads=0;
 uint64_t nextRmsReads=0;std::array<uint32_t,5120> nextNorm={};
 bool nextProjection=false;uint64_t nextProjectionReads=0;
 bool nextConv=false;uint64_t nextConvReads=0;
 bool nextGdn=false;uint64_t nextGdnReads=0;
 bool nextOutput=false;uint64_t nextOutputReads=0;
 bool nextResidualRms=false;uint64_t nextResidualReads=0;std::array<uint32_t,5120> nextPostNorm={};
 bool nextFfn=false;uint64_t nextFfnReads=0;
 std::array<uint32_t,48> nextNegativeA={},nextDtBias={};std::array<uint32_t,128> nextGdnNorm={};
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
 explicit OriginalSource(bool gdn=false,bool recurrent=false,bool gated=false,bool native=false,bool output=false,bool residual=false,bool selectedFfn=false,bool next=false,bool projectionsNext=false,bool convolutionNext=false,bool gdnNext=false,bool outputNext=false,bool residualNext=false,bool ffnNext=false):gdnInputs(gdn),gdnRecurrent(recurrent),gdnNorm(gated),gdnNative(native),gdnOutput(output),residualRms(residual),ffn(selectedFfn),nextRms(next),nextProjection(projectionsNext),nextConv(convolutionNext),nextGdn(gdnNext),nextOutput(outputNext),nextResidualRms(residualNext),nextFfn(ffnNext){}
 bool Open(){return original.Open() && original.Ready();}
 bool Ready()const override{return prepared && original.Ready() && plan.MatchesSource(original.Model());}
 bool Prepare(unsigned token)override{
  if(attempted || sealed || token>=248320 || !original.Ready())return false;
  attempted=true;if(!plan.Build(original.Model(),8192,true,true))return false;
  auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();if(!canonical->Build() || canonical->Count()!=plan.Parts().size())return false;
  unsigned projectionsCount=0,convCount=0,convRows=0,embeddingCount=0,normTensor=UINT32_MAX,embeddingTensor=UINT32_MAX,pages=0,gdnParts=0,gdnNormTensor=UINT32_MAX,outputStripes=0,outputRows=0,postNorms=0;
  unsigned ffnStripes[3]={},ffnRows[3]={},nextNorms=0;
  unsigned nextFfnStripes[3]={},nextFfnRows[3]={};
  unsigned nextStripes[4]={},nextRows[4]={};
  unsigned nextConvStripes=0,nextConvRows=0,nextGdnParts=0;
 unsigned nextOutputStripes=0,nextOutputRows=0,nextPostNorms=0;
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
   if(nextFfn && p.layer==1 && p.role>=2 && p.role<=4){
    unsigned role=p.role-2;const char* names[]={"blk.1.ffn_gate.weight","blk.1.ffn_up.weight","blk.1.ffn_down.weight"};
    if(c.type!=12 || p.rowBytes!=(role==2?9792U:2880U) || tensors[p.tensor].name!=names[role]
     || p.firstRow!=nextFfnRows[role] || !p.rows || p.rows%16 || p.bytes!=p.rows*p.rowBytes || !p.allocated || p.allocated%4096)return false;
    Stripe s={p.role,p.layer,PioneerQ38ProjectionRoute(p.bank),p.proposedAddress,p.bytes,std::vector<uint8_t>(p.allocated)};
    for(unsigned page=0;page<p.allocated/4096;page++)if(!original.ReadWeightPage(plan,i,page,s.data.data()+page*4096,4096))return false;
    pages+=p.allocated/4096;nextFfnStripes[role]++;nextFfnRows[role]+=p.rows;stripes.push_back(std::move(s));continue;
   }
   if(nextResidualRms && p.layer==1 && p.role==1){
    if(c.type || p.firstRow || p.rows!=1 || p.rowBytes!=20480 || p.bytes!=20480 || p.allocated!=20480
     || tensors[p.tensor].name!="blk.1.post_attention_norm.weight")return false;
    Stripe s={p.role,p.layer,PioneerQ38ProjectionRoute(p.bank),p.proposedAddress,p.bytes,std::vector<uint8_t>(p.allocated)};
    for(unsigned page=0;page<5;page++)if(!original.ReadWeightPage(plan,i,page,s.data.data()+page*4096,4096))return false;
    for(unsigned j=0;j<5120;j++){const auto* b=s.data.data()+j*4;nextPostNorm[j]=uint32_t(b[0])|(uint32_t(b[1])<<8)|(uint32_t(b[2])<<16)|(uint32_t(b[3])<<24);}
    pages+=5;nextPostNorms++;stripes.push_back(std::move(s));continue;
   }
   if(nextOutput && p.layer==1 && p.role==13){
    if(c.type!=8 || p.rowBytes!=6912 || p.firstRow!=nextOutputRows || !p.rows || p.rows%16 || p.bytes!=p.rows*6912
     || !p.allocated || p.allocated%4096 || tensors[p.tensor].name!="blk.1.ssm_out.weight")return false;
    Stripe s={p.role,p.layer,PioneerQ38ProjectionRoute(p.bank),p.proposedAddress,p.bytes,std::vector<uint8_t>(p.allocated)};
    for(unsigned page=0;page<p.allocated/4096;page++)if(!original.ReadWeightPage(plan,i,page,s.data.data()+page*4096,4096))return false;
    pages+=p.allocated/4096;nextOutputStripes++;nextOutputRows+=p.rows;stripes.push_back(std::move(s));continue;
   }
   if(nextGdn && p.layer==1 && p.role>=7 && p.role<=9){
    unsigned bytes=p.role==9?512:192,bit=1U<<(p.role-7);
    const char* names[]={"blk.1.ssm_a","blk.1.ssm_dt.bias","blk.1.ssm_norm.weight"};
    if((nextGdnParts&bit) || c.type || p.firstRow || p.rows!=1 || p.rowBytes!=bytes || p.bytes!=bytes || p.allocated!=4096
     || tensors[p.tensor].name!=names[p.role-7])return false;
    Stripe s={p.role,p.layer,PioneerQ38ProjectionRoute(p.bank),p.proposedAddress,p.bytes,std::vector<uint8_t>(p.allocated)};
    if(!original.ReadWeightPage(plan,i,0,s.data.data(),4096))return false;
    uint32_t* values=p.role==7?nextNegativeA.data():p.role==8?nextDtBias.data():nextGdnNorm.data();
    for(unsigned j=0;j<bytes/4;j++){const auto* b=s.data.data()+j*4;values[j]=uint32_t(b[0])|(uint32_t(b[1])<<8)|(uint32_t(b[2])<<16)|(uint32_t(b[3])<<24);}
    pages++;nextGdnParts|=bit;stripes.push_back(std::move(s));continue;
   }
   if(nextConv && p.layer==1 && p.role==12){
    if(c.type || p.rowBytes!=16 || p.allocated!=24576 || p.firstRow!=nextConvRows || !p.rows || p.bytes!=p.rows*16
     || tensors[p.tensor].name!="blk.1.ssm_conv1d.weight")return false;
    Stripe s={p.role,p.layer,PioneerQ38ProjectionRoute(p.bank),p.proposedAddress,p.bytes,std::vector<uint8_t>(p.allocated)};
    for(unsigned page=0;page<6;page++)if(!original.ReadWeightPage(plan,i,page,s.data.data()+page*4096,4096))return false;
    pages+=6;nextConvStripes++;nextConvRows+=p.rows;stripes.push_back(std::move(s));continue;
   }
   if(nextProjection && p.layer==1 && PioneerQ38ProjectionResultRows(p.role)){
    unsigned slot=PioneerQ38ProjectionResultSlot(p.role);PioneerQ38Tensor expected;
    if(!PioneerQ38Expected(1,p.role,expected) || tensors[p.tensor].name!=expected.name || c.type!=8 || p.rowBytes!=5760)return false;
    Stripe s={p.role,p.layer,PioneerQ38ProjectionRoute(p.bank),p.proposedAddress,p.bytes,std::vector<uint8_t>(p.allocated)};
    for(unsigned page=0;page<p.allocated/4096;page++)if(!original.ReadWeightPage(plan,i,page,s.data.data()+page*4096,4096))return false;
    pages+=p.allocated/4096;nextStripes[slot]++;nextRows[slot]+=p.rows;stripes.push_back(std::move(s));continue;
   }
   if(nextRms && p.layer==1 && !p.role){
    if(c.type || p.firstRow || p.rows!=1 || p.rowBytes!=20480 || p.bytes!=20480 || p.allocated!=20480 || tensors[p.tensor].name!="blk.1.attn_norm.weight")return false;
    Stripe s={p.role,p.layer,PioneerQ38ProjectionRoute(p.bank),p.proposedAddress,p.bytes,std::vector<uint8_t>(p.allocated)};
    for(unsigned page=0;page<5;page++)if(!original.ReadWeightPage(plan,i,page,s.data.data()+page*4096,4096))return false;
    for(unsigned j=0;j<5120;j++){const auto* b=s.data.data()+j*4;nextNorm[j]=uint32_t(b[0])|(uint32_t(b[1])<<8)|(uint32_t(b[2])<<16)|(uint32_t(b[3])<<24);}
    pages+=5;nextNorms++;stripes.push_back(std::move(s));continue;
   }
   if(p.layer || (p.role!=0 && p.role!=5 && p.role!=6 && p.role!=10 && p.role!=11 && p.role!=12
    && (!gdnInputs || (p.role!=7 && p.role!=8)) && (!gdnNorm || p.role!=9) && (!gdnOutput || p.role!=13) && (!residualRms || p.role!=1)
    && (!ffn || p.role<2 || p.role>4)))continue;
   Stripe s={p.role,p.layer,PioneerQ38ProjectionRoute(p.bank),p.proposedAddress,p.bytes,std::vector<uint8_t>(p.allocated)};
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
   || pages!=(gdnInputs?23225U:23223U)+(gdnNorm?1U:0U)+(gdnOutput?8640U:0U)+(residualRms?5U:0U)+(ffn?36720U:0U)+(nextRms?5U:0U)+(nextProjection?23176U:0U)+(nextConv?42U:0U)+(nextGdn?3U:0U)+(nextOutput?8640U:0U)+(nextResidualRms?5U:0U)+(nextFfn?36720U:0U) || gdnParts!=(gdnInputs?2U:0U)
   || outputStripes!=(gdnOutput?7U:0U) || outputRows!=(gdnOutput?5120U:0U) || postNorms!=(residualRms?1U:0U)
   || gdnNorm!=(gdnNormTensor!=UINT32_MAX) || nextNorms!=(nextRms?1U:0U)
   || nextConvStripes!=(nextConv?7U:0U) || nextConvRows!=(nextConv?10240U:0U) || nextGdnParts!=(nextGdn?7U:0U)
   || nextOutputStripes!=(nextOutput?7U:0U) || nextOutputRows!=(nextOutput?5120U:0U) || nextPostNorms!=(nextResidualRms?1U:0U))return false;
  for(unsigned role=0;role<3;role++)if(ffnStripes[role]!=(ffn?7U:0U) || ffnRows[role]!=(ffn?(role==2?5120U:17408U):0U))return false;
  for(unsigned role=0;role<3;role++)if(nextFfnStripes[role]!=(nextFfn?7U:0U) || nextFfnRows[role]!=(nextFfn?(role==2?5120U:17408U):0U))return false;
  for(unsigned slot=0;slot<4;slot++)if(nextStripes[slot]!=(nextProjection?(slot<2?1U:7U):0U)
   || nextRows[slot]!=(nextProjection?(slot<2?48U:slot==2?6144U:10240U):0U))return false;
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
   if(s.layer==1){
    if(s.role>=2 && s.role<=4){
     unsigned at=address-s.address;if(!nextFfn || bytes!=2304 || at%2304 || bytes>s.bytes-at)return false;
     memcpy(out,s.data.data()+at,bytes);nextFfnReads++;return Ready();
    }
    if(s.role==1){
     unsigned at=address-s.address;if(!nextResidualRms || s.bytes!=20480 || (at==0?bytes!=16384:at!=16384 || bytes!=4096))return false;
     memcpy(out,s.data.data()+at,bytes);nextResidualReads++;return Ready();
    }
    if(s.role==13){
     unsigned at=address-s.address;if(!nextOutput || bytes!=576 || at%576 || bytes>s.bytes-at)return false;
     memcpy(out,s.data.data()+at,bytes);nextOutputReads++;return Ready();
    }
    if(s.role>=7 && s.role<=9){
     unsigned at=address-s.address,tile=s.role==9?512:64;
     if(!nextGdn || bytes!=tile || at%tile || bytes>s.bytes-at)return false;
     memcpy(out,s.data.data()+at,bytes);nextGdnReads++;return Ready();
    }
    if(s.role==12){
     unsigned at=address-s.address;if(!nextConv || bytes!=256 || at%256 || bytes>s.bytes-at)return false;
     memcpy(out,s.data.data()+at,bytes);nextConvReads++;return Ready();
    }
    if(s.role){unsigned at=address-s.address;if(!nextProjection || !PioneerQ38ProjectionResultRows(s.role) || bytes!=576 || at%576 || bytes>s.bytes-at)return false;
     memcpy(out,s.data.data()+at,bytes);nextProjectionReads++;return Ready();}
    unsigned at=address-s.address;if(!nextRms || s.role || s.bytes!=20480 || (at==0?bytes!=16384:at!=16384 || bytes!=4096))return false;
    memcpy(out,s.data.data()+at,bytes);nextRmsReads++;return Ready();
   }
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
 bool NextRmsWeightsReady()const override{return nextRms && ffn && sealed && Ready();}
 uint64_t NextRmsReads()const override{return nextRmsReads;}
 bool NextProjectionWeightsReady()const override{return nextProjection && nextRms && sealed && Ready();}
 uint64_t NextProjectionReads()const override{return nextProjectionReads;}
 bool NextConvWeightsReady()const override{return nextConv && nextProjection && sealed && Ready();}
 uint64_t NextConvReads()const override{return nextConvReads;}
 bool NextGdnWeightsReady()const override{return nextGdn && NextConvWeightsReady();}
 bool NextGdnOutputWeightsReady()const override{return nextOutput && NextGdnWeightsReady();}
 uint64_t NextGdnOutputReads()const override{return nextOutputReads;}
 bool NextResidualRmsWeightsReady()const override{return nextResidualRms && NextGdnOutputWeightsReady();}
 bool NextFfnWeightsReady()const override{return nextFfn && NextResidualRmsWeightsReady();}
 uint64_t NextFfnReads()const override{return nextFfnReads;}
 bool CheckNextFfnResidual(const std::vector<uint32_t>& before,const std::vector<uint32_t>& after)const override{
  if(!NextFfnWeightsReady() || nextFfnReads!=65280 || before.size()!=524288 || after.size()!=524288)return false;
  for(unsigned i=0;i<5120;i++){
   float raw=PioneerAttentionFloat(before[TT_Q38_RMS_BACKUP/4+i]),down=PioneerAttentionFloat(after[274880/4+i]);
   if(!isfinite(raw) || !isfinite(down) || after[i]!=PioneerAttentionBits(raw+down))return false;
  }
  return Ready(); // Actual matrix quantizer/math have separate emitted-byte observers.
 }
 uint64_t NextResidualRmsReads()const override{return nextResidualReads;}
 bool CheckNextResidualRms(const std::vector<uint32_t>& before,const std::vector<uint32_t>& after)const override{
  if(!NextResidualRmsWeightsReady() || nextResidualReads!=2 || before.size()!=524288 || after.size()!=524288)return false;
  // Independent post-return observer. Original norm bytes were cached BEFORE
  // context;none of these products are provided to the native controller.
  std::array<float,5120> sum;double squares=0;
  for(unsigned i=0;i<5120;i++){
   float value=PioneerAttentionFloat(before[TT_Q38_RMS_BACKUP/4+i])+PioneerAttentionFloat(before[TT_Q38_GDN_OUTPUT_RESULT/4+i]);
   if(!isfinite(value) || after[TT_Q38_RMS_BACKUP/4+i]!=PioneerAttentionBits(value))return false;
   sum[i]=value;squares+=double(value*value);
  }
  float scale=1.f/sqrtf(float(squares/5120)+1e-6f);
  for(unsigned i=0;i<5120;i++){
   float expected=(sum[i]*scale)*PioneerAttentionFloat(nextPostNorm[i]);
   if(!isfinite(expected) || after[i]!=PioneerAttentionBits(expected))return false;
  }
  return Ready();
 }
 bool CheckNextGdnOutput(const std::vector<uint32_t>& before,const std::vector<uint32_t>& after)override{
  if(!NextGdnOutputWeightsReady() || nextOutputReads!=61440 || before.size()!=524288 || after.size()!=524288)return false;
  const unsigned gated=TT_Q38_GDN_GATED_OUTPUT/4,result=TT_Q38_GDN_OUTPUT_RESULT/4;
  if(memcmp(before.data()+gated,after.data()+gated,24576) || memcmp(before.data()+0x18000/4,after.data()+0x18000/4,20480))return false;
  struct OutputReference {
   OriginalSource& parent;bool Ready()const{return parent.NextGdnOutputWeightsReady();}
   const auto& Model()const{return parent.original.Model();}
   bool ReadReferencePacked(unsigned tensor,uint64_t offset,unsigned bytes,void* out,unsigned capacity){
    return Ready() && parent.original.ReadReferencePacked(tensor,offset,bytes,out,capacity) && Ready();
   }
  } reference{*this};
  auto expected=std::make_unique<PioneerQ38NextGdnOutputReferenceResult>();
  return PioneerQ38NextGdnOutputReference(reference,before.data()+gated,6144,expected.get())
   && !memcmp(expected->operand,after.data()+TT_Q38_PROJECTION_NATIVE_OFFSET/4,12288)
   && !memcmp(expected->scale,after.data()+TT_Q38_PROJECTION_COMPACT_OFFSET/4,768)
   && !memcmp(expected->values,after.data()+result,20480) && Ready();
 }
 uint64_t NextGdnReads()const override{return nextGdnReads;}
 bool CheckNextGdn(unsigned half,const std::vector<uint32_t>& before,const uint32_t* preparedValues,
  const uint32_t* nextValues,const uint32_t* rawValues,const uint32_t* gatedValues)const override{
  if(!NextGdnWeightsReady() || half>=96 || before.size()!=524288 || !preparedValues || !nextValues || !rawValues || !gatedValues)return false;
  unsigned head=half/2,keyHead=head%16;
  const auto* conv=before.data()+TT_Q38_CONV_OUTPUT/4;
  float qs=0,ks=0;for(unsigned i=0;i<128;i++){
   float q=PioneerAttentionFloat(conv[keyHead*128+i]),k=PioneerAttentionFloat(conv[2048+keyHead*128+i]);qs+=q*q;ks+=k*k;
  }
  float qi=1.f/sqrtf(qs+1e-6f),ki=1.f/sqrtf(ks+1e-6f);
  std::array<float,128> q={},k={},rawHead={};std::vector<float> expectedState(8192);
  for(unsigned i=0;i<128;i++){q[i]=PioneerAttentionFloat(conv[keyHead*128+i])*qi;k[i]=PioneerAttentionFloat(conv[2048+keyHead*128+i])*ki;}
  float shifted=PioneerAttentionFloat(before[PioneerQ38ProjectionResultOffset(10)/4+head])+PioneerAttentionFloat(nextDtBias[head]);
  double x=shifted;float softplus=float(x>0?x+log1p(exp(-x)):log1p(exp(x)));
  float lambda=expf(PioneerAttentionFloat(nextNegativeA[head])*softplus);
  double b=PioneerAttentionFloat(before[PioneerQ38ProjectionResultOffset(11)/4+head]),e=exp(-fabs(b));
  float beta=float(b>=0?1/(1+e):e/(1+e));
  auto close=[](uint32_t actual,float expected){return isfinite(expected) && isfinite(PioneerAttentionFloat(actual))
   && fabs(double(PioneerAttentionFloat(actual))-expected)<=3e-6*(1+fabs(double(expected)));};
  for(unsigned i=0;i<128;i++)if(!close(preparedValues[i],q[i]) || !close(preparedValues[128+i],k[i]))return false;
  for(unsigned value=0;value<128;value++){
   float v=PioneerAttentionFloat(conv[4096+head*128+value]),delta=v*beta,sum=0;
   for(unsigned key=0;key<128;key++){
    float updated=k[key]*delta;sum+=updated*q[key];
    if(value/64==half%2)expectedState[key*64+value%64]=updated;
   }
   rawHead[value]=sum*0.0883883461356163f;
   if(value/64==half%2 && (!close(preparedValues[256+value%64],v) || !close(rawValues[value%64],rawHead[value])))return false;
  }
  for(unsigned i=0;i<8192;i++)if(!close(nextValues[i],expectedState[i]))return false;
  if(!close(preparedValues[320],lambda) || !close(preparedValues[321],beta))return false;
  const float positive[]={lambda,beta};for(unsigned i=0;i<2;i++){
   float actual=PioneerAttentionFloat(preparedValues[320+i]),expected=positive[i];
   if(expected>=PioneerAttentionFloat(0x00800000)){if(actual<=0 || fabs(double(actual)-expected)>3e-6*expected)return false;}
   else if(actual!=0)return false;
  }
  float sum=0;for(float raw:rawHead)sum+=raw*raw;
  float inverse=1.f/sqrtf(sum*.0078125f+1e-6f);
  for(unsigned i=0;i<64;i++){
   unsigned index=(half%2)*64+i;
   float weighted=(rawHead[index]*inverse)*PioneerAttentionFloat(nextGdnNorm[index]);
   double z=PioneerAttentionFloat(before[PioneerQ38ProjectionResultOffset(5)/4+head*128+index]),n=exp(-fabs(z));
   float silu=float(z*(z>=0?1/(1+n):n/(1+n)));
   if(!close(gatedValues[i],weighted*silu))return false;
  }
  return Ready(); // independent original learned bytes/libm OBSERVER,never native products or readiness
 }
 bool CheckNextConv(const std::vector<uint32_t>& before,const std::vector<uint32_t>& after)override{
  if(!NextConvWeightsReady() || nextConvReads!=640 || before.size()!=524288 || after.size()!=524288)return false;
  const auto* tensor=original.Model().Find("blk.1.ssm_conv1d.weight");
  if(!tensor || tensor->type || tensor->rank!=2 || tensor->inputs!=4 || tensor->outputs!=10240 || tensor->bytes!=163840)return false;
  unsigned index=unsigned(tensor-original.Model().Tensors().data());std::vector<uint8_t> weights(163840);
  if(!original.ReadReferencePacked(index,0,unsigned(weights.size()),weights.data(),unsigned(weights.size())) || !Ready())return false;
  for(unsigned c=0;c<10240;c++){
   float weight[4];for(unsigned j=0;j<4;j++){
    const auto* b=weights.data()+c*16+j*4;weight[j]=PioneerAttentionFloat(uint32_t(b[0])|(uint32_t(b[1])<<8)|(uint32_t(b[2])<<16)|(uint32_t(b[3])<<24));
   }
   uint32_t input=before[PioneerQ38ProjectionResultOffset(6)/4+c];float sum=0;
   for(unsigned j=0;j<3;j++)sum+=0.f*weight[j];
   sum+=PioneerAttentionFloat(input)*weight[3];
   double x=sum,e=exp(-fabs(x)),expected=x*(x>=0?1/(1+e):e/(1+e));
   double actual=PioneerAttentionFloat(after[TT_Q38_CONV_OUTPUT/4+c]);
   if(!isfinite(expected) || !isfinite(actual) || fabs(actual-expected)>3e-6*(1+fabs(expected)))return false;
   for(unsigned j=0;j<3;j++)if(after[TT_Q38_CONV_NEXT/4+c*3+j]!=(j==2?input:0U))return false;
  }
  return Ready(); // independent ORIGINAL-weight observer only,never native inputs
 }
 bool CheckNextProjections(const std::vector<uint32_t>& before,const std::vector<uint32_t>& after)override{
  if(!NextProjectionWeightsReady() || nextProjectionReads!=164800 || before.size()!=524288 || after.size()!=524288)return false;
  if(memcmp(before.data(),after.data(),20480) || memcmp(before.data()+0x18000/4,after.data()+0x18000/4,20480))return false;
  struct NextReference {
   OriginalSource& parent;
   bool Ready()const{return parent.NextProjectionWeightsReady();}
   const auto& Model()const{return parent.original.Model();}
   bool ReadReferencePacked(unsigned tensor,uint64_t offset,unsigned bytes,void* out,unsigned capacity){
    return Ready() && parent.original.ReadReferencePacked(tensor,offset,bytes,out,capacity) && Ready();
   }
  } reference{*this};
  for(unsigned role:{10U,11U,5U,6U}){
   unsigned n=PioneerQ38ProjectionResultRows(role),offset=PioneerQ38ProjectionResultOffset(role)/4;
   std::vector<uint32_t> expected(n);
   if(!PioneerQ38NextProjectionReference(reference,role,before.data(),5120,expected.data(),n)
    || memcmp(expected.data(),after.data()+offset,n*4))return false;
  }return Ready();
 }
 bool CheckNextRms(const std::vector<uint32_t>& before,const std::vector<uint32_t>& after)const override{
  if(!NextRmsWeightsReady() || nextRmsReads!=2 || before.size()!=524288 || after.size()!=524288)return false;
  double sum=0;for(unsigned i=0;i<5120;i++){float x=PioneerAttentionFloat(before[i]);if(!isfinite(x))return false;sum+=double(x*x);}
  float scale=1.f/sqrtf(float(sum/5120)+1e-6f);
  for(unsigned i=0;i<5120;i++)if(after[TT_Q38_RMS_BACKUP/4+i]!=before[i]
   || after[i]!=PioneerAttentionBits((PioneerAttentionFloat(before[i])*scale)*PioneerAttentionFloat(nextNorm[i])))return false;
  return Ready(); // independent TEST observer,never a native input/result
 }
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
std::unique_ptr<DecoderOriginalSource> OpenNextRmsOriginalSource(const char* path){
 if(!path || !*path)return nullptr;originalPath=path;auto source=std::make_unique<OriginalSource>(true,true,true,true,true,true,true,true);
 if(!source->Open())return nullptr;return source;
}
std::unique_ptr<DecoderOriginalSource> OpenNextProjectionOriginalSource(const char* path){
 if(!path || !*path)return nullptr;originalPath=path;auto source=std::make_unique<OriginalSource>(true,true,true,true,true,true,true,true,true);
 if(!source->Open())return nullptr;return source;
}
std::unique_ptr<DecoderOriginalSource> OpenNextConvOriginalSource(const char* path){
 if(!path || !*path)return nullptr;originalPath=path;auto source=std::make_unique<OriginalSource>(true,true,true,true,true,true,true,true,true,true);
 if(!source->Open())return nullptr;return source;
}
std::unique_ptr<DecoderOriginalSource> OpenNextGdnOriginalSource(const char* path){
 if(!path || !*path)return nullptr;originalPath=path;auto source=std::make_unique<OriginalSource>(true,true,true,true,true,true,true,true,true,true,true);
 if(!source->Open())return nullptr;return source;
}
std::unique_ptr<DecoderOriginalSource> OpenNextGdnOutputOriginalSource(const char* path){
 if(!path || !*path)return nullptr;originalPath=path;auto source=std::make_unique<OriginalSource>(true,true,true,true,true,true,true,true,true,true,true,true);
 if(!source->Open())return nullptr;return source;
}
std::unique_ptr<DecoderOriginalSource> OpenNextResidualRmsOriginalSource(const char* path){
 if(!path || !*path)return nullptr;originalPath=path;auto source=std::make_unique<OriginalSource>(true,true,true,true,true,true,true,true,true,true,true,true,true);
 if(!source->Open())return nullptr;return source;
}
std::unique_ptr<DecoderOriginalSource> OpenNextFfnResidualOriginalSource(const char* path){
 if(!path || !*path)return nullptr;originalPath=path;auto source=std::make_unique<OriginalSource>(true,true,true,true,true,true,true,true,true,true,true,true,true,true);
 if(!source->Open())return nullptr;return source;
}
