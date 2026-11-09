/* SPDX-License-Identifier: MIT */
// Execute the actual linked trace helper with Owner and Transfer intercepted.
// This checks the emitted call sequence, NOT NoC, ROOT lifecycle or hardware.
#define PIONEER_TENSIX_MACHINE_ONLY
#include "pioneer_tensix_image_test.cpp"
#include <array>
#include <map>
#include <string>
#include <vector>

struct TraceMachine:Machine {
 std::vector<uint8_t> text,conv;std::map<std::string,unsigned> symbols;std::map<uint32_t,uint32_t> memory;
 std::vector<std::array<uint32_t,3>> transfers;
 static constexpr unsigned Return=0x17ff80,Owner=0x70000,Fault=0x60000;
 explicit TraceMachine(const char* path){
  text=Read((std::string(path)+"/text.bin").c_str());conv=Read((std::string(path)+"/conv.bin").c_str());
  std::ifstream file(std::string(path)+"/program.symbols");assert(file.good());std::string line;
  while(std::getline(file,line)){unsigned at=0;char type=0,name[256]={};if(sscanf(line.c_str(),"%x %c %255s",&at,&type,name)==3)symbols[name]=at;}
 }
 static bool Range(unsigned at,unsigned first,unsigned size){return at>=first && at-first<size;}
 bool CodeAddress(uint32_t at)const override{return Range(at,0x40000,unsigned(text.size())) || Range(at,0x64000,unsigned(conv.size()));}
 unsigned Index(uint32_t at)override{
  assert(!(at&3) && CodeAddress(at));
  l1[8189]=Range(at,0x40000,unsigned(text.size()))?Word(text,at-0x40000):Word(conv,at-0x64000);return 8189;
 }
 uint32_t Load(uint32_t at)override{assert(!(at&3) && memory.count(at));return memory.at(at);}
 void Store(uint32_t at,uint32_t value)override{assert(!(at&3) && Range(at,0x6e000,8192));memory[at]=value;}
 bool Call(unsigned layer,unsigned workspace,unsigned phase,unsigned position,unsigned completed,unsigned storedLayer){
  const char* trace="_ZN11GlobalChainL10TraceLayerEj";const char* owner="_ZN11GlobalChainL5OwnerEv";
  const char* transfer="_ZN11GlobalChainL8TransferEjjb";
  assert(symbols.count(trace) && symbols.count(owner) && symbols.count(transfer));
  memory[Owner+1024]=phase;memory[Owner+832]=position;memory[Owner+780]=completed;
  memory[Owner+776]=storedLayer;memory[Owner+772]=workspace;
  pc=symbols.at(trace);regs[1]=Return;regs[2]=0x70000;regs[10]=layer;
  while(pc!=Return && pc!=Fault){
   if(pc==symbols.at(owner)){regs[10]=Owner;pc=regs[1];}
   else if(pc==symbols.at(transfer)){transfers.push_back({regs[10],regs[11],regs[12]});pc=regs[1];}
   else Step();
  }
  return pc==Return;
 }
};
int main(int argc,char** argv){
 assert(argc==2);
 for(unsigned layer=0;layer<64;layer++)for(unsigned base:{0x04000000U,0xc0000000U}){
  TraceMachine m(argv[1]);assert(m.Call(layer,base,1,0,layer+1,layer));
  const std::vector<std::array<uint32_t,3>> expected={
   {base+0x427000,16384,0},{base+0x3500000+layer*20480,16384,1},
   {base+0x42b000,4096,0},{base+0x3500000+layer*20480+16384,4096,1}};
  assert(m.transfers==expected && m.regs[2]==0x70000);
 }
 for(unsigned fault=0;fault<4;fault++){
  TraceMachine m(argv[1]);assert(!m.Call(0,0x04000000,fault==0?2:1,fault==1?1:0,fault==2?2:1,fault==3?1:0));
  assert(m.transfers.empty());
 }
 puts("PASS emitted trace helper: all64 boundaries, address extremes, four scoped transfer calls, four state refusals; intercepted Owner/Transfer, NO ROOT lifecycle/device authority");
}
