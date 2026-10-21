/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_PREPARED_Q4_BLOCK
#include "../PioneerQ38PreparedQ4.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>
int main() {
 alignas(4) std::array<uint8_t,2304> original;
 alignas(4) std::array<uint8_t,2816> prepared;
 std::array<uint32_t,258> scratch;
 for(unsigned fixture=0;fixture<64;fixture++) {
  for(unsigned i=0;i<original.size();i++)original[i]=uint8_t(i*71+fixture*89);
  for(unsigned kind=0;kind<2;kind++)for(unsigned row=0;row<16;row++) {
   uint16_t half=uint16_t((fixture*2011+row*1249+kind*97)%0x7c00);if((fixture+row+kind)&1)half|=0x8000;
   original[kind*32+row]=uint8_t(half);original[kind*32+16+row]=uint8_t(half>>8);
  }
  assert(PioneerQ38PreparedQ4::Prepare(original.data(),2304,prepared.data(),2816));
  scratch.fill(0xdeadbeef);assert(PioneerQ38PreparedQ4::Block(prepared.data(),2816,scratch.data()+1,256));
  assert(scratch.front()==0xdeadbeef && scratch.back()==0xdeadbeef);
  for(unsigned group=0;group<8;group++) {
   PioneerQ38Panel expected={};assert(PioneerQ38AffineOnly(12,original.data(),2304,group*2,&expected));
   for(unsigned row=0;row<16;row++) {
    assert(scratch[1+group*32+row]==expected.coefficient[row]);
    assert(scratch[1+group*32+16+row]==expected.minimum[row]);
   }
  }
 }
 const auto valid=prepared;
 for(unsigned word=0;word<256;word++)for(unsigned fault=0;fault<3;fault++) {
  prepared=valid;unsigned at=2048+word*3;
  if(!fault)prepared[at+2]|=0x80;
  if(fault==1){prepared[at]=prepared[at+1]=0;prepared[at+2]=46;}
  if(fault==2){prepared[at]=1;prepared[at+1]=0;prepared[at+2]=63;}
  scratch.fill(0xdeadbeef);const auto before=scratch;
  assert(!PioneerQ38PreparedQ4::Block(prepared.data(),2816,scratch.data()+1,256) && scratch==before);
 }
 prepared=valid;scratch.fill(0xdeadbeef);const auto before=scratch;
 assert(!PioneerQ38PreparedQ4::Block(nullptr,2816,scratch.data()+1,256));
 assert(!PioneerQ38PreparedQ4::Block(prepared.data(),2815,scratch.data()+1,256));
 assert(!PioneerQ38PreparedQ4::Block(prepared.data()+1,2816,scratch.data()+1,256));
 assert(!PioneerQ38PreparedQ4::Block(prepared.data(),2816,nullptr,256));
 assert(!PioneerQ38PreparedQ4::Block(prepared.data(),2816,scratch.data()+1,255));
 assert(!PioneerQ38PreparedQ4::Block(prepared.data(),2816,reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(scratch.data())+1),256));
 assert(!PioneerQ38PreparedQ4::Block(prepared.data(),2816,reinterpret_cast<uint32_t*>(prepared.data()),256));
 assert(scratch==before && prepared==valid);
 puts("PASS OFFLINE prepared-block DATA:16384 FP32 metadata words exact,768 malformed-first/middle/last codes atomic refusal,canary/null/size/alias/alignment guards;original24-bit codec/quant bytes unchanged;NO native/device IO");
}
