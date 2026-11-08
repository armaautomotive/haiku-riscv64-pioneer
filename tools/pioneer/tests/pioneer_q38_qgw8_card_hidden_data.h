/* SPDX-License-Identifier: MIT */
#if !defined(PIONEER_Q38_CARD_HEAD_REPLAY) || PIONEER_Q38_CARD_HEAD_REPLAY != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(__FAST_MATH__)
#error Card hidden DATA is permitted only for isolated CPU head replay
#endif
#ifndef PIONEER_Q38_QGW8_CARD_HIDDEN_DATA_H
#define PIONEER_Q38_QGW8_CARD_HIDDEN_DATA_H
#include "../PioneerModelWeightCache.h"
#include <cmath>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
namespace Q38CardHiddenData {
constexpr unsigned Width=5120, Words=16+Width+1;
inline bool Decode(const std::vector<uint32_t>& words,const std::string& expected,std::vector<uint32_t>& output){
 if(words.size()!=Words || expected.size()!=64 || output.size()!=0)return false;
 for(char c:expected)if(!((c>='0' && c<='9') || (c>='a' && c<='f')))return false;
 if(PioneerModelWeightCache::Hex(PioneerModelWeightCache::Hash(words.data(),words.size()*4))!=expected)return false;
 const uint32_t h[]={0x31444851,1,0,248319,64,Width,1,1,0x00010129,0x20261107};
 for(unsigned i=0;i<10;i++)if(words[i]!=h[i])return false;
 if(!(words[10] || words[11]) || words[12]!=2 || words[13] || words[14] || words[15]
  || words.back()!=0x454e4448)return false;
 for(unsigned i=0;i<Width;i++){
  float value;std::memcpy(&value,&words[16+i],4);if(!std::isfinite(value))return false;
 }
 output.assign(words.begin()+16,words.end()-1);return true;
}
}
#endif
