/* SPDX-License-Identifier: MIT */
// Word-memory endpoint for standalone actual RV32 plan instructions only.
// It models no NoC, card, concurrency or native ownership.
#ifndef PIONEER_Q38_SMALL_PLAN_MACHINE_H
#define PIONEER_Q38_SMALL_PLAN_MACHINE_H
#include <map>
#include <fstream>
struct PioneerQ38SmallPlanMachine:Machine {
 std::vector<uint8_t> code;std::map<uint32_t,uint32_t> memory;unsigned plan=0,copy=0,writes=0,minSP=0x70000;
 explicit PioneerQ38SmallPlanMachine(const char* path){
  code=Read((std::string(path)+"/text.bin").c_str());assert(!code.empty() && !(code.size()&3) && code.size()<=32768);
  std::ifstream symbols(std::string(path)+"/program.symbols");std::string a,k,n;
  while(symbols>>a>>k>>n){if(n=="q38_small_plan")plan=unsigned(std::stoul(a,nullptr,16));if(n=="q38_small_copy")copy=unsigned(std::stoul(a,nullptr,16));}
  assert(plan>=0x40000 && plan<0x40000+code.size() && copy>=0x40000 && copy<0x40000+code.size());
  for(unsigned a=0x6e000;a<0x70000;a+=4)memory[a]=0xdeadbeef;
 }
 static bool In(unsigned a,unsigned start,unsigned bytes){return a>=start && a-start<bytes;}
 bool CodeAddress(uint32_t a)const override{return In(a,0x40000,unsigned(code.size()));}
 unsigned Index(uint32_t a)override{assert(CodeAddress(a) && !(a&3));l1[8190]=Word(code,a-0x40000);return 8190;}
 uint32_t Load(uint32_t a)override{
  if(CodeAddress(a))return Word(code,a-0x40000);
  assert(!(a&3));auto it=memory.find(a);if(it==memory.end())std::fprintf(stderr,"missing small plan word %x pc %x\n",a,pc);
  assert(it!=memory.end());return it->second;
 }
 void Store(uint32_t a,uint32_t value)override{
  assert(!(a&3) && (In(a,0x6e000,8192) || In(a,0x92000,4*268) || In(a,0x95000,2*200)));
  if(!In(a,0x6e000,8192))writes++;memory[a]=value;
 }
 unsigned StepLimit()const override{return 10000000;}
 unsigned Call(unsigned entry,const uint32_t* args,unsigned count){
  assert(count<=8);std::fill(std::begin(regs),std::end(regs),0);regs[1]=0x60004;regs[2]=0x70000;
  for(unsigned i=0;i<count;i++)regs[10+i]=args[i];pc=entry;steps=0;writes=0;
  while(pc!=0x60004){Step();assert(regs[2]>=0x6e000 && regs[2]<=0x70000);minSP=std::min(minSP,regs[2]);}
  return regs[10];
 }
};
#endif
