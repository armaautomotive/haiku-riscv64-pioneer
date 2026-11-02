/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_TOKEN_MATRIX_PHASE_DATA_TEST 1
#include "../PioneerQ38TokenMatrixPhase.h"
#include <cassert>
#include <cstdio>
#include <initializer_list>
#include <limits>

static void Complete(PioneerQ38TokenMatrixPhase& p,unsigned k,unsigned groups,uint64_t& now){
 for(unsigned g=0;g<groups;g++)for(unsigned b=0;b<k/256;b++){
  now+=17;assert(p.Begin(g,b,now));now+=3;assert(p.Fetched(now));
  for(unsigned panel=0;panel<16;panel++){
   now+=5;assert(p.Decoded(panel,now));now+=7;assert(p.Executed(panel,now));
  }
 }
}
int main(){
 unsigned positive=0,negative=0;
 for(unsigned w=1;w<120;w++)for(unsigned k:{5120U,6144U,17408U})for(unsigned groups:{1U,3U,16U}){
  PioneerQ38TokenMatrixPhase p;p.Init(w,2026,k,groups);uint64_t now=UINT64_C(0x10000000000);
  Complete(p,k,groups,now);uint32_t words[16];assert(p.Complete() && p.Describe(words,true));
  const uint64_t blocks=uint64_t(groups)*(k/256);
  assert(words[0]==p.Magic && words[1]==p.Version && words[2]==2026 && words[3]==w
   && words[4]==12 && words[5]==k && words[6]==groups && words[7]==1);
  for(unsigned bucket=0;bucket<3;bucket++)assert(((uint64_t(words[9+bucket*2])<<32)|words[8+bucket*2])
   ==blocks*(bucket==0?3U:bucket==1?80U:112U));
  assert(!words[14] && !words[15]);positive++;
  assert(!p.Describe(words,false) && !words[7]);
  for(unsigned i=8;i<16;i++)assert(!words[i]);negative++;
 }
 for(unsigned mode=0;mode<15;mode++){
  PioneerQ38TokenMatrixPhase p;p.Init(69,7,5120,1);uint64_t now=100;
  switch(mode){
   case 0:p.Init(0,7,5120,1);break;
   case 1:p.Init(120,7,5120,1);break;
   case 2:p.Init(69,0,5120,1);break;
   case 3:p.Init(69,0x7fffffff,5120,1);break;
   case 4:p.Init(69,7,5121,1);break;
   case 5:p.Init(69,7,5120,0);break;
   case 6:p.Init(69,7,5120,17);break;
   case 7:p.BadClock();break;
   case 8:assert(!p.Fetched(now));break;
   case 9:assert(!p.Decoded(0,now));break;
   case 10:assert(!p.Executed(0,now));break;
   case 11:assert(!p.Begin(0,1,now));break;
   case 12:assert(p.Begin(0,0,now));assert(!p.Fetched(now-1));break;
   case 13:assert(p.Begin(0,0,now));assert(p.Fetched(now));assert(!p.Decoded(1,now));break;
   case 14:assert(!p.Begin(0,0,std::numeric_limits<uint64_t>::max()));break;
  }
  uint32_t words[16];assert(!p.Complete() && !p.Describe(words,true));
  assert(!p.Begin(0,0,now+1) && !p.Fetched(now+2) && !p.Decoded(0,now+3) && !p.Executed(0,now+4));
  for(unsigned i=7;i<16;i++)assert(!words[i]);negative++;
 }
 {
  PioneerQ38TokenMatrixPhase p;p.Init(119,0x7ffffffe,17408,16);
  uint64_t now=UINT64_C(0x8000000000000000);Complete(p,17408,16,now);
  uint32_t words[16];assert(p.Describe(words,true) && words[2]==0x7ffffffe);positive++;
 }
 std::printf("PASS_MATRIX_PHASE_DATA synthetic shapes%u invalidOrIncomplete%u;NO clock,ISA,native,physical or timing proof\n",positive,negative);
}
