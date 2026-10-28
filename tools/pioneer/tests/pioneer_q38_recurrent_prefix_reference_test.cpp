/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_RECURRENT_PREFIX_VALIDATION_REFERENCE 1
#define PIONEER_Q38_RECURRENT_BLOCK_VALIDATION_REFERENCE 1
#define PIONEER_Q38_CPU_VALIDATION 1
#include "../PioneerQ38RecurrentPrefixValidationReference.h"
#include <cassert>
#include <cstdio>
struct PrefixReferenceTensor {std::string name;unsigned type,rank,inputs,outputs;uint64_t bytes,offset;};
struct PrefixReferenceModel {
 std::vector<PrefixReferenceTensor> tensors;uint64_t epoch=1;
 uint64_t Epoch()const{return epoch;}const auto& Tensors()const{return tensors;}
};
struct PrefixReferenceSource {
 PrefixReferenceModel model;unsigned reads=0,embeddingReads=0,layerReads[3]={},failRead=0,mutateEarlierAt=0;
 bool ready=true,residual=false;
 PrefixReferenceSource(){
  const char* suffix[]={"attn_norm.weight","post_attention_norm.weight","ssm_alpha.weight","ssm_beta.weight","attn_gate.weight",
   "attn_qkv.weight","ssm_conv1d.weight","ssm_a","ssm_dt.bias","ssm_norm.weight","ssm_out.weight","ffn_gate.weight","ffn_up.weight","ffn_down.weight"};
  const unsigned types[]={12,0,0,8,8,8,8,0,0,0,0,8,12,12,12};
  const unsigned inputs[]={5120,5120,5120,5120,5120,5120,5120,4,48,48,128,6144,5120,5120,17408};
  const unsigned rows[]={248320,1,1,48,48,6144,10240,10240,1,1,1,5120,17408,17408,5120};
  for(unsigned layer=0;layer<3;layer++)for(unsigned slot=layer?1U:0U;slot<15;slot++){
   unsigned type=types[slot],block=type==8?32:type==12?256:1,packed=type==8?34:type==12?144:4;
   std::string name=slot?"blk."+std::to_string(layer)+"."+suffix[slot-1]:"token_embd.weight";
   model.tensors.push_back({name,type,(type || slot==7)?2U:1U,inputs[slot],rows[slot],uint64_t(inputs[slot]/block)*packed*rows[slot],uint64_t(model.tensors.size())<<32});
  }
  while(model.tensors.size()<851){unsigned i=unsigned(model.tensors.size());model.tensors.push_back({"unused"+std::to_string(i),0,1,1,1,4,uint64_t(i)<<32});}
 }
 bool Ready()const{return ready;}const auto& Model()const{return model;}
 bool ReadReferencePacked(unsigned index,uint64_t at,unsigned bytes,void* output,unsigned capacity){
  assert(ready && index<43 && output && bytes && capacity>=bytes && at<=model.tensors[index].bytes && bytes<=model.tensors[index].bytes-at);
  reads++;if(reads==failRead){ready=false;return false;}
  if(reads==mutateEarlierAt)model.tensors[1].name+="_changed";
  unsigned layer=index?(index-1)/14:0,slot=index?(index-1)%14+1:0;
  if(index)layerReads[layer]++;else embeddingReads++;
  auto* p=static_cast<uint8_t*>(output);std::memset(p,0,bytes);
  const auto& tensor=model.tensors[index];
  if(!tensor.type){
   assert(!(bytes%4));uint32_t word=slot==8?0xbf800000:slot==9?0:0x3f800000;
   for(unsigned i=0;i<bytes;i++)p[i]=uint8_t(word>>((i%4)*8));
  }else if(residual && (!slot || slot>=12)){
   assert(tensor.type==12 && !(at%144) && !(bytes%144));
   const unsigned scale=!slot?0x3c00:slot<14?0x1400:0x0100*(layer+1);
   for(unsigned b=0;b<bytes;b+=144){
    p[b]=uint8_t(scale);p[b+1]=uint8_t(scale>>8);
    for(unsigned i=4;i<8;i++)p[b+i]=1;for(unsigned i=12;i<16;i++)p[b+i]=1;
    for(unsigned i=16;i<144;i++)p[b+i]=0x11;
   }
  }
  return true;
 }
};
static void Sentinel(const PioneerRecurrentPrefixReferenceProducts& p){
 const auto* words=reinterpret_cast<const uint32_t*>(&p);
 for(unsigned i=0;i<sizeof(p)/4;i++)assert(words[i]==0xa5a5a5a5);
}
int main(){
 using namespace PioneerRecurrentReferenceDetail;
 auto products=std::make_unique<PioneerRecurrentPrefixReferenceProducts>();
 PrefixReferenceSource zero;
 assert(PioneerRecurrentPrefixOriginalReference(zero,products.get()) && zero.reads==11557 && zero.embeddingReads==1);
 for(unsigned layer=0;layer<3;layer++){
  assert(zero.layerReads[layer]==3852);
  const auto* words=reinterpret_cast<const uint32_t*>(&products->layer[layer]);
  for(unsigned i=0;i<sizeof(products->layer[layer])/4;i++)assert(!words[i]);
 }
 puts("PASS prefix CPU reference ALL3 zero layers/54stages+all NEXT words/11557row reads/ONE embedding read/actual layer selection");
 PrefixReferenceSource distinct;distinct.residual=true;
 assert(PioneerRecurrentPrefixOriginalReference(distinct,products.get()) && distinct.reads==11557 && distinct.embeddingReads==1);
 for(unsigned layer=1;layer<3;layer++)for(unsigned i=0;i<5120;i++)assert(products->layer[layer].embedding[i]==products->layer[layer-1].finalHidden[i]);
 assert(products->layer[0].finalHidden[0]!=products->layer[0].embedding[0]);
 float delta[3];
 for(unsigned layer=0;layer<3;layer++){
  assert(distinct.layerReads[layer]==3852);
  delta[layer]=Float(products->layer[layer].finalHidden[0])-Float(products->layer[layer].embedding[0]);
  assert(delta[layer]>0);
 }
 assert(delta[1]>delta[0]*1.5f && delta[2]>delta[1]*1.2f);
 PrefixReferenceSource baseline;baseline.residual=true;auto old=std::make_unique<PioneerRecurrentReferenceProducts>();
 assert(PioneerRecurrentOriginalReference(baseline,old.get()) && baseline.reads==3853
  && !std::memcmp(old.get(),&products->layer[0],sizeof(*old)));
 puts("PASS nonzero distinct FFN residuals/preceding CPU final feeds next layer BIT-EXACT/no repeated embedding/qualified old layer0 ALL18stages bit-identical");
 for(unsigned fault:{1U,11557U}){
  PrefixReferenceSource source;source.failRead=fault;std::memset(products.get(),0xa5,sizeof(*products));
  assert(!PioneerRecurrentPrefixOriginalReference(source,products.get()));Sentinel(*products);
 }
 PrefixReferenceSource changed;changed.mutateEarlierAt=3854;std::memset(products.get(),0xa5,sizeof(*products));
 assert(!PioneerRecurrentPrefixOriginalReference(changed,products.get()));Sentinel(*products);
 for(unsigned which:{0U,1U,2U}){
  PrefixReferenceSource bad;
  if(which==0)bad.model.tensors[42].bytes--;
  if(which==1)bad.model.tensors[850].name=bad.model.tensors[15].name;
  if(which==2)bad.model.epoch=0;
  assert(!PioneerRecurrentPrefixOriginalReference(bad,products.get()) && !bad.reads);Sentinel(*products);
 }
 PrefixReferenceSource rounding;std::fesetround(FE_DOWNWARD);
 assert(!PioneerRecurrentPrefixOriginalReference(rounding,products.get()) && !rounding.reads);std::fesetround(FE_TONEAREST);Sentinel(*products);
 puts("PASS prefix CPU first+last source loss/earlier-layer metadata changed during later layer/last tensor+duplicate+epoch+rounding refusals/whole-output atomic publication;NO native/device/Work products");
}
