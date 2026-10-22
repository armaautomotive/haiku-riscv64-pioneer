/* SPDX-License-Identifier: MIT */
#define PIONEER_GRAPH_MATH_MACHINE_ONLY
#include "pioneer_graph_math_image_test.cpp"
#include "pioneer_qwen38_activation_reference.h"
#include "../PioneerQ38FfnActivation.h"

// Emulates the ACTUAL cross-compiled instruction bytes,including BabyRISCV
// operand/result FTZ. No mocked arithmetic completion,NoC or native device.
struct FfnActivationMachine:ScalarMachine {
 unsigned activatedWrites=0,quantizedWrites=0;
 FfnActivationMachine(const char* path,unsigned trial,unsigned block):ScalarMachine(path,256,0) {
  memory.clear();memory[0x74000]=0x41384651;memory[0x74004]=256;memory[0x74008]=292;memory[0x7400c]=0;
  for(unsigned i=0;i<256;i++) {
   unsigned lane=block*256+i;uint32_t hash=(lane+1)*2654435761U+(trial+1)*2246822519U;
   float gate=float(sin(double(lane)*.713))*31.99f,up=float(cos(double(lane)*.219))*10;
   if(trial==0){gate=PioneerAttentionFloat((i&1)<<31);up=PioneerAttentionFloat((i&2)<<30);}
   if(trial==2) {
    const float g[]={0x1p-40f,-0x1p-40f,32,-32},u[]={0x1p-40f,-0x1p-40f,0x1p20f,-0x1p20f};
    gate=g[i%4];up=u[(i/4)%4];
   }
   if(trial==3){gate=1;up=float(int(i)-128)*.5f;}
   if(trial==4){gate=-32;up=(i&1?-1:1)*0x1p-40f;}
   if(trial>=5) {
    gate=PioneerAttentionFloat((hash&0x807fffff)|((87+(hash>>24)%45)<<23));
    up=PioneerAttentionFloat(((hash*3266489917U)&0x807fffff)|((87+(hash>>20)%60)<<23));
    if(trial==6 && !(i%7))gate=PioneerAttentionFloat(hash&0x80000000);
    if(trial==7 && !(i%5))up=PioneerAttentionFloat(hash&0x80000000);
   }
   memory[0x90000+i*4]=PioneerAttentionBits(gate);memory[0x90400+i*4]=PioneerAttentionBits(up);
   memory[0x90800+i*4]=0xdeadbeef;
  }
  for(unsigned i=0;i<73;i++)memory[0x90c00+i*4]=0xdeadbeef;
  for(uint32_t guard:{0x73ffcU,0x74010U,0x8fffcU,0x90d24U})memory[guard]=0xdecafbad;
  memory[0x17ffc0]=memory[0x17ffc4]=memory[0x17ffc8]=0;
 }
 uint32_t Load(uint32_t a) override {
  assert(!(a&3));if(Range(a,0x64000,unsigned(code.size())))return l1[Index(a)];
  assert(Range(a,0x74000,16) || Range(a,0x6e000,8192) || Range(a,0x90000,3364) || Range(a,0x17ffc0,12));
  assert(memory.count(a));return memory.at(a);
 }
 void Store(uint32_t a,uint32_t v) override {
  assert(!(a&3));assert(Range(a,0x6e000,8192) || Range(a,0x90800,1316) || Range(a,0x17ffc0,12));
  if(Range(a,0x6e000,8192))minStack=std::min(minStack,unsigned(a));
  if(Range(a,0x90800,1024))activatedWrites++;
  if(Range(a,0x90c00,292))quantizedWrites++;
  memory[a]=v;
 }
 bool Run() {
  while(pc!=0x64038 && pc!=0x64058)StepScalar();
  assert(l1[Index(pc)]==0x0000006f && minStack>=0x6e000);
  for(uint32_t guard:{0x73ffcU,0x74010U,0x8fffcU,0x90d24U})assert(memory.at(guard)==0xdecafbad);
  if(pc==0x64058){assert(memory.at(0x17ffc8)==0x46463851 && !memory.at(0x17ffc4));return false;}
  assert(memory.at(0x17ffc0)==0x6400c && memory.at(0x17ffc8)==0x31463851);
  return memory.at(0x17ffc4)==1;
 }
 void Verify() {
  float gate[256],up[256],host[256],actual[256];PioneerQ38ActivationK q;uint32_t wanted[73];
  for(unsigned i=0;i<256;i++) {
   gate[i]=PioneerAttentionFloat(memory.at(0x90000+i*4));up[i]=PioneerAttentionFloat(memory.at(0x90400+i*4));
   actual[i]=PioneerAttentionFloat(memory.at(0x90800+i*4));
   float reference=(gate[i]/(1.0f+expf(-gate[i])))*up[i];
   assert(isfinite(actual[i]) && fabsf(actual[i]-reference)<=3e-6f*fabsf(reference));
   assert(!actual[i] || (PioneerAttentionBits(actual[i])&0x7fffffff)>=0x00800000);
  }
  assert(PioneerQ38FfnActivation(gate,up,256,host,&q));
  for(unsigned i=0;i<256;i++)assert(PioneerAttentionBits(host[i])==PioneerAttentionBits(actual[i]));
  Q38ActivationReference(actual,wanted);
  for(unsigned i=0;i<73;i++)assert(memory.at(0x90c00+i*4)==wanted[i] && q.words[i]==wanted[i]);
  assert(activatedWrites==256 && quantizedWrites==73);
 }
};

static unsigned HostRefusals()
{
 float gate[256],up[256],activated[256];PioneerQ38ActivationK q;
 for(unsigned i=0;i<256;i++){gate[i]=1;up[i]=2;activated[i]=-13;}
 for(auto& word:q.words)word=0xdeadbeef;
 auto unchanged=[&](){for(float value:activated)assert(value==-13);for(auto word:q.words)assert(word==0xdeadbeef);};
 unsigned refused=0;
 auto check=[&](const float* g,const float* u,unsigned n,float* a,PioneerQ38ActivationK* quant) {
  assert(!PioneerQ38FfnActivation(g,u,n,a,quant));unchanged();refused++;
 };
 for(unsigned count:{0U,1U,255U,257U,17408U,UINT32_MAX})check(gate,up,count,activated,&q);
 for(unsigned field=0;field<4;field++)for(unsigned invalid=0;invalid<3;invalid++) {
  // nullptr,unaligned,and wrapping ranges are refused BEFORE input reads.
  auto bad=invalid==0?static_cast<__UINTPTR_TYPE__>(0):invalid==1?reinterpret_cast<__UINTPTR_TYPE__>(gate)+1:~static_cast<__UINTPTR_TYPE__>(3);
  check(field==0?reinterpret_cast<float*>(bad):gate,field==1?reinterpret_cast<float*>(bad):up,256,
   field==2?reinterpret_cast<float*>(bad):activated,field==3?reinterpret_cast<PioneerQ38ActivationK*>(bad):&q);
 }
 // Every pair must be disjoint,including partially overlapping output.
 for(unsigned first=0;first<4;first++)for(unsigned second=first+1;second<4;second++)for(unsigned offset:{0U,4U}) {
  __UINTPTR_TYPE__ pointers[]={reinterpret_cast<__UINTPTR_TYPE__>(gate),reinterpret_cast<__UINTPTR_TYPE__>(up),
   reinterpret_cast<__UINTPTR_TYPE__>(activated),reinterpret_cast<__UINTPTR_TYPE__>(&q)};
  pointers[second]=pointers[first]+offset;
  check(reinterpret_cast<float*>(pointers[0]),reinterpret_cast<float*>(pointers[1]),256,
   reinterpret_cast<float*>(pointers[2]),reinterpret_cast<PioneerQ38ActivationK*>(pointers[3]));
 }
 return refused;
}
int main(int argc,char** argv)
{
 assert(argc==2);unsigned positive=0,negative=0,maxSteps=0,minStack=0x70000;
 unsigned hostRefusals=HostRefusals();
 // Eight COMPLETE17408-element FFN activation vectors,split at the actual
 // Q4_K block size. This is local arithmetic only,not matrix integration.
 for(unsigned trial=0;trial<8;trial++) {
  for(unsigned block=0;block<68;block++) {
   FfnActivationMachine m(argv[1],trial,block);auto before=m.memory;assert(m.Run());m.Verify();positive++;
   maxSteps=std::max(maxSteps,m.steps);minStack=std::min(minStack,m.minStack);
   for(const auto& p:before)if(p.first<0x90800 || p.first>=0x90d24) {
    if(!ScalarMachine::Range(p.first,0x17ffc0,12))assert(m.memory.at(p.first)==p.second);
   }
  }
  printf("OFFLINE actual RV32 FFN SiLU*up/Q8_K trial%u ALL17408 lanes+19856 quantized bytes PASS\n",trial);fflush(stdout);
 }
 for(unsigned field=0;field<4;field++)for(unsigned bit=0;bit<32;bit++) {
  FfnActivationMachine m(argv[1],1,0);m.memory[0x74000+field*4]^=1U<<bit;
  assert(!m.Run() && !m.activatedWrites && !m.quantizedWrites);negative++;
 }
 for(unsigned source=0;source<2;source++)for(unsigned lane=0;lane<256;lane++) {
  const uint32_t upper=source?0x49800001:0x42000001;
  for(uint32_t invalid:{1U,0x80000001U,0x00800000U,0x2b7fffffU,0x7f800000U,0xff800000U,0x7fc00001U,0x7f7fffffU,upper,upper|0x80000000}) {
   FfnActivationMachine m(argv[1],1,0);m.memory[0x90000+source*1024+lane*4]=invalid;
   assert(!m.Run() && !m.activatedWrites && !m.quantizedWrites);negative++;
  }
 }
 printf("PASS OFFLINE ACTUAL RV32 FFN activation blocks%u descriptor/domain refusals%u host-range refusals%u maxsteps%u minstack%08x;independent expf tolerance3e-6relative/independent signed-F32scale nearest-even ALL Q8_K bytes+sums/FTZ-normal bounded box/strict local memory/immutable inputs/atomic refusal;NO_NATIVE_OR_BACKEND_OR_SPEED_AUTHORITY\n",positive,negative,hostRefusals,maxSteps,minStack);
}
