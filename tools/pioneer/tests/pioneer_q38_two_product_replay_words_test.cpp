/* SPDX-License-Identifier: MIT */
#include "pioneer_q38_register_affine_replay_model.h"
#include "../PioneerQ38ExactQ4TwoProductReplay.h"
#include <cassert>
#include <cstdio>
#include <vector>
struct Words {std::vector<unsigned> words;void Op(unsigned op){words.push_back(op);}};
int main(){
 constexpr uint32_t Unknown=0xbadf0001;Words body;PioneerQ38VectorAffineFloatDot(body);
 const std::vector<unsigned> golden={0x90000440,0x86013962,0x8f000000,0x86023972,0x8f000000,
  0x86060902,0x8f000000,0x86074942,0x8f000000,0x850a0400,0x8f000000,0x850a5000,0x8f000000};
 assert(body.words==golden);unsigned refusals=0,expanded=0;
 ExactRegisterAffineReplay replay;replay.Poison();const auto before=replay.words;
 std::vector<unsigned> observed;auto emit=[&](unsigned op){observed.push_back(op);};
 for(unsigned job=0;job<2;job++){
  replay.Poison();assert(replay.Push(0x040300d3,Unknown,emit));
  for(auto op:golden)assert(replay.Push(op,Unknown,emit));
  for(unsigned i=0;i<12;i++)assert(replay.words[i]==before[i]);
  for(unsigned i=25;i<32;i++)assert(replay.words[i]==before[i]);
  for(unsigned repeat=0;repeat<5;repeat++)assert(replay.Push(0x040300d0,Unknown,emit));
 }
 assert(replay.records==2 && replay.replays==10 && replay.expanded==12*13 && !replay.remaining);
 for(unsigned i=0;i<observed.size();i++){assert(observed[i]==golden[i%13]);expanded++;}
 for(unsigned bad:{0x040300d0U,0x040300d1U,0x040300d2U,0x04030003U,0x040300c3U,0x040300e3U,0x040700d3U,0x040304d3U}){
  ExactRegisterAffineReplay m;bool refused=false;try{m.Push(bad,Unknown,emit);}catch(uint32_t marker){refused=marker==Unknown;}
  assert(refused);refusals++;
 }
 for(unsigned bad:{0x70030020U,0x7203006cU,0x420000c1U,0x26004020U,0x040300d0U,0xb2050000U,0x3700000fU}){
  ExactRegisterAffineReplay m;assert(m.Push(0x040300d3,Unknown,emit));bool refused=false;
  try{m.Push(bad,Unknown,emit);}catch(uint32_t marker){refused=marker==Unknown;}
  assert(refused);refusals++;
 }
 printf("PASS_LOCAL_REPLAY13 golden_original_affine_words13 expanded%u faults%u slots12to24_only fresh_record_each_job;NO_NATIVE_HARDWARE_OR_TPS\n",expanded,refusals);
}
