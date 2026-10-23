/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_PROJECTION_VALIDATION_CLIENT 1
#define PIONEER_Q38_CPU_VALIDATION 1
#define PIONEER_Q38_LEARNED_RMS_VALIDATION_REFERENCE 1
#include "pioneer_q38_projection_validation_fixture.h"
#include "../PioneerQ38ProjectionValidationReference.h"
#include "../PioneerQ38CpuValidation.h"
#include <array>
struct ProjectionReferenceSource:ProjectionValidationSource {
 std::vector<PioneerQ38Tensor> tensors;unsigned drop=0;
 explicit ProjectionReferenceSource(FILE* f):ProjectionValidationSource(f),tensors(model.Tensors()){}
 const auto& Model()const{return *this;}const auto& Tensors()const{return tensors;}
 const PioneerQ38Tensor* Find(const char* name)const{for(const auto& t:tensors)if(t.name==name)return &t;return nullptr;}
 bool ReadReferencePacked(unsigned tensor,uint64_t offset,unsigned bytes,void* out,unsigned capacity){
  bool okay=ProjectionValidationSource::ReadReferencePacked(tensor,offset,bytes,out,capacity);
  if(referenceReads==drop)valid=false;return okay;
 }
};
static void SeedQ8ProjectionEdges(Fixture& fixture){
 for(unsigned role:{10U,11U,5U,6U}){
  PioneerQ38Tensor expected;assert(PioneerQ38Expected(0,role,expected));
  for(const auto& t:fixture.expected)if(t.name==expected.name){
   for(unsigned first:{0U,unsigned(t.outputs)-16}){
    std::vector<uint8_t> data(16*5440);
    for(unsigned row=0;row<16;row++)for(unsigned block=0;block<160;block++){
     auto* b=data.data()+row*5440+block*34;b[1]=0x3c; // exact original half scale1
     for(unsigned i=0;i<32;i++)b[i+2]=uint8_t(int((i*37+block*11+row*7+first+role)%256)-128);
    }fixture.Write(t.offset+uint64_t(first)*5440,data);
   }
  }
 }
}
int main(){
 setvbuf(stdout,nullptr,_IOLBF,0);Fixture fixture;SeedProjectionValidationEmbedding(fixture,0);SeedQ8ProjectionEdges(fixture);
 ProjectionReferenceSource source(fixture.file);std::array<uint32_t,5120> input;std::array<float,5120> x;
 for(unsigned i=0;i<5120;i++){x[i]=float(int((i*23+11)%257)-128)*.03125f;input[i]=PioneerAttentionBits(x[i]);}
 unsigned positives=0,refusals=0;std::vector<uint32_t> output(10240);
 for(unsigned role:{10U,11U,5U,6U}){
  unsigned n=PioneerQ38ProjectionResultRows(role);output.assign(n,0xdeadbeef);
  assert(PioneerQ38ProjectionReference(source,role,input.data(),5120,output.data(),n));
  PioneerQ38Tensor expected;assert(PioneerQ38Expected(0,role,expected));auto* t=source.Find(expected.name.c_str());assert(t);
  std::vector<float> actual(n);bool nonzero=false;
  for(unsigned i=0;i<n;i++){actual[i]=PioneerAttentionFloat(output[i]);nonzero|=(output[i]&0x7fffffff)!=0;}
  assert(nonzero && PioneerQ38CpuValidate(source,unsigned(t-source.tensors.data()),x.data(),5120,actual.data(),n));
  // Original -128,+127 edges retained; no SM7 clipping/re-quantization.
  for(unsigned i=16;i<n-16;i++)assert(!output[i]);positives++;
  printf("PASS projection original-Q8 DEBUG reference role%u/all%uoutputs/trusted independent row oracle\n",role,n);
 }
 output.assign(10240,0xdeadbeef);
 auto refuse=[&](unsigned role,const uint32_t* in,uint32_t* out,unsigned k,unsigned n){
  std::fill(output.begin(),output.end(),0xdeadbeef);auto before=input;
  assert(!PioneerQ38ProjectionReference(source,role,in,k,out,n));assert(input==before);
  for(auto v:output)assert(v==0xdeadbeef);refusals++;
 };
 for(unsigned role:{0U,1U,4U,7U,9U,12U,UINT32_MAX})refuse(role,input.data(),output.data(),5120,48);
 refuse(10,nullptr,output.data(),5120,48);refuse(10,input.data(),nullptr,5120,48);
 refuse(10,input.data(),output.data(),5119,48);refuse(10,input.data(),output.data(),5120,47);
 refuse(10,input.data(),output.data(),5120,49);
 refuse(10,input.data(),input.data(),5120,48);refuse(10,input.data(),input.data()+1,5120,48);
 refuse(10,reinterpret_cast<const uint32_t*>(reinterpret_cast<const uint8_t*>(input.data())+1),output.data(),5120,48);
 refuse(10,input.data(),reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(output.data())+1),5120,48);
 refuse(10,reinterpret_cast<const uint32_t*>(UINTPTR_MAX-3),output.data(),5120,48);
 refuse(10,input.data(),reinterpret_cast<uint32_t*>(UINTPTR_MAX-3),5120,48);
 source.valid=false;refuse(10,input.data(),output.data(),5120,48);source.valid=true;
 PioneerQ38Tensor expected;assert(PioneerQ38Expected(0,10,expected));unsigned tensor=unsigned(source.Find(expected.name.c_str())-source.tensors.data());
 for(unsigned field=0;field<6;field++){
  auto original=source.tensors[tensor];auto& t=source.tensors[tensor];
  if(field==0)t.name="blk.1.attn_beta.weight";if(field==1)t.type=12;if(field==2)t.rank=1;
  if(field==3)t.inputs--;if(field==4)t.outputs--;if(field==5)t.bytes--;
  refuse(10,input.data(),output.data(),5120,48);source.tensors[tensor]=original;
 }
 for(unsigned at:{0U,5119U})for(uint32_t bits:{0x7f800000U,0xff800000U,0x7fc00000U}){
  auto before=input[at];input[at]=bits;refuse(10,input.data(),output.data(),5120,48);input[at]=before;
 }
 for(unsigned at:{1U,3U}){
  source.referenceReads=0;source.failReference=at;refuse(10,input.data(),output.data(),5120,48);assert(source.referenceReads==at);source.failReference=0;
  source.referenceReads=0;source.drop=at;refuse(10,input.data(),output.data(),5120,48);assert(source.referenceReads==at);source.drop=0;source.valid=true;
 }
 // Invalid final original packed scale: no partially published result.
 const auto& t=source.tensors[tensor];std::vector<uint8_t> bad(34);bad[1]=0x7c;
 fixture.Write(t.offset+uint64_t(47)*5440+159*34,bad);refuse(10,input.data(),output.data(),5120,48);
 printf("PASS independent projection DEBUG original packed-row reference positives%u/refusals%u;nonzero edges/ALL4roles/staged output/unaligned-overflow-alias/source/final-scale faults;NO device access,NOT physical-card arithmetic\n",positives,refusals);
}
