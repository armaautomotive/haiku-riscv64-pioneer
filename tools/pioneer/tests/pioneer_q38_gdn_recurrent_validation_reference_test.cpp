/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_GDN_RECURRENT_VALIDATION_CLIENT 1
#define PIONEER_Q38_CPU_VALIDATION 1
#define PIONEER_Q38_LEARNED_RMS_VALIDATION_REFERENCE 1
#include "../PioneerQ38GdnRecurrentValidationReference.h"
#include <cassert>
#include <cstdio>
struct RecurrentReferenceSource {
 struct Tensor{std::string name;unsigned type=0,rank=1,inputs=48,outputs=1,bytes=192;};
 struct ModelType {
  std::vector<Tensor> tensors={{"blk.0.ssm_a"},{"blk.0.ssm_dt.bias"}};
  const Tensor* Find(const char* name)const{for(const auto& t:tensors)if(t.name==name)return &t;return nullptr;}
  const auto& Tensors()const{return tensors;}
 } model;
 unsigned reads=0,failRead=0;bool ready=true;float a=-1,dt=0;
 bool Ready()const{return ready;}const auto& Model()const{return model;}
 bool ReadReferencePacked(unsigned tensor,uint64_t offset,unsigned bytes,void* out,unsigned capacity){
  assert(tensor<2 && !offset && bytes==192 && capacity==192);
  if(++reads==failRead)return false;
  for(unsigned i=0;i<48;i++){uint32_t value=PioneerAttentionBits(tensor?dt:a);memcpy(static_cast<uint8_t*>(out)+i*4,&value,4);}return ready;
 }
};
int main(){
 setvbuf(stdout,nullptr,_IOLBF,0);std::vector<uint32_t> conv(40960),alpha(48),beta(48),output(823488,0xdeadbeef);
 unsigned positives=0,refusals=0;
 for(unsigned dense=0;dense<2;dense++){
  std::fill(conv.begin(),conv.end(),0);
  for(unsigned h=0;h<16;h++)for(unsigned i=0;i<(dense?128U:1U);i++){
   conv[h*128+i]=PioneerAttentionBits(dense?1.f:3.f);conv[2048+h*128+i]=PioneerAttentionBits(dense?1.f:4.f);
  }
  for(unsigned h=0;h<48;h++)for(unsigned i=0;i<128;i++)conv[4096+h*128+i]=PioneerAttentionBits(float(int((h*128+i)%17)-8)*.25f);
  RecurrentReferenceSource source;
  assert(PioneerQ38GdnRecurrentReference(source,conv.data(),40960,alpha.data(),beta.data(),48,output.data(),823488) && source.reads==2);
  // Closed-form independent FP64 zero-CURRENT outer product. Explicit Q
  // near1 in one-hot case catches accidental pre-scaling/double scaling.
  double q=dense?1/sqrt(128.+1e-6):3/sqrt(9.+1e-6),k=dense?1/sqrt(128.+1e-6):4/sqrt(16.+1e-6);
  for(unsigned half=0;half<96;half++){
   const auto* result=output.data()+half*8578;
   assert(fabs(PioneerAttentionFloat(result[0])-q)<3e-7 && fabs(PioneerAttentionFloat(result[128])-k)<3e-7);
   assert(PioneerAttentionFloat(result[320])==.5f && PioneerAttentionFloat(result[321])==.5f);
   for(unsigned value=0;value<64;value++){
    double v=PioneerAttentionFloat(conv[4096+(half/2)*128+(half&1)*64+value]),sum=0;
    for(unsigned i=0;i<128;i++){
     double expected=(dense || !i)?k*v*.5:0;
     assert(fabs(PioneerAttentionFloat(result[322+i*64+value])-expected)<3e-7*(1+fabs(expected)));
     sum+=expected*((dense || !i)?q:0);
    }
    double expected=sum/sqrt(128.);
    assert(fabs(PioneerAttentionFloat(result[8514+value])-expected)<3e-6*(1+fabs(expected)));
   }
  }
  positives++;
 }
 for(unsigned kind=0;kind<13;kind++){
  RecurrentReferenceSource source;auto c=conv,a=alpha,b=beta;std::fill(output.begin(),output.end(),0xdeadbeef);
  unsigned words=823488,cwords=40960,heads=48;const uint32_t* in=c.data();uint32_t* out=output.data();
  if(kind==0)words--;if(kind==1)cwords--;if(kind==2)heads--;if(kind==3)in=nullptr;if(kind==4)source.ready=false;
  if(kind==5)source.failRead=1;if(kind==6)source.failRead=2;if(kind==7)source.a=0;
  if(kind==8)c[2047]=0x7fc00000;if(kind==9)a[47]=0x7f800000;if(kind==10)b[47]=0xff800000;
  if(kind==11)source.model.tensors[0].bytes=191;if(kind==12)source.model.tensors[1].rank=2;
  assert(!PioneerQ38GdnRecurrentReference(source,in,cwords,a.data(),b.data(),heads,out,words));
  for(auto value:output)assert(value==0xdeadbeef);refusals++;
 }
 puts("PASS recurrent DEBUG independent nonzero ONE_HOT+DENSE/ALL823488values/FP64 closed-form observer/UNSCALED_Q_once/13atomic refusals;NO GPU/reference uploads/Work fallback");
 assert(positives==2 && refusals==13);
}
