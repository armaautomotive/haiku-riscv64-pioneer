/* SPDX-License-Identifier: MIT */
#ifdef _KERNEL_MODE
#error Offline instruction profiling is not a native execution path
#endif
#ifndef PIONEER_Q38_TEST_INSTRUCTION_PROFILE_H
#define PIONEER_Q38_TEST_INSTRUCTION_PROFILE_H
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
// PC -> actual ELF symbol mapping is diagnostic metadata ONLY. It cannot
// select/change ROM, PC, registers, clocks, products or an execution grant.
class PioneerQ38InstructionProfile {
 enum {TextWords=32768,ConvWords=8192,Slots=TextWords+ConvWords+3};
 std::array<uint16_t,Slots> owner={};
 std::vector<std::string> names={"unmapped entry/terminal/symbol"};
 std::vector<uint64_t> counts={0};uint64_t total=0;
 static unsigned Slot(unsigned pc){
  assert(!(pc&3));
  if(pc>=0x40000 && pc<0x60000)return (pc-0x40000)/4;
  if(pc>=0x64000 && pc<0x6c000)return TextWords+(pc-0x64000)/4;
  if(pc==0x6000)return TextWords+ConvWords;
  if(pc==0x60000 || pc==0x60004)return TextWords+ConvWords+1+(pc-0x60000)/4;
  assert(false && "profile PC outside unchanged ROOT ROM map");return Slots;
 }
public:
 explicit PioneerQ38InstructionProfile(const char* path=nullptr){
  if(!path)return;
  std::ifstream input(path);assert(input);std::string line;unsigned ranges=0;
  while(std::getline(input,line)){
   std::istringstream row(line);unsigned begin=0,bytes=0;std::string name;
   assert(row>>std::hex>>begin>>bytes);std::getline(row>>std::ws,name);
   assert(bytes && !(bytes&3) && begin<=UINT32_MAX-bytes && !name.empty() && names.size()<65535);
   unsigned first=Slot(begin),last=Slot(begin+bytes-4);assert(last>=first && last-first+1==bytes/4);
   uint16_t id=uint16_t(names.size());names.push_back(name);counts.push_back(0);
   for(unsigned at=first;at<=last;at++){assert(!owner[at]);owner[at]=id;}
   ranges++;
  }
  assert(ranges);
 }
 void Record(unsigned pc){counts[owner[Slot(pc)]]++;total++;}
 uint64_t Total()const{return total;}
 void Dump(uint64_t expected)const{
  assert(total==expected);uint64_t sum=0;std::vector<unsigned> rank;
  for(unsigned i=0;i<counts.size();i++){sum+=counts[i];if(counts[i])rank.push_back(i);}assert(sum==total);
  std::sort(rank.begin(),rank.end(),[&](unsigned a,unsigned b){return counts[a]!=counts[b]?counts[a]>counts[b]:a<b;});
  printf("ROOT_INSTRUCTION_PROFILE total%llu/functions%zu/EXCLUSIVE actual PCs incl SIMULATED polling/NOT physical cycles or token speed\n",(unsigned long long)total,rank.size());
  for(unsigned n=0;n<std::min(unsigned(rank.size()),20U);n++){
   unsigned i=rank[n];printf("ROOT_INSTRUCTION_FUNCTION %llu %s\n",(unsigned long long)counts[i],names[i].c_str());
  }
 }
};
#endif
