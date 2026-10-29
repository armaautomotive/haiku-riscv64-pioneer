/* SPDX-License-Identifier: MIT */
#include <assert.h>
#include <stdio.h>
#include <vector>
#include "../PioneerQ38TokenFifoQuiescence.h"
struct IO {
 uint32_t available,overflow;unsigned fail=0;std::vector<unsigned> reads={};
 bool TokenFifoReadChecked(unsigned worker,bool over,uint32_t& output){
  assert(worker==119);reads.push_back(over?0x68U:0x64U);
  if(fail==reads.size())return false;
  output=over?overflow:available;return true;
 }
};
int main(){
 unsigned cases=0;
 for(uint32_t config:{0U,0x4000U,0x10000U,0x14000U,0xffffffffU}){
  const uint32_t empty=(config&0x10000U)?0x10101010U:0U;
  IO good{empty,0};assert(PioneerQ38TokenFifoEmpty(good,119,config) && good.reads==std::vector<unsigned>({0x64,0x68}));cases++;
  for(unsigned byte=0;byte<4;byte++)for(unsigned slots=0;slots<256;slots++){
   const uint32_t available=(empty&~(0xffU<<(byte*8)))|uint32_t(slots)<<(byte*8);
   if(available==empty)continue;
   IO bad{available,0};assert(!PioneerQ38TokenFifoEmpty(bad,119,config) && bad.reads==std::vector<unsigned>({0x64}));cases++;
  }
  for(unsigned bit=0;bit<32;bit++){
   IO bad{empty,uint32_t(1U<<bit)};assert(!PioneerQ38TokenFifoEmpty(bad,119,config) && bad.reads==std::vector<unsigned>({0x64,0x68}));cases++;
  }
  for(unsigned fail=1;fail<=2;fail++){
   IO bad{empty,0,fail};assert(!PioneerQ38TokenFifoEmpty(bad,119,config) && bad.reads.size()==fail);cases++;
  }
 }
 printf("PASS FIFO DATA/read-order primitive/%u cases/all4 free-byte positions/disabled values/overflow bits/access failures/FIRST bad observed field stops reads;NO source/STOP/HOLD/native ownership/restore/END/hardware/Work/TPS grant\n",cases);
}
