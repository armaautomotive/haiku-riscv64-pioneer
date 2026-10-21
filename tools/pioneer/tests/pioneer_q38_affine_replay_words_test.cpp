/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_WORK_AFFINE_REPLAY
#include "../PioneerQ38AffineReplay.h"
#include "../PioneerQ38NativeOperand.h"
#include <array>
#include <vector>
#include <cassert>
#include <cstdio>
struct E {std::vector<uint32_t> words;void Op(uint32_t word){words.push_back(word);}};
struct Replay {
 std::array<uint32_t,32> word={};std::array<bool,32> defined={};
 std::vector<uint32_t> expanded;
 bool Run(const std::vector<uint32_t>& program) {
  if(program.empty())return false;uint32_t op=program[0];unsigned index=(op>>14)&31,count=(op>>4)&63;
  if(op>>24!=4 || !count || index+count>32)return false;
  if(op&1) {
   if(program.size()!=count+1)return false;
   for(unsigned i=0;i<count;i++) {
    unsigned code=program[i+1]>>24;
    if(code!=0x71 && code!=0x7e && code!=0x79 && code!=0x7a && code!=0x90 && code!=0x85 && code!=0x86 && code!=0x8f)return false;
   }
   for(unsigned i=0;i<count;i++){word[index+i]=program[i+1];defined[index+i]=true;if(op&2)expanded.push_back(program[i+1]);}
  }else {
   if(program.size()!=1)return false;for(unsigned i=0;i<count;i++)if(!defined[index+i])return false;
   for(unsigned i=0;i<count;i++)expanded.push_back(word[index+i]);
  }
  return true;
 }
};
int main() {
 const std::vector<uint32_t> canonical={
#ifndef PIONEER_Q38_WORK_DOT_ZERO_ELISION
  0x71607fff,0x716affff,0x7e000671,0x79000674,0x7afe1773,0x7e000701
#endif
 };
 const std::vector<uint32_t> q8={0x90000000,0x86013962,0x8f000000,0x86060902,0x8f000000,0x850a5000,0x8f000000};
 const std::vector<uint32_t> q4={0x90000000,0x90000440,0x86013962,0x8f000000,0x86023972,0x8f000000,
  0x86060902,0x8f000000,0x86074942,0x8f000000,0x850a0400,0x8f000000,0x850a5000,0x8f000000};
 Replay memory;
 for(unsigned job=0;job<16;job++)for(bool zero:{false,true}) {
  // Model an intervening Q6 decoder clobbering0..16. A fresh first
  // record MUST replace every affine slot consumed by the NEXT group.
  for(unsigned i=0;i<17;i++){memory.word[i]=0xdead0000+i;memory.defined[i]=true;}
  auto low=memory.word;E record,replay;PioneerQ38AffineReplay(record,zero,true);PioneerQ38AffineReplay(replay,zero,false);
  std::vector<uint32_t> expected=canonical;const auto& tail=zero?q8:q4;expected.insert(expected.end(),tail.begin(),tail.end());
  assert(expected.size()==canonical.size()+(zero?7U:14U) && record.words.size()==expected.size()+1 && replay.words.size()==1);
  assert(record.words[0]==(0x04030000U|(unsigned(expected.size())<<4)|3));
  assert(replay.words[0]==(record.words[0]&~3U));
  assert(std::vector<uint32_t>(record.words.begin()+1,record.words.end())==expected);
  memory.expanded.clear();assert(memory.Run(record.words));for(unsigned i=0;i<12;i++)assert(memory.word[i]==low[i]);
  for(unsigned repeat=0;repeat<5;repeat++)assert(memory.Run(replay.words));
  assert(memory.expanded.size()==6*expected.size());
  for(unsigned i=0;i<memory.expanded.size();i++)assert(memory.expanded[i]==expected[i%expected.size()]);
  for(uint32_t illegal:{0x70040020U,0x7203006cU,0x420000c1U,0x26000020U,0x04000010U,0xb2050000U}) {
   auto bad=record.words;bad.back()=illegal;Replay refused;assert(!refused.Run(bad) && refused.expanded.empty());
  }
  Replay undefined;assert(!undefined.Run(replay.words) && undefined.expanded.empty());
 }
 for(unsigned p=0;p<2;p++)assert(PioneerQ38NativeOperandCount(8,p)<=12);
 for(unsigned p=0;p<16;p++)assert(PioneerQ38NativeOperandCount(12,p)<=12);
#ifdef PIONEER_Q38_WORK_DOT_ZERO_ELISION
 puts("PASS affine-replay exact7/14 raw-dot golden words,ALL FP32 latency/rounding ops retained,slots12..31 disjoint Q8/Q4 operand replay,fresh record after Q6 clobber,undefined/nested/memory/config/matrix instructions refuse;NO native IO");
#else
 puts("PASS affine-replay exact13/20 canonical golden words,ALL FP32 latency/rounding ops retained,slots12..31 disjoint Q8/Q4 operand replay,fresh record after Q6 clobber,undefined/nested/memory/config/matrix instructions refuse;NO native IO");
#endif
}
