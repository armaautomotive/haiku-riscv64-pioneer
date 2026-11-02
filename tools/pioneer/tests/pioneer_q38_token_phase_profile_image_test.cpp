/* SPDX-License-Identifier: MIT */
// Execute helpers from the ENTIRE linked candidate, not a host substitute.
// This is direct helper execution ONLY, not full ROOT/model qualification.
#define PIONEER_TENSIX_MACHINE_ONLY
#include "pioneer_tensix_image_test.cpp"
#include "../PioneerQ38TokenPhaseProfileData.h"
#include <array>
#include <map>
#include <memory>
#include <string>
namespace Q=PioneerQ38TokenPhaseProfileData;
struct PhaseMachine:Machine {
 std::vector<uint8_t> text,conv;
 std::map<std::string,unsigned> symbols;
 std::map<uint32_t,uint32_t> memory;
 std::vector<std::pair<unsigned,uint32_t>> clocks;
 uint64_t epoch=UINT64_C(1)<<40;
 bool rollover=false,unstable=false,backward=false,large=false;
 unsigned writes=0,minStack=0x70000;
 static constexpr unsigned Return=0x17ff80;
 PhaseMachine(const char* path){
  text=Read((std::string(path)+"/text.bin").c_str());conv=Read((std::string(path)+"/conv.bin").c_str());
  assert(text.size()<=0x20000 && conv.size()<=0x8000);
  std::ifstream file(std::string(path)+"/program.symbols");assert(file.good());std::string line;
  while(std::getline(file,line)){unsigned at=0;char type=0,name[256]={};if(sscanf(line.c_str(),"%x %c %255s",&at,&type,name)==3)symbols[name]=at;}
  for(unsigned i=0;i<64;i++)memory[0x75700+i*4]=i<4?0xa55a0000+i:0;
 }
 static bool Range(unsigned at,unsigned first,unsigned size){return at>=first && at-first<size;}
 bool CodeAddress(uint32_t at)const override{return Range(at,0x40000,unsigned(text.size())) || Range(at,0x64000,unsigned(conv.size()));}
 unsigned Index(uint32_t at)override{
  assert(!(at&3) && CodeAddress(at));l1[8189]=Range(at,0x40000,unsigned(text.size()))?Word(text,at-0x40000):Word(conv,at-0x64000);return 8189;
 }
 uint32_t Load(uint32_t at)override{
  assert(!(at&3) && (Range(at,Q::Address,Q::Words*4) || Range(at,0x6e000,8192)));
  assert(memory.count(at));return memory.at(at);
 }
 void Store(uint32_t at,uint32_t value)override{
  assert(!(at&3) && (Range(at,Q::Address,Q::Words*4) || Range(at,0x6e000,8192)));
  if(Range(at,0x6e000,8192))minStack=std::min(minStack,unsigned(at));else writes++;
  memory[at]=value;
 }
 uint32_t ReadCycleCsr(unsigned csr)override{
  assert(csr==0xb00 || csr==0xb80);
  uint64_t now=epoch+uint64_t(steps)*19;
  if(large)now+=uint64_t(clocks.size()/3)*(UINT64_C(1)<<34);
  if(backward && clocks.size()>=3)now-=UINT64_C(1)<<39;
  uint32_t value=csr==0xb00?uint32_t(now):uint32_t(now>>32);
  if(rollover && clocks.empty())value--;
  if(unstable && csr==0xb80 && clocks.size()%3==2)value++;
  clocks.emplace_back(csr,value);return value;
 }
 unsigned StepLimit()const override{return 10000000;}
 void Call(const char* name,unsigned argument=0){
  assert(symbols.count(name));pc=symbols.at(name);regs[1]=Return;regs[2]=0x70000;regs[10]=argument;
  for(unsigned r=8;r<10;r++)regs[r]=0xcaf00000+r;
  for(unsigned r=18;r<28;r++)regs[r]=0xcaf00000+r;
  while(pc!=Return)Step();
  assert(regs[2]==0x70000 && minStack>=0x6e000);
  for(unsigned r=8;r<10;r++)assert(regs[r]==0xcaf00000+r);
  for(unsigned r=18;r<28;r++)assert(regs[r]==0xcaf00000+r);
  for(unsigned i=0;i<4;i++)assert(memory.at(0x75700+i*4)==0xa55a0000+i);
  assert(!matrixInstructions && !configWrites && !drains && !inputWrites && !outputReads);
 }
 std::array<uint32_t,Q::Words> Record()const{
  std::array<uint32_t,Q::Words> out;for(unsigned i=0;i<Q::Words;i++)out[i]=memory.at(Q::Address+i*4);return out;
 }
 void Full(unsigned tokens){
  Call("q38_token_phase_begin");
  for(unsigned token=0;token<tokens;token++){
   for(unsigned layer=0;layer<64;layer++){Call("q38_token_phase_mark",Q::Metadata);Call("q38_token_phase_mark",layer%4==3?Q::Dense:Q::Recurrent);}
   for(unsigned phase:{Q::HeadNormalize,Q::HeadMatrix,Q::Feedback,Q::StateCommit,Q::Publication})Call("q38_token_phase_mark",phase);
  }
  Call("q38_token_phase_mark",Q::FinalStop);Call("q38_token_phase_complete",tokens);
 }
 void Verify(unsigned tokens){
  const auto p=Record();assert(Q::Closed(p.data()) && p[Q::Committed]==tokens);
  std::vector<uint64_t> samples;assert(clocks.size()%3==0);
  for(unsigned i=0;i<clocks.size();i+=3){
   assert(clocks[i].first==0xb80 && clocks[i+1].first==0xb00 && clocks[i+2].first==0xb80);
   if(clocks[i].second==clocks[i+2].second)samples.push_back((uint64_t(clocks[i].second)<<32)|clocks[i+1].second);
  }
  assert(samples.size()==133*tokens+3);
  std::array<uint64_t,Q::Phases> expected={};unsigned sample=1,current=Q::Bootstrap;
  auto mark=[&](unsigned next){assert(sample<samples.size());expected[current]+=samples[sample]-samples[sample-1];sample++;current=next;};
  for(unsigned token=0;token<tokens;token++){
   for(unsigned layer=0;layer<64;layer++){mark(Q::Metadata);mark(layer%4==3?Q::Dense:Q::Recurrent);}
   for(unsigned phase:{Q::HeadNormalize,Q::HeadMatrix,Q::Feedback,Q::StateCommit,Q::Publication})mark(phase);
  }
  mark(Q::FinalStop);mark(Q::Phases);assert(sample==samples.size());
  for(unsigned phase=0;phase<Q::Phases;phase++)assert(Q::Get(p.data(),Q::Buckets+phase*3+1)==expected[phase]);
  assert(Q::Get(p.data(),Q::BeginCycle)==samples.front() && Q::Get(p.data(),Q::EndCycle)==samples.back());
 }
};
int main(int argc,char** argv){
 assert(argc==2);setvbuf(stdout,nullptr,_IOLBF,0);unsigned cases=0;
 for(unsigned tokens:{1U,2U,34U})for(bool rollover:{false,true})for(bool large:{false,true}){
  auto m=std::make_unique<PhaseMachine>(argv[1]);m->rollover=rollover;m->large=large;m->Full(tokens);m->Verify(tokens);
  const auto before=m->Record();unsigned reads=unsigned(m->clocks.size()),writes=m->writes;
  m->Call("q38_token_phase_mark",Q::Metadata);m->Call("q38_token_phase_complete",tokens);
  assert(m->Record()==before && reads==m->clocks.size() && writes==m->writes);cases++;
 }
 for(unsigned fault=0;fault<9;fault++){
  auto m=std::make_unique<PhaseMachine>(argv[1]);m->unstable=fault==0;m->Call("q38_token_phase_begin");
  if(fault==1)m->backward=true;
  if(fault==2)m->unstable=true;
  if(fault==3)m->memory[Q::Address+Q::Current*4]=Q::Phases;
  if(fault==4)m->memory[Q::Address+Q::Buckets*4]=UINT32_MAX;
  if(fault==5)m->memory[Q::Address+Q::Transitions*4]=UINT32_MAX;
  if(fault==6){m->memory[Q::Address+(Q::Buckets+1)*4]=UINT32_MAX;m->memory[Q::Address+(Q::Buckets+2)*4]=UINT32_MAX;}
  m->Call("q38_token_phase_mark",fault==7?Q::Phases+1:Q::Metadata);
  if(fault==8)m->Call("q38_token_phase_complete",1);
  const auto before=m->Record();assert(before[Q::State]==Q::Invalid && !Q::Closed(before.data()));
  unsigned reads=unsigned(m->clocks.size()),writes=m->writes;
  m->Call("q38_token_phase_mark",Q::FinalStop);m->Call("q38_token_phase_complete",34);
  assert(before==m->Record() && reads==m->clocks.size() && writes==m->writes);cases++;
 }
 printf("PASS ACTUAL linked phase-helper RV32:%uhistories/1+2+34 positions/rawCSR stream exact per10buckets/rollover/64-bit sums/9faults sticky metric invalid/no later CSR or metric writes/ABI and first4progress untouched/NO MMIO or coprocessor IO;DIRECT_HELPERS_ONLY,fabricated clocks,NOT full ROOT,native,physical frequency or speed\n",cases);
}
