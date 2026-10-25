/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_RESIDENT_LAYER_VALIDATION_REFERENCE 1
#define PIONEER_Q38_NEXT_GDN_OUTPUT_VALIDATION_REFERENCE 1
#define PIONEER_Q38_RESIDENT_DECODER_VALIDATION_CLIENT 1
#define PIONEER_Q38_CPU_VALIDATION 1
#include "../PioneerQ38ResidentLayerValidationReference.h"
#include "pioneer_qwen38_activation_reference.h"
#include <cassert>
#include <cstdio>
#include <string>
struct LayerReferenceSource {
 struct Tensor {std::string name;unsigned type,rank,inputs,outputs;uint64_t bytes,offset;};
 struct Index {
  std::vector<Tensor> tensors={
   {"blk.0.ssm_out.weight",8,2,6144,5120,UINT64_C(5120)*6528,1000},
   {"blk.0.post_attention_norm.weight",0,1,5120,1,20480,2000},
   {"blk.0.ffn_gate.weight",12,2,5120,17408,UINT64_C(17408)*2880,3000},
   {"blk.0.ffn_up.weight",12,2,5120,17408,UINT64_C(17408)*2880,4000},
   {"blk.0.ffn_down.weight",12,2,17408,5120,UINT64_C(5120)*9792,5000}};
  const auto& Tensors()const{return tensors;}
  const Tensor* Find(const char* name)const{for(const auto& t:tensors)if(t.name==name)return &t;return nullptr;}
 } model;
 bool valid=true,zero=false;unsigned reads=0,failRead=0,mutateRead=0,dropRead=0,badRead=0,roundRead=0;
 bool Ready()const{return valid;}const auto& Model()const{return model;}
 static unsigned Scale(unsigned row,unsigned block,unsigned group){return (row+block+group*11)%64;}
 static unsigned Minimum(unsigned row,unsigned block,unsigned group){return (row*7+block+group*3)%64;}
 static unsigned Quant(unsigned row,unsigned block,unsigned lane){return (row*7+block*3+lane*5)%16;}
 bool ReadReferencePacked(unsigned tensor,uint64_t offset,unsigned bytes,void* output,unsigned capacity){
  assert(tensor<5 && bytes==capacity && offset+bytes<=model.tensors[tensor].bytes);if(++reads==failRead)return false;
  auto* p=static_cast<uint8_t*>(output);std::memset(p,0,bytes);
  if(tensor==1){assert(!offset && bytes==20480);for(unsigned i=0;i<5120;i++){p[i*4+2]=0x80;p[i*4+3]=0x3f;}}
  else if(!zero && tensor>=2){
   unsigned inputs=tensor==4?17408:5120,blocks=inputs/256,rowBytes=blocks*144;assert(bytes==16*rowBytes && !(offset%rowBytes));
   unsigned first=unsigned(offset/rowBytes);
   for(unsigned row=0;row<16;row++)for(unsigned b=0;b<blocks;b++){
    auto* block=p+row*rowBytes+b*144;block[1]=0x0c;block[3]=0x08;unsigned d[8],m[8];
    for(unsigned g=0;g<8;g++){d[g]=Scale(first+row,b,g);m[g]=Minimum(first+row,b,g);}
    for(unsigned g=0;g<4;g++){block[4+g]=uint8_t(d[g]|((d[g+4]>>4)<<6));block[8+g]=uint8_t(m[g]|((m[g+4]>>4)<<6));
     block[12+g]=uint8_t((d[g+4]&15)|((m[g+4]&15)<<4));}
    for(unsigned g=0;g<4;g++)for(unsigned k=0;k<32;k++)block[16+g*32+k]=uint8_t(Quant(first+row,b,g*64+k)|(Quant(first+row,b,g*64+32+k)<<4));
   }
  }
  if(reads==badRead && tensor>=2){p[0]=0;p[1]=0x7c;}
  if(reads==mutateRead)model.tensors[tensor].offset++;
  if(reads==dropRead)valid=false;if(reads==roundRead)assert(!std::fesetround(FE_DOWNWARD));return true;
 }
};
int main(){
 using namespace PioneerQ38ResidentLayerReferenceDetail;setvbuf(stdout,nullptr,_IOLBF,0);assert(std::fegetround()==FE_TONEAREST);
 unsigned domains=0,refusals=0;uint32_t random=0x514c5631;
 for(unsigned n:{5120U,17408U})for(unsigned trial=0;trial<16;trial++){
  std::vector<uint32_t> x(n);for(auto& v:x){random=random*1664525+1013904223;v=Bits(float(int((random>>8)%25301)-12650)*.00031f);}
  if(!trial)for(unsigned i=0;i<n;i++)x[i]=i&1?0x80000000:0;
  if(trial==1 || trial==2)for(unsigned b=0;b<n/256;b++){
   x[b*256]=Bits(trial==1?127.f:-127.f);x[b*256+1]=Bits(trial==1?-127.f:127.f);
   x[b*256+2]=Bits(.5f);x[b*256+3]=Bits(-.5f);x[b*256+4]=Bits(1.5f);x[b*256+5]=Bits(-1.5f);
  }
  std::vector<int> quant;std::vector<float> scales;assert(InputK(x.data(),n,quant,scales));
  for(unsigned b=0;b<n/256;b++){
   uint32_t expected[73];float input[256];for(unsigned i=0;i<256;i++)input[i]=Float(x[b*256+i]);Q38ActivationReference(input,expected);
   assert(Bits(scales[b])==expected[0]);for(unsigned i=0;i<256;i++){
    unsigned v=(expected[1+i/4]>>((i%4)*8))&255;assert(quant[b*256+i]==int(v)-int((v&128)*2));
   }
  }domains++;
 }
 for(unsigned role:{0U,1U,2U}){
  unsigned n=role==2?17408:5120,rows=role==2?5120:17408;
  std::vector<uint32_t> input(n),output(rows,0xdeadbeef);std::vector<float> activation(n);
  for(unsigned i=0;i<n;i++)input[i]=Bits(float(int((i*29+17)%255)-127)*.00390625f);
  for(unsigned b=0;b<n/256;b++){
   float x[256];uint32_t q[73];for(unsigned i=0;i<256;i++)x[i]=Float(input[b*256+i]);Q38ActivationReference(x,q);
   for(unsigned i=0;i<256;i++){unsigned v=(q[1+i/4]>>((i%4)*8))&255;activation[b*256+i]=Float(q[0])*float(int(v)-int((v&128)*2));}
  }
  LayerReferenceSource source;assert(MatrixK(source,role,input.data(),output.data()) && source.reads==rows/16);
  for(unsigned row:{0U,15U,16U,rows/2,rows-1}){
   double expected=0,absolute=0;for(unsigned i=0;i<n;i++){
    unsigned block=i/256,group=(i%256)/32;double weight=0x1p-12*LayerReferenceSource::Scale(row,block,group)*LayerReferenceSource::Quant(row,block,i%256)
     -0x1p-13*LayerReferenceSource::Minimum(row,block,group);
    double term=weight*double(activation[i]);expected+=term;absolute+=std::fabs(term);
   }assert(std::fabs(double(Float(output[row]))-expected)<=1e-6+absolute*2e-6);
  }
  auto refuse=[&](LayerReferenceSource& bad){std::fill(output.begin(),output.end(),0xdeadbeef);
   assert(!MatrixK(bad,role,input.data(),output.data()));for(auto v:output)assert(v==0xdeadbeef);refusals++;
  };
  for(unsigned kind=0;kind<5;kind++){
   LayerReferenceSource bad;unsigned at=kind<2?rows/16:1;
   if(kind==0)bad.failRead=at;if(kind==1)bad.mutateRead=at;if(kind==2)bad.dropRead=at;if(kind==3)bad.badRead=at;if(kind==4)bad.roundRead=at;
   refuse(bad);assert(bad.reads==at && !std::fesetround(FE_TONEAREST));
  }
 }
 std::array<uint32_t,5120> raw;std::array<uint32_t,6144> gated;
 for(unsigned i=0;i<5120;i++)raw[i]=Bits(.125f+float(i%127)*.00390625f);for(auto& v:gated)v=Bits(.25f);
 auto result=std::make_unique<PioneerQ38ResidentLayerReferenceProducts>();LayerReferenceSource zero;zero.zero=true;
 assert(PioneerQ38ResidentLayerOriginalReference(zero,raw.data(),gated.data(),result.get()) && zero.reads==2817);
 double squares=0;for(auto v:raw){float x=Float(v);squares+=double(x*x);}float scale=1.f/std::sqrt(float(squares/5120)+1e-6f);
 for(unsigned i=0;i<5120;i++)assert(!result->projection[i] && result->residual[i]==raw[i] && result->normalized[i]==Bits(Float(raw[i])*scale)
  && !result->down[i] && result->finalHidden[i]==raw[i]);
 for(unsigned i=0;i<17408;i++)assert(!result->gate[i] && !result->up[i] && !result->activated[i]);
 for(unsigned kind=0;kind<10;kind++){
  LayerReferenceSource bad;auto* out=result.get();const auto* r=raw.data();const auto* g=gated.data();
  if(kind==0)r=nullptr;if(kind==1)g=nullptr;if(kind==2)out=nullptr;
  if(kind==3)r=reinterpret_cast<const uint32_t*>(UINTPTR_MAX-3);
  if(kind==4)g=reinterpret_cast<const uint32_t*>(UINTPTR_MAX-3);
  if(kind==5)out=reinterpret_cast<PioneerQ38ResidentLayerReferenceProducts*>(UINTPTR_MAX-3);
  if(kind==6)r=result->finalHidden;if(kind==7)g=result->activated;
  if(kind==8)r=reinterpret_cast<const uint32_t*>(reinterpret_cast<uintptr_t>(r)+1);
  if(kind==9)bad.valid=false;
  std::memset(result.get(),0xa5,sizeof(*result));assert(!PioneerQ38ResidentLayerOriginalReference(bad,r,g,out) && !bad.reads);
  const auto* bytes=reinterpret_cast<const uint8_t*>(result.get());for(unsigned i=0;i<sizeof(*result);i++)assert(bytes[i]==0xa5);refusals++;
 }
 for(unsigned at:{1U,321U,322U,2817U}){
  LayerReferenceSource bad;bad.zero=true;bad.failRead=at;std::memset(result.get(),0xa5,sizeof(*result));
  assert(!PioneerQ38ResidentLayerOriginalReference(bad,raw.data(),gated.data(),result.get()) && bad.reads==at);
  const auto* bytes=reinterpret_cast<const uint8_t*>(result.get());for(unsigned i=0;i<sizeof(*result);i++)assert(bytes[i]==0xa5);refusals++;
 }
 printf("PASS separate ORIGINAL row-major complete-layer CPU reference:quantizationDomains%u/ALL3fullQ4matrices/FP64selectedrowbounds/2817completepostGDNreads/atomicRefusals%u;libmSiLU reference only,NO device/controller/ready/upload/Work/fallback or speed claim\n",domains,refusals);
}
