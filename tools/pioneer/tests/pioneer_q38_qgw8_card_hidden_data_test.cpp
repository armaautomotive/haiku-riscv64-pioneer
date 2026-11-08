/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_CARD_HEAD_REPLAY 1
#include "pioneer_q38_qgw8_card_hidden_data.h"
#include <cassert>
#include <cstdio>

static std::string Digest(const std::vector<uint32_t>& w){
 return PioneerModelWeightCache::Hex(PioneerModelWeightCache::Hash(w.data(),w.size()*4));
}
static std::vector<uint32_t> Fixture(){
 std::vector<uint32_t> w(Q38CardHiddenData::Words,0);
 const uint32_t h[]={0x31444851,1,0,248319,64,5120,1,1,0x00010129,0x20261107,5,0,2,0,0,0};
 for(unsigned i=0;i<16;i++)w[i]=h[i];
 for(unsigned i=0;i<5120;i++)w[16+i]=0x3f800000;
 w.back()=0x454e4448;return w;
}
int main(){
 auto w=Fixture();std::vector<uint32_t> out;
 assert(Q38CardHiddenData::Decode(w,Digest(w),out) && out.size()==5120 && out[0]==0x3f800000);
 assert(!Q38CardHiddenData::Decode(w,Digest(w),out));out.clear();
 assert(!Q38CardHiddenData::Decode(w,std::string(64,'0'),out) && out.empty());
 assert(!Q38CardHiddenData::Decode(w,std::string(64,'Z'),out) && out.empty());
 const unsigned indexes[]={0,1,2,3,4,5,6,7,8,9,10,12,13,14,15,16,Q38CardHiddenData::Words-1};
 for(unsigned index:indexes){
  auto bad=Fixture();bad[index]=index==16?0x7fc00000:index==10?0:bad[index]^1;
  assert(!Q38CardHiddenData::Decode(bad,Digest(bad),out) && out.empty());
 }
 w.pop_back();assert(!Q38CardHiddenData::Decode(w,Digest(w),out));
 puts("PASS isolated card hidden parser; synthetic only, no device/model");
}
