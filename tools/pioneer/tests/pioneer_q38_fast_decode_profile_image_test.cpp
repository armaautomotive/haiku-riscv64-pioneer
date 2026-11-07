/* SPDX-License-Identifier: MIT */
// Actual linked RV32 helper instructions; fabricated clocks, NOT model/IO.
#define PIONEER_TENSIX_MACHINE_ONLY
#include "pioneer_tensix_image_test.cpp"
#include "../PioneerQ38FastDecodeProfileData.h"
#include <array>
#include <map>
#include <memory>
#include <string>
namespace Q=PioneerQ38FastDecodeProfileData;
struct FastDecodeMachine:Machine {
 std::vector<uint8_t> text,conv,helper;
 std::map<std::string,unsigned> symbols;
 std::map<uint32_t,uint32_t> memory;
 std::vector<std::pair<unsigned,uint32_t>> clocks;
 std::vector<unsigned> marks;
 bool rollover=false,unstable=false,backward=false,large=false;
 unsigned writes=0,minStack=0x70000;
 static constexpr unsigned Return=0x17ff80;
 explicit FastDecodeMachine(const char* path){
  const std::string root(path);
  text=Read((root+"/text.bin").c_str());conv=Read((root+"/conv.bin").c_str());helper=Read((root+"/fast_decode.bin").c_str());
  assert(text.size()<=0x20000 && conv.size()<=0x8000 && !helper.empty() && helper.size()<=0x2000);
  std::ifstream file(root+"/program.symbols");assert(file.good());std::string line;
  while(std::getline(file,line)){unsigned at=0;char type=0,name[256]={};if(sscanf(line.c_str(),"%x %c %255s",&at,&type,name)==3)symbols[name]=at;}
  for(unsigned i=0;i<64;i++)memory[0x75700+i*4]=i<4?0xa55a0000+i:0;
 }
 static bool Range(unsigned at,unsigned first,unsigned size){return at>=first && at-first<size;}
 bool CodeAddress(uint32_t at)const override{return Range(at,0x40000,unsigned(text.size())) || Range(at,0x64000,unsigned(conv.size())) || Range(at,0x6c000,unsigned(helper.size()));}
 unsigned Index(uint32_t at)override{
  assert(!(at&3) && CodeAddress(at));
  l1[8189]=Range(at,0x40000,unsigned(text.size()))?Word(text,at-0x40000):
   Range(at,0x64000,unsigned(conv.size()))?Word(conv,at-0x64000):Word(helper,at-0x6c000);return 8189;
 }
 uint32_t Load(uint32_t at)override{
  assert(!(at&3) && (Range(at,Q::Address,Q::Words*4) || Range(at,0x6e000,8192)) && memory.count(at));return memory.at(at);
 }
 void Store(uint32_t at,uint32_t value)override{
  assert(!(at&3) && (Range(at,Q::Address,Q::Words*4) || Range(at,0x6e000,8192)));
  if(Range(at,0x6e000,8192))minStack=std::min(minStack,unsigned(at));else writes++;
  memory[at]=value;
 }
 uint32_t ReadCycleCsr(unsigned csr)override{
  assert(csr==0xb00 || csr==0xb80);
  uint64_t now=(UINT64_C(1)<<40)+uint64_t(steps)*19;
  if(large)now+=uint64_t(clocks.size()/3)*(UINT64_C(1)<<34);
  if(backward && clocks.size()>=3)now-=UINT64_C(1)<<39;
  uint32_t value=csr==0xb00?uint32_t(now):uint32_t(now>>32);
  if(rollover && clocks.empty())value--;
  if(unstable && csr==0xb80 && clocks.size()%3==2)value++;
  clocks.emplace_back(csr,value);return value;
 }
 unsigned StepLimit()const override{return 100000000;}
 void Call(const char* name,unsigned argument=0,unsigned second=0){
  assert(symbols.count(name));pc=symbols.at(name);regs[1]=Return;regs[2]=0x70000;regs[10]=argument;regs[11]=second;
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
 void Mark(unsigned phase){
  const bool active=memory.at(Q::Address+Q::State*4)==Q::Active;
  const auto before=clocks.size();Call("q38_token_phase_mark",phase);
  if(active)marks.push_back(phase);else assert(clocks.size()==before);
 }
 void Full(unsigned prompts,unsigned end){
  Call("q38_token_phase_begin");assert(clocks.empty());
  for(unsigned position=0;position<end;position++){
   Call("q38_fast_decode_profile_position",position,prompts);
   if(position<prompts)assert(clocks.empty());
   for(unsigned layer=0;layer<64;layer++){
    Mark(Q::Metadata);const unsigned outer=layer%4==3?Q::Dense:Q::Recurrent;Mark(outer);
    for(unsigned job=0;job<(layer%4==3?7U:8U);job++){
     Call("q38_fast_decode_profile_enter",Q::MatrixPrepare);
     if(position<prompts){assert(regs[10]==Q::Phases && clocks.empty());}
     else {assert(regs[10]==outer);marks.push_back(Q::MatrixPrepare);}
     for(unsigned phase:std::array<unsigned,4>{Q::MatrixDispatch,Q::MatrixGatherWait,Q::MatrixFinish,outer})Mark(phase);
    }
   }
   for(unsigned phase:{Q::HeadNormalize,Q::HeadMatrix,Q::Feedback,Q::StateCommit,Q::Publication})Mark(phase);
  }
  Mark(Q::FinalStop);Call("q38_token_phase_complete",end);marks.push_back(Q::Phases);
 }
 void Verify(unsigned prompts,unsigned end){
  const auto p=Record();assert(Q::Closed(p.data()) && p[Q::Committed]==end && p[Q::WindowStart]==prompts);
  std::vector<uint64_t> samples;assert(clocks.size()%3==0);
  for(unsigned i=0;i<clocks.size();i+=3){
   assert(clocks[i].first==0xb80 && clocks[i+1].first==0xb00 && clocks[i+2].first==0xb80);
   if(clocks[i].second==clocks[i+2].second)samples.push_back((uint64_t(clocks[i].second)<<32)|clocks[i+1].second);
  }
  assert(samples.size()==marks.size()+1 && marks.size()==2613*(end-prompts)+2);
  std::array<uint64_t,Q::Phases> expected={};unsigned current=Q::Bootstrap;
  for(unsigned i=0;i<marks.size();i++){expected[current]+=samples[i+1]-samples[i];current=marks[i];}
  for(unsigned phase=0;phase<Q::Phases;phase++)assert(Q::Get(p.data(),Q::Buckets+phase*3+1)==expected[phase]);
  assert(Q::Get(p.data(),Q::BeginCycle)==samples.front() && Q::Get(p.data(),Q::EndCycle)==samples.back());
 }
};
int main(int argc,char** argv){
 assert(argc==2);setvbuf(stdout,nullptr,_IOLBF,0);unsigned positives=0,refused=0;
 for(auto window:{std::array<unsigned,2>{31,62},{1,3},{9,10}}){
  auto m=std::make_unique<FastDecodeMachine>(argv[1]);m->Full(window[0],window[1]);m->Verify(window[0],window[1]);positives++;
 }
 for(bool rollover:{false,true})for(bool large:{false,true}){
  auto m=std::make_unique<FastDecodeMachine>(argv[1]);m->rollover=rollover;m->large=large;m->Full(1,2);m->Verify(1,2);positives++;
 }
 for(unsigned fault=0;fault<13;fault++){
  auto m=std::make_unique<FastDecodeMachine>(argv[1]);m->Call("q38_token_phase_begin");
  if(fault==0){m->Call("q38_token_phase_complete",1);}
  else {
   m->Call("q38_fast_decode_profile_position",0,1);m->unstable=fault==1;m->Call("q38_fast_decode_profile_position",1,1);
   if(fault==2)m->backward=true;
   if(fault==3)m->unstable=true;
   if(fault==4)m->memory[Q::Address+Q::Current*4]=Q::Phases;
   if(fault==5)m->memory[Q::Address+Q::Buckets*4]=UINT32_MAX;
   if(fault==6)m->memory[Q::Address+Q::Transitions*4]=UINT32_MAX;
   if(fault==7){m->memory[Q::Address+(Q::Buckets+1)*4]=UINT32_MAX;m->memory[Q::Address+(Q::Buckets+2)*4]=UINT32_MAX;}
   if(fault==8)m->Call("q38_fast_decode_profile_position",2,2);
   if(fault==9)m->Call("q38_fast_decode_profile_position",1,1);
   m->Call("q38_token_phase_mark",fault==10?Q::Phases+1:Q::Metadata);
   if(fault==11)m->Call("q38_token_phase_complete",2);
   if(fault==12){m->Call("q38_token_phase_mark",Q::FinalStop);m->Call("q38_token_phase_complete",1);}
  }
  const auto before=m->Record();assert(before[Q::State]==Q::Invalid && !Q::Closed(before.data()));
  const auto clocks=m->clocks.size();const auto writes=m->writes;
  m->Call("q38_token_phase_mark",Q::FinalStop);m->Call("q38_token_phase_complete",62);
  m->Call("q38_fast_decode_profile_position",2,1);m->Call("q38_fast_decode_profile_enter",Q::MatrixPrepare);
  assert(m->regs[10]==Q::Phases && m->Record()==before && m->clocks.size()==clocks && m->writes==writes);refused++;
 }
 printf("PASS_ACTUAL_FAST_DECODE_RV32 positive%u refused%u rawCSR_exact14buckets_31of62_no_prefillCSR_ABI_progress_bounds_sticky_invalid;HELPERS_ONLY_SYNTHETIC_CLOCKS_NOT_MODEL_NATIVE_OR_SPEED\n",positives,refused);
}
