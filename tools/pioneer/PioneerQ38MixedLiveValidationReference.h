/* SPDX-License-Identifier: MIT */
// Deliberately before the guard: late role mixing must fail too.
#if !defined(PIONEER_Q38_MIXED_LIVE_VALIDATION_REFERENCE) || PIONEER_Q38_MIXED_LIVE_VALIDATION_REFERENCE != 1 \
 || !defined(PIONEER_Q38_CPU_VALIDATION) || PIONEER_Q38_CPU_VALIDATION != 1
#error Exact separate dense-block ORIGINAL CPU reference role required
#endif
#if defined(_KERNEL_MODE) || defined(__FAST_MATH__) || defined(PIONEER_ATTENTION_FREESTANDING) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) || defined(PIONEER_Q38_NATIVE_Q8_LAYOUT) \
 || defined(PIONEER_Q38_DECODER_RUNTIME) || defined(PIONEER_Q38_EMBEDDING_NATIVE) || defined(PIONEER_Q38_LEARNED_RMS_NATIVE) \
 || defined(PIONEER_Q38_MIXED_LIVE_NATIVE_CONTEXT) || defined(PIONEER_Q38_MIXED_LIVE_CONTEXT_COMPILE_ONLY) \
 || defined(PIONEER_Q38_MIXED_LIVE_EMIT_CONTEXT_BODY) || defined(PIONEER_Q38_MIXED_LIVE_VALIDATION_NATIVE) \
 || defined(PIONEER_Q38_MIXED_LIVE_VALIDATION_CLIENT) || defined(PIONEER_Q38_RESIDENT_LAYER_VALIDATION_REFERENCE) \
 || defined(PIONEER_Q38_RESIDENT_LAYER_VALIDATION_CLIENT) || defined(PIONEER_Q38_RESIDENT_LAYER_VALIDATION_NATIVE) \
 || defined(PIONEER_Q38_RESIDENT_LAYER_NATIVE_CONTEXT) || defined(PIONEER_Q38_RESIDENT_OUTPUT_OWNER) \
 || defined(PIONEER_Q38_PROJECTION_EXECUTION_OWNER) || defined(PIONEER_Q38_RESIDENT_DECODER_VALIDATION_CLIENT) \
 || defined(PIONEER_Q38_EMBEDDING_VALIDATION_CLIENT) || defined(PIONEER_Q38_LEARNED_RMS_VALIDATION_CLIENT) \
 || defined(PIONEER_Q38_PROJECTION_VALIDATION_CLIENT) || defined(PIONEER_Q38_GDN_INPUT_VALIDATION_CLIENT) \
 || defined(PIONEER_Q38_GDN_RECURRENT_VALIDATION_CLIENT) || defined(PIONEER_Q38_GDN_OUTPUT_VALIDATION_CLIENT) \
 || defined(PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_CLIENT) || defined(PIONEER_Q38_RESIDENT_OUTPUT_VALIDATION_CLIENT) \
 || defined(PIONEER_Q38_LEARNED_RMS_VALIDATION_REFERENCE) || defined(PIONEER_Q38_NEXT_GDN_OUTPUT_VALIDATION_REFERENCE) \
 || defined(PIONEER_Q38_MIXED_LIVE_BLOCK_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_BLOCK_ROOT_IMAGE) \
 || defined(PIONEER_Q38_MIXED_JOINED_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_PROJECTION_ROOT_IMAGE) \
 || defined(PIONEER_Q38_MIXED_DENSE_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_WORKER_IMAGE) \
 || defined(PIONEER_Q38_MIXED_CAPSULE_IMAGE) || defined(PIONEER_Q38_DENSE_PREPARE_IMAGE) \
 || defined(PIONEER_Q38_DENSE_ATTENTION_IMAGE) || defined(PIONEER_Q38_DENSE_PARTITION_IMAGE) \
 || defined(PIONEER_Q38_DENSE_PARTITION_ROOT_IMAGE) || defined(PIONEER_Q38_DENSE_PROJECTION_IMAGE) \
 || defined(PIONEER_Q38_RESIDENT_LAYER_ROOT_IMAGE) || defined(PIONEER_Q38_RESIDENT_LAYER_NORM_IMAGE) \
 || defined(PIONEER_Q38_RESIDENT_LAYER_FFN_IMAGE) || defined(PIONEER_Q38_RESIDENT_CONTINUE_IMAGE) \
 || defined(PIONEER_Q38_RESIDENT_ADVANCE_IMAGE) || defined(PIONEER_Q38_RESIDENT_LAYER_LINK_IMAGE)
#error Dense ORIGINAL CPU products must never enter native, image, old DEBUG, client, kernel or Work roles, including defined0
#endif
#ifndef PIONEER_Q38_MIXED_LIVE_VALIDATION_REFERENCE_H
#define PIONEER_Q38_MIXED_LIVE_VALIDATION_REFERENCE_H
#include <array>
#include <cfenv>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>
#include "PioneerQ38MixedLiveComparison.h"
// CPU-only ideal-libm observer for the fixed standalone layer3/position0
// operator, NOT a valid GDN0..2/model prefix. Compute BEFORE accelerator FD.
// Original row-major GGUF bytes only: no tiled/native quantizer, device math,
// input/output readback, ready/owner/producer or GPU callback is accepted.
struct PioneerQ38MixedLiveReferenceProducts {
 uint32_t embedding[5120],normalized[5120],qg[12288],key[1024],value[1024];
 uint32_t query[6144],gate[6144],halfKey[1024],halfValue[1024],attention[6144];
 uint32_t projection[5120],residual[5120],postNormalized[5120];
 uint32_t ffnGate[17408],ffnUp[17408],activated[17408],down[5120],finalHidden[5120];
};
static_assert(std::is_trivially_copyable<PioneerQ38MixedLiveReferenceProducts>::value,"atomic CPU-only publication");
namespace PioneerQ38MixedLiveReferenceDetail {
inline uint32_t Bits(float x){uint32_t b;std::memcpy(&b,&x,4);return b;}
inline float Float(uint32_t b){float x;std::memcpy(&x,&b,4);return x;}
inline bool Domain(uint32_t b){unsigned a=b&0x7fffffff;return !a || (a>=0x00800000 && a<0x7f800000);}
inline bool RmsDomain(uint32_t b){unsigned a=b&0x7fffffff;return !a || (a>=0x26800000 && a<=0x477fe000);}
inline float HalfFloat(unsigned h){
 unsigned e=(h>>10)&31,m=h&1023;
 float x=e==31?std::numeric_limits<float>::infinity():std::ldexp(float(e?1024+m:m),e?int(e)-25:-24);
 return std::copysign(x,h&0x8000?-1.f:1.f);
}
// Host libm scaling/nearbyint is independent of card integer bit rounding.
// It also works on Haiku targets without a native _Float16 arithmetic type.
inline uint16_t HalfRne(float x){
 unsigned sign=Bits(x)>>16&0x8000;float a=std::fabs(x);if(!a)return uint16_t(sign);
 if(!std::isfinite(a))return uint16_t(sign|0x7c00);
 int e=0;std::frexp(a,&e);
 if(e< -13)return uint16_t(sign|unsigned(std::nearbyint(std::ldexp(a,24))));
 if(e>16)return uint16_t(sign|0x7c00);
 unsigned q=unsigned(std::nearbyint(std::ldexp(a,11-e)));if(q==2048){q=1024;e++;}
 if(e>16)return uint16_t(sign|0x7c00);
 return uint16_t(sign|(unsigned(e+14)*1024+(q-1024)));
}
inline unsigned U16(const uint8_t* p){return unsigned(p[0])|(unsigned(p[1])<<8);}
inline int Signed(unsigned b){return int(b)-int((b&128)*2);}
inline unsigned Scale4(const uint8_t* p,unsigned g){return g<4?p[4+g]&63:(p[8+g]&15)|((p[g]>>6)<<4);}
inline unsigned Min4(const uint8_t* p,unsigned g){return g<4?p[8+g]&63:(p[8+g]>>4)|((p[4+g]>>6)<<4);}
inline int Quant(unsigned type,const uint8_t* p,unsigned i){
 if(type==8)return Signed(p[2+i]);
 if(type==12)return int((p[16+(i/64)*32+i%32]>>((i/32%2)*4))&15);
 unsigned h=i/128,g=i%128/32,l=i%32;
 return int(((p[h*64+(g%2)*32+l]>>((g/2)*4))&15)|(((p[128+h*32+l]>>(g*2))&3)<<4))-32;
}
inline bool Input(unsigned type,const uint32_t* input,unsigned n,std::vector<int>& quant,std::vector<float>& scales){
 if(!input || (type!=8 && type!=12 && type!=14) || !n || n>17408 || n%256 || std::fegetround()!=FE_TONEAREST)return false;
 uintptr_t at=reinterpret_cast<uintptr_t>(input);if((at&3) || at>UINTPTR_MAX-n*4)return false;
 const unsigned block=type==8?32:256;quant.assign(n,0);scales.assign(n/block,0);
 for(unsigned at=0;at<n;at+=block){
  float maximum=0,selected=0;
  for(unsigned i=0;i<block;i++){
   if(!Domain(input[at+i]))return false;
   float x=Float(input[at+i]);
   if(std::fabs(x)>maximum){maximum=std::fabs(x);selected=x;}
  }
  if(!maximum)continue;
  float inverse=type==8?1.f/(maximum/127.f):127.f/maximum;
  float scale=type==8?HalfFloat(HalfRne(maximum/127.f)):1.f/inverse;
  if(!Domain(Bits(inverse)) || !inverse || !Domain(Bits(scale)))return false;
  if(type!=8){if(scale<0x1p-32f || scale>65536.f)return false;inverse=std::copysign(inverse,-selected);scale=std::copysign(scale,-selected);}
  else if(scale>65536.f)return false;
  scales[at/block]=scale;
  for(unsigned i=0;i<block;i++){
   float x=Float(input[at+i])*inverse,q=type==8?std::round(x):std::nearbyint(x);
   if(!std::isfinite(q) || q<-127 || q>127)return false;
   quant[at+i]=int(q);
  }
 }return std::fegetround()==FE_TONEAREST;
}
// Capture ALL selected metadata before the FIRST read. Every read brackets
// source/model/epoch/vector identity and all12 tensor descriptors, including
// not-yet-used tensors. No changed source or late retarget is accepted.
template<class Source>class View {
 Source& source;const void* model=nullptr;const void* data=nullptr;uint64_t epoch=0;bool ready=false;
 struct Tensor {std::string name;unsigned index,type,rank,inputs,outputs;uint64_t bytes,offset;};
 std::array<Tensor,12> tensor;
public:
 explicit View(Source& s):source(s){}
 bool Bind(){
  if(ready || !source.Ready() || std::fegetround()!=FE_TONEAREST || source.Model().Tensors().size()!=851 || !source.Model().Epoch())return false;
  const char* names[]={"token_embd.weight","blk.3.attn_norm.weight","blk.3.post_attention_norm.weight","blk.3.attn_q.weight",
   "blk.3.attn_k.weight","blk.3.attn_v.weight","blk.3.attn_q_norm.weight","blk.3.attn_k_norm.weight","blk.3.attn_output.weight",
   "blk.3.ffn_gate.weight","blk.3.ffn_up.weight","blk.3.ffn_down.weight"};
  const unsigned type[]={12,0,0,8,8,8,0,0,14,12,12,12},inputs[]={5120,5120,5120,5120,5120,5120,256,256,6144,5120,5120,17408};
  const unsigned rows[]={248320,1,1,12288,1024,1024,1,1,5120,17408,17408,5120};
  model=&source.Model();data=source.Model().Tensors().data();epoch=source.Model().Epoch();
  for(unsigned i=0;i<12;i++){
   unsigned found=851;for(unsigned j=0;j<851;j++)if(source.Model().Tensors()[j].name==names[i]){if(found!=851)return false;found=j;}
   if(found==851)return false;
   const auto* p=&source.Model().Tensors()[found];
   unsigned block=type[i]==8?32:type[i]?256:1,bytes=type[i]==8?34:type[i]==12?144:type[i]==14?210:4;
   if(p->type!=type[i] || p->rank!=(type[i]?2U:1U) || p->inputs!=inputs[i] || p->outputs!=rows[i]
    || p->bytes!=uint64_t(inputs[i]/block)*bytes*rows[i])return false;
   tensor[i]={p->name,found,p->type,p->rank,unsigned(p->inputs),unsigned(p->outputs),p->bytes,p->offset};
  }
  ready=true;return Current();
 }
 bool Current()const{
  if(!ready || !source.Ready() || std::fegetround()!=FE_TONEAREST || &source.Model()!=model || source.Model().Epoch()!=epoch
   || source.Model().Tensors().size()!=851 || source.Model().Tensors().data()!=data)return false;
  for(const auto& t:tensor){const auto& p=source.Model().Tensors()[t.index];
   if(p.name!=t.name || p.type!=t.type || p.rank!=t.rank || p.inputs!=t.inputs || p.outputs!=t.outputs || p.bytes!=t.bytes || p.offset!=t.offset)return false;}
  return true;
 }
 bool Read(unsigned i,uint64_t offset,unsigned bytes,void* out){
  return i<12 && out && bytes && bytes<=1048576 && offset<=tensor[i].bytes && bytes<=tensor[i].bytes-offset && Current()
   && source.ReadReferencePacked(tensor[i].index,offset,bytes,out,bytes) && Current();
 }
 bool Weights(unsigned i,std::vector<uint32_t>& weight){
  if(i>=12 || tensor[i].type || tensor[i].outputs!=1)return false;
  std::vector<uint8_t> bytes(tensor[i].inputs*4);if(!Read(i,0,unsigned(bytes.size()),bytes.data()))return false;
  weight.resize(tensor[i].inputs);
  for(unsigned n=0;n<weight.size();n++){const auto* p=bytes.data()+n*4;weight[n]=uint32_t(p[0])|(uint32_t(p[1])<<8)|(uint32_t(p[2])<<16)|(uint32_t(p[3])<<24);if(!RmsDomain(weight[n]))return false;}
  return true;
 }
 bool Embedding(unsigned token,uint32_t* output){
  if(token>=248320 || !output)return false;
  std::array<uint8_t,2880> bytes;
  if(!Read(0,uint64_t(token)*2880,unsigned(bytes.size()),bytes.data()))return false;
  for(unsigned b=0;b<20;b++){
   const auto* p=bytes.data()+b*144;float d=HalfFloat(U16(p)),m=HalfFloat(U16(p+2));if(!std::isfinite(d) || !std::isfinite(m))return false;
   for(unsigned i=0;i<256;i++){float c=d*float(Scale4(p,i/32)),v=m*float(Min4(p,i/32));output[b*256+i]=Bits(c*float(Quant(12,p,i))-v);if(!RmsDomain(output[b*256+i]))return false;}
  }return Current();
 }
 bool Matrix(unsigned slot,const uint32_t* input,uint32_t* output){
  if((slot!=3 && slot!=4 && slot!=5 && slot!=8 && slot!=9 && slot!=10 && slot!=11) || !Current() || !input || !output)return false;
  const auto& t=tensor[slot];unsigned type=t.type;
  uintptr_t in=reinterpret_cast<uintptr_t>(input),out=reinterpret_cast<uintptr_t>(output);unsigned inBytes=t.inputs*4,outBytes=t.outputs*4;
  if((in&3) || (out&3) || in>UINTPTR_MAX-inBytes || out>UINTPTR_MAX-outBytes || (in<out+outBytes && out<in+inBytes))return false;
  if(type!=8 && type!=12 && type!=14)return false;
  const unsigned width=type==8?32:256,packed=type==8?34:type==12?144:210,blocks=t.inputs/width,rowBytes=blocks*packed;
  std::vector<int> q;std::vector<float> scale;if(!Input(type,input,t.inputs,q,scale))return false;
  std::vector<uint8_t> bytes(16*rowBytes);std::vector<uint32_t> stage(t.outputs);
  for(unsigned first=0;first<t.outputs;first+=16){
   if(!Read(slot,uint64_t(first)*rowBytes,unsigned(bytes.size()),bytes.data()))return false;
   float sum[16]={};
   for(unsigned b=0;b<blocks;b++)for(unsigned panel=0;panel<width/16;panel++){
    int total=0;for(unsigned lane=0;lane<16;lane++)total+=q[b*width+panel*16+lane];
    for(unsigned row=0;row<16;row++){
     const auto* p=bytes.data()+row*rowBytes+b*packed;unsigned half=U16(p+(type==14?208:0));
     float d=HalfFloat(half),minimum=0;if(!std::isfinite(d))return false;
     int factor=type==12?int(Scale4(p,panel/2)):type==14?Signed(p[192+panel]):1;
     if(type==12){float m=HalfFloat(U16(p+2));if(!std::isfinite(m))return false;minimum=m*float(-int(Min4(p,panel/2)));}
     int dot=0;for(unsigned lane=0;lane<16;lane++)dot+=Quant(type,p,panel*16+lane)*q[b*width+panel*16+lane];
     float coefficient=d*float(factor),c=coefficient*scale[b],v=minimum*scale[b],product=c*float(dot),correction=v*float(total);
     sum[row]+=product+correction;if(!Domain(Bits(sum[row])))return false;
    }
   }
   for(unsigned row=0;row<16;row++)stage[first+row]=Bits(sum[row]);
  }
  if(!Current())return false;
  std::memcpy(output,stage.data(),stage.size()*4);return true;
 }
};
inline bool Rms(const uint32_t* input,const uint32_t* weight,unsigned n,uint32_t* output){
 if(!input || !weight || !output || (n!=256 && n!=5120) || std::fegetround()!=FE_TONEAREST)return false;
 const uintptr_t a[]={reinterpret_cast<uintptr_t>(input),reinterpret_cast<uintptr_t>(weight),reinterpret_cast<uintptr_t>(output)};
 for(unsigned i=0;i<3;i++){if((a[i]&3) || a[i]>UINTPTR_MAX-n*4)return false;for(unsigned j=0;j<i;j++)if(a[i]<a[j]+n*4 && a[j]<a[i]+n*4)return false;}
 double sum=0;for(unsigned i=0;i<n;i++){if(!RmsDomain(input[i]) || !RmsDomain(weight[i]))return false;float x=Float(input[i]);sum+=double(x*x);}
 float scale=1.f/std::sqrt(float(sum/n)+1e-6f);if(!RmsDomain(Bits(scale)) || !(scale>0))return false;
 for(unsigned i=0;i<n;i++){output[i]=Bits((Float(input[i])*scale)*Float(weight[i]));if(!Domain(output[i]))return false;}return true;
}
inline void RopeZero(uint32_t* head){
 // Even position0's exact cos=1/sin=+0 has IEEE signed-zero semantics.
 // Explicit host FMA is independent of the device software-FMA routine.
 for(unsigned i=0;i<32;i++){float a=Float(head[i]),b=Float(head[i+32]);head[i]=Bits(std::fma(a,1.f,-(b*0.f)));head[i+32]=Bits(std::fma(a,0.f,b*1.f));}
}
inline float SigmoidIdeal(float x){double e=std::exp(-std::fabs(double(x)));return float(x>=0?1/(1+e):e/(1+e));}
inline float Flush(float x){unsigned b=Bits(x);return b&0x7f800000?x:Float(b&0x80000000);}
}
template<class Source>bool PioneerQ38MixedLiveOriginalReference(Source& source,unsigned token,PioneerQ38MixedLiveReferenceProducts* output){
 using namespace PioneerQ38MixedLiveReferenceDetail;
 static_assert(sizeof(float)==4 && std::numeric_limits<float>::is_iec559,"IEEE binary32 reference");
 uintptr_t out=reinterpret_cast<uintptr_t>(output);
 if(!output || (out&3) || out>UINTPTR_MAX-sizeof(*output) || token>=248320 || std::fegetround()!=FE_TONEAREST)return false;
 View<Source> view(source);if(!view.Bind())return false;
 auto stage=std::make_unique<PioneerQ38MixedLiveReferenceProducts>();std::vector<uint32_t> norm,post,qnorm,knorm;
 if(!view.Weights(1,norm) || !view.Weights(2,post) || !view.Weights(6,qnorm) || !view.Weights(7,knorm)
  || !view.Embedding(token,stage->embedding) || !Rms(stage->embedding,norm.data(),5120,stage->normalized)
  || !view.Matrix(3,stage->normalized,stage->qg) || !view.Matrix(4,stage->normalized,stage->key) || !view.Matrix(5,stage->normalized,stage->value))return false;
 for(unsigned group=0;group<4;group++){
  if(!Rms(stage->key+group*256,knorm.data(),256,stage->halfKey+group*256))return false;
  for(unsigned i=0;i<256;i++)if(!RmsDomain(stage->halfKey[group*256+i]))return false;
  RopeZero(stage->halfKey+group*256);
  for(unsigned i=0;i<256;i++){
   if(!RmsDomain(stage->halfKey[group*256+i]) || !RmsDomain(stage->value[group*256+i]))return false;
   stage->halfKey[group*256+i]=Bits(HalfFloat(HalfRne(Float(stage->halfKey[group*256+i]))));
   stage->halfValue[group*256+i]=Bits(HalfFloat(HalfRne(Float(stage->value[group*256+i]))));
   if(!Domain(stage->halfKey[group*256+i]) || !Domain(stage->halfValue[group*256+i]))return false;
  }
  for(unsigned head=0;head<6;head++){
   unsigned h=group*6+head;if(!Rms(stage->qg+h*512,qnorm.data(),256,stage->query+h*256))return false;
   for(unsigned i=0;i<256;i++)if(!RmsDomain(stage->query[h*256+i]))return false;
   RopeZero(stage->query+h*256);
   // Position0 RoPE is identity. There is exactly ONE current KV token:
   // softmax=1, so its value passes through binary16 then the Q/G gate.
   for(unsigned i=0;i<256;i++){
    if(!RmsDomain(stage->query[h*256+i]) || !RmsDomain(stage->qg[h*512+256+i]))return false;
    stage->gate[h*256+i]=stage->qg[h*512+256+i];
    float value=Flush(Float(stage->halfValue[group*256+i])*SigmoidIdeal(Float(stage->gate[h*256+i])));
    stage->attention[h*256+i]=Bits(value);if(!Domain(Bits(value)))return false;
   }
  }
 }
 if(!view.Matrix(8,stage->attention,stage->projection))return false;
 for(unsigned i=0;i<5120;i++){
  if(!RmsDomain(stage->projection[i]))return false;
  stage->residual[i]=Bits(Float(stage->embedding[i])+Float(stage->projection[i]));
 }
 if(!Rms(stage->residual,post.data(),5120,stage->postNormalized)
  || !view.Matrix(9,stage->postNormalized,stage->ffnGate) || !view.Matrix(10,stage->postNormalized,stage->ffnUp))return false;
 for(unsigned i=0;i<17408;i++){
  if(!RmsDomain(stage->ffnGate[i]) || !RmsDomain(stage->ffnUp[i]))return false;
  float gate=Float(stage->ffnGate[i]),value=(gate*SigmoidIdeal(gate))*Float(stage->ffnUp[i]);
  stage->activated[i]=Bits(value);if(!RmsDomain(stage->activated[i]))return false;
 }
 if(!view.Matrix(11,stage->activated,stage->down))return false;
 for(unsigned i=0;i<5120;i++){
  if(!RmsDomain(stage->down[i]))return false;
  stage->finalHidden[i]=Bits(Float(stage->residual[i])+Float(stage->down[i]));
  if(!RmsDomain(stage->finalHidden[i]))return false;
 }
 if(!view.Current())return false;
 std::memcpy(output,stage.get(),sizeof(*output));return true;
}
// Strict post-return report against the already computed ORIGINAL reference.
// No tolerance is invented from observed hardware values. Ideal-libm vs a
// device instruction-policy difference remains a failure to diagnose, never
// a reason to relabel expected products or promote an unqualified Work path.
inline bool PioneerQ38CompareMixedLiveStage(const uint32_t* values,unsigned words,
 const PioneerQ38MixedLiveReferenceProducts& reference,PioneerQ38MixedLiveReferenceStage stage,PioneerQ38MixedLiveReferenceComparison* output){
 using namespace PioneerQ38MixedLiveReferenceDetail;
 const uint32_t* expected=nullptr;unsigned count=0;
#define PIONEER_MIXED_REFERENCE_STAGE(tag,field) case tag:expected=reference.field;count=unsigned(sizeof(reference.field)/4);break
 switch(stage){
  PIONEER_MIXED_REFERENCE_STAGE(PioneerQ38MixedEmbedding,embedding);
  PIONEER_MIXED_REFERENCE_STAGE(PioneerQ38MixedNormalized,normalized);
  PIONEER_MIXED_REFERENCE_STAGE(PioneerQ38MixedQg,qg);
  PIONEER_MIXED_REFERENCE_STAGE(PioneerQ38MixedKey,key);
  PIONEER_MIXED_REFERENCE_STAGE(PioneerQ38MixedValue,value);
  PIONEER_MIXED_REFERENCE_STAGE(PioneerQ38MixedQuery,query);
  PIONEER_MIXED_REFERENCE_STAGE(PioneerQ38MixedGate,gate);
  PIONEER_MIXED_REFERENCE_STAGE(PioneerQ38MixedHalfKey,halfKey);
  PIONEER_MIXED_REFERENCE_STAGE(PioneerQ38MixedHalfValue,halfValue);
  PIONEER_MIXED_REFERENCE_STAGE(PioneerQ38MixedAttention,attention);
  PIONEER_MIXED_REFERENCE_STAGE(PioneerQ38MixedProjection,projection);
  PIONEER_MIXED_REFERENCE_STAGE(PioneerQ38MixedResidual,residual);
  PIONEER_MIXED_REFERENCE_STAGE(PioneerQ38MixedPostNormalized,postNormalized);
  PIONEER_MIXED_REFERENCE_STAGE(PioneerQ38MixedFfnGate,ffnGate);
  PIONEER_MIXED_REFERENCE_STAGE(PioneerQ38MixedFfnUp,ffnUp);
  PIONEER_MIXED_REFERENCE_STAGE(PioneerQ38MixedActivated,activated);
  PIONEER_MIXED_REFERENCE_STAGE(PioneerQ38MixedDown,down);
  PIONEER_MIXED_REFERENCE_STAGE(PioneerQ38MixedFinal,finalHidden);
  default:return false;
 }
#undef PIONEER_MIXED_REFERENCE_STAGE
 const unsigned bytes=count*4;
 uintptr_t a=reinterpret_cast<uintptr_t>(values),r=reinterpret_cast<uintptr_t>(&reference),o=reinterpret_cast<uintptr_t>(output);
 if(!values || !output || words!=count || (a&3) || o%alignof(PioneerQ38MixedLiveReferenceComparison)
  || a>UINTPTR_MAX-bytes || r>UINTPTR_MAX-sizeof(reference) || o>UINTPTR_MAX-sizeof(*output)
  || (a<o+sizeof(*output) && o<a+bytes) || (r<o+sizeof(*output) && o<r+sizeof(reference))
  || std::fegetround()!=FE_TONEAREST)return false;
 for(unsigned i=0;i<count;i++)if(!Domain(expected[i]))return false;
 PioneerQ38MixedLiveReferenceComparison result;double error=0,norm=0;
 for(unsigned i=0;i<count;i++){
  double e=Float(expected[i]),v=Float(values[i]);norm+=e*e;
  result.differences+=values[i]!=expected[i];
  if(!std::isfinite(v)){result.nonfinite++;continue;}
  double d=v-e;error+=d*d;result.maxAbsolute=std::fmax(result.maxAbsolute,std::fabs(d));
 }
 result.checked=count;result.relativeL2=std::sqrt(error/(norm+1e-300));result.passed=!(result.nonfinite || result.differences);
 if(result.nonfinite)result.relativeL2=result.maxAbsolute=std::numeric_limits<double>::infinity();
 if(std::fegetround()!=FE_TONEAREST)return false;
 *output=result;return true;
}
inline bool PioneerQ38CompareMixedLiveFinal(const uint32_t* values,unsigned words,
 const PioneerQ38MixedLiveReferenceProducts& reference,PioneerQ38MixedLiveReferenceComparison* output){
 return PioneerQ38CompareMixedLiveStage(values,words,reference,PioneerQ38MixedFinal,output);
}
#endif
