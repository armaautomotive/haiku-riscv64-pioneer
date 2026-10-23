/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_CPU_VALIDATION 1
#define PIONEER_Q38_LEARNED_RMS_VALIDATION_REFERENCE 1
#include "../PioneerQwen38Gguf.h"
#include "../PioneerQ38LearnedRmsValidationReference.h"
#include <array>
#include <cassert>
struct RmsReferenceSource {
 std::vector<PioneerQ38Tensor> tensors={{"blk.0.attn_norm.weight",5120,1,0,20480,0,1}};
 std::array<uint32_t,5120> weight;
 bool valid=true,fail=false,drop=false;unsigned reads=0;
 RmsReferenceSource(){for(unsigned i=0;i<5120;i++)weight[i]=PioneerAttentionBits(.5f);}
 const auto& Model()const{return *this;}const auto& Tensors()const{return tensors;}
 bool Ready()const{return valid;}
 bool ReadReferencePacked(unsigned tensor,uint64_t offset,unsigned bytes,void* output,unsigned capacity){
  reads++;assert(!tensor && !offset && bytes==20480 && capacity==bytes);
  if(fail)return false;
  auto* p=static_cast<uint8_t*>(output);for(unsigned i=0;i<5120;i++)for(unsigned b=0;b<4;b++)p[i*4+b]=uint8_t(weight[i]>>(b*8));
  if(drop)valid=false;return true;
 }
};
int main(){
 RmsReferenceSource source;std::array<uint32_t,5120> input,output;
 unsigned positives=0,refusals=0;
 for(unsigned trial=0;trial<9;trial++){
  for(unsigned i=0;i<5120;i++){
   float x=trial==0?0.f:trial==1?2.f:trial==2?-2.f:trial==3?65504.f:trial==4?PioneerAttentionFloat(0x26800000):float(int((i*37+trial*13)%511)-255)*.03125f;
   input[i]=PioneerAttentionBits(x);source.weight[i]=PioneerAttentionBits(trial&1?-.5f:.5f);
  }
  assert(PioneerQ38LearnedRmsReference(source,0,input.data(),output.data()));
  long double sum=0;for(auto bits:input){float x=PioneerAttentionFloat(bits),square=x*x;sum+=square;}
  float scale=1.f/sqrtf(float(sum/5120)+1e-6f);
  for(unsigned i=0;i<5120;i++)assert(output[i]==PioneerAttentionBits((PioneerAttentionFloat(input[i])*scale)*PioneerAttentionFloat(source.weight[i])));
  positives++;
 }
 auto refuse=[&](const uint32_t* x,uint32_t* out,unsigned tensor=0){output.fill(0xdeadbeef);auto before=input;
  assert(!PioneerQ38LearnedRmsReference(source,tensor,x,out));assert(input==before);for(auto v:output)assert(v==0xdeadbeef);refusals++;};
 refuse(nullptr,output.data());refuse(input.data(),nullptr);refuse(input.data(),output.data(),1);
 // Aliased output must NOT corrupt the independent raw-residual reference.
 refuse(input.data(),input.data());refuse(input.data(),input.data()+1);
 source.valid=false;refuse(input.data(),output.data());source.valid=true;
 for(unsigned field=0;field<6;field++){auto old=source.tensors[0];auto& t=source.tensors[0];
  if(field==0)t.name="blk.1.attn_norm.weight";if(field==1)t.type=8;if(field==2)t.rank=2;
  if(field==3)t.inputs--;if(field==4)t.outputs++;if(field==5)t.bytes--;
  refuse(input.data(),output.data());source.tensors[0]=old;
 }
 for(bool learned:{false,true})for(unsigned i:{0U,5119U})for(uint32_t bad:{1U,0x267fffffU,0x47800000U,0x7f800000U,0x7fc00000U}){
  auto& word=learned?source.weight[i]:input[i];auto old=word;word=bad;refuse(input.data(),output.data());word=old;
 }
 source.fail=true;refuse(input.data(),output.data());source.fail=false;
 source.drop=true;refuse(input.data(),output.data());source.drop=false;source.valid=true;
 printf("PASS DEBUG learned RMS independent original-F32 reference:positives%u/refusals%u,exact5120 outputs/raw input immutable/staged failure output unchanged/no alias/finite domain/source identity,NO device capability\n",positives,refusals);
}
