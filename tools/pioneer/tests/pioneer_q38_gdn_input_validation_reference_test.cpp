/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_GDN_INPUT_VALIDATION_CLIENT 1
#define PIONEER_Q38_CPU_VALIDATION 1
#define PIONEER_Q38_LEARNED_RMS_VALIDATION_REFERENCE 1
#include "pioneer_q38_projection_validation_fixture.h"
#include "../PioneerQ38GdnInputValidationReference.h"
struct GdnReferenceSource:ProjectionValidationSource {
 std::vector<PioneerQ38Tensor> tensors;unsigned drop=0;
 explicit GdnReferenceSource(FILE* f):ProjectionValidationSource(f),tensors(model.Tensors()){}
 const auto& Model()const{return *this;}const auto& Tensors()const{return tensors;}
 const PioneerQ38Tensor* Find(const char* name)const{for(const auto& t:tensors)if(t.name==name)return &t;return nullptr;}
 bool ReadReferencePacked(unsigned tensor,uint64_t offset,unsigned bytes,void* out,unsigned capacity){
  bool okay=ProjectionValidationSource::ReadReferencePacked(tensor,offset,bytes,out,capacity);
  if(referenceReads==drop)valid=false;return okay;
 }
};
int main(){
 setvbuf(stdout,nullptr,_IOLBF,0);Fixture fixture;SeedProjectionValidationEmbedding(fixture,0);
 std::vector<uint8_t> learned[2]={std::vector<uint8_t>(192),std::vector<uint8_t>(192)};
 for(unsigned role=0;role<2;role++){
  for(unsigned h=0;h<48;h++){
   uint32_t bits=PioneerAttentionBits(role?-3.5f:-float(h+1)*.25f);
   for(unsigned b=0;b<4;b++)learned[role][h*4+b]=uint8_t(bits>>(8*b));
  }
  for(const auto& t:fixture.expected)if(t.name==(role?"blk.0.ssm_dt.bias":"blk.0.ssm_a"))fixture.Write(t.offset,learned[role]);
 }
 GdnReferenceSource source(fixture.file);std::vector<uint32_t> conv(40960),alpha(48),beta(48),output(30912,0xdeadbeef);
 for(unsigned i=0;i<10240;i++)conv[i]=PioneerAttentionBits(float(sin(i*.071)*2.7));
 for(unsigned h=0;h<48;h++){alpha[h]=PioneerAttentionBits(float(h)*.13f-2);beta[h]=PioneerAttentionBits(float(h)*.07f-2);}
 assert(PioneerQ38GdnInputReference(source,conv.data(),40960,alpha.data(),beta.data(),48,output.data(),30912) && source.referenceReads==2);
 for(unsigned half=0;half<96;half++){
  unsigned h=half>>1,key=h%16;float qs=0,ks=0;
  for(unsigned i=0;i<128;i++){float q=PioneerAttentionFloat(conv[key*128+i]),k=PioneerAttentionFloat(conv[2048+key*128+i]);qs+=q*q;ks+=k*k;}
  float qi=(1.f/sqrtf(qs+1e-6f))*0.0883883461356163f,ki=1.f/sqrtf(ks+1e-6f);
  for(unsigned i=0;i<128;i++){
   assert(output[half*322+i]==PioneerAttentionBits(PioneerAttentionFloat(conv[key*128+i])*qi));
   assert(output[half*322+128+i]==PioneerAttentionBits(PioneerAttentionFloat(conv[2048+key*128+i])*ki));
  }
  for(unsigned i=0;i<64;i++)assert(output[half*322+256+i]==conv[4096+h*128+(half&1)*64+i]);
  double logit=PioneerAttentionFloat(beta[h]),e=exp(-fabs(logit));float shifted=PioneerAttentionFloat(alpha[h])-3.5f;
  float sp=float(log1p(exp(double(shifted)))),exponent=(-float(h+1)*.25f)*sp;
  assert(fabs(PioneerAttentionFloat(output[half*322+320])-exp(double(exponent)))<=3e-6*exp(double(exponent)));
  assert(fabs(PioneerAttentionFloat(output[half*322+321])-(logit>=0?1/(1+e):e/(1+e)))<3e-6);
 }
 unsigned refused=0;
 auto refuse=[&](const uint32_t* in,const uint32_t* a,const uint32_t* b,uint32_t* out,unsigned n=40960,unsigned h=48,unsigned words=30912){
  std::fill(output.begin(),output.end(),0xdeadbeef);auto before=conv;
  assert(!PioneerQ38GdnInputReference(source,in,n,a,b,h,out,words) && conv==before);
  for(auto v:output)assert(v==0xdeadbeef);refused++;
 };
 refuse(nullptr,alpha.data(),beta.data(),output.data());refuse(conv.data(),nullptr,beta.data(),output.data());
 refuse(conv.data(),alpha.data(),nullptr,output.data());refuse(conv.data(),alpha.data(),beta.data(),nullptr);
 refuse(conv.data(),alpha.data(),beta.data(),conv.data());refuse(conv.data(),alpha.data(),beta.data(),output.data(),40959);
 refuse(conv.data(),alpha.data(),beta.data(),output.data(),40960,47);refuse(conv.data(),alpha.data(),beta.data(),output.data(),40960,48,30911);
 for(uintptr_t p:{uintptr_t(1),UINTPTR_MAX-3}){
  refuse(reinterpret_cast<const uint32_t*>(p),alpha.data(),beta.data(),output.data());
  refuse(conv.data(),reinterpret_cast<const uint32_t*>(p),beta.data(),output.data());
  refuse(conv.data(),alpha.data(),reinterpret_cast<const uint32_t*>(p),output.data());
  refuse(conv.data(),alpha.data(),beta.data(),reinterpret_cast<uint32_t*>(p));
 }
 source.valid=false;refuse(conv.data(),alpha.data(),beta.data(),output.data());source.valid=true;
 for(unsigned role=0;role<2;role++){
  unsigned tensor=unsigned(source.Find(role?"blk.0.ssm_dt.bias":"blk.0.ssm_a")-source.tensors.data());
  for(unsigned field=0;field<6;field++){
   auto saved=source.tensors[tensor];auto& t=source.tensors[tensor];
   if(field==0)t.name="foreign";if(field==1)t.type=8;if(field==2)t.rank=2;if(field==3)t.inputs--;if(field==4)t.outputs--;if(field==5)t.bytes--;
   refuse(conv.data(),alpha.data(),beta.data(),output.data());source.tensors[tensor]=saved;
  }
  for(uint32_t bits:{0x7f800000U,0x7fc00000U,0x00000001U,0x477fe001U}){
   auto bad=learned[role];for(unsigned b=0;b<4;b++)bad[47*4+b]=uint8_t(bits>>(8*b));
   fixture.Write(source.tensors[tensor].offset,bad);assert(source.model.Open(source.file));
   refuse(conv.data(),alpha.data(),beta.data(),output.data());
  }
  fixture.Write(source.tensors[tensor].offset,learned[role]);assert(source.model.Open(source.file));
  if(!role)for(uint32_t bits:{0U,0x80000000U,0x3f800000U}){
   auto bad=learned[role];for(unsigned b=0;b<4;b++)bad[47*4+b]=uint8_t(bits>>(8*b));
   fixture.Write(source.tensors[tensor].offset,bad);assert(source.model.Open(source.file));
   refuse(conv.data(),alpha.data(),beta.data(),output.data());
  }
  fixture.Write(source.tensors[tensor].offset,learned[role]);assert(source.model.Open(source.file));
 }
 for(unsigned at:{0U,10239U})for(uint32_t bad:{0x7f800000U,0x7fc00000U}){
  uint32_t saved=conv[at];conv[at]=bad;refuse(conv.data(),alpha.data(),beta.data(),output.data());conv[at]=saved;
 }
 for(auto* v:{&alpha,&beta})for(uint32_t bad:{0x7f800000U,0x7fc00000U,0x477fe001U}){
  uint32_t saved=(*v)[47];(*v)[47]=bad;refuse(conv.data(),alpha.data(),beta.data(),output.data());(*v)[47]=saved;
 }
 for(unsigned at:{1U,2U}){
  source.referenceReads=0;source.failReference=at;refuse(conv.data(),alpha.data(),beta.data(),output.data());source.failReference=0;
  source.referenceReads=0;source.drop=at;refuse(conv.data(),alpha.data(),beta.data(),output.data());source.drop=0;source.valid=true;
 }
 printf("PASS independent GDN DEBUG reference ALL96halves/30912outputs/tiled-key h%%16/original negativeA-dt/relative normal gates/refusals%u/atomic publication;SYNTHETIC_NO_DEVICE_OR_SPEED\n",refused);
}
