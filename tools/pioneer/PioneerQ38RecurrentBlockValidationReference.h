/* SPDX-License-Identifier: MIT */
// CPU-only ORIGINAL layer0/position0 from token0;NO observed card input.
#if !defined(PIONEER_Q38_RECURRENT_BLOCK_VALIDATION_REFERENCE) || PIONEER_Q38_RECURRENT_BLOCK_VALIDATION_REFERENCE != 1 \
 || !defined(PIONEER_Q38_CPU_VALIDATION) || PIONEER_Q38_CPU_VALIDATION != 1
#error Exact separate recurrent original CPU reference role required
#endif
#if defined(_KERNEL_MODE) || defined(__FAST_MATH__) || defined(PIONEER_ATTENTION_FREESTANDING) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) || defined(PIONEER_Q38_NATIVE_Q8_LAYOUT) \
 || defined(PIONEER_Q38_DECODER_RUNTIME) || defined(PIONEER_Q38_EMBEDDING_NATIVE) || defined(PIONEER_Q38_RECURRENT_BLOCK_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_MIXED_LIVE_NATIVE_CONTEXT) || defined(PIONEER_Q38_MIXED_LIVE_VALIDATION_REFERENCE) \
 || defined(PIONEER_Q38_MIXED_RECURRENT_BLOCK_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_RECURRENT_COHORT_ROOT_IMAGE) \
 || defined(PIONEER_Q38_MIXED_WORKER_IMAGE) || defined(PIONEER_Q38_RECURRENT_BLOCK_ORIGINAL_NATIVE_TEST)
#error Independent recurrent CPU products cannot enter native,image,old dense-reference or Work roles,including defined0
#endif
#if defined(PIONEER_Q38_MIXED_LIVE_VALIDATION_REFERENCE) \
 || defined(_KERNEL_MODE) \
 || defined(__FAST_MATH__) \
 || defined(PIONEER_ATTENTION_FREESTANDING) \
 || defined(PIONEER_Q38_WORK_INFERENCE) \
 || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_NATIVE_Q8_LAYOUT) \
 || defined(PIONEER_Q38_DECODER_RUNTIME) \
 || defined(PIONEER_Q38_EMBEDDING_NATIVE) \
 || defined(PIONEER_Q38_LEARNED_RMS_NATIVE) \
 || defined(PIONEER_Q38_MIXED_LIVE_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_MIXED_LIVE_CONTEXT_COMPILE_ONLY) \
 || defined(PIONEER_Q38_MIXED_LIVE_EMIT_CONTEXT_BODY) \
 || defined(PIONEER_Q38_MIXED_LIVE_VALIDATION_NATIVE) \
 || defined(PIONEER_Q38_MIXED_LIVE_VALIDATION_CLIENT) \
 || defined(PIONEER_Q38_RESIDENT_LAYER_VALIDATION_REFERENCE) \
 || defined(PIONEER_Q38_RESIDENT_LAYER_VALIDATION_CLIENT) \
 || defined(PIONEER_Q38_RESIDENT_LAYER_VALIDATION_NATIVE) \
 || defined(PIONEER_Q38_RESIDENT_LAYER_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_RESIDENT_OUTPUT_OWNER) \
 || defined(PIONEER_Q38_PROJECTION_EXECUTION_OWNER) \
 || defined(PIONEER_Q38_RESIDENT_DECODER_VALIDATION_CLIENT) \
 || defined(PIONEER_Q38_EMBEDDING_VALIDATION_CLIENT) \
 || defined(PIONEER_Q38_LEARNED_RMS_VALIDATION_CLIENT) \
 || defined(PIONEER_Q38_PROJECTION_VALIDATION_CLIENT) \
 || defined(PIONEER_Q38_GDN_INPUT_VALIDATION_CLIENT) \
 || defined(PIONEER_Q38_GDN_RECURRENT_VALIDATION_CLIENT) \
 || defined(PIONEER_Q38_GDN_OUTPUT_VALIDATION_CLIENT) \
 || defined(PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_CLIENT) \
 || defined(PIONEER_Q38_RESIDENT_OUTPUT_VALIDATION_CLIENT) \
 || defined(PIONEER_Q38_LEARNED_RMS_VALIDATION_REFERENCE) \
 || defined(PIONEER_Q38_NEXT_GDN_OUTPUT_VALIDATION_REFERENCE) \
 || defined(PIONEER_Q38_MIXED_LIVE_BLOCK_ROOT_IMAGE) \
 || defined(PIONEER_Q38_MIXED_BLOCK_ROOT_IMAGE) \
 || defined(PIONEER_Q38_MIXED_JOINED_ROOT_IMAGE) \
 || defined(PIONEER_Q38_MIXED_PROJECTION_ROOT_IMAGE) \
 || defined(PIONEER_Q38_MIXED_DENSE_ROOT_IMAGE) \
 || defined(PIONEER_Q38_MIXED_WORKER_IMAGE) \
 || defined(PIONEER_Q38_MIXED_CAPSULE_IMAGE) \
 || defined(PIONEER_Q38_DENSE_PREPARE_IMAGE) \
 || defined(PIONEER_Q38_DENSE_ATTENTION_IMAGE) \
 || defined(PIONEER_Q38_DENSE_PARTITION_IMAGE) \
 || defined(PIONEER_Q38_DENSE_PARTITION_ROOT_IMAGE) \
 || defined(PIONEER_Q38_DENSE_PROJECTION_IMAGE) \
 || defined(PIONEER_Q38_RESIDENT_LAYER_ROOT_IMAGE) \
 || defined(PIONEER_Q38_RESIDENT_LAYER_NORM_IMAGE) \
 || defined(PIONEER_Q38_RESIDENT_LAYER_FFN_IMAGE) \
 || defined(PIONEER_Q38_RESIDENT_CONTINUE_IMAGE) \
 || defined(PIONEER_Q38_RESIDENT_ADVANCE_IMAGE) \
 || defined(PIONEER_Q38_RESIDENT_LAYER_LINK_IMAGE) \
 || defined(PIONEER_Q38_RECURRENT_BLOCK_CONTEXT_COMPILE_ONLY) \
 || defined(PIONEER_Q38_RECURRENT_BLOCK_EMIT_CONTEXT_BODY) \
 || defined(PIONEER_Q38_RECURRENT_BLOCK_FINAL_RELEASE) \
 || defined(PIONEER_Q38_FFN_NATIVE_GRAPH) \
 || defined(PIONEER_Q38_FFN_MODEL_INIT) \
 || defined(PIONEER_Q38_GDN_MUTABLE_STATE)
#error Recurrent CPU reference must not be mixed with any older native,image,client or dense-reference role,including defined0
#endif
#ifndef PIONEER_Q38_RECURRENT_BLOCK_VALIDATION_REFERENCE_H
#define PIONEER_Q38_RECURRENT_BLOCK_VALIDATION_REFERENCE_H
#include "PioneerQ38RecurrentBlockOriginalSource.h"
#include <array>
#include <cfenv>
#include <cmath>
#include <cstring>
#include <limits>
#include <string>
#include <vector>
struct PioneerRecurrentReferenceProducts {
 uint32_t embedding[5120],normalized[5120],alpha[48],beta[48],z[6144],qkv[10240],conv[10240];
 uint32_t nextState[786432],raw[6144],gated[6144],projection[5120],residual[5120],postNormalized[5120];
 uint32_t ffnGate[17408],ffnUp[17408],activated[17408],down[5120],finalHidden[5120];
};
namespace PioneerRecurrentReferenceDetail {
inline uint32_t Bits(float x){uint32_t b;std::memcpy(&b,&x,4);return b;}
inline float Float(uint32_t b){float x;std::memcpy(&x,&b,4);return x;}
inline bool Domain(uint32_t b){unsigned a=b&0x7fffffff;return !a || (a>=0x00800000 && a<0x7f800000);}
inline bool RmsDomain(uint32_t b){unsigned a=b&0x7fffffff;return !a || (a>=0x26800000 && a<=0x477fe000);}
inline unsigned U16(const uint8_t* p){return unsigned(p[0])|(unsigned(p[1])<<8);}
inline float Half(unsigned h){
 unsigned e=(h>>10)&31,m=h&1023;
 float x=e==31?std::numeric_limits<float>::infinity():std::ldexp(float(e?1024+m:m),e?int(e)-25:-24);
 return std::copysign(x,h&0x8000?-1.f:1.f);
}
inline uint16_t HalfRne(float x){
 unsigned sign=Bits(x)>>16&0x8000;float a=std::fabs(x);if(!a)return uint16_t(sign);
 if(!std::isfinite(a))return uint16_t(sign|0x7c00);
 int e=0;std::frexp(a,&e);if(e< -13)return uint16_t(sign|unsigned(std::nearbyint(std::ldexp(a,24))));
 if(e>16)return uint16_t(sign|0x7c00);
 unsigned q=unsigned(std::nearbyint(std::ldexp(a,11-e)));if(q==2048){q=1024;e++;}
 return e>16?uint16_t(sign|0x7c00):uint16_t(sign|(unsigned(e+14)*1024+(q-1024)));
}
inline int Signed(unsigned b){return int(b)-int((b&128)*2);}
inline unsigned Scale4(const uint8_t* p,unsigned g){return g<4?p[4+g]&63:(p[8+g]&15)|((p[g]>>6)<<4);}
inline unsigned Min4(const uint8_t* p,unsigned g){return g<4?p[8+g]&63:(p[8+g]>>4)|((p[4+g]>>6)<<4);}
inline int Quant(unsigned type,const uint8_t* p,unsigned i){return type==8?Signed(p[2+i]):int((p[16+(i/64)*32+i%32]>>((i/32%2)*4))&15);}
inline float Sigmoid(float x){double e=std::exp(-std::fabs(double(x)));return float(x>=0?1/(1+e):e/(1+e));}
inline float Flush(float x){unsigned b=Bits(x);return b&0x7f800000?x:Float(b&0x80000000);}
inline bool Input(unsigned type,const uint32_t* input,unsigned n,std::vector<int>& q,std::vector<float>& scale){
 if(!input || (type!=8 && type!=12) || !n || n>17408 || n%256 || std::fegetround()!=FE_TONEAREST)return false;
 unsigned block=type==8?32:256;q.assign(n,0);scale.assign(n/block,0);
 for(unsigned at=0;at<n;at+=block){
  float maximum=0,selected=0;
  for(unsigned i=0;i<block;i++){if(!Domain(input[at+i]))return false;
  float x=Float(input[at+i]);if(std::fabs(x)>maximum){maximum=std::fabs(x);selected=x;}}
  if(!maximum)continue;
  float inverse=type==8?1.f/(maximum/127.f):127.f/maximum,s=type==8?Half(HalfRne(maximum/127.f)):1.f/inverse;
  if(!Domain(Bits(inverse)) || !inverse || !Domain(Bits(s)) || s>65536.f || (type!=8 && s<0x1p-32f))return false;
  if(type!=8){inverse=std::copysign(inverse,-selected);s=std::copysign(s,-selected);}scale[at/block]=s;
  for(unsigned i=0;i<block;i++){float x=Float(input[at+i])*inverse,v=type==8?std::round(x):std::nearbyint(x);if(!std::isfinite(v) || v< -127 || v>127)return false;
  q[at+i]=int(v);}
 }return std::fegetround()==FE_TONEAREST;
}
inline bool Rms(const uint32_t* input,const uint32_t* weight,uint32_t* output){
 if(!input || !weight || !output)return false;
 double sum=0;
 for(unsigned i=0;i<5120;i++){if(!RmsDomain(input[i]) || !RmsDomain(weight[i]))return false;
 float x=Float(input[i]);sum+=double(x*x);}
 float inverse=1.f/std::sqrt(float(sum/5120)+1e-6f);if(!RmsDomain(Bits(inverse)) || !(inverse>0))return false;
 for(unsigned i=0;i<5120;i++){output[i]=Bits((Float(input[i])*inverse)*Float(weight[i]));if(!Domain(output[i]))return false;}return true;
}
template<class Source> class View {
 Source& source;const void* object=nullptr;const void* data=nullptr;uint64_t epoch=0;bool bound=false;
 struct Tensor {std::string name;unsigned index,type,rank,inputs,outputs;uint64_t bytes,offset;};
 std::array<Tensor,15> tensor;
public:
 explicit View(Source& s):source(s){}
 bool Bind(){
  if(bound || !source.Ready() || std::fegetround()!=FE_TONEAREST || source.Model().Tensors().size()!=851 || !source.Model().Epoch())return false;
  const char* names[]={"token_embd.weight","blk.0.attn_norm.weight","blk.0.post_attention_norm.weight","blk.0.ssm_alpha.weight","blk.0.ssm_beta.weight",
   "blk.0.attn_gate.weight","blk.0.attn_qkv.weight","blk.0.ssm_conv1d.weight","blk.0.ssm_a","blk.0.ssm_dt.bias","blk.0.ssm_norm.weight",
   "blk.0.ssm_out.weight","blk.0.ffn_gate.weight","blk.0.ffn_up.weight","blk.0.ffn_down.weight"};
  const unsigned types[]={12,0,0,8,8,8,8,0,0,0,0,8,12,12,12};
  const unsigned inputs[]={5120,5120,5120,5120,5120,5120,5120,4,48,48,128,6144,5120,5120,17408};
  const unsigned rows[]={248320,1,1,48,48,6144,10240,10240,1,1,1,5120,17408,17408,5120};
  object=&source.Model();data=source.Model().Tensors().data();epoch=source.Model().Epoch();
  for(unsigned i=0;i<15;i++){
   unsigned found=851;for(unsigned j=0;j<851;j++)if(source.Model().Tensors()[j].name==names[i]){if(found!=851)return false;
   found=j;}
   if(found==851)return false;
   const auto& p=source.Model().Tensors()[found];
   unsigned block=types[i]==8?32:types[i]==12?256:1,bytes=types[i]==8?34:types[i]==12?144:4;
   if(p.type!=types[i] || p.rank!=((types[i] || i==7)?2U:1U) || p.inputs!=inputs[i] || p.outputs!=rows[i]
    || p.bytes!=uint64_t(inputs[i]/block)*bytes*rows[i])return false;
   tensor[i]={p.name,found,p.type,p.rank,unsigned(p.inputs),unsigned(p.outputs),p.bytes,p.offset};
  }bound=true;return Current();
 }
 bool Current()const{
  if(!bound || !source.Ready() || std::fegetround()!=FE_TONEAREST || &source.Model()!=object || source.Model().Epoch()!=epoch
   || source.Model().Tensors().size()!=851 || source.Model().Tensors().data()!=data)return false;
  for(const auto& t:tensor){const auto& p=source.Model().Tensors()[t.index];if(p.name!=t.name || p.type!=t.type || p.rank!=t.rank
   || p.inputs!=t.inputs || p.outputs!=t.outputs || p.bytes!=t.bytes || p.offset!=t.offset)return false;}return true;
 }
 bool Read(unsigned slot,uint64_t at,unsigned bytes,void* output){
  return slot<15 && output && bytes && bytes<=1048576 && at<=tensor[slot].bytes && bytes<=tensor[slot].bytes-at && Current()
   && source.ReadReferencePacked(tensor[slot].index,at,bytes,output,bytes) && Current();
 }
 bool F32(unsigned slot,std::vector<uint32_t>& output){
  if(slot>=15 || tensor[slot].type)return false;
  const auto& t=tensor[slot];std::vector<uint8_t> raw(t.bytes);if(!Read(slot,0,unsigned(raw.size()),raw.data()))return false;
  output.resize(raw.size()/4);for(unsigned i=0;i<output.size();i++){const auto* p=raw.data()+i*4;output[i]=uint32_t(p[0])|(uint32_t(p[1])<<8)|(uint32_t(p[2])<<16)|(uint32_t(p[3])<<24);if(!Domain(output[i]))return false;}return true;
 }
 bool Embedding(uint32_t* output){
  std::array<uint8_t,2880> raw;if(!output || !Read(0,0,unsigned(raw.size()),raw.data()))return false;
  for(unsigned b=0;b<20;b++){const auto* p=raw.data()+b*144;float d=Half(U16(p)),m=Half(U16(p+2));if(!std::isfinite(d) || !std::isfinite(m))return false;
   for(unsigned i=0;i<256;i++){float c=d*float(Scale4(p,i/32)),v=m*float(Min4(p,i/32));output[b*256+i]=Bits(c*float(Quant(12,p,i))-v);if(!RmsDomain(output[b*256+i]))return false;}}
  return Current();
 }
 bool Matrix(unsigned slot,const uint32_t* input,uint32_t* output){
  if(slot>=15 || !Current() || !input || !output)return false;
  const auto& t=tensor[slot];if(t.type!=8 && t.type!=12)return false;
  unsigned width=t.type==8?32:256,packed=t.type==8?34:144,blocks=t.inputs/width,rowBytes=blocks*packed;
  std::vector<int> q;std::vector<float> scales;if(!Input(t.type,input,t.inputs,q,scales))return false;
  std::vector<uint8_t> raw(16*rowBytes);std::vector<uint32_t> result(t.outputs);
  for(unsigned first=0;first<t.outputs;first+=16){
   if(!Read(slot,uint64_t(first)*rowBytes,unsigned(raw.size()),raw.data()))return false;
   float sum[16]={};
   for(unsigned block=0;block<blocks;block++)for(unsigned panel=0;panel<width/16;panel++){
    int total=0;for(unsigned lane=0;lane<16;lane++)total+=q[block*width+panel*16+lane];
    for(unsigned row=0;row<16;row++){
     const auto* p=raw.data()+row*rowBytes+block*packed;float d=Half(U16(p)),minimum=0;int factor=1;
     if(!std::isfinite(d))return false;
     if(t.type==12){float m=Half(U16(p+2));if(!std::isfinite(m))return false;
     factor=int(Scale4(p,panel/2));minimum=m*float(-int(Min4(p,panel/2)));}
     int dot=0;for(unsigned lane=0;lane<16;lane++)dot+=Quant(t.type,p,panel*16+lane)*q[block*width+panel*16+lane];
     float coefficient=d*float(factor),c=coefficient*scales[block],v=minimum*scales[block];sum[row]+=(c*float(dot))+(v*float(total));
     if(!Domain(Bits(sum[row])))return false;
    }
   }for(unsigned row=0;row<16;row++)result[first+row]=Bits(sum[row]);
  }if(!Current())return false;
  std::memcpy(output,result.data(),result.size()*4);return true;
 }
};
} // independent host row-major/libm math,NO card quantizer or device code
template<class Source> bool PioneerRecurrentOriginalReference(Source& source,PioneerRecurrentReferenceProducts* output){
 using namespace PioneerRecurrentReferenceDetail;
 if(!output || (uintptr_t(output)&3) || uintptr_t(output)>UINTPTR_MAX-sizeof(*output) || std::fegetround()!=FE_TONEAREST)return false;
 View<Source> v(source);if(!v.Bind())return false;
 auto p=std::make_unique<PioneerRecurrentReferenceProducts>();
 std::vector<uint32_t> norm,post,conv,a,dt,gatedNorm;
 if(!v.F32(1,norm) || !v.F32(2,post) || !v.F32(7,conv) || !v.F32(8,a) || !v.F32(9,dt) || !v.F32(10,gatedNorm)
  || !v.Embedding(p->embedding) || !Rms(p->embedding,norm.data(),p->normalized)
  || !v.Matrix(3,p->normalized,p->alpha) || !v.Matrix(4,p->normalized,p->beta)
  || !v.Matrix(5,p->normalized,p->z) || !v.Matrix(6,p->normalized,p->qkv))return false;
 for(unsigned c=0;c<10240;c++){
  float sum=0;for(unsigned tap=0;tap<4;tap++)sum+=(tap==3?Float(p->qkv[c]):0.f)*Float(conv[c*4+tap]);
  double x=sum;p->conv[c]=Bits(float(x/(1+std::exp(-x))));if(!Domain(p->conv[c]))return false;
 }
 float q[2048],k[2048];
 for(unsigned head=0;head<16;head++){
  float qs=0,ks=0;for(unsigned i=0;i<128;i++){float x=Float(p->conv[head*128+i]),y=Float(p->conv[2048+head*128+i]);qs+=x*x;ks+=y*y;}
  float qi=1.f/std::sqrt(qs+1e-6f),ki=1.f/std::sqrt(ks+1e-6f);if(!(qi>0) || !(ki>0) || !Domain(Bits(qi)) || !Domain(Bits(ki)))return false;
  for(unsigned i=0;i<128;i++){q[head*128+i]=Float(p->conv[head*128+i])*qi;k[head*128+i]=Float(p->conv[2048+head*128+i])*ki;}
 }
 for(unsigned head=0;head<48;head++){
  if(!(Float(a[head])<0))return false;
  float shifted=Float(p->alpha[head])+Float(dt[head]);double x=shifted;
  float softplus=float(x>0?x+std::log1p(std::exp(-x)):std::log1p(std::exp(x))),decay=float(std::exp(double(Float(a[head])*softplus)));
  if(!std::isfinite(decay))return false;
  float beta=Flush(Sigmoid(Float(p->beta[head])));
  for(unsigned value=0;value<128;value++){
   float delta=Float(p->conv[4096+head*128+value])*beta,sum=0;
   for(unsigned i=0;i<128;i++){float next=k[(head%16)*128+i]*delta;p->nextState[head*16384+i*128+value]=Bits(next);sum+=next*q[(head%16)*128+i];}
   p->raw[head*128+value]=Bits(sum*0.0883883461356163f);
  }
  float sum=0;for(unsigned i=0;i<128;i++){float y=Float(p->raw[head*128+i]);sum+=Flush(y*y);}
  float inverse=1.f/std::sqrt(Flush(sum*0.0078125f)+1e-6f);
  if(!(inverse>0) || !Domain(Bits(inverse)))return false;
  for(unsigned i=0;i<128;i++){unsigned at=head*128+i;float weighted=Flush(Flush(Float(p->raw[at])*inverse)*Float(gatedNorm[i]));double z=Float(p->z[at]);p->gated[at]=Bits(Flush(float(double(weighted)*z/(1+std::exp(-z)))));if(!Domain(p->gated[at]))return false;}
 }
 if(!v.Matrix(11,p->gated,p->projection))return false;
 for(unsigned i=0;i<5120;i++)p->residual[i]=Bits(Float(p->embedding[i])+Float(p->projection[i]));
 if(!Rms(p->residual,post.data(),p->postNormalized) || !v.Matrix(12,p->postNormalized,p->ffnGate) || !v.Matrix(13,p->postNormalized,p->ffnUp))return false;
 for(unsigned i=0;i<17408;i++){float gate=Float(p->ffnGate[i]);p->activated[i]=Bits((gate*Sigmoid(gate))*Float(p->ffnUp[i]));if(!RmsDomain(p->activated[i]))return false;}
 if(!v.Matrix(14,p->activated,p->down))return false;
 for(unsigned i=0;i<5120;i++){p->finalHidden[i]=Bits(Float(p->residual[i])+Float(p->down[i]));if(!RmsDomain(p->finalHidden[i]))return false;}
 if(!v.Current())return false;
 std::memcpy(output,p.get(),sizeof(*output));return true;
}
inline bool PioneerRecurrentCompareStage(PioneerRecurrentOriginalStage which,const uint32_t* values,unsigned words,
 const PioneerRecurrentReferenceProducts& reference,PioneerRecurrentOriginalComparison* output){
 using namespace PioneerRecurrentReferenceDetail;const uint32_t* expected=nullptr;unsigned count=0;
#define RECURRENT_REFERENCE_STAGE(tag,field) case tag:expected=reference.field;count=unsigned(sizeof(reference.field)/4);break
 switch(which){
  RECURRENT_REFERENCE_STAGE(PioneerRecurrentEmbedding,embedding);RECURRENT_REFERENCE_STAGE(PioneerRecurrentNormalized,normalized);
  RECURRENT_REFERENCE_STAGE(PioneerRecurrentAlpha,alpha);RECURRENT_REFERENCE_STAGE(PioneerRecurrentBeta,beta);
  RECURRENT_REFERENCE_STAGE(PioneerRecurrentZ,z);RECURRENT_REFERENCE_STAGE(PioneerRecurrentQkv,qkv);RECURRENT_REFERENCE_STAGE(PioneerRecurrentConv,conv);
  RECURRENT_REFERENCE_STAGE(PioneerRecurrentState,nextState);RECURRENT_REFERENCE_STAGE(PioneerRecurrentRaw,raw);RECURRENT_REFERENCE_STAGE(PioneerRecurrentGated,gated);
  RECURRENT_REFERENCE_STAGE(PioneerRecurrentProjection,projection);RECURRENT_REFERENCE_STAGE(PioneerRecurrentResidual,residual);
  RECURRENT_REFERENCE_STAGE(PioneerRecurrentPostNormalized,postNormalized);RECURRENT_REFERENCE_STAGE(PioneerRecurrentFfnGate,ffnGate);
  RECURRENT_REFERENCE_STAGE(PioneerRecurrentFfnUp,ffnUp);RECURRENT_REFERENCE_STAGE(PioneerRecurrentActivated,activated);
  RECURRENT_REFERENCE_STAGE(PioneerRecurrentDown,down);RECURRENT_REFERENCE_STAGE(PioneerRecurrentFinal,finalHidden);default:return false;
 }
#undef RECURRENT_REFERENCE_STAGE
 uintptr_t a=reinterpret_cast<uintptr_t>(values),o=reinterpret_cast<uintptr_t>(output),r=reinterpret_cast<uintptr_t>(&reference);
 if(!values || !output || words!=count || (a&3) || o%alignof(PioneerRecurrentOriginalComparison) || a>UINTPTR_MAX-count*4
  || o>UINTPTR_MAX-sizeof(*output) || (a<o+sizeof(*output) && o<a+count*4) || (r<o+sizeof(*output) && o<r+sizeof(reference))
  || std::fegetround()!=FE_TONEAREST)return false;
 PioneerRecurrentOriginalComparison result;double error=0,norm=0;
 for(unsigned i=0;i<count;i++){
  if(!Domain(expected[i]))return false;
  double e=Float(expected[i]),v=Float(values[i]);norm+=e*e;result.differences+=values[i]!=expected[i];
  if(!std::isfinite(v)){result.nonfinite++;continue;}double d=v-e;error+=d*d;result.maxAbsolute=std::fmax(result.maxAbsolute,std::fabs(d));
 }result.checked=count;result.relativeL2=std::sqrt(error/(norm+1e-300));result.passed=!(result.differences || result.nonfinite);
 if(result.nonfinite)result.relativeL2=result.maxAbsolute=std::numeric_limits<double>::infinity();
 *output=result;return true; // strict independent report,NEVER fitted to observed device outputs
}
#endif
