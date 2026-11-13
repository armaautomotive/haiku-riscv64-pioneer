/* SPDX-License-Identifier: MIT */
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>
extern "C" bool Q38SyntheticGdnExpected(const uint32_t*,const uint32_t*,const uint32_t*,uint32_t*);
extern "C" bool Q38SyntheticGdnGatedExpected(const uint32_t*,const uint32_t*,const uint32_t*,const uint32_t*,uint32_t*);
static uint32_t Bits(float value){uint32_t bits;memcpy(&bits,&value,4);return bits;}
static float Float(uint32_t bits){float value;memcpy(&value,&bits,4);return value;}
int main(){
 std::vector<uint32_t> conv(40960),alpha(48),beta(48),expected(823488);
 for(unsigned h=0;h<16;h++){conv[h*128]=Bits(3);conv[2048+h*128]=Bits(4);}
 for(unsigned i=0;i<6144;i++)conv[4096+i]=Bits(2);
 assert(Q38SyntheticGdnExpected(conv.data(),alpha.data(),beta.data(),expected.data()));
 for(unsigned half=0;half<96;half++){
  const auto* e=expected.data()+half*8578;
  for(unsigned j=0;j<8192;j++)assert(std::fabs(Float(e[322+j])-(j<64?1.f:0.f))<1e-6);
 }
 assert(!Q38SyntheticGdnExpected(nullptr,alpha.data(),beta.data(),expected.data()));
 std::vector<uint32_t> gate(6144,Bits(2)),gated(829632);
 assert(Q38SyntheticGdnGatedExpected(conv.data(),alpha.data(),beta.data(),gate.data(),gated.data()));
 const double raw=1/std::sqrt(128.0),reference=raw/std::sqrt(raw*raw+1e-6)*2/(1+std::exp(-2.0));
 for(unsigned half=0;half<96;half++)for(unsigned i=0;i<64;i++)
  assert(std::fabs(double(Float(gated[half*8642+8578+i]))-reference)<1e-5);
 assert(!Q38SyntheticGdnGatedExpected(conv.data(),alpha.data(),beta.data(),nullptr,gated.data()));
 puts("PASS synthetic GDN gated observer/6144 one-hot values and null refusal; NO device/model/TPS");
 puts("PASS synthetic GDN observer/96 one-hot state halves and null refusal; NO device/model/TPS");
}
