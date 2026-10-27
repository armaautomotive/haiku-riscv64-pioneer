/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_RECURRENT_BLOCK_VALIDATION_REFERENCE 1
#define PIONEER_Q38_CPU_VALIDATION 1
#include "../PioneerQ38RecurrentBlockValidationReference.h"
#include <cassert>
#include <cstdio>
struct ReferenceTensor {std::string name;unsigned type,rank,inputs,outputs;uint64_t bytes,offset;};
struct ReferenceModel {
 std::vector<ReferenceTensor> tensors;uint64_t Epoch()const{return 1;}const auto& Tensors()const{return tensors;}
};
struct ZeroReferenceSource {
 ReferenceModel model;unsigned reads=0,failRead=0;bool ready=true,constantWeights=false;
 ZeroReferenceSource(){
  const char* names[]={"token_embd.weight","blk.0.attn_norm.weight","blk.0.post_attention_norm.weight","blk.0.ssm_alpha.weight","blk.0.ssm_beta.weight",
   "blk.0.attn_gate.weight","blk.0.attn_qkv.weight","blk.0.ssm_conv1d.weight","blk.0.ssm_a","blk.0.ssm_dt.bias","blk.0.ssm_norm.weight",
   "blk.0.ssm_out.weight","blk.0.ffn_gate.weight","blk.0.ffn_up.weight","blk.0.ffn_down.weight"};
  const unsigned types[]={12,0,0,8,8,8,8,0,0,0,0,8,12,12,12};
  const unsigned inputs[]={5120,5120,5120,5120,5120,5120,5120,4,48,48,128,6144,5120,5120,17408};
  const unsigned rows[]={248320,1,1,48,48,6144,10240,10240,1,1,1,5120,17408,17408,5120};
  for(unsigned i=0;i<851;i++){
   if(i>=15){model.tensors.push_back({"unused"+std::to_string(i),0,1,1,1,4,1000000+i*4});continue;}
   unsigned block=types[i]==8?32:types[i]==12?256:1,packed=types[i]==8?34:types[i]==12?144:4;
   model.tensors.push_back({names[i],types[i],(types[i] || i==7)?2U:1U,inputs[i],rows[i],uint64_t(inputs[i]/block)*packed*rows[i],i*1000});
  }
 }
 bool Ready()const{return ready;}const auto& Model()const{return model;}
 bool ReadReferencePacked(unsigned index,uint64_t at,unsigned bytes,void* output,unsigned capacity){
  assert(ready && index<15 && output && bytes && capacity>=bytes && at<=model.tensors[index].bytes && bytes<=model.tensors[index].bytes-at);
  reads++;if(reads==failRead){ready=false;return false;}
  std::memset(output,0,bytes);
  if(model.tensors[index].type && constantWeights){
   unsigned block=model.tensors[index].type==8?34:144;assert(!(at%block) && !(bytes%block));auto* p=static_cast<uint8_t*>(output);
   for(unsigned b=0;b<bytes;b+=block){
    p[b+1]=0x3c; // exact binary16 scale1;all row-major quantized weights1
    if(block==34)for(unsigned i=2;i<34;i++)p[b+i]=1;
    else{for(unsigned i=4;i<8;i++)p[b+i]=1;for(unsigned i=12;i<16;i++)p[b+i]=1;for(unsigned i=16;i<144;i++)p[b+i]=0x11;}
   }
  }
  if(!model.tensors[index].type){
   assert(!(bytes%4));uint32_t word=index==8?0xbf800000:index==9?0:0x3f800000;auto* p=static_cast<uint8_t*>(output);
   for(unsigned i=0;i<bytes;i++)p[i]=uint8_t(word>>((i%4)*8));
  }return true;
 }
};
int main(){
 using namespace PioneerRecurrentReferenceDetail;
 auto reference=std::make_unique<PioneerRecurrentReferenceProducts>();ZeroReferenceSource source;
 assert(PioneerRecurrentOriginalReference(source,reference.get()) && source.reads==3853);
 const auto* all=reinterpret_cast<const uint32_t*>(reference.get());for(unsigned i=0;i<sizeof(*reference)/4;i++)assert(!all[i]);
 for(unsigned fault:{1U,3853U}){
  ZeroReferenceSource bad;bad.failRead=fault;std::memset(reference.get(),0xa5,sizeof(*reference));
  assert(!PioneerRecurrentOriginalReference(bad,reference.get()));
  for(unsigned i=0;i<sizeof(*reference)/4;i++)assert(all[i]==0xa5a5a5a5);
 }
 ZeroReferenceSource bad;bad.model.tensors[14].bytes--;assert(!PioneerRecurrentOriginalReference(bad,reference.get()) && !bad.reads);
 ZeroReferenceSource restore;assert(PioneerRecurrentOriginalReference(restore,reference.get()));
 ZeroReferenceSource constant;constant.constantWeights=true;View<ZeroReferenceSource> view(constant);assert(view.Bind());
 std::array<uint32_t,5120> ones;ones.fill(Bits(1.f));std::array<uint32_t,48> alpha;std::vector<uint32_t> ffn(17408);
 assert(view.Matrix(3,ones.data(),alpha.data()));for(auto value:alpha)assert(value==Bits(5119.6875f));
 assert(view.Matrix(12,ones.data(),ffn.data()));for(auto value:ffn)assert(value==Bits(5120.f));
 PioneerRecurrentOriginalComparison report;
 assert(PioneerRecurrentCompareStage(PioneerRecurrentFinal,reference->finalHidden,5120,*reference,&report) && report.passed && report.checked==5120);
 std::array<uint32_t,5120> changed={};changed[0]=Bits(1.f);
 assert(PioneerRecurrentCompareStage(PioneerRecurrentFinal,changed.data(),5120,*reference,&report) && !report.passed && report.differences==1);
 auto readCount=restore.reads;std::fesetround(FE_DOWNWARD);
 assert(!PioneerRecurrentOriginalReference(restore,reference.get()) && restore.reads==readCount);
 std::fesetround(FE_TONEAREST);
 puts("PASS independent recurrent original CPU reference/zero learned case ALL18stages+786432state words/3853row-major reads/analytical constant1 Q8+Q4 matrices/first+last source-loss atomic refusal/last-tensor metadata refusal/strict postreturn comparison/rounding refusal;NO native or accelerator products");
}
