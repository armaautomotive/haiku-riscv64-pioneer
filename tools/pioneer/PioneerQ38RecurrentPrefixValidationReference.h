/* SPDX-License-Identifier: MIT */
// Private CPU-only token0 prefix reference. NEVER device/native/Work input.
#if !defined(PIONEER_Q38_RECURRENT_PREFIX_VALIDATION_REFERENCE) || PIONEER_Q38_RECURRENT_PREFIX_VALIDATION_REFERENCE != 1 \
 || !defined(PIONEER_Q38_RECURRENT_BLOCK_VALIDATION_REFERENCE) || PIONEER_Q38_RECURRENT_BLOCK_VALIDATION_REFERENCE != 1 \
 || !defined(PIONEER_Q38_CPU_VALIDATION) || PIONEER_Q38_CPU_VALIDATION != 1
#error Exact separate prefix CPU reference and pure arithmetic parent required
#endif
#if defined(PIONEER_Q38_RECURRENT_PREFIX_SCHEDULER_IMAGE) || defined(PIONEER_Q38_RECURRENT_PREFIX_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_RECURRENT_PREFIX_VALIDATION_NATIVE) || defined(PIONEER_Q38_RECURRENT_PREFIX_VALIDATION_CLIENT) \
 || defined(PIONEER_Q38_RECURRENT_PREFIX_ORIGINAL_NATIVE_TEST) || defined(PIONEER_Q38_RECURRENT_BLOCK_VALIDATION_CLIENT)
#error Private prefix CPU products cannot enter image/native/client/Work roles,including defined0
#endif
#include "PioneerQ38RecurrentBlockValidationReference.h"
#ifndef PIONEER_Q38_RECURRENT_PREFIX_VALIDATION_REFERENCE_H
#define PIONEER_Q38_RECURRENT_PREFIX_VALIDATION_REFERENCE_H
struct PioneerRecurrentPrefixReferenceProducts {PioneerRecurrentReferenceProducts layer[3];};
namespace PioneerRecurrentPrefixReferenceDetail {
using namespace PioneerRecurrentReferenceDetail;
template<class Source>class View {
 Source& source;unsigned layer;const void* object=nullptr;const void* data=nullptr;uint64_t epoch=0;bool bound=false;
 struct Tensor {std::string name;unsigned index,type,rank,inputs,outputs;uint64_t bytes,offset;};
 std::array<Tensor,15> tensor;
public:
 View(Source& s,unsigned selected):source(s),layer(selected){}
 bool Bind(){
  if(bound || layer>=3 || !source.Ready() || std::fegetround()!=FE_TONEAREST || source.Model().Tensors().size()!=851 || !source.Model().Epoch())return false;
  const char* suffix[]={"attn_norm.weight","post_attention_norm.weight","ssm_alpha.weight","ssm_beta.weight","attn_gate.weight",
   "attn_qkv.weight","ssm_conv1d.weight","ssm_a","ssm_dt.bias","ssm_norm.weight","ssm_out.weight","ffn_gate.weight","ffn_up.weight","ffn_down.weight"};
  const unsigned types[]={12,0,0,8,8,8,8,0,0,0,0,8,12,12,12};
  const unsigned inputs[]={5120,5120,5120,5120,5120,5120,5120,4,48,48,128,6144,5120,5120,17408};
  const unsigned rows[]={248320,1,1,48,48,6144,10240,10240,1,1,1,5120,17408,17408,5120};
  object=&source.Model();data=source.Model().Tensors().data();epoch=source.Model().Epoch();
  for(unsigned i=0;i<15;i++){
   const std::string name=i?"blk."+std::to_string(layer)+"."+suffix[i-1]:"token_embd.weight";
   unsigned found=851;
   for(unsigned j=0;j<851;j++)if(source.Model().Tensors()[j].name==name){if(found!=851)return false;found=j;}
   if(found==851)return false;
   const auto& p=source.Model().Tensors()[found];
   unsigned block=types[i]==8?32:types[i]==12?256:1,bytes=types[i]==8?34:types[i]==12?144:4;
   if(p.type!=types[i] || p.rank!=((types[i] || i==7)?2U:1U) || p.inputs!=inputs[i] || p.outputs!=rows[i]
    || p.bytes!=uint64_t(inputs[i]/block)*bytes*rows[i])return false;
   tensor[i]={p.name,found,p.type,p.rank,unsigned(p.inputs),unsigned(p.outputs),p.bytes,p.offset};
  }
  bound=true;return Current();
 }
 bool Current()const{
  if(!bound || !source.Ready() || std::fegetround()!=FE_TONEAREST || &source.Model()!=object || source.Model().Epoch()!=epoch
   || source.Model().Tensors().size()!=851 || source.Model().Tensors().data()!=data)return false;
  for(const auto& t:tensor){const auto& p=source.Model().Tensors()[t.index];
   if(p.name!=t.name || p.type!=t.type || p.rank!=t.rank || p.inputs!=t.inputs || p.outputs!=t.outputs || p.bytes!=t.bytes || p.offset!=t.offset)return false;}
  return true;
 }
 unsigned Layer()const{return layer;}
 bool Read(unsigned slot,uint64_t at,unsigned bytes,void* output){
  return slot<15 && (slot || !layer) && output && bytes && bytes<=1048576 && at<=tensor[slot].bytes && bytes<=tensor[slot].bytes-at && Current()
   && source.ReadReferencePacked(tensor[slot].index,at,bytes,output,bytes) && Current();
 }
 bool F32(unsigned slot,std::vector<uint32_t>& output){
  if(slot>=15 || tensor[slot].type)return false;
  const auto& t=tensor[slot];std::vector<uint8_t> raw(t.bytes);if(!Read(slot,0,unsigned(raw.size()),raw.data()))return false;
  output.resize(raw.size()/4);
  for(unsigned i=0;i<output.size();i++){const auto* p=raw.data()+i*4;output[i]=uint32_t(p[0])|(uint32_t(p[1])<<8)|(uint32_t(p[2])<<16)|(uint32_t(p[3])<<24);if(!Domain(output[i]))return false;}
  return true;
 }
 bool Embedding(uint32_t* output){
  std::array<uint8_t,2880> raw;if(layer || !output || !Read(0,0,unsigned(raw.size()),raw.data()))return false;
  for(unsigned b=0;b<20;b++){
   const auto* p=raw.data()+b*144;float d=Half(U16(p)),m=Half(U16(p+2));if(!std::isfinite(d) || !std::isfinite(m))return false;
   for(unsigned i=0;i<256;i++){float c=d*float(Scale4(p,i/32)),v=m*float(Min4(p,i/32));output[b*256+i]=Bits(c*float(Quant(12,p,i))-v);if(!RmsDomain(output[b*256+i]))return false;}
  }
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
     if(t.type==12){float m=Half(U16(p+2));if(!std::isfinite(m))return false;factor=int(Scale4(p,panel/2));minimum=m*float(-int(Min4(p,panel/2)));}
     int dot=0;for(unsigned lane=0;lane<16;lane++)dot+=Quant(t.type,p,panel*16+lane)*q[block*width+panel*16+lane];
     float coefficient=d*float(factor),c=coefficient*scales[block],v=minimum*scales[block];sum[row]+=(c*float(dot))+(v*float(total));
     if(!Domain(Bits(sum[row])))return false;
    }
   }
   for(unsigned row=0;row<16;row++)result[first+row]=Bits(sum[row]);
  }
  if(!Current())return false;
  std::memcpy(output,result.data(),result.size()*4);return true;
 }
};
template<class V>bool Layer(V& view,const uint32_t* input,PioneerRecurrentReferenceProducts* output){
 if(!output || (uintptr_t(output)&3) || uintptr_t(output)>UINTPTR_MAX-sizeof(*output) || std::fegetround()!=FE_TONEAREST
  || (view.Layer()==0)!=(input==nullptr) || !view.Current())return false;
 if(input){
  const uintptr_t a=reinterpret_cast<uintptr_t>(input),o=reinterpret_cast<uintptr_t>(output);
  if((a&3) || a>UINTPTR_MAX-20480 || (a<o+sizeof(*output) && o<a+20480))return false;
 }
 auto p=std::make_unique<PioneerRecurrentReferenceProducts>();
 std::vector<uint32_t> norm,post,conv,a,dt,gatedNorm;
 if(!view.F32(1,norm) || !view.F32(2,post) || !view.F32(7,conv) || !view.F32(8,a) || !view.F32(9,dt) || !view.F32(10,gatedNorm))return false;
 if(input)std::memcpy(p->embedding,input,20480);else if(!view.Embedding(p->embedding))return false;
 if(!Rms(p->embedding,norm.data(),p->normalized) || !view.Matrix(3,p->normalized,p->alpha) || !view.Matrix(4,p->normalized,p->beta)
  || !view.Matrix(5,p->normalized,p->z) || !view.Matrix(6,p->normalized,p->qkv))return false;
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
  float inverse=1.f/std::sqrt(Flush(sum*0.0078125f)+1e-6f);if(!(inverse>0) || !Domain(Bits(inverse)))return false;
  for(unsigned i=0;i<128;i++){
   unsigned at=head*128+i;float weighted=Flush(Flush(Float(p->raw[at])*inverse)*Float(gatedNorm[i]));double z=Float(p->z[at]);
   p->gated[at]=Bits(Flush(float(double(weighted)*z/(1+std::exp(-z)))));if(!Domain(p->gated[at]))return false;
  }
 }
 if(!view.Matrix(11,p->gated,p->projection))return false;
 for(unsigned i=0;i<5120;i++)p->residual[i]=Bits(Float(p->embedding[i])+Float(p->projection[i]));
 if(!Rms(p->residual,post.data(),p->postNormalized) || !view.Matrix(12,p->postNormalized,p->ffnGate) || !view.Matrix(13,p->postNormalized,p->ffnUp))return false;
 for(unsigned i=0;i<17408;i++){float gate=Float(p->ffnGate[i]);p->activated[i]=Bits((gate*Sigmoid(gate))*Float(p->ffnUp[i]));if(!RmsDomain(p->activated[i]))return false;}
 if(!view.Matrix(14,p->activated,p->down))return false;
 for(unsigned i=0;i<5120;i++){p->finalHidden[i]=Bits(Float(p->residual[i])+Float(p->down[i]));if(!RmsDomain(p->finalHidden[i]))return false;}
 if(!view.Current())return false;
 std::memcpy(output,p.get(),sizeof(*output));return true;
}
}
template<class Source>bool PioneerRecurrentPrefixOriginalReference(Source& source,PioneerRecurrentPrefixReferenceProducts* output){
 using V=PioneerRecurrentPrefixReferenceDetail::View<Source>;
 if(!output || (uintptr_t(output)&3) || uintptr_t(output)>UINTPTR_MAX-sizeof(*output) || std::fegetround()!=FE_TONEAREST)return false;
 V views[]={V(source,0),V(source,1),V(source,2)};
 for(auto& view:views)if(!view.Bind())return false;
 auto stage=std::make_unique<PioneerRecurrentPrefixReferenceProducts>();
 for(unsigned layer=0;layer<3;layer++){
  for(const auto& view:views)if(!view.Current())return false;
  if(!PioneerRecurrentPrefixReferenceDetail::Layer(views[layer],layer?stage->layer[layer-1].finalHidden:nullptr,&stage->layer[layer]))return false;
  for(const auto& view:views)if(!view.Current())return false;
 }
 std::memcpy(output,stage.get(),sizeof(*output));return true; // atomic CPU-private publication only
}
#endif
