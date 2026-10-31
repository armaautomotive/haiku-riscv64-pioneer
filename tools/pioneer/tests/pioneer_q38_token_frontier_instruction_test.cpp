/* SPDX-License-Identifier: MIT */
// Isolated emitted leaf only. Synthetic ABI arguments are not model events.
#define PIONEER_TENSIX_MACHINE_ONLY
#include "pioneer_tensix_image_test.cpp"
#include <memory>
#include <string>
struct FrontierMachine:Machine {
 const std::vector<uint8_t>& text;const std::vector<uint8_t>& terminal;
 unsigned count,target=0x7570c,writes=0,stackStores=0,stackReads=0;
 uint32_t observed=0,savedS0=0,initialS0=0;
 FrontierMachine(const std::vector<uint8_t>& code,const std::vector<uint8_t>& stop,unsigned entry,unsigned value):
  text(code),terminal(stop),count(value){
  for(unsigned reg=1;reg<32;reg++)regs[reg]=0x13570000U+reg*0x147U;
  regs[1]=0x60004;regs[2]=0x70000;regs[10]=count;pc=entry;
  initialS0=regs[8];
 }
 bool CodeAddress(unsigned address)const override{
  return (address>=0x40000 && address-0x40000<text.size()) || (address>=0x60000 && address<0x60008);
 }
 unsigned Index(unsigned address)override{
  assert(CodeAddress(address) && !(address&3));
  l1[8180]=address<0x60000?Word(text,address-0x40000):Word(terminal,address-0x60000);return 8180;
 }
 uint32_t Load(uint32_t address)override{
  if(address==0x6fffc){assert(stackStores==1 && !stackReads++);return savedS0;}
  assert(CodeAddress(address));return l1[Index(address)]; // NO argument,MMIO,CSR or device read
 }
 void Store(uint32_t address,uint32_t value)override{
  // Actual toolchain retains a16-byte ABI frame,even for this small leaf.
  // Permit ONLY its exact s0 save/reload slot,not arbitrary stack writes.
  if(address==0x6fffc){assert(!stackStores++ && value==initialS0);savedS0=value;return;}
  assert(address==target && !writes && value==count);writes++;observed=value;
 }
 unsigned StepLimit()const override{return 4096;}
 void Run(){
  uint32_t before[32];std::copy(regs,regs+32,before);
  for(unsigned n=0;pc!=0x60004;n++){assert(n<32);Step();}
  assert(writes==1 && observed==count && stackStores==1 && stackReads==1);
  for(unsigned reg:{2U,3U,4U,8U,9U,18U,19U,20U,21U,22U,23U,24U,25U,26U,27U})assert(regs[reg]==before[reg]);
  for(unsigned n=0;n<2048;n++)Step();
  assert(pc==0x60004 && writes==1 && stackStores==1 && stackReads==1); // returned helper stays inert
 }
};
int main(int argc,char** argv){
 assert(argc==3);const std::string directory=argv[1];
 const auto text=Read((directory+"/text.bin").c_str()),terminal=Read((directory+"/terminal.bin").c_str());
 const unsigned entry=unsigned(std::stoul(argv[2],nullptr,16));assert(entry>=0x40000 && entry<0x40000+text.size());
 unsigned cases=0;
 for(unsigned value=0;value<=8192;value++){
  auto machine=std::make_unique<FrontierMachine>(text,terminal,entry,value);machine->Run();cases++;
 }
 auto raw=std::make_unique<FrontierMachine>(text,terminal,entry,UINT32_MAX);raw->Run();cases++;
 printf("PASS isolated actual frontier leaf/%u synthetic ABI values/one aligned32-bit telemetry write ONLY7570c+fence/exact16B frame and ONE s0 save+reload ONLY6fffc/NO MMIO,CSR,NoC,FP32 or other reads+writes/callee-saved+SP preserved/inert2048;raw DATA only,NOT actual token publication,owner/native/model/physical/timing/TPS qualification\n",cases);
}
