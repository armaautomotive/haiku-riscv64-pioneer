/* SPDX-License-Identifier: MIT */
// Runs the linked writer with synthetic NoC and intercepted seal/quiet.
// This is not a complete ROOT lifecycle or physical device test.
#define PIONEER_TENSIX_MACHINE_ONLY
#include "pioneer_tensix_image_test.cpp"
#include <array>
#include <map>
#include <string>
#include <vector>

struct DenseTraceMachine:Machine {
 std::vector<uint8_t> text,conv;std::map<std::string,unsigned> symbols;std::map<uint32_t,uint32_t> memory;
 std::vector<std::array<uint32_t,3>> transfers;std::array<uint32_t,10> command={};
 bool pending=false,neverComplete=false,corruptAfterFirst=false;unsigned polls=0;
 static constexpr unsigned Return=0x17ff80,Owner=0x70000,Fault=0x60000;
 explicit DenseTraceMachine(const char* path){
  text=Read((std::string(path)+"/text.bin").c_str());conv=Read((std::string(path)+"/conv.bin").c_str());
  std::ifstream in(std::string(path)+"/program.symbols");assert(in.good());std::string line;
  while(std::getline(in,line)){unsigned at=0;char type=0,name[256]={};if(sscanf(line.c_str(),"%x %c %255s",&at,&type,name)==3)symbols[name]=at;}
 }
 static bool Range(uint32_t at,uint32_t first,uint32_t size){return at>=first && at-first<size;}
 bool CodeAddress(uint32_t at)const override{return Range(at,0x40000,unsigned(text.size())) || Range(at,0x64000,unsigned(conv.size()));}
 unsigned Index(uint32_t at)override{assert(!(at&3) && CodeAddress(at));l1[8189]=Range(at,0x40000,unsigned(text.size()))?Word(text,at-0x40000):Word(conv,at-0x64000);return 8189;}
 unsigned StepLimit()const override{return 20000000;}
 void Complete(){
  assert(pending);for(unsigned i=0;i<command[8];i+=4){assert(memory.count(command[0]+i));memory[command[3]+i]=memory.at(command[0]+i);}
  for(unsigned offset:{0x210U,0x204U,0x228U})memory[0xffb20000+offset]++;
  memory[0xffb20000+0x220]+=command[8]/64;memory[0xffb20040]=0;pending=false;
  if(corruptAfterFirst && transfers.size()==1)memory[0xffb20000]^=64;
 }
 uint32_t Load(uint32_t at)override{
  assert(!(at&3));if(CodeAddress(at))return l1[Index(at)];
  if(at==0xffb20040 && pending && !neverComplete && polls && !--polls)Complete();
  assert(memory.count(at));return memory.at(at);
 }
 void Store(uint32_t at,uint32_t value)override{
  assert(!(at&3));
  if(at==0xffb20040){
   assert(value==1 && !pending);for(unsigned i=0;i<10;i++)command[i]=memory.at(0xffb20000+i*4);
   assert(command[0]==0x78000 && command[2]==129 && command[7]==0x212 && (command[8]==16384 || command[8]==4096));
   transfers.push_back({command[3],command[8],command[5]});pending=true;polls=4;memory[at]=1;return;
  }
  assert(Range(at,0x6e000,8192) || Range(at,0x78000,16384) || Range(at,0xffb20000,40));memory[at]=value;
 }
 bool Call(unsigned workspace,unsigned layer,unsigned position,unsigned stage,unsigned phase){
  const char* writer="_ZN5BlockL15TraceDenseStageEPN30PioneerQ38MixedBlockRootLayout11PrivateSealEj";
  const char* seal="q38_layer_return_dense_seal";const char* quiet="_ZN18DensePartitionRootL5QuietEv";
  assert(symbols.count(writer) && symbols.count(seal) && symbols.count(quiet));
  memory[Owner+268]=layer;memory[Owner+272]=position;memory[Owner+288]=0x1234;memory[Owner+292]=workspace;memory[Owner+1024]=phase;
  for(unsigned offset:{0x204U,0x208U,0x20cU,0x210U,0x214U,0x220U,0x228U})memory[0xffb20000+offset]=0;
  for(unsigned i=0;i<20480;i+=4){memory[0x80000+i]=0x51a00000U+i;memory[0x156000+i]=0x52b00000U+i;}
  pc=symbols.at(writer);regs[1]=Return;regs[2]=0x70000;regs[10]=Owner;regs[11]=stage;
  while(pc!=Return && pc!=Fault){
   if(pc==symbols.at(seal)){pc=regs[1];}
   else if(pc==symbols.at(quiet)){regs[10]=1;pc=regs[1];}
   else Step();
  }
  return pc==Return;
 }
};
int main(int argc,char** argv){
 assert(argc==2);
 for(unsigned base:{0x04000000U,0xc0000000U})for(unsigned stage:{0U,1U}){
  DenseTraceMachine m(argv[1]);assert(m.Call(base,3,0,stage,stage?7:6));
  const unsigned dest=base+0x3500000+stage*20480;
  const std::vector<std::array<uint32_t,3>> expected={{dest,16384,0x1234},{dest+16384,4096,0x1234}};
  assert(m.transfers==expected && !m.pending && m.regs[2]==0x70000);
  for(unsigned i=0;i<20480;i+=4)assert(m.memory.at(dest+i)==(stage?0x52b00000U:0x51a00000U)+i);
 }
 for(unsigned layer:{0U,2U,4U,63U}){DenseTraceMachine m(argv[1]);assert(m.Call(0x04000000,layer,0,0,6) && m.transfers.empty());}
 {DenseTraceMachine m(argv[1]);assert(m.Call(0x04000000,3,1,0,6) && m.transfers.empty());}
 for(unsigned stage:{0U,1U,2U}){DenseTraceMachine m(argv[1]);assert(!m.Call(0x04000000,3,0,stage,stage?6:7) && m.transfers.empty());}
 for(unsigned workspace:{0x03fff000U,0x04000001U,0xc0001000U}){
  DenseTraceMachine m(argv[1]);assert(!m.Call(workspace,3,0,0,6) && m.transfers.empty());
 }
 {DenseTraceMachine m(argv[1]);m.corruptAfterFirst=true;assert(!m.Call(0x04000000,3,0,0,6) && m.transfers.size()==1);}
 {DenseTraceMachine m(argv[1]);m.neverComplete=true;assert(!m.Call(0x04000000,3,0,0,6) && m.transfers.size()==1 && m.pending);}
 puts("PASS emitted dense stage writer: 2 stages, address extremes, delayed synthetic NoC, phase/range refusals, corrupt command and timeout; SEAL/QUIET INTERCEPTED, NO DEVICE");
}
