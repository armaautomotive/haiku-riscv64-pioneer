/* SPDX-License-Identifier: MIT */
// ACTUAL norm.bin from the NEW COMPLETE linked ROOT image,at158000.
// Direct subroutine instruction test ONLY;not a simulated completed prefix
// or native/ROOT/FFN lifetime qualification. No numeric results injected.
#define PIONEER_GRAPH_MATH_MACHINE_ONLY
#include "pioneer_graph_math_image_test.cpp"
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentLayerPlan.h"
#include <array>
#include <memory>
struct LayerNormMachine:ScalarMachine {
 std::array<uint32_t,64> descriptor={};
 std::array<uint32_t,5120> projected,raw,weight,resident,backup;
 uint32_t command[10]={},counts[8]={},source[16]={};
 unsigned packets=0,reads=0,writes=0,sumStores=0,normStores=0,failPacket=0,sourceFault=0,descriptorFault=0,badCounter=0;
 bool pending=false,unknown=false;
 static constexpr unsigned Return=0x17ff80,Fault=0x72060;
 static unsigned Counter(unsigned i){const unsigned o[]={0x208,0x20c,0x210,0x214,0x204,0x220,0x210,0x228};return o[i];}
 static unsigned Source(unsigned i){return !i?0x100:i<=6?0x118+(i-1)*4:i<=12?0x130+(i-7)*4:i==13?0x150:i==14?0x154:0x16c;}
 LayerNormMachine(const char* image,unsigned trial):ScalarMachine(5120,0){
  code=Read((std::string(image)+"/norm.bin").c_str());assert(!code.empty() && code.size()<=16384 && !(code.size()&3));
  for(unsigned i=0;i<code.size()/4;i++)l1[1+i]=Word(code,i*4);
  std::ifstream symbols(std::string(image)+"/program.symbols");std::string line;unsigned entry=0;
  while(std::getline(symbols,line)){
   unsigned at=0;char type=0,name[256]={};
   if(sscanf(line.c_str(),"%x %c %255s",&at,&type,name)==3 && !strcmp(name,"q38_resident_layer_norm_once"))entry=at;
  }
  assert(CodeAddress(entry));pc=entry;regs[1]=Return;regs[2]=0x70000;
  auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(canonical->Build());
  auto plan=std::make_unique<PioneerQ38ResidentLayerPlan>();std::array<uint32_t,1920> sources;
  std::array<uint32_t,119> sequence={};sources.fill(0x16005);
  for(unsigned i=0;i<16;i++)sources[i]=source[i]=i?0x13570000+i*0x12345:0x16005;
  unsigned layer=trial&1?62:0,bank=trial&1?6:0,position=trial&1?8191:0,token=trial&1?248319:0;
  assert(plan->Build(*canonical,layer,position,bank,trial&1,token,17,{1,2,3,4},sources.data(),sequence.data()));
  PioneerQ38ResidentLayerNormDescriptor(plan->LayerManifest(),descriptor.data());SetDescriptor();
  for(unsigned i=0;i<8;i++)counts[i]=0xfffffff8;
  for(unsigned i=0;i<5120;i++){
   float x=float(int((i*47+trial*97)%511)-255)*.03125f,y=float(int((i*31+trial*13)%255)-127)*.015625f;
   if(trial==1){x=65504.f;y=-32752.f;}
   if(trial==2)x=y=PioneerAttentionFloat(0x26800000);
   if(trial==3)x=y=-0.f;
   if(trial==4)x=y=0;
   if(trial==5){x=PioneerAttentionFloat(0x3f800001);y=-1.f;}
   raw[i]=PioneerAttentionBits(x);projected[i]=PioneerAttentionBits(y);
   weight[i]=PioneerAttentionBits(float(int((i*71+trial*13)%127)-63)*.0234375f);
   resident[i]=backup[i]=0xdeadbeef;
   for(unsigned a:{0x80000U,0x88000U,0x90000U,0x98000U,0xa0000U})memory[a+i*4]=0xa5a5a5a5;
  }
 }
 void SetDescriptor(){for(unsigned i=0;i<64;i++)memory[TT_Q38_FFN_RESIDENT_DESCRIPTOR+i*4]=descriptor[i];}
 unsigned Index(uint32_t at)override{assert(CodeAddress(at) && !(at&3));return 1+(at-TT_Q38_RESIDENT_LAYER_NORM_ROM)/4;}
 bool CodeAddress(uint32_t at)const override{return Range(at,TT_Q38_RESIDENT_LAYER_NORM_ROM,unsigned(code.size()));}
 uint32_t Load(uint32_t at)override{
  assert(!(at&3));if(CodeAddress(at))return l1[Index(at)];
  if(Range(at,0xffb20000,0x2000)){
   unsigned offset=at-0xffb20000;
   if(offset<40)return command[offset/4];
   for(unsigned i=0;i<4;i++)if(offset==0x40+i*0x800)return i?0:unsigned(pending);
   if(offset>=0x240 && offset<0x2c0)return 0;
   for(unsigned i=0;i<16;i++)if(offset==Source(i))return source[i];
   for(unsigned i=0;i<8;i++)if(offset==Counter(i))return counts[i];
   assert(!"unqualified norm CSR");
  }
  assert(Range(at,0x6e000,8192) || Range(at,0x74200,384) || Range(at,0x80000,20480)
   || Range(at,0x88000,20480) || Range(at,0x90000,20480) || Range(at,0x98000,20480) || Range(at,0xa0000,20480));
  return memory.at(at);
 }
 void Store(uint32_t at,uint32_t value)override{
  assert(!(at&3));
  if(Range(at,0xffb20000,40)){assert(!pending && !unknown);command[(at-0xffb20000)/4]=value;return;}
  if(at==0xffb20040){assert(value==1 && !pending && !unknown);Packet();return;}
  assert(Range(at,0x6e000,8192) || Range(at,0x74300,128) || Range(at,0x98000,20480) || Range(at,0xa0000,20480));
  if(Range(at,0x6e000,8192))minStack=std::min(minStack,unsigned(at));
  if(Range(at,0x98000,20480))sumStores++;
  if(Range(at,0xa0000,20480))normStores++;
  memory[at]=value;
 }
 void Packet(){
  assert(packets<10);unsigned p=packets++;bool write=p>=6,learned=p>=4 && p<6,rawRead=p>=2 && p<4,normalized=p>=8;
  unsigned offset=(p&1)*16384,bytes=p&1?4096:16384;
  unsigned local=(write?(normalized?0xa0000:0x98000):learned?0x90000:rawRead?0x88000:0x80000)+offset;
  unsigned remote=(learned?descriptor[11]:descriptor[4])+(learned?0U:write?(normalized?0U:98304U):rawRead?98304U:1048576U)+offset;
  unsigned route=learned?descriptor[12]:descriptor[8];
  const uint32_t expected[]={write?local:remote,0,write?129U:route,write?remote:local,0,write?route:129U,0,write?0x212U:0,bytes,0};
  for(unsigned i=0;i<10;i++)assert(command[i]==expected[i]);
  if(write){assert(reads==6 && sumStores==5120 && normStores==5120);writes++;}else reads++;
  if(packets==failPacket){pending=true;return;}
  for(unsigned i=0;i<bytes/4;i++){
   if(!write)memory[local+i*4]=(learned?weight:rawRead?raw:projected)[offset/4+i];
   else (normalized?resident:backup)[offset/4+i]=memory.at(local+i*4);
  }
  unsigned at=write?4:0;counts[at]++;counts[at+1]+=bytes/64;counts[at+3]++;counts[2]++;counts[6]++;
  if(packets==sourceFault)source[11]^=1;
  if(packets==descriptorFault)memory[0x74200+41*4]^=1;
  if(packets==badCounter)counts[at+3]++;
 }
 bool Run(){while(pc!=Return && pc!=Fault)StepScalar();unknown=pc==Fault;assert(minStack>=0x6e000);return !unknown && regs[10]==5120;}
 void Verify(){
  assert(packets==10 && reads==6 && writes==4 && sumStores==5120 && normStores==5120);
  std::array<float,5120> sum;double square=0;
  for(unsigned i=0;i<5120;i++){
   sum[i]=PioneerAttentionFloat(raw[i])+PioneerAttentionFloat(projected[i]);square+=double(sum[i]*sum[i]);
   assert(backup[i]==PioneerAttentionBits(sum[i]));
  }
  float scale=1.f/sqrtf(float(square/5120)+1e-6f);
  for(unsigned i=0;i<5120;i++)assert(resident[i]==PioneerAttentionBits((sum[i]*scale)*PioneerAttentionFloat(weight[i])));
  assert(memory.at(0x74300+24*4)==descriptor[40] && memory.at(0x74300+27*4)==descriptor[50]);
 }
};
int main(int argc,char** argv){
 assert(argc==2);setvbuf(stdout,nullptr,_IOLBF,0);unsigned positives=0,faults=0,maxStack=0;
 for(unsigned trial=0;trial<6;trial++){
  LayerNormMachine m(argv[1],trial);assert(m.Run());m.Verify();maxStack=std::max(maxStack,0x70000-m.minStack);positives++;
 }
 for(unsigned kind=0;kind<4;kind++)for(unsigned at=1;at<=10;at++){
  LayerNormMachine m(argv[1],0);if(kind==0)m.failPacket=at;if(kind==1)m.sourceFault=at;if(kind==2)m.descriptorFault=at;if(kind==3)m.badCounter=at;
  assert(!m.Run() && m.unknown && m.packets==at);unsigned before=m.packets;
  assert(!m.Run() && m.packets==before);faults++;
 }
 for(unsigned kind=0;kind<3;kind++){
  LayerNormMachine m(argv[1],0);if(kind==0)m.projected[5119]=0x7fc00000;if(kind==1)m.raw[5119]=0x7f800000;if(kind==2)m.weight[5119]=1;
  assert(!m.Run() && m.unknown && m.packets==6 && !m.writes);faults++;
 }
 for(unsigned at=0;at<64;at++){
  LayerNormMachine m(argv[1],0);m.descriptor[at]^=1;m.SetDescriptor();
  if(PioneerQ38ResidentLayerNormShape(m.descriptor.data()))continue;
  assert(!m.Run() && !m.unknown && !m.packets);faults++;
 }
 printf("PASS ACTUAL COMPLETE-image norm.bin@158000/subroutine:positives%u ALL5120residual+normalized BITEXACT to independent F32add+FP64reduce/sqrtf;faults%u/40packet-source-descriptor-counter failures/late bad math before ANY GDDRwrite/invalid descriptors inert/maxStack%u;NO completed-prefix/native/fullROOT/FFN/silicon/token-rate claim\n",positives,faults,maxStack);
}
