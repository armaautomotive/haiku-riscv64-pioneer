/* SPDX-License-Identifier: MIT */
// Execute the ACTUAL emitted PublishToken caller and frontier leaf. The
// Head/Input/Feedback seals and Packet transport below are explicit TEST
// dependencies, NOT executed arithmetic, real NoC, native owners or timing.
#define PIONEER_TENSIX_MACHINE_ONLY
#include "pioneer_tensix_image_test.cpp"
#include <array>
#include <memory>
#include <string>
#include "../PioneerQ38TokenLayout.h"

namespace PT=PioneerQ38TokenLayout;
namespace PC=PioneerQ38GlobalChainLayout;
struct PublicationMachine:Machine {
#include "actual_token_publication_private.inc"
 enum {Parent=0x6f000,Stack=0x6e000,StackEnd=Parent,Return=0x60004,
  Frontier=PC::Progress+12,Header=PC::Head};
 struct Image {
  std::vector<uint8_t> text,conv,terminal;
  unsigned publish,packet,head,input,feedback;bool frontier;
 };
 const Image& image;
 Private owner={};
 std::array<uint32_t,(PC::R::Scratch+256-Stack)/4> memory={};
 std::array<uint32_t,64> record={},expected={};
 std::vector<unsigned> events,wanted;
 unsigned packetCalls=0,readbackLoads=0,frontierWrites=0,fences=0;
 unsigned failEvent=0,corruptRead=0,corruptWord=0;
 bool feedback;
 PublicationMachine(const Image& code,unsigned position,unsigned bank,bool feed):image(code),feedback(feed){
  owner.phase=6;owner.state[Bank]=bank;owner.state[Workspace]=0x4000000;
  owner.state[Position]=position;owner.state[Token]=position?17:248319;
  owner.state[Committed]=position+1;owner.state[Slot]=(position+1)&1;owner.state[PromptCount]=2;
  for(unsigned i=0;i<64;i++)owner.boot[i]=0x13570000U+i*0x147U;
  for(unsigned i=0;i<8;i++)owner.state[Counts+i]=0x24680000U+i*0x237U;
  const unsigned head[]={PT::TokenMagic,1,position,owner.state[Token],70253,0x413bc1de,
   position+1,(position+1)&1,unsigned(position+1<2),unsigned(feed),64,973};
  std::copy(head,head+12,expected.begin());
  for(unsigned i=0;i<8;i++){expected[12+i]=owner.boot[8+i];expected[36+i]=owner.state[Counts+i];}
  for(unsigned i=0;i<16;i++)expected[20+i]=owner.boot[16+i];
  expected[44]=position+1;expected[45]=48;expected[46]=119;
  wanted={1,2};if(feed)wanted.push_back(3);
  for(unsigned i=0;i<4;i++)wanted.push_back(4+i);
  wanted.push_back(1);wanted.push_back(2);if(feed)wanted.push_back(3);
  for(unsigned reg=1;reg<32;reg++)regs[reg]=0x13570000U+reg*0x147U;
  regs[1]=Return;regs[2]=StackEnd;regs[27]=Parent;
  regs[10]=Header;regs[11]=70253;regs[12]=0x413bc1de;regs[13]=unsigned(feed);pc=image.publish;
 }
 uint32_t& Data(unsigned address){
  assert(!(address&3) && address>=Stack && address<PC::R::Scratch+256);
  return memory[(address-Stack)/4];
 }
 uint32_t OwnerWord(unsigned ordinal)const{
  assert(ordinal<257);
  return ordinal<64?owner.boot[ordinal]:ordinal<192?owner.outer[ordinal-64]:ordinal<256?owner.state[ordinal-192]:owner.phase;
 }
 bool CodeAddress(unsigned address)const override{
  return (address>=0x40000 && address-0x40000<image.text.size())
   || (address>=0x64000 && address-0x64000<image.conv.size()) || (address>=0x60000 && address<0x60008);
 }
 uint32_t CodeWord(unsigned address)const{
  assert(CodeAddress(address) && !(address&3));
  return address>=0x64000?Word(image.conv,address-0x64000):address>=0x60000?Word(image.terminal,address-0x60000):Word(image.text,address-0x40000);
 }
 unsigned Index(unsigned address)override{l1[8180]=CodeWord(address);return 8180;}
 uint32_t Load(uint32_t address)override{
  assert(!(address&3));
  if(address>=Parent && address<Parent+sizeof(owner))return OwnerWord((address-Parent)/4);
  if(address>=Stack && address<StackEnd)return Data(address);
  if(address>=PC::R::Scratch && address<PC::R::Scratch+256){
   assert(packetCalls==2 || packetCalls==4);readbackLoads++;return Data(address);
  }
  return CodeWord(address); // no MMIO,CSR,other owned state or hidden reads
 }
 void Store(uint32_t address,uint32_t value)override{
  assert(!(address&3));
  if(address==Frontier){
   assert(image.frontier && !frontierWrites && events==wanted && readbackLoads==128
    && packetCalls==4 && record==expected && value==owner.state[Committed]);
   frontierWrites++;Data(address)=value;return;
  }
  assert((address>=Stack && address<StackEnd) || (address>=PC::R::Scratch && address<PC::R::Scratch+256));
  Data(address)=value; // ONLY actual caller stack/scratch,not parent/record/progress mutation
 }
 unsigned StepLimit()const override{return 16384;}
 bool Dependency(){
  const unsigned event=pc==image.head?1:pc==image.input?2:pc==image.feedback?3:pc==image.packet?4+packetCalls:0;
  if(!event)return false;
  assert(events.size()<wanted.size() && wanted[events.size()]==event && !frontierWrites && !Data(Frontier));
  if(event!=2)assert(regs[10]==Header);
  events.push_back(event);
  if(failEvent==events.size()){pc=0x60000;return true;} // explicit failing TEST dependency
  if(event>=4){
   const unsigned address=owner.state[Workspace]+PT::OutputOffset+owner.state[Position]*256;
   assert(regs[11]==PioneerQ38ProjectionRoute(owner.state[Bank]) && regs[12]==address);
   assert(regs[13]==(packetCalls==2?4U:256U) && regs[14]==unsigned(!(packetCalls&1)));
   if(packetCalls==0){
    for(unsigned i=0;i<64;i++){record[i]=Data(PC::R::Scratch+i*4);assert(record[i]==(i?expected[i]:0U));}
   }else if(packetCalls==2){
    assert(readbackLoads==64 && Data(PC::R::Scratch)==PT::TokenMagic);record[0]=Data(PC::R::Scratch);
    assert(record==expected);
   }else{
    for(unsigned i=0;i<64;i++)Data(PC::R::Scratch+i*4)=record[i];
    if(corruptRead==(packetCalls==1?1U:2U))Data(PC::R::Scratch+corruptWord*4)^=1;
   }
   packetCalls++;
  }
  pc=regs[1];return true;
 }
 bool Run(){
  const Private before=owner;uint32_t original[32];std::copy(regs,regs+32,original);
  while(pc!=Return && pc!=0x60000){
   assert(steps<StepLimit());if(Dependency())continue;
   if(CodeWord(pc)==0x0ff0000f){assert(image.frontier && frontierWrites==1);fences++;}
   Step();
  }
  assert(!memcmp(&owner,&before,sizeof(owner)));
  if(pc==0x60000){assert(!frontierWrites && !Data(Frontier) && !fences);return false;}
  assert(events==wanted && packetCalls==4 && readbackLoads==128 && record==expected);
  assert(frontierWrites==unsigned(image.frontier) && fences==unsigned(image.frontier)
   && Data(Frontier)==(image.frontier?owner.state[Committed]:0U));
  for(unsigned reg:{2U,3U,4U,8U,9U,18U,19U,20U,21U,22U,23U,24U,25U,26U,27U})assert(regs[reg]==original[reg]);
  for(unsigned i=0;i<64;i++)if(i!=3)assert(!Data(PC::Progress+i*4));
  for(unsigned i=0;i<2048;i++)Step();
  assert(pc==Return && frontierWrites==unsigned(image.frontier));return true;
 }
};

int main(int argc,char** argv){
 assert(argc==9);const std::string directory=argv[1];
 PublicationMachine::Image image={Read((directory+"/text.bin").c_str()),Read((directory+"/conv.bin").c_str()),
  Read((directory+"/terminal.bin").c_str()),unsigned(std::stoul(argv[2],nullptr,16)),unsigned(std::stoul(argv[3],nullptr,16)),
  unsigned(std::stoul(argv[4],nullptr,16)),unsigned(std::stoul(argv[5],nullptr,16)),unsigned(std::stoul(argv[6],nullptr,16)),std::string(argv[7])=="1"};
 assert(std::string(argv[7])==(image.frontier?"1":"0") && std::string(argv[8])=="--mocked-dependencies");
 unsigned good=0,bad=0;
 for(unsigned position=0;position<8192;position++){
  auto machine=std::make_unique<PublicationMachine>(image,position,position%7,(position&1)!=0);
  assert(machine->Run());good++;
 }
 for(bool feedback:{false,true}){
  for(unsigned read=1;read<=2;read++)for(unsigned word=0;word<64;word++){
   auto machine=std::make_unique<PublicationMachine>(image,33,6,feedback);
   machine->corruptRead=read;machine->corruptWord=word;assert(!machine->Run());bad++;
  }
  const unsigned eventCount=feedback?10:8;
  for(unsigned event=1;event<=eventCount;event++){
   auto machine=std::make_unique<PublicationMachine>(image,33,6,feedback);
   machine->failEvent=event;assert(!machine->Run());bad++;
  }
  for(unsigned phase=0;phase<8;phase++)if(phase!=6){
   auto machine=std::make_unique<PublicationMachine>(image,33,6,feedback);
   machine->owner.phase=phase;assert(!machine->Run() && machine->events.empty());bad++;
  }
  for(unsigned committed:{0U,33U,35U,UINT32_MAX}){
   auto machine=std::make_unique<PublicationMachine>(image,33,6,feedback);
   machine->owner.state[PublicationMachine::Committed]=committed;
   assert(!machine->Run() && machine->events.empty());bad++;
  }
 }
 printf("PASS actual emitted %s PublishToken caller/%u complete positions/%u readback+dependency+phase+frontier failures/actual Owner+record construction+64word comparisons+branch+return+ABI/no parent mutation/4 exact MOCK NoC packets/6-or4 MOCK seals/%s/inert2048;NO seal-body,transport,model,native,physical,timing or TPS proof\n",
  image.frontier?"frontier":"unchanged-control",good,bad,image.frontier?"count+fence ONLY AFTER both readbacks and final seals":"NO counter publication");
}
