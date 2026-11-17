/* SPDX-License-Identifier: MIT */
// Execute the activation preparation between CaptureRecipe and the next
// whole-frame Seal. Synthetic entry locals only; no MMIO, workers or timings.
#define PIONEER_TENSIX_MACHINE_ONLY
#include "pioneer_tensix_image_test.cpp"
#include "../PioneerQ38MixedRecurrentProjectionRootLayout.h"
#include <array>
#include <memory>
#include <string>

namespace R=PioneerQ38MixedRecurrentProjectionRootLayout;

struct Prepare : Machine {
 enum {Entry=0x51bc8,Seal=0x4e598,Fault=0x60000,Quantizer=0x498bc,
  Frame=0x6fb00,Sp=0x6f700,Chunk=0x7e000,Width=5120,ScaleWords=32};
 const std::array<std::vector<uint8_t>,2> rom;
 std::vector<uint32_t> ram=std::vector<uint32_t>(0x180000/4);
 uint32_t fp[32]={};
 unsigned calls=0, writes=0;

 explicit Prepare(const std::string& dir):rom{Read((dir+"/text.bin").c_str()),Read((dir+"/conv.bin").c_str())}{}
 static bool In(unsigned a,unsigned base,unsigned bytes){return a>=base && a-base<bytes;}
 const std::vector<uint8_t>* Section(unsigned a,unsigned& base)const{
  base=a>=0x64000?0x64000:0x40000;
  const auto& r=rom[base==0x64000];return !(a&3) && In(a,base,unsigned(r.size()))?&r:nullptr;
 }
 bool CodeAddress(uint32_t a)const override{unsigned base;return Section(a,base);}
 unsigned StepLimit()const override{return 2000000;}
 unsigned Index(uint32_t a)override{
  unsigned base;const auto* section=Section(a,base);assert(section);
  l1[8190]=Word(*section,a-base);return 8190;
 }
 uint32_t& Data(unsigned a){assert(!(a&3) && a>=0x6e000 && a<0x180000);return ram.at(a/4);}
 uint32_t Load(uint32_t a)override{
  unsigned base;const auto* section=Section(a,base);
  if(section)return Word(*section,a-base);
  const bool allowed=In(a,Sp-4096,4096) || In(a,Frame,256)
   || In(a,R::Input,Width*4) || In(a,Chunk,576)
   || In(a,R::InputShadow,Width*4) || In(a,R::Native,Width*2) || In(a,R::NativeShadow,Width*2)
   || In(a,R::Scales,ScaleWords*4) || In(a,R::ScaleShadow,ScaleWords*4);
  assert(allowed);return Data(a);
 }
 void Store(uint32_t a,uint32_t value)override{
  if(!In(a,Sp-4096,4096)){
   assert(In(a,Chunk,576) || In(a,R::InputShadow,Width*4)
    || In(a,R::Native,Width*2) || In(a,R::NativeShadow,Width*2)
    || In(a,R::Scales,ScaleWords*4) || In(a,R::ScaleShadow,ScaleWords*4));
   writes++;
  }
  Data(a)=value;
 }
 void StepScalar(){
  const unsigned instruction=l1[Index(pc)],op=instruction&127;
  if(op!=7 && op!=0x27 && op!=0x53){Step();return;}
  assert(++steps<StepLimit());
  const unsigned d=(instruction>>7)&31,f=(instruction>>12)&7,s=(instruction>>15)&31,
   t=(instruction>>20)&31,high=instruction>>25;
  if(op==7){assert(f==2);fp[d]=Load(regs[s]+Sign(instruction>>20,12));}
  else if(op==0x27){assert(f==2);Store(regs[s]+Sign((high<<5)|d,12),fp[t]);}
  else if(high==0x70){assert(!t && !f);if(d)regs[d]=fp[s];}
  else if(high==0x78){assert(!t && !f);fp[d]=regs[s];}
  else {
   assert(f==0 || f==7);
   const float a=Float(fp[s]),b=Float(fp[t]);
   switch(high){
    case 0:fp[d]=Bits(a+b);break;
    case 4:fp[d]=Bits(a-b);break;
    case 8:fp[d]=Bits(a*b);break;
    case 12:fp[d]=Bits(a/b);break;
    default:assert(false && "unsupported scalar opcode");
   }
  }
  regs[0]=0;pc+=4;
 }
 bool Run(unsigned role){
  memset(regs,0,sizeof(regs));memset(fp,0,sizeof(fp));calls=writes=steps=0;
  Data(Frame+14*4)=Width;Data(Frame+15*4)=12;Data(Frame+43*4)=role;Data(Frame+53*4)=R::Input;
  regs[2]=Sp-848;regs[8]=Sp;regs[18]=Frame;pc=Entry;
  while(pc!=Seal && pc!=Fault){calls+=pc==Quantizer;StepScalar();}
  if(pc==Seal)assert(regs[10]==Frame && regs[2]==Sp-848);
  return pc==Seal;
 }
 void Fixture(unsigned fixture){
  for(unsigned i=0;i<Width;i++){
   const int v=int(((29+fixture*2)*i+19*(i/256)+7*fixture)%173)-86;
   const float x=fixture==0?0.0f:float(v)*float(int(i/256)+1)*.00390625f;
   Data(R::Input+4*i)=Bits(x);
  }
 }
};

int main(int argc,char** argv){
 assert(argc==3);
 auto reuse=std::make_unique<Prepare>(argv[1]),retained=std::make_unique<Prepare>(argv[2]);
 struct Range {unsigned address,words;};
 const Range ranges[]={{R::Input,Prepare::Width},{R::InputShadow,Prepare::Width},
  {R::Native,Prepare::Width/2},{R::NativeShadow,Prepare::Width/2},
  {R::Scales,Prepare::ScaleWords},{R::ScaleShadow,Prepare::ScaleWords}};
 unsigned compared=0,refused=0;
 for(unsigned fixture=0;fixture<4;fixture++){
  reuse->Fixture(fixture);retained->Fixture(fixture);
  // Reuse control still quantizes ROOT input for up. The retained build
  // quantizes gate, then must not write any packed input at all for up.
  assert(reuse->Run(3) && reuse->calls==20 && reuse->writes>0);
  assert(retained->Run(2) && retained->calls==20 && retained->writes>0);
  assert(retained->Run(3) && retained->calls==0 && retained->writes==0);
  for(const auto& range:ranges)for(unsigned i=0;i<range.words;i++){
   const unsigned a=range.address+4*i;
   assert(retained->Data(a)==reuse->Data(a));compared++;
  }
  if(fixture)assert(retained->Data(R::Scales));
  for(const auto& range:ranges)for(unsigned i:{0U,range.words/2,range.words-1}){
   const unsigned a=range.address+4*i;retained->Data(a)^=1;
   assert(!retained->Run(3) && retained->calls==0 && retained->writes==0);
   retained->Data(a)^=1;refused++;
  }
  assert(retained->Run(3) && retained->calls==0 && retained->writes==0);
 }
 printf("PASS pinned actual RV32 preparation, %u word comparisons and %u pre-dispatch refusals; 20 gate/control-up quantizer calls vs 0 retained-up; no device timing\n",
  compared,refused);
}
