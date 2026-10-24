/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_CLIENT 1
#define PIONEER_Q38_CPU_VALIDATION 1
#define PIONEER_Q38_LEARNED_RMS_VALIDATION_REFERENCE 1
#include "../PioneerQ38GdnRendezvousValidationReference.h"
#include <cassert>
#include <cstdio>
struct TimedReferenceSource {
 struct Tensor{std::string name;unsigned type=0,rank=1,inputs=48,outputs=1,bytes=192;};
 struct ModelType {
  std::vector<Tensor> tensors={{"blk.0.ssm_a"},{"blk.0.ssm_dt.bias"},{"blk.0.ssm_norm.weight",0,1,128,1,512}};
  const Tensor* Find(const char* name)const{for(const auto& t:tensors)if(t.name==name)return &t;return nullptr;}
  const auto& Tensors()const{return tensors;}
 } model;
 unsigned reads=0,failRead=0;bool ready=true;float a=-1,dt=0,norm=1;
 bool Ready()const{return ready;}const auto& Model()const{return model;}
 bool ReadReferencePacked(unsigned tensor,uint64_t offset,unsigned bytes,void* out,unsigned capacity){
  assert(tensor<3 && !offset && bytes==(tensor==2?512U:192U) && capacity==bytes);
  if(++reads==failRead)return false;
  for(unsigned i=0;i<bytes/4;i++){
   uint32_t value=PioneerAttentionBits(tensor==2?norm:tensor?dt:a);memcpy(static_cast<uint8_t*>(out)+i*4,&value,4);
  }return ready;
 }
};
int main(){
 setvbuf(stdout,nullptr,_IOLBF,0);std::vector<uint32_t> conv(40960),alpha(48),beta(48),z(6144),output(829632,0xdeadbeef);
 unsigned positive=0,refusals=0;
 for(unsigned trial=0;trial<3;trial++){
  TimedReferenceSource source;if(trial==2)source.norm=65504;
  bool dense=trial==1;std::fill(conv.begin(),conv.end(),0);
  for(unsigned h=0;h<16;h++)for(unsigned i=0;i<(dense?128U:1U);i++){
   conv[h*128+i]=PioneerAttentionBits(dense?1.f:3.f);conv[2048+h*128+i]=PioneerAttentionBits(dense?1.f:4.f);
  }
  for(unsigned h=0;h<48;h++)for(unsigned i=0;i<128;i++){
   conv[4096+h*128+i]=PioneerAttentionBits(float(int((h*128+i)%17)-8)*.25f);
   z[h*128+i]=PioneerAttentionBits(trial==2?-100.f:float(int(i%9)-4)*.5f);
  }
  assert(PioneerQ38GdnRendezvousReference(source,conv.data(),40960,alpha.data(),beta.data(),48,z.data(),6144,output.data(),829632) && source.reads==3);
  double q=dense?1/sqrt(128.+1e-6):3/sqrt(9.+1e-6),k=dense?1/sqrt(128.+1e-6):4/sqrt(16.+1e-6);
  for(unsigned head=0;head<48;head++){
   double raw[128],sum=0;
   for(unsigned i=0;i<128;i++){
    double v=PioneerAttentionFloat(conv[4096+head*128+i]);raw[i]=k*v*.5*q*(dense?128:1)/sqrt(128.);sum+=raw[i]*raw[i];
   }
   for(unsigned i=0;i<128;i++){
    unsigned half=head*2+i/64;const auto* result=output.data()+half*8642;
    assert(fabs(PioneerAttentionFloat(result[8514+i%64])-raw[i])<3e-6*(1+fabs(raw[i])));
    double gate=PioneerAttentionFloat(z[head*128+i]);
    double expected=raw[i]/sqrt(sum/128+1e-6)*source.norm*gate/(1+exp(-gate));
    double actual=PioneerAttentionFloat(result[8578+i%64]);
    if(fabs(expected)>=PioneerAttentionFloat(0x00800000))assert(fabs(actual-expected)<=3e-6*fabs(expected));
    else assert(actual==0);
   }
  }positive++;
 }
 for(unsigned kind=0;kind<15;kind++){
  TimedReferenceSource source;auto c=conv,a=alpha,b=beta,g=z;std::fill(output.begin(),output.end(),0xdeadbeef);
  unsigned words=829632,zWords=6144;const uint32_t* in=g.data();uint32_t* out=output.data();
  if(kind==0)words--;if(kind==1)zWords--;if(kind==2)in=nullptr;if(kind==3)source.ready=false;
  if(kind==4)source.failRead=1;if(kind==5)source.failRead=2;if(kind==6)source.failRead=3;
  if(kind==7)source.norm=INFINITY;if(kind==8)source.model.tensors[2].bytes=511;
  if(kind==9)source.model.tensors[2].rank=2;if(kind==10)g[6143]=0x7fc00000;
  if(kind==11)out=c.data();if(kind==12)source.a=0;if(kind==13)a[47]=0x7f800000;if(kind==14)c[2047]=0x7fc00000;
  assert(!PioneerQ38GdnRendezvousReference(source,c.data(),40960,a.data(),b.data(),48,in,zWords,out,words));
  for(auto value:output)assert(value==0xdeadbeef);refusals++;
 }
 assert(positive==3 && refusals==15);
 puts("PASS DEBUG52 independent ONE_HOT/DENSE/large-weight-z-100 NORMAL gated outputs/ALL829632values/FP64 closed-form observer/15atomic refusals;NO GPU or reference uploads");
}
