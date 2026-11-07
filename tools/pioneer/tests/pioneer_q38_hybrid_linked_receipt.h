/* SPDX-License-Identifier: MIT */
// Execute the actual linked coordinator receipt function against a simulated
// worker's produced proof. This is NOT a full ROOT lifecycle or device test.
#ifndef PIONEER_Q38_HYBRID_LINKED_RECEIPT_TEST_H
#define PIONEER_Q38_HYBRID_LINKED_RECEIPT_TEST_H
#include <map>
#include <memory>
#include <string>
struct HybridLinkedReceipt:Machine {
 std::vector<std::pair<unsigned,std::vector<uint8_t>>> rom;
 std::map<unsigned,unsigned> memory;unsigned entry=0;
 explicit HybridLinkedReceipt(const char* directory){
  for(auto part:{std::make_pair("bf16",0x20000U),{"text",0x40000U},{"terminal",0x60000U},{"conv",0x64000U}})
   rom.push_back({part.second,Read((std::string(directory)+"/"+part.first+".bin").c_str())});
  std::ifstream symbols(std::string(directory)+"/program.symbols");std::string address,kind,name;
  while(symbols>>address>>kind>>name){
   if(name=="_ZN29PioneerQ38HybridMatrixReceipt6MatrixEPKjS1_S1_S1_S1_"){
    assert(!entry);entry=unsigned(std::stoul(address,nullptr,16));
   }
  }
  assert(entry);
 }
 const std::vector<uint8_t>* Rom(unsigned address,unsigned& base)const{
  for(const auto& part:rom)if(address>=part.first && address-part.first<part.second.size()){
   base=part.first;return &part.second;
  }
  return nullptr;
 }
 bool CodeAddress(unsigned address)const override{unsigned base=0;return Rom(address,base)!=nullptr;}
 unsigned StepLimit()const override{return 1000000;}
 unsigned Index(unsigned address)override{
  unsigned base=0;const auto* part=Rom(address,base);assert(part && !(address&3));
  l1[8190]=Word(*part,address-base);return 8190;
 }
 unsigned Load(unsigned address)override{
  unsigned base=0;if(const auto* part=Rom(address,base))return Word(*part,address-base);
  const auto it=memory.find(address);assert(it!=memory.end());return it->second;
 }
 void Store(unsigned address,unsigned value)override{
  assert(!(address&3) && address>=0x6e000 && address<0x70000);memory[address]=value;
 }
 bool Run(const uint32_t* proof,const uint32_t* job,const uint32_t* boot,const uint32_t* before,const uint32_t* after){
  memory.clear();const uint32_t* data[]={proof,job,boot,before,after};
  for(unsigned group=0;group<5;group++)for(unsigned i=0;i<(group<3?64U:8U);i++)memory[0x80000+group*0x1000+i*4]=data[group][i];
  std::fill(std::begin(regs),std::end(regs),0);regs[1]=0x60004;regs[2]=0x70000;
  for(unsigned i=0;i<5;i++)regs[10+i]=0x80000+i*0x1000;
  pc=entry;steps=0;
  while(pc!=0x60004 && pc!=0x60000){Step();assert(regs[2]>=0x6e000 && regs[2]<=0x70000);}
  return pc==0x60004 && regs[10]==1;
 }
};
#endif
