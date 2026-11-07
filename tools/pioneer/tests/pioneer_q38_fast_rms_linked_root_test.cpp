/* SPDX-License-Identifier: MIT */
// Execute the real linked QGW5 ROOT RMS entry in a scalar RV32 simulator.
#define PIONEER_Q38_FAST_RMS_DIAGNOSTIC 1
#define main prior_prompt_test_main
#include "pioneer_q38_fast_rms_prompt_image_test.cpp"
#undef main
#include "../PioneerQ38FastF32HiddenRms.h"
#include <sstream>

struct LinkedRmsMachine:ScalarMachine {
 std::vector<std::pair<uint32_t,std::vector<uint8_t>>> rom;uint32_t entry=0;
 explicit LinkedRmsMachine(const char* directory):ScalarMachine(5120,0){
  for(auto p:{std::make_pair("bf16.bin",0x20000U),{"text.bin",0x40000U},
              {"terminal.bin",0x60000U},{"conv.bin",0x64000U}})
   rom.push_back({p.second,Read((std::string(directory)+"/"+p.first).c_str())});
  std::ifstream symbols(std::string(directory)+"/program.symbols");std::string line;
  while(std::getline(symbols,line)){
   std::istringstream row(line);std::string address,kind,name;row>>address>>kind>>name;
   if(name=="q38_global_chain_learned_rms"){assert(!entry);entry=uint32_t(std::stoul(address,nullptr,16));}
  }
  assert(entry);
 }
 const std::vector<uint8_t>* Rom(uint32_t address,uint32_t& base)const{
  for(const auto& part:rom)if(Range(address,part.first,unsigned(part.second.size()))){base=part.first;return &part.second;}
  return nullptr;
 }
 bool CodeAddress(uint32_t address)const override{uint32_t base=0;return Rom(address,base)!=nullptr;}
 unsigned Index(uint32_t address)override{
  uint32_t base=0;const auto* part=Rom(address,base);assert(part && !(address&3));l1[8190]=Word(*part,address-base);return 8190;
 }
 uint32_t Load(uint32_t address)override{
  uint32_t base=0;if(const auto* part=Rom(address,base))return Word(*part,address-base);
  auto found=memory.find(address);assert(found!=memory.end());return found->second;
 }
 void Store(uint32_t address,uint32_t value)override{
  assert(!(address&3) && !CodeAddress(address));
  assert(Range(address,0x6e000,8192) || Range(address,0x90000,20480));
  if(Range(address,0x6e000,8192))minStack=std::min(minStack,unsigned(address));
  if(Range(address,0x90000,20480))outputWrites++;
  memory[address]=value;
 }
 bool Run(const std::array<uint32_t,5120>& input){
  for(unsigned i=0;i<5120;i++){
   memory[0x80000+i*4]=input[i];memory[0x88000+i*4]=PioneerAttentionBits(1.f);memory[0x90000+i*4]=0xdeadbeef;
  }
  regs[1]=0x60004;regs[2]=0x70000;regs[10]=0x80000;regs[11]=0x88000;regs[12]=0x90000;pc=entry;
  while(pc!=0x60004 && pc!=0x60000)StepScalar();
  assert(minStack>=0x6e000);return pc==0x60004 && regs[10]==1;
 }
};
int main(int argc,char** argv){
 (void)&RmsPlanTests;(void)&PioneerQ38LearnedRms;(void)&PioneerQ38F32HiddenResidualRms;
 (void)&PioneerQ38FastF32HiddenResidualRms;
 assert(argc==5);std::vector<uint8_t> data;
 if(!ReadPinned(argv[3],20880,"75dfd208b58e224e0575e211a735f03b34b316a9e548bf63d1d48e10a261e594",data)
  || Word(data.data())!=0x31464951 || Word(data.data()+4)!=1 || Word(data.data()+8)!=248319 || Word(data.data()+12)!=5120)return 2;
 std::array<std::array<uint32_t,5120>,3> inputs={};
 for(unsigned i=0;i<5120;i++)inputs[0][i]=Word(data.data()+16+i*4);
 if(!ReadPinned(argv[4],290848,"85d4ada0aa8cfa847bb06e7a4296dd9992829947e8c5bf03b7ed9dadfd39f523",data)
  || Word(data.data())!=0x31464651 || Word(data.data()+4)!=1 || Word(data.data()+8)!=248319
  || Word(data.data()+12)!=5120 || Word(data.data()+16)!=17408 || Word(data.data()+20)!=72704
  || Word(data.data()+24) || Word(data.data()+28))return 2;
 for(unsigned i=0;i<5120;i++){
  inputs[1][i]=Word(data.data()+32+5120*4+i*4);inputs[2][i]=Word(data.data()+270368+i*4);
 }
 unsigned long long preciseSteps=0,fastSteps=0;unsigned changed=0;double worst=0;
 std::array<float,5120> in={},weight={},expected={};weight.fill(1);
 for(const auto& input:inputs){
  auto precise=std::make_unique<LinkedRmsMachine>(argv[1]),fast=std::make_unique<LinkedRmsMachine>(argv[2]);
  assert(precise->Run(input) && fast->Run(input) && precise->outputWrites==5120 && fast->outputWrites==5120);
  for(unsigned i=0;i<5120;i++)in[i]=PioneerAttentionFloat(input[i]);
  assert(PioneerQ38FastF32HiddenRms(in.data(),weight.data(),expected.data()));
  double err=0,norm=0;
  for(unsigned i=0;i<5120;i++){
   const uint32_t a=precise->memory.at(0x90000+i*4),b=fast->memory.at(0x90000+i*4);
   assert(b==PioneerAttentionBits(expected[i]));const double av=PioneerAttentionFloat(a),bv=PioneerAttentionFloat(b);
   err+=(av-bv)*(av-bv);norm+=av*av;changed+=a!=b;
  }
  worst=std::max(worst,std::sqrt(err/(norm+1e-300)));preciseSteps+=precise->steps;fastSteps+=fast->steps;
 }
 assert(worst<0.00002 && fastSteps<preciseSteps);
 auto invalid=inputs[0];invalid[5119]=0x7f800000;
 auto rejected=std::make_unique<LinkedRmsMachine>(argv[2]);assert(!rejected->Run(invalid) && !rejected->outputWrites);
 printf("PASS real linked QGW5 ROOT RMS entry on 3 pinned Qwen-derived vectors/unit weights: precise_instructions%llu fast_instructions%llu changed_words%u worst_relative_l2%.9g; "
  "SIMULATED ONLY, NO full layer/model, learned RMS weights, P100A time or tokens/sec\n",preciseSteps,fastSteps,changed,worst);
}
