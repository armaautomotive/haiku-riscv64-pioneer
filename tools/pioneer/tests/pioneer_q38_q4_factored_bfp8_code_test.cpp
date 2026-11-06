/* SPDX-License-Identifier: MIT */
#define PIONEER_NATIVE_F32_Q8_MACHINE_ONLY
#include "pioneer_runtime_q8_native_f32_image_test.cpp"
#define PIONEER_Q38_Q4_FACTORED_BFP8_OFFBOARD 1
#include "../PioneerQ38Q4FactoredBfp8.h"
#include <sstream>
struct FactoredCodeMachine:Machine {
 std::vector<uint8_t> code;std::vector<unsigned> issued;
 std::array<unsigned,2048> stack{};unsigned ports=0,directs=0,minSP=0x70000;bool direct;
 FactoredCodeMachine(std::vector<uint8_t> bytes,bool inlineMode):code(std::move(bytes)),direct(inlineMode){
  assert(!code.empty() && code.size()<=4096 && !(code.size()&3));
  for(unsigned i=0;i<code.size()/4;i++)l1[i]=Word(code,i*4);
 }
 bool CodeAddress(unsigned a)const override{return a>=0x70000 && a-0x70000<code.size() && !(a&3);}
 bool InlineTensix()const override{return direct;}
 void Instruction(unsigned w)override{assert(direct);directs++;issued.push_back(w);}
 unsigned Index(unsigned a)override{assert(CodeAddress(a));return (a-0x70000)/4;}
 unsigned Load(unsigned a)override{
  if(a>=0x6e000 && a<0x70000){assert(!(a&3));return stack[(a-0x6e000)/4];}
  assert(CodeAddress(a));return Word(code,a-0x70000);
 }
 void Store(unsigned a,unsigned v)override{
  if(a>=0x6e000 && a<0x70000){assert(!(a&3));if(a<minSP)minSP=a;stack[(a-0x6e000)/4]=v;return;}
  assert(a==0xffe40000);ports++;issued.push_back(v);
 }
};
struct CapturedWords{std::vector<unsigned> words;void Op(unsigned w){words.push_back(w);}};
int main(int argc,char** argv){
 assert(argc==4 && (!strcmp(argv[3],"mmio") || !strcmp(argv[3],"inline")));
 const bool direct=!strcmp(argv[3],"inline");const auto code=Read(argv[1]);
 std::ifstream symbols(argv[2]);assert(symbols.good());unsigned entries[4]={};std::string line;
 const char* names[]={"pioneer_q4_factored_init","pioneer_q4_factored_low","pioneer_q4_factored_high","pioneer_q4_factored_affine"};
 while(std::getline(symbols,line)){
  std::istringstream in(line);std::string address,type,name;in>>address>>type>>name;
  for(unsigned i=0;i<4;i++)if(name==names[i]){assert(type=="T" && !entries[i]);entries[i]=unsigned(std::stoul(address,nullptr,16));}
 }
 CapturedWords expected[4];using F=PioneerQ38Q4FactoredBfp8;
 F::InitCodes(expected[0]);F::Codes<false>(expected[1]);F::Codes<true>(expected[2]);F::GroupAffine(expected[3]);
 unsigned steps=0,words=0,maxStack=0;
 for(unsigned i=0;i<4;i++){
  assert(entries[i]);FactoredCodeMachine m(code,direct);m.pc=entries[i];m.regs[1]=0xdead0000;m.regs[2]=0x70000;
  for(unsigned r:{8U,9U,18U,19U,20U,21U,22U,23U,24U,25U,26U,27U})m.regs[r]=0x5a000000|r;
  while(m.pc!=0xdead0000){assert(m.steps<256);m.Step();}
  assert(m.regs[2]==0x70000 && m.issued==expected[i].words);
  for(unsigned r:{8U,9U,18U,19U,20U,21U,22U,23U,24U,25U,26U,27U})assert(m.regs[r]==(0x5a000000|r));
  assert(m.ports==(direct?0U:m.issued.size()) && m.directs==(direct?m.issued.size():0U));
  if(0x70000-m.minSP>maxStack)maxStack=0x70000-m.minSP;
  steps+=m.steps;words+=unsigned(m.issued.size());
 }
 assert(words==12);
 printf("PASS_COMPILED_FACTORED_Q4 functions4 words%u RV32steps%u touchedStack%u mode%s actualROM_exactTensixWords_ABI_and_boundedStores;NO_PACK_MATRIX_FIRMWARE_NATIVE_QUALITY_OR_TPS\n",words,steps,maxStack,argv[3]);
}
