/* SPDX-License-Identifier: MIT */
// ACTUAL new QDP1 RV32 instructions; independent arithmetic OBSERVER only.
#define PIONEER_GRAPH_MATH_MACHINE_ONLY
#include "pioneer_graph_math_image_test.cpp"
#include "../PioneerQ38DensePrepare.h"
#include "../PioneerQ38DensePrepareLayout.h"
#include <array>
namespace D=PioneerQ38DensePrepare;
namespace L=PioneerQ38DensePrepareLayout;
static uint16_t HalfReference(float value){_Float16 h=_Float16(value);uint16_t bits;memcpy(&bits,&h,2);return bits;}
struct DensePrepareMachine:ScalarMachine {
 std::array<uint32_t,64> header={};std::array<uint32_t,4160> raw={};std::array<unsigned,3328> writes={};
 DensePrepareMachine(const char* image,unsigned group,unsigned position,unsigned trial=0):ScalarMachine(image,1536,0){
  memory.clear();const unsigned first[]={L::Magic,1,64,trial&1?63U:3U,position,group,17,5120,24,4,256,64,6,3072,1536,1536};
  for(unsigned i=0;i<16;i++)header[i]=first[i];SetHeader();
  for(unsigned h=0;h<6;h++)for(unsigned i=0;i<256;i++){
   float x=float(sin((h+group*6)*.193+i*.071)*2.7);
   if(trial==1)x=i&1?-65504.f:65504.f;
   if(trial==2)x=PioneerAttentionFloat(i&1?0xa6800000:0x26800000);
   if(trial==3)x=i&1?-0.f:0.f;
   raw[h*512+i]=PioneerAttentionBits(x);
   raw[h*512+256+i]=PioneerAttentionBits(float(int(h*256+i)-769)*.015625f);
  }
  for(unsigned i=0;i<256;i++){
   float k=float(cos(i*.113+group*.31)*1.9f);
   if(trial==1)k=i&1?65504.f:-65504.f;
   if(trial==2)k=PioneerAttentionFloat(0x26800000);
   if(trial==3)k=i&1?0.f:-0.f;
   raw[(L::Key-L::Qg)/4+i]=PioneerAttentionBits(k);
   raw[(L::Value-L::Qg)/4+i]=PioneerAttentionBits(i?float(sin(i*.213+group*.117)*1.7):-0.f);
   raw[(L::QWeight-L::Qg)/4+i]=PioneerAttentionBits(float(1+sin(i*.013)*.1));
   raw[(L::KWeight-L::Qg)/4+i]=PioneerAttentionBits(float(1+cos(i*.019)*.13));
  }
  // Raw fixed-position table DATA. No host-normalized/rotated Q/K products.
  for(unsigned i=0;i<32;i++){
   float angle=float(position)*powf(10000000.f,-float(i)/32);
   raw[(L::Rope-L::Qg)/4+i*2]=PioneerAttentionBits(cosf(angle));
   raw[(L::Rope-L::Qg)/4+i*2+1]=PioneerAttentionBits(sinf(angle));
  }
  SetRaw();for(unsigned a=L::Query;a<L::End;a+=4)memory[a]=0xdeadbeef;
  memory[0x17ffc0]=memory[0x17ffc4]=memory[0x17ffc8]=0;
  for(unsigned a:std::array<unsigned,8>{L::Header-4,L::Header+256,L::Qg-4,L::Rope+256,L::Query-4,L::End,0x17ffbcU,0x17ffccU})memory[a]=0xdecafbad;
 }
 void SetHeader(){for(unsigned i=0;i<64;i++)memory[L::Header+i*4]=header[i];}
 void SetRaw(){for(unsigned i=0;i<raw.size();i++)memory[L::Qg+i*4]=raw[i];}
 uint32_t Load(uint32_t a)override{
  assert(!(a&3));if(Range(a,0x64000,unsigned(code.size())))return l1[Index(a)];
  assert(Range(a,0x6e000,8192) || Range(a,L::Header,256) || Range(a,L::Qg,16640)
   || Range(a,L::Query,L::End-L::Query) || Range(a,0x17ffc0,12));
  assert(memory.count(a));return memory.at(a);
 }
 void Store(uint32_t a,uint32_t value)override{
  assert(!(a&3));assert(Range(a,0x6e000,8192) || Range(a,L::Query,L::End-L::Query) || Range(a,0x17ffc0,12));
  if(Range(a,0x6e000,8192))minStack=std::min(minStack,unsigned(a));
  if(Range(a,L::Query,L::End-L::Query)){outputWrites++;assert(++writes[(a-L::Query)/4]==1);}
  memory[a]=value; // NO input/code/NoC/CSR/PCI/reset or GDDR stores
 }
 bool Run(){
  while(pc!=0x64038 && pc!=0x64058)StepScalar();
  assert(l1[Index(pc)]==0x0000006f && minStack>=0x6e000);
  for(unsigned a:std::array<unsigned,8>{L::Header-4,L::Header+256,L::Qg-4,L::Rope+256,L::Query-4,L::End,0x17ffbcU,0x17ffccU})assert(memory.at(a)==0xdecafbad);
  for(unsigned i=0;i<raw.size();i++)assert(memory.at(L::Qg+i*4)==raw[i]);
  for(unsigned i=0;i<64;i++)assert(memory.at(L::Header+i*4)==header[i]);
  unsigned before=outputWrites;for(unsigned i=0;i<32;i++)StepScalar();assert(before==outputWrites);
  if(pc==0x64058){assert(memory.at(0x17ffc8)==L::Fault && !memory.at(0x17ffc4));return false;}
  assert(memory.at(0x17ffc8)==L::Magic && memory.at(0x17ffc0)==0x6400c);return memory.at(0x17ffc4)==1536;
 }
 float Raw(unsigned a)const{return PioneerAttentionFloat(raw[(a-L::Qg)/4]);}
 std::array<float,256> Reference(unsigned at,unsigned weight){
  std::array<float,256> result;double sum=0;
  for(unsigned i=0;i<256;i++){float x=Raw(at+i*4);sum+=double(x*x);}
  float scale=1.f/sqrtf(float(sum/256)+1e-6f);
  for(unsigned i=0;i<256;i++)result[i]=(Raw(at+i*4)*scale)*Raw(weight+i*4);
  for(unsigned i=0;i<32;i++){
   float a=result[i],b=result[i+32],c=Raw(L::Rope+i*8),s=Raw(L::Rope+i*8+4);
   result[i]=fmaf(a,c,-(b*s));result[i+32]=fmaf(a,s,b*c);
  }
  return result;
 }
 void Verify(){
  assert(outputWrites==3328);for(unsigned n:writes)assert(n==1);
  for(unsigned h=0;h<6;h++){
   auto expected=Reference(L::Qg+h*2048,L::QWeight);
   for(unsigned i=0;i<256;i++){
    assert(memory.at(L::Query+(h*256+i)*4)==PioneerAttentionBits(expected[i]));
    assert(memory.at(L::Gate+(h*256+i)*4)==raw[h*512+256+i]);
   }
  }
  auto key=Reference(L::Key,L::KWeight);
  for(unsigned i=0;i<128;i++){
   uint32_t k=HalfReference(key[i*2])|(uint32_t(HalfReference(key[i*2+1]))<<16);
   uint32_t v=HalfReference(Raw(L::Value+i*8))|(uint32_t(HalfReference(Raw(L::Value+i*8+4)))<<16);
   assert(memory.at(L::PackedK+i*4)==k && memory.at(L::PackedV+i*4)==v);
  }
 }
};
#ifndef PIONEER_Q38_DENSE_PREPARE_MACHINE_ONLY
static void HostMathAndPointers(){
 uint32_t random=0x635cf19;unsigned means=0;
 for(unsigned trial=0;trial<2000;trial++){
  PioneerQ38RmsSum sum;double expected=0;
  for(unsigned i=0;i<256;i++){
   random=random*1664525U+1013904223U;
   float term=PioneerAttentionFloat(((random%200+20)<<23)|(random&0x7fffff));
   assert(sum.Add(term));expected+=double(term);
  }
  float actual;assert(D::Mean256(sum,actual) && PioneerAttentionBits(actual)==PioneerAttentionBits(float(expected/256)));means++;
 }
 std::array<float,3072> qg={};std::array<float,256> k={},v={},qw,kw;std::array<float,64> rope={};
 qw.fill(1);kw.fill(1);for(unsigned i=0;i<32;i++)rope[i*2]=1;
 std::array<float,1536> q,g;std::array<uint32_t,128> pk,pv;
 const uintptr_t pointers[]={uintptr_t(qg.data()),uintptr_t(k.data()),uintptr_t(v.data()),uintptr_t(qw.data()),uintptr_t(kw.data()),
  uintptr_t(rope.data()),uintptr_t(q.data()),uintptr_t(g.data()),uintptr_t(pk.data()),uintptr_t(pv.data())};
 auto run=[](const uintptr_t* p){return D::Run(reinterpret_cast<const float*>(p[0]),reinterpret_cast<const float*>(p[1]),
  reinterpret_cast<const float*>(p[2]),reinterpret_cast<const float*>(p[3]),reinterpret_cast<const float*>(p[4]),
  reinterpret_cast<const float*>(p[5]),reinterpret_cast<float*>(p[6]),reinterpret_cast<float*>(p[7]),
  reinterpret_cast<uint32_t*>(p[8]),reinterpret_cast<uint32_t*>(p[9]));};
 assert(run(pointers));unsigned refusals=0;
 for(unsigned i=0;i<10;i++)for(uintptr_t bad:{uintptr_t(0),uintptr_t(1),UINTPTR_MAX-3}){
  uintptr_t edited[10];memcpy(edited,pointers,sizeof(edited));edited[i]=bad;assert(!run(edited));refusals++;
 }
 for(unsigned i=0;i<10;i++)for(unsigned j=0;j<i;j++){
  uintptr_t edited[10];memcpy(edited,pointers,sizeof(edited));edited[i]=edited[j];assert(!run(edited));refusals++;
 }
 printf("PASS host DATA Mean256%u/512000software-binary64 additions,75null/misalignment/wrap/overlap refusals;NOT device/native reference feedback\n",means);assert(refusals==75);
}
int main(int argc,char** argv){
 assert(argc==2);setvbuf(stdout,nullptr,_IOLBF,0);HostMathAndPointers();
 unsigned positives=0,refusals=0,maxSteps=0,maxStack=0;
 for(unsigned group=0;group<4;group++)for(unsigned position:{0U,1U,17U,1023U,8191U}){
  DensePrepareMachine m(argv[1],group,position);assert(m.Run());m.Verify();positives++;
  maxSteps=std::max(maxSteps,m.steps);maxStack=std::max(maxStack,0x70000-m.minStack);
 }
 for(unsigned trial=1;trial<4;trial++){
  DensePrepareMachine m(argv[1],3,8191,trial);assert(m.Run());m.Verify();positives++;
  maxSteps=std::max(maxSteps,m.steps);maxStack=std::max(maxStack,0x70000-m.minStack);
 }
 for(unsigned field=0;field<64;field++)for(unsigned bit=0;bit<32;bit++){
  DensePrepareMachine m(argv[1],0,0);m.header[field]^=1U<<bit;
  if(L::Shape(m.header.data()))continue;m.SetHeader();assert(!m.Run() && !m.outputWrites);refusals++;
 }
 for(unsigned at:std::array<unsigned,12>{L::Qg,L::Qg+12284,L::Key,L::Key+1020,L::Value,L::Value+1020,L::QWeight,L::QWeight+1020,L::KWeight,L::KWeight+1020,L::Rope,L::Rope+252})
  for(uint32_t bad:{1U,0x807fffffU,0x47800000U,0x7f800000U,0xff800000U,0x7fc00000U}){
   DensePrepareMachine m(argv[1],0,17);m.raw[(at-L::Qg)/4]=bad;m.SetRaw();assert(!m.Run() && !m.outputWrites);refusals++;
  }
 // A valid-domain LAST head can fail arithmetic after Q/gate LOCAL writes.
 // No K/V packet/commit or successful terminal may be invented from them.
 DensePrepareMachine late(argv[1],0,17);
 for(unsigned i=0;i<256;i++){late.raw[(L::Key-L::Qg)/4+i]=i?0:0x3f800000;late.raw[(L::KWeight-L::Qg)/4+i]=0x477fe000;}
 late.SetRaw();assert(!late.Run() && late.outputWrites==3072);
 for(unsigned i=3072;i<3328;i++)assert(!late.writes[i]);refusals++;
 printf("PASS ACTUAL QDP1 local27B RV32 preparation positives%u/refusals%u/maxSteps%u/maxStack%u;ALL24Q/4KV heads,6x[Q256,gate256] interleaving,learnedRMS256/partialRoPE64+unrotated192/F16K-V BITEXACT independent double/sqrtf/fmaf/_Float16 observer;NO NoC/GDDR/KV commit/native/fullattention/token loop/inference timing\n",positives,refusals,maxSteps,maxStack);
}
#endif
