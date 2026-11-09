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
 std::vector<std::array<uint32_t,3>> transfers;bool executeTransfers=false,corruptAfterFirst=false,neverComplete=false;
 std::array<uint32_t,10> pending={};unsigned pendingPolls=0,delayReads=0;bool pendingActive=false;
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
 unsigned StepLimit()const override{return 20000000;}
 void Complete(){
  assert(pendingActive);const auto& c=pending;const bool write=c[2]==129;
  for(unsigned i=0;i<c[8];i+=4){assert(memory.count(c[0]+i));memory[c[3]+i]=memory.at(c[0]+i);}
  const unsigned offsets[]={0x208,0x20c,0x210,0x214,0x204,0x220,0x228};
  for(unsigned i=0;i<7;i++){
   const unsigned delta=i==2?1U:i==(write?4U:0U) || i==(write?6U:3U)?1U:i==(write?5U:1U)?c[8]/64:0U;
   memory[0xffb20000+offsets[i]]+=delta;
  }
  memory[0xffb20040]=0;pendingActive=false;
  if(corruptAfterFirst && transfers.size()==1)memory[0xffb20000]^=64;
 }
 uint32_t Load(uint32_t at)override{
  assert(!(at&3));if(CodeAddress(at))return l1[Index(at)];
  if(at==0xffb20040 && pendingActive){
   delayReads++;if(!neverComplete && pendingPolls && !--pendingPolls)Complete();
  }
  assert(memory.count(at));return memory.at(at);
 }
 void Store(uint32_t at,uint32_t value)override{
  assert(!(at&3));
  if(executeTransfers && at==0xffb20040){
   assert(value==1 && !pendingActive);uint32_t c[10];for(unsigned i=0;i<10;i++)c[i]=memory.at(0xffb20000+i*4);
   const bool write=c[2]==129;assert((write && c[0]==0x78000 && c[7]==0x212)
    || (!write && c[3]==0x78000 && c[5]==129 && !c[7]));
   assert(c[8]==16384 || c[8]==4096);transfers.push_back({write?c[3]:c[0],c[8],unsigned(write)});
   for(unsigned i=0;i<10;i++)pending[i]=c[i];pendingActive=true;pendingPolls=4;memory[at]=1;return;
  }
  assert(Range(at,0x6e000,8192) || (executeTransfers && (Range(at,Owner,1100) || Range(at,0xffb20000,40))));memory[at]=value;
 }
 bool Call(unsigned layer,unsigned workspace,unsigned phase,unsigned position,unsigned completed,unsigned storedLayer){
  const char* trace="_ZN11GlobalChainL10TraceLayerEj";const char* owner="_ZN11GlobalChainL5OwnerEv";
  const char* transfer="_ZN11GlobalChainL8TransferEjjb";const char* scope="_ZN11GlobalChainL5ScopeEb";
  assert(symbols.count(trace) && symbols.count(owner) && symbols.count(transfer) && symbols.count(scope));
  memory[Owner+1024]=phase;memory[Owner+832]=position;memory[Owner+780]=completed;
  memory[Owner+776]=storedLayer;memory[Owner+772]=workspace;
  if(executeTransfers){
   memory[Owner+768]=0;for(unsigned i=0;i<8;i++)memory[Owner+800+i*4]=0;
   for(unsigned offset:{0x204U,0x208U,0x20cU,0x210U,0x214U,0x220U,0x228U})memory[0xffb20000+offset]=0;
   for(unsigned i=0;i<20480;i+=4)memory[workspace+0x427000+i]=0x51a00000U+i;
  }
  pc=symbols.at(trace);regs[1]=Return;regs[2]=0x70000;regs[10]=layer;
  while(pc!=Return && pc!=Fault){
   if(pc==symbols.at(owner)){regs[10]=Owner;pc=regs[1];}
   else if(pc==symbols.at(scope)){pc=regs[1];}
   else if(!executeTransfers && pc==symbols.at(transfer)){transfers.push_back({regs[10],regs[11],regs[12]});pc=regs[1];}
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
 for(unsigned layer:{0U,31U,63U}){
  TraceMachine m(argv[1]);m.executeTransfers=true;const unsigned base=0x04000000;
  assert(m.Call(layer,base,1,0,layer+1,layer) && m.transfers.size()==4 && m.delayReads==16 && !m.pendingActive);
  for(unsigned i=0;i<20480;i+=4)assert(m.memory.at(base+0x3500000+layer*20480+i)==0x51a00000U+i);
 }
 {
  TraceMachine m(argv[1]);m.executeTransfers=true;m.corruptAfterFirst=true;
  assert(!m.Call(0,0x04000000,1,0,1,0) && m.transfers.size()==1);
 }
 {
  TraceMachine m(argv[1]);m.executeTransfers=true;m.neverComplete=true;
  assert(!m.Call(0,0x04000000,1,0,1,0) && m.transfers.size()==1 && m.pendingActive);
  assert(!m.memory.count(0x04000000+0x3500000));
 }
 puts("PASS emitted trace helper: all64 boundaries, address extremes, four scoped transfer calls, four state refusals; intercepted Owner/Transfer, NO ROOT lifecycle/device authority");
 puts("PASS actual emitted Transfer: delayed synthetic NoC at layers 0/31/63, corrupted command and timeout refusal; Scope/Owner intercepted, NO DEVICE");
}
