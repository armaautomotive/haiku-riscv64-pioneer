/* SPDX-License-Identifier: MIT */
// Actual linked configuration/panel entry points with a synthetic backend.
// This does not supply the full worker's weight source, matrix or receipt.
#define PIONEER_TENSIX_MACHINE_ONLY
#include "pioneer_tensix_image_test.cpp"
#include "../PioneerQ38TwinNativeQ8.h"
#include <array>
#include <utility>

namespace N=PioneerQ38TwinNativeQ8;
namespace {
constexpr unsigned Text=0x72000, Affine=0x70b00, Stack=0x6e000, Top=0x70000;
constexpr unsigned Ack=0xffe000fc, Config=0xffef0000, Job=0x74000;
constexpr unsigned Return=0x60004;

struct PanelMachine:Machine {
 const std::vector<uint8_t>& text;
 const std::vector<uint8_t>& affine;
 const unsigned fault;
 std::array<uint32_t,(Top-Stack)/4> stack={};
 std::array<uint32_t,16> job={};
 std::vector<std::pair<unsigned,unsigned>> writes;
 std::vector<unsigned> ops;
 unsigned ack=0,acks=0,readbacks=0,checking=0;
 bool stopped=false,waiting=false;
 PanelMachine(const std::vector<uint8_t>& t,const std::vector<uint8_t>& a,unsigned bad):text(t),affine(a),fault(bad){}
 bool CodeAddress(unsigned a)const override{
  return !(a&3) && ((a>=Text && a-Text<text.size()) || (a>=Affine && a-Affine<affine.size()));
 }
 unsigned Index(unsigned a)override{assert(CodeAddress(a));l1[8191]=a>=Text?Word(text,a-Text):Word(affine,a-Affine);return 8191;}
 bool InlineTensix()const override{return true;}
 uint32_t Load(uint32_t a)override{
  assert(!(a&3));
  if(a>=Stack && a<Top)return stack[(a-Stack)/4];
  if(a>=Job && a<Job+64)return job[(a-Job)/4];
  if(a==Ack)return ack;
  if(checking && a==checking){readbacks++;checking=0;return writes.back().second;}
  assert(false && "unexpected emitted Q8 load");return 0;
 }
 void Store(unsigned a,uint32_t value)override{
  assert(!(a&3));
  if(a>=Stack && a<Top){stack[(a-Stack)/4]=value;return;}
  if(a==Ack){assert(value==0 && !waiting && !checking);ack=0;return;}
  if(a==0xffe40000){Instruction(value);return;}
  assert(!checking && !waiting && a>=Config && a<Config+224*4);
  checking=a;writes.emplace_back((a-Config)/4,value);
 }
 void Instruction(uint32_t op)override{
  assert(!checking);
  if(op==0xa2101816){assert(!waiting && !ack);waiting=true;}
  else if(op==0x4500017e){assert(waiting && !ack);waiting=false;ack=1;acks++;}
  else{assert(!waiting && ack);ops.push_back(op);}
 }
 bool Run(unsigned entry,unsigned a,unsigned b){
  for(unsigned i=1;i<32;i++)regs[i]=0x12345000+i;
  regs[1]=Return;regs[2]=Top;regs[10]=a;regs[11]=b;pc=entry;
  while(pc!=Return && pc!=fault)Step();
  stopped=pc==fault;
  assert(!waiting && !checking && readbacks==writes.size());
  if(!stopped){
   assert(regs[2]==Top);
   for(unsigned i:{8U,9U,18U,19U,20U,21U,22U,23U,24U,25U,26U,27U})assert(regs[i]==0x12345000+i);
  }
  return !stopped;
 }
 bool Inert()const{return writes.empty() && ops.empty() && !acks;}
};
}

int main(int argc,char** argv){
 assert(argc==6);
 const std::string image=argv[1];
 const auto text=Read((image+"/text.bin").c_str()),affine=Read((image+"/affine.bin").c_str());
 const unsigned format=unsigned(std::stoul(argv[2],nullptr,16)),panel=unsigned(std::stoul(argv[3],nullptr,16));
 const unsigned compute=unsigned(std::stoul(argv[4],nullptr,16)),bad=unsigned(std::stoul(argv[5],nullptr,16));
 for(bool enter:{false,true}){
  PanelMachine m(text,affine,bad);
  assert(m.Run(format,enter,0) && m.acks==2);
  const std::vector<std::pair<unsigned,unsigned>> expected={{0,enter?0x423eU:0x4231U},
   {1,enter?0xa03c0000U:0xa03c8000U},{73,enter?3U:0x13U}};
  assert(m.writes==expected && m.ops==(enter?std::vector<unsigned>{0x8a000002}:std::vector<unsigned>{}));
 }
 const std::vector<unsigned> sequence={0x3700000f,0x5e23fc00,0x5120000a,0x5420000f,0x420000c1};
 unsigned panels=0,refusals=0;
 for(unsigned base:{unsigned(N::T::Buffer0),unsigned(N::T::Buffer1)})for(unsigned i=0;i<4;i++)for(unsigned p=0;p<2;p++){
  PanelMachine m(text,affine,bad);
  const unsigned at=base+i*576;
  assert(m.Run(panel,at,p) && m.acks==2 && m.ops==sequence);
  assert((m.writes==std::vector<std::pair<unsigned,unsigned>>{{76,(at+32+256*p-16)/16}}));panels++;
 }
 for(unsigned at:{0x90000U,0x98001U,0x98900U,0x9a001U,0xa0000U,0xffffffffU}){
  PanelMachine m(text,affine,bad);assert(!m.Run(panel,at,0) && m.Inert());refusals++;
 }
 {PanelMachine m(text,affine,bad);assert(!m.Run(panel,N::T::Buffer0,2) && m.Inert());refusals++;}
 for(unsigned type:{8U,12U,14U})for(unsigned mode:{2U,3U,0xffffffffU}){
  if(type==8 && mode==2)continue;
  PanelMachine m(text,affine,bad);m.job[1]=type;m.job[14]=mode;
  assert(m.Run(compute,0,0) && !m.regs[10] && m.Inert());refusals++;
 }
 printf("PASS linked Q8 setup/restore, %u panel launches and %u inert refusals; real RV32/config readbacks, synthetic ACK, NO full native Q8 job or card timing\n",
  panels,refusals);
}
