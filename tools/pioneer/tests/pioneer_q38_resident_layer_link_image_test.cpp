/* SPDX-License-Identifier: MIT */
// ACTUAL link instructions, synthetic predecessor DATA only. This direct
// subroutine test does NOT establish that a preceding layer ran. The full
// joined test separately runs that real layer and retains its byte outputs.
#define PIONEER_GRAPH_MATH_MACHINE_ONLY
#include "pioneer_graph_math_image_test.cpp"
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentLayerLinkPlan.h"
#include <array>
#include <memory>
struct LayerLinkMachine:ScalarMachine {
 std::vector<uint8_t> normCode;
 std::array<uint32_t,128> descriptor={};std::array<uint32_t,3520> parent={};
 std::array<uint32_t,5120> input={},weight={},resident={},backup={};
 uint32_t command[10]={},counts[8]={},initial[8]={},source[16]={};
 unsigned capture=0,entry=0,packets=0,reads=0,writes=0,normStores=0;
 unsigned failPacket=0,sourceFault=0,descriptorFault=0,parentFault=0,badCounter=0,commandFault=0;
 bool pending=false,unknown=false;
 static constexpr unsigned Return=0x17ff80,Fault=0x72060;
 static unsigned Counter(unsigned i){const unsigned o[]={0x208,0x20c,0x210,0x214,0x204,0x220,0x210,0x228};return o[i];}
 static unsigned Source(unsigned i){return !i?0x100:i<=6?0x118+(i-1)*4:i<=12?0x130+(i-7)*4:i==13?0x150:i==14?0x154:0x16c;}
 LayerLinkMachine(const char* image,unsigned trial):ScalarMachine(5120,0){
  code=Read((std::string(image)+"/link.bin").c_str());normCode=Read((std::string(image)+"/norm.bin").c_str());
  assert(!code.empty() && code.size()<=0x2a00 && normCode.size()==5404);
  std::ifstream symbols(std::string(image)+"/program.symbols");std::string line;
  while(std::getline(symbols,line)){
   unsigned at=0;char type=0,name[256]={};if(sscanf(line.c_str(),"%x %c %255s",&at,&type,name)!=3)continue;
   if(!strcmp(name,"q38_resident_layer_link_capture"))capture=at;
   if(!strcmp(name,"q38_resident_layer_link_once"))entry=at;
  }
  assert(CodeAddress(capture) && CodeAddress(entry));
  auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(canonical->Build());
  auto plan=std::make_unique<PioneerQ38ResidentLayerPlan>();std::array<uint32_t,1920> sources;
  std::array<uint32_t,119> sequence={};sources.fill(0x16005);
  for(unsigned i=0;i<16;i++)sources[i]=source[i]=i?0x13570000+i*0x12345:0x16005;
  unsigned layer=trial&1?62:0,bank=trial&1?6:0,position=trial&1?8191:0,token=trial&1?248319:0;
  assert(plan->Build(*canonical,layer,position,bank,trial&1,token,17,{1,2,3,4},sources.data(),sequence.data()));
  PioneerQ38ResidentLayerLinkPlan link;assert(link.Build(*canonical,*plan));
  for(unsigned i=0;i<128;i++)descriptor[i]=link.Descriptor()[i];
  for(unsigned i=0;i<3520;i++)parent[i]=plan->LayerManifest()[i];
  for(unsigned i=0;i<128;i++)memory[TT_Q38_LAYER_LINK_MANIFEST+i*4]=descriptor[i];
  for(unsigned i=0;i<3520;i++)memory[TT_Q38_RESIDENT_LAYER_MANIFEST+i*4]=memory[TT_Q38_RESIDENT_LAYER_SHADOW+i*4]=parent[i];
  for(unsigned i=0;i<8;i++)initial[i]=counts[i]=memory[TT_Q38_FFN_RESIDUAL_GRAPH_STATS+(24+i)*4]=0xfffffff8;
  const uint32_t header[]={TT_Q38_RESIDENT_LAYER_RECEIPT_MARKER,2,256,layer,position,token,parent[5],parent[6],parent[7],bank,descriptor[8],descriptor[72],TT_Q38_RESIDENT_LAYER_MARKER,5120,3,1};
  for(unsigned i=0;i<16;i++){
   memory[TT_Q38_RESIDENT_LAYER_RECEIPT+i*4]=header[i];memory[TT_Q38_RESIDENT_LAYER_RECEIPT+(16+i)*4]=source[i];
  }
  for(unsigned i=0;i<8;i++)memory[TT_Q38_RESIDENT_LAYER_RECEIPT+(224+i)*4]=descriptor[32+i];
  const uint32_t stats[]={TT_Q38_FFN_RESIDUAL_GRAPH_MARKER,3,1};
  for(unsigned i=0;i<3;i++)memory[TT_Q38_FFN_RESIDUAL_GRAPH_STATS+i*4]=stats[i];
  const uint32_t end[]={9,3,20,68,20480,descriptor[9],layer,1,0x33524651,20480,4,2,descriptor[9],position,token};
  for(unsigned i=0;i<15;i++)memory[TT_Q38_FFN_RESIDUAL_GRAPH_STATS+(49+i)*4]=end[i];
  const uint32_t last[]={0x9d000,0,129,descriptor[8]+16384,0,descriptor[72],0,0x212,4096,0};
  for(unsigned i=0;i<10;i++)command[i]=last[i];
  for(unsigned i=0;i<5120;i++){
   float x=float(int((i*47+trial*97)%511)-255)*.03125f;
   if(trial==1)x=65504.f;if(trial==2)x=PioneerAttentionFloat(0x26800000);if(trial==3)x=-0.f;
   input[i]=resident[i]=PioneerAttentionBits(x);backup[i]=0xdeadbeef;
   weight[i]=PioneerAttentionBits(float(int((i*71+trial*13)%127)-63)*.0234375f);
  }
 }
 bool CodeAddress(uint32_t at)const override{return Range(at,TT_Q38_LAYER_LINK_ROM,unsigned(code.size())) || Range(at,0x158000,unsigned(normCode.size()));}
 uint32_t CodeWord(uint32_t at)const{return Range(at,0x158000,unsigned(normCode.size()))?Word(normCode,at-0x158000):Word(code,at-TT_Q38_LAYER_LINK_ROM);}
 unsigned Index(uint32_t at)override{assert(CodeAddress(at) && !(at&3));l1[1]=CodeWord(at);return 1;}
 uint32_t Load(uint32_t at)override{
  assert(!(at&3));if(CodeAddress(at))return CodeWord(at);
  if(Range(at,0xffb20000,0x2000)){
   unsigned offset=at-0xffb20000;
   if(offset<40)return command[offset/4];
   for(unsigned i=0;i<4;i++)if(offset==0x40+i*0x800)return i?0:unsigned(pending);
   if(offset>=0x240 && offset<0x2c0)return 0;
   for(unsigned i=0;i<16;i++)if(offset==Source(i))return source[i];
   for(unsigned i=0;i<8;i++)if(offset==Counter(i))return counts[i];
   assert(!"unqualified link CSR");
  }
  assert(Range(at,0x6e000,8192) || Range(at,0x80000,20480) || Range(at,0x88000,20480) || Range(at,0x90000,20480)
   || Range(at,TT_Q38_RESIDENT_LAYER_MANIFEST,3520*4) || Range(at,TT_Q38_RESIDENT_LAYER_SHADOW,3520*4)
   || Range(at,TT_Q38_RESIDENT_LAYER_RECEIPT,1024) || Range(at,TT_Q38_FFN_RESIDUAL_GRAPH_STATS,256)
   || Range(at,TT_Q38_LAYER_LINK_MANIFEST,1408));return memory.at(at);
 }
 void Store(uint32_t at,uint32_t value)override{
  assert(!(at&3));
  if(Range(at,0xffb20000,40)){assert(!pending && !unknown);command[(at-0xffb20000)/4]=value;return;}
  if(at==0xffb20040){assert(value==1 && !pending && !unknown);Packet();return;}
  assert(Range(at,0x6e000,8192) || Range(at,0x90000,20480) || Range(at,TT_Q38_LAYER_LINK_SHADOW,512)
   || Range(at,TT_Q38_LAYER_LINK_RECEIPT,384));
  if(Range(at,0x6e000,8192))minStack=std::min(minStack,unsigned(at));
  if(Range(at,0x90000,20480))normStores++;
  memory[at]=value;
 }
 void Packet(){
  unsigned p=packets++;assert(p<8);bool write=p>=4,learned=p>=2 && p<4,normalized=p>=6;
  unsigned offset=(p&1)*16384,bytes=p&1?4096:16384;
  unsigned local=(write?(normalized?0x90000:0x80000):learned?0x88000:0x80000)+offset;
  unsigned remote=(learned?descriptor[75]:descriptor[8])+offset+(write && !normalized?98304:0);
  unsigned route=learned?descriptor[76]:descriptor[72];
  const uint32_t expect[]={write?local:remote,0,write?129U:route,write?remote:local,0,write?route:129U,0,write?0x212U:0,bytes,0};
  for(unsigned i=0;i<10;i++)assert(command[i]==expect[i]);
  if(write){assert(reads==4 && normStores==5120);writes++;}else reads++;
  if(packets==failPacket){pending=true;return;}
  for(unsigned i=0;i<bytes/4;i++){
   if(!write)memory[local+i*4]=(learned?weight:input)[offset/4+i];
   else (normalized?resident:backup)[offset/4+i]=memory.at(local+i*4);
  }
  unsigned at=write?4:0;counts[at]++;counts[at+1]+=bytes/64;counts[at+3]++;counts[2]++;counts[6]++;
  if(packets==sourceFault)source[11]^=1;
  if(packets==descriptorFault)memory[TT_Q38_LAYER_LINK_MANIFEST+75*4]^=1;
  if(packets==parentFault)memory[TT_Q38_RESIDENT_LAYER_MANIFEST+13*4]^=1;
  if(packets==badCounter)counts[at+3]++;
  if(packets==commandFault)command[1]^=1;
 }
 bool Call(unsigned at){pc=at;regs[1]=Return;regs[2]=0x70000;while(pc!=Return && pc!=Fault)StepScalar();unknown=pc==Fault;assert(minStack>=0x6e000);return !unknown;}
 void Verify(){
  assert(packets==8 && reads==4 && writes==4 && normStores==5120);
  double square=0;for(unsigned i=0;i<5120;i++){float x=PioneerAttentionFloat(input[i]);square+=double(x*x);assert(backup[i]==input[i]);}
  float scale=1.f/sqrtf(float(square/5120)+1e-6f);
  for(unsigned i=0;i<5120;i++)assert(resident[i]==PioneerAttentionBits((PioneerAttentionFloat(input[i])*scale)*PioneerAttentionFloat(weight[i])));
  std::array<uint32_t,96> r;for(unsigned i=0;i<96;i++)r[i]=memory.at(TT_Q38_LAYER_LINK_RECEIPT+i*4);
  assert(PioneerQ38ResidentLayerLinkReceipt(r.data(),descriptor.data(),parent.data(),initial,counts,command));
  for(unsigned i=0;i<96;i++){r[i]^=1;assert(!PioneerQ38ResidentLayerLinkReceipt(r.data(),descriptor.data(),parent.data(),initial,counts,command));r[i]^=1;}
 }
};
int main(int argc,char** argv){
 assert(argc==2);setvbuf(stdout,nullptr,_IOLBF,0);unsigned faults=0;
 for(unsigned trial=0;trial<4;trial++){
  LayerLinkMachine m(argv[1],trial);assert(m.Call(m.capture) && !m.packets && m.Call(m.entry));m.Verify();
  unsigned before=m.packets;assert(!m.Call(m.entry) && m.packets==before); // actual counter ancestry refuses duplicate
 }
 for(unsigned kind=0;kind<6;kind++)for(unsigned at=1;at<=8;at++){
  LayerLinkMachine m(argv[1],0);assert(m.Call(m.capture));
  if(kind==0)m.failPacket=at;if(kind==1)m.sourceFault=at;if(kind==2)m.descriptorFault=at;
  if(kind==3)m.parentFault=at;if(kind==4)m.badCounter=at;if(kind==5)m.commandFault=at;
  assert(!m.Call(m.entry) && m.packets==at && !m.memory.at(TT_Q38_LAYER_LINK_RECEIPT));
  assert(!m.Call(m.entry) && m.packets==at);faults++;
 }
 for(unsigned kind=0;kind<4;kind++){
  LayerLinkMachine m(argv[1],0);assert(m.Call(m.capture));
  if(kind==0)m.input[5119]=0x7fc00000;if(kind==1)m.weight[5119]=1;
  if(kind==2)m.memory[TT_Q38_RESIDENT_LAYER_RECEIPT+5*4]^=1;
  if(kind==3)m.memory[TT_Q38_FFN_RESIDUAL_GRAPH_STATS+61*4]^=1;
  assert(!m.Call(m.entry) && m.packets==(kind<2?4U:0U) && !m.writes && !m.memory.at(TT_Q38_LAYER_LINK_RECEIPT));faults++;
 }
 printf("PASS ACTUAL link+retained norm instructions:4 boundary/domain positives ALL5120 normalized+raw BITEXACT independent CPU observer/384receipt mutations/4duplicate refusals/%u packet-source-manifest-parent-counter-command-math-predecessor faults;NO CPU feedback/no native/full-layer/silicon/rate claim\n",faults);
}
